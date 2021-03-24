# production/db/memory_manager
This is a folder for the database engine memory manager code.

## Design/implementation notes

### Note on current implmentation

The notes below cover the planned implementation. However, the current implementation is simpler. The Memory Manager exposes a simple interface for doing incremental allocations that is used directly by the client for each object allocation. The Chunk Manager is not used at all. After the client will be updated to use memory in chunk increments via a chunk manager, the incremental server allocation method will be dropped and the design will start aligning with the description below.

### Use cases

The Memory Manager contains two main components: the memory manager itself and a chunk manager:

* Memory manager (MM) component:
  * This is used by the server to manage a large memory range by allocating memory from it and then ensuring that memory continues to be available by compacting allocations.
  * All allocations are done as 4MB 'chunks'. The chunks can be further used through instances of the chunk manager class.
* Chunk manager (CM) component:
  * This is used by the client to allocate memory from a chunk. It can also be used by the server to process the allocations made by the client in a chunk.
  * It is meant to be used by the client across multiple transactions. As such, it allows quickly reverting all allocations made since the last commit.
  * The allocation unit inside a chunk is 64B. There is a chunk metadata block that uses a bitmap to indicate the status of all allocation units in the block.
  * A chunk manager is allocated on the heap and is meant to be used by a single thread at a time.

The MM is a server component. The CM can be initialized on either server or client. On client, it will manage the memory assigned to the client session. On server, it can be initialized to help read the memory written by a client.

### Operation details

* MM:
  * Memory allocations are made from the start of the memory range, moving left->right if we imagine memory as increasing in address from left to right. If the end of the range is reached, allocations will wrap around and restart from the beginning of the range. To permit this approach, a compaction process will ensure that allocated memory is always packed "behind" the next allocation offset.
* CM
  * The CM uses a metadata block to track the use of allocation units within the chunk it manages. This block is allocated at the beginning of the memory block managed by the CM.
  * CM allocations start from the beginning of the block and grow left->right.

### Metadata

* MM
  * Currently, MM doesn't track any metadata internally and only relies on the metadata stored in each chunk.
* CM
  * The CM metadata tracks the following:
    * The last allocated 64B allocation unit index.
    * The last committed 64B allocation unit index
    * A bitmap indicating the use of each allocation unit.

Determining the size of an allocation given just an address can be done by deserializing the database object at that address.

### Access synchronization

The MM expects to be executed in a single process with its methods getting called by multiple threads.

Synchronization of MM operations is achieved via:
* atomic operations.
