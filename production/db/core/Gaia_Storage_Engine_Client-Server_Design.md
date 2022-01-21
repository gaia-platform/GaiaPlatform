# Gaia Database Client/Server Design

## Overview

The Gaia database was originally implemented as an embedded library, where clients directly execute all database
operations. To accommodate Q2 features, which involve out-of-process clients such as the rules engine and relational
query processor, as well as more sophisticated concurrency control requiring a transaction coordinator, we need to
separate the database into a standalone server process and a client library which can be embedded in any process running
on the same machine as the server process. Since the client and server continue to share memory for many of the
database's data structures, most database operations can still be executed directly by the client on shared memory.
However, there are some operations
(such as transaction validation and persistence) that require central coordination by the server, and thus require the
development of a client/server messaging protocol. This document describes that protocol, as well as other design
changes required to support out-of-process clients.

## Definitions

<dl>
<dt>Database</dt>
<dd>The component of the Gaia Platform responsible for maintaining data at the key-value level.</dd>
<dt>Database client</dt>
<dd>The component of the database which implements Gaia Platform data APIs and exposes them to third-party code running
    in external processes.</dd>
<dt>Database server</dt>
<dd>The component of the database which manages the storage of data in memory and on disk, and coordinates client access
    to data, including transactional reads and writes.</dd>
<dt>Data segment</dt>
<dd>A shared memory segment managed by the database server but readable and writable by all clients, which contains all
    objects created by clients, including obsolete versions. The data segment's memory is managed by a simple "bump allocator",
    which allocates memory for each object consecutively (on 8-byte boundaries), with no attempt to reuse memory from
    obsolete object versions.</dd>
<dt>Locator segment</dt>
<dd>A shared memory segment managed by the database server but readable by all clients (and privately writable), which
    contains the current pointer addresses (actually offsets in the data segment) of all objects created by clients,
    which are addressed by "locators", or offsets in the locator segment.</dd>
<dt>Log segment</dt>
<dd>A shared memory segment created per-transaction by clients, which contains a log record for each object mutated by
    the transaction, enabling the server to validate the transaction and apply it to the locator segment.</dd>
<dt>Cursor</dt>
<dd>A unidirectional data stream from server to client, initiated by a <tt>REQUEST_STREAM</tt> control message sent over
    a session channel and implemented as a stream socket pair. A cursor is exposed to application code as a C++ range or iterator.</dd>
</dl>

## Assumptions and Decisions

The fundamental premise of this design is that the Gaia Database must support out-of-process clients in order to
implement essential features of the Gaia platform. These features include the rules engine, which requires process
isolation to ensure that untrusted user code cannot compromise the security or reliability of the database, as well as
external query processors such as the Postgres foreign data wrapper extension, which might in principle be implemented
in-process with the database, but currently need to be out-of-process for integration with their host database engines.
Additionally, users should be able to use Gaia direct access APIs from arbitrary code, not just rules, and from any
process running that code (subject to proper authentication and authorization). That requirement implies that the
database APIs must be accessible from a client library (whether statically or dynamically linked) that can be invoked
from any host process.

Some additional assumptions and design decisions:

- Directly exposing shared memory segments accessible through a well-known name is fundamentally insecure (even if these
  objects can be theoretically secured by filesystem permissions). A simple capability model provides better security:
  clients can access a shared memory object only if the server grants them access, by sending them a token usable only
  in the client's own process (which ensures it can't be leaked to other code). The current design fulfills this
  requirement by mediating all client access to server-controlled shared memory through file descriptors passed over
  Unix sockets, which are only transmitted after the client has connected to and been authenticated by the server.
- All transactions for a given client take place on a single thread on both the client and the server. This
  automatically serializes transaction begin and commit with respect to a single client. The client must explicitly
  initiate a session with the server before submitting any transactions. The client must also wait for a submitted
  transaction to complete or abort before initiating another transaction. Any process hosting the client library can
  support any number of sessions, but each session requires a separate thread.
- Any server-to-client data channel ("cursor") must be owned by a single session thread on both the server and client,
  which is responsible for releasing all associated resources when the session terminates. The cursor's client-side
  iterator can only be accessed by the originating session's client-side thread.
- Message formats in the client-server protocol should support revision without breaking forward or backward
  compatibility. We can easily support this requirement by using a protocol schema language such as FlatBuffers. There
  are no plans to explicitly version the protocol, though, given that it seems unlikely that the client library and the
  server would be running different versions of the Gaia software.
- The client/server model does not extend to remote clients, i.e., client processes running on different machines. The
  IPC mechanisms adopted here (namely shared memory and file descriptor passing over Unix sockets) do not readily
  generalize to remote clients.
- Asynchronous, process-directed signals should be masked on all threads and handled synchronously (via `sigwait()`
  /`sigwaitinfo()`)
  by a single dedicated thread, to avoid nondeterminism in the choice of handling thread and synchronization and
  reentrancy issues in asynchronous signal handlers.
- There is currently no asynchronous notification mechanism for clients to inform them of server-side events (e.g.,
  changes to data or schema). Since the client session is single-threaded, the client must explicitly request the server
  to inform it of events of interest, blocking the session thread to do so.
- When the server process is shut down in response to a signal (e.g. `SIGTERM`), clients should be notified and given a
  chance to gracefully disconnect. The notification mechanism currently implemented is for the server to send
  end-of-file to the client, and then wait for the client to do the same to the server. (How this notification is
  exposed to user code is TBD, given the absence of an asynchronous notification mechanism noted above.)

## Architecture

### Summary

When the database server process starts, it initializes two shared memory segments for the database's shared
data structures: the data segment and the locator segment. It maps these two segments into its own address space but
caches the file descriptors for transmission to clients. Then the server spawns a thread which listens on a well-known
Unix socket for client connections and dispatches each new connection to a dedicated session thread. The server-side
session thread creates a dedicated socket for the connected client and polls the socket in a loop, along with a file
descriptor representing a server shutdown event. All transactions are executed over this socket and shared memory
exchanged over the socket. When the server is shut down in response to a signal, all session threads are gracefully
terminated (notifying clients first).

### Client/Server Messaging Protocol

#### Overview

The client and server communicate over both control and data channels. Currently the data channels are confined to
shared memory segments, but in the future they might include channels for unbounded data, such as Unix stream sockets.
The control channel is implemented using Unix datagram sockets, but in a connection-oriented mode (`SOCK_SEQPACKET`), to
avoid multiplexing and support detecting client disconnection. The control message format is defined in a FlatBuffers
schema, for consistency with the rest of the system, and includes event identifiers for triggering transitions in the
client's state machine, as well as auxiliary information such as the client's old and new session state on the server.
The messages may also include Unix socket ancillary data separate from the payload, in our case file descriptors
representing shared memory segments. These are used when a session is established to communicate the data and locator
segments to the client, and at transaction commit to communicate the client's transaction log segment to the server.
Since the messages are transmitted using a datagram protocol, no framing is necessary, but a data channel using stream
sockets would need some framing such as a message length prefix (such a data channel has already been prototyped
using `socketpair()` with stream sockets and file descriptor passing).

#### Protocol Sequence

When the server starts up, it opens a listening Unix socket on a dedicated thread. When the socket receives a client
connection, it authenticates the client (not yet specified but quite straightforward using Unix credentials), launches a
dedicated thread for the new session, and replies to the client with file descriptors for the data and locator segments.
The client may now send the server messages to either begin a transaction, abort a transaction in progress, commit a
transaction, or end the session. The server in turn can send the client messages containing a transaction commit
decision, or notifying it that a server shutdown is in progress. (Both of these shutdown notifications are implicitly
encoded in a socket half-close, i.e. `shutdown(SHUT_WR)`, which makes the other end return `EOF` from its next `read()`
on the socket.)

#### Protocol State Machine

Rather than encoding the protocol implicitly in the code that parses and acts on messages, the server maintains an
explicit state machine for each client session that allows valid protocol transitions and rejects invalid transitions.
The state machine is encoded as a table of `(event, state, transition)` tuples, where `transition` is in turn
represented as a `(new_state, handler)` pair. When the server either receives an event from a new client message, or
generates an event from its own transition handlers (e.g. a transaction decision from the transaction commit handler),
it consults the state machine table to find a valid transition for the new event and the current session state. If it
finds such a transition, the session state is advanced to the transition's new state, and the transition's handler is
invoked. The handler can generate further events, which may invoke further transitions, so handlers must be reentrant in
general (but not threadsafe, since they are invoked only on the dedicated session thread). Otherwise, if no transition
is found compatible with the event and the current session state, an exception is thrown (which could either terminate
the server process, terminate the session by closing the session socket, or be marshaled back to the client for
rethrowing in the calling thread).

#### Transaction Protocol

When the client wishes to open a transaction (assuming it already has opened a session with the server), it sends
a `BEGIN_TXN` message to the server, which advances the session state on the server to `TXN_IN_PROGRESS` but otherwise
requires no action on the server. The client then allocates a new log segment and remaps its locator segment to a
copy-on-write private mapping. Each object update requires a new copy to be constructed in the shared data segment, and
is logged with the locator index and old and new object pointer values in the transaction's log segment. If the client
decides to roll back the transaction at any point, it will notify the server by sending it an `ROLLBACK_TXN` message,
but as with opening a transaction, this does nothing but advance the session's state to `CONNECTED`. When the client
decides to commit the transaction, it sends a `COMMIT_TXN` message to the server containing the file descriptor of the
log segment it allocated. The server maps the log segment and uses it to validate the transaction by verifying that all
pointer values in the locators of updated objects have not changed since the transaction was opened. The server replies
with a message containing its commit decision, either `DECIDE_TXN_COMMIT` or `DECIDE_TXN_ABORT`. The client blocks on
receipt of this message and then delivers the result to the caller. The server then unmaps the log segment and closes
its file descriptor, and the client does the same.

#### Cursor Protocol

When the client wishes to open a unidirectional data stream from the server (called a "cursor"), it sends
a `REQUEST_STREAM` control message to the server over its session socket, which contains any stream-specific information
that the server needs (such as a type ID for a table scan or range information for an index scan). The server then
creates an iterator representing the data source for the stream, a new socket pair to serve as the communication channel
for the stream, and a new "cursor thread" owned by the server-side session thread, which blocks in a poll loop waiting
for the stream socket to become writable. As soon as the stream socket is writable, the cursor thread advances the
iterator and writes a new batch of results into the socket. After the cursor thread is created, the server replies to
the client with the client-side socket from the stream socket pair. The client creates an iterator over its stream
socket for use by application code, which reads a batch of items from the socket, deserializes all items in the batch,
and returns the next item to the calling thread. Since all cursors are created within the context of a session, these
iterators are confined to the session thread that created them. Also, unlike the server, which has a separate thread to
keep the client's socket buffer full, the client only removes items from its socket buffer on demand, when the session
thread advances the iterator. When the server has reached the end of its stream, it sends EOF over the stream socket to
the client. The client detects EOF when a read from its stream socket returns 0 bytes, and it signals the consuming
thread that the stream is exhausted by returning `std::nullopt` from the stream iterator. When the client has finished
reading all data from its stream socket, it signals the server by closing the socket. When the client has closed its end
of the stream socket, the server-side cursor thread releases all resources associated with the cursor (including its
stream socket) and exits. When a session terminates (e.g. because of a client call to `end_session()`, orderly server
shutdown, or a socket error from the client), all cursor threads owned by that session are notified and must release
their resources and exit before the session thread can exit.

#### Termination Handling

When the server receives an asynchronous signal (e.g., `SIGINT`, `SIGTERM`, `SIGQUIT`), in its set of blocked signals,
it interrupts a dedicated signal handling thread (blocking in `sigwait()`), which then signals an `eventfd` file
descriptor which is polled by the client dispatch thread and all session threads. The session threads will notify their
connected clients that they are shutting down, by performing a half-close on the socket using `shutdown(SHUT_WR)`. The
client library currently exposes the server shutdown event to the user by throwing an exception when the next client
operation tries to read from or write to the now-defunct session socket (if the client is currently trying to read from
the session socket it will receive EOF, and if it is trying to write to the socket it will receive `EPIPE`). Each
session thread also notifies all threads it owns of its pending termination (through a session-specific `eventfd`), and
waits for them to terminate before exiting.
