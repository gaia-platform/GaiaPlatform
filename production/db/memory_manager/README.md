# production/db/memory_manager
This is a folder for the database engine memory manager code.

## Design/implementation notes

### Note on current implementation

The notes below cover the planned implementation. However, the current implementation is simpler. The Memory Manager exposes a simple interface for doing incremental allocations that is used directly by the client for each object allocation. The Chunk Manager is not used at all. After the client will be updated to use memory in chunk increments via a chunk manager, the incremental server allocation method will be dropped and the design will start aligning with the description below.

### Use cases

The Memory Manager contains two main components: the memory manager itself and a chunk manager:

* Memory manager (MM) component:
  * This is used by both the server and the client to allocate 4MB chunks of memory from which memory is allocated by the client using a chunk manager.
  * There is a metadata block that will hold a bitmap tracking the use of each chunk.
  * The server will ensure that memory continues to be available by compacting allocations within and across chunks.
  * A memory manager is expected to be used for chunk allocation by multiple threads/processes at a time. However, its garbage collection/compaction operation is expected to occur on a single thread.
* Chunk manager (CM) component:
  * This is used by the client to allocate memory from a chunk. It can also be used by the server to process the allocations made by the client in a chunk, for garbasge collection (GC) and compaction purposes.
  * It is meant to be used by the client across multiple transactions. As such, it allows quickly reverting all allocations made since the last commit.
  * The allocation unit inside a chunk is 64B. There is a chunk metadata block that uses a bitmap to indicate the use of each allocation unit inside the chunk.
  * A chunk manager is meant to be used by a single thread at a time.

Both MM and CM can be initialized on either server or client, although they will be used in different ways.
  * The MM is used by the server mainly for performing GC and compaction operations. It is used by the client for allocating new chunks.
  * The CM is used by the client to perform allocations within a chunk. It is used by the server to process those allocations.

### Operation details

* MM:
  * The MM uses a metadata block to track the use chunks within the memory it manages. This block is allocated within the first chunk.
  * Memory allocations are made from the start of the memory range, moving left->right if we imagine memory as increasing in address from left to right. The end of the memory should only be reached when the server is about to run out of memory. Otherwise, the server is expected to keep reusing chunk that are freed at lower memory addresses.
* CM
  * The CM uses a metadata block to track the use of allocation units within the chunk it manages. This block is allocated at the beginning of the memory block managed by the CM.
  * CM allocations start from the beginning of the block and grow left->right.

### Metadata

* MM
  * A bitmap indicating the use of each chunk.
  * An offset for the first available chunk that was never used before.
* CM
  * The CM metadata tracks the following:
    * A bitmap indicating the use of each allocation unit.
    * The last committed 64B allocation unit index

Determining the size of an allocation given just an address can be done by deserializing the database object at that address.

### Access synchronization

The MM chunk allocations can be done from multiple processes that synchronize through atomic operations over the next available chunk offset. The MM GC/compaction operations would only be executed by the server.

Because a chunk is usually operated on by a single thread, CM operations don't need to worry about synchronization. The server needs to track the ownership of each chunk and take over if a client dies.
