# production/db/memory_manager
This is a folder for the database engine memory manager code.

## Design/implementation notes

### Use cases

The Memory Manager contains two main components: the memory manager itself and a stack allocator:

* Memory manager (MM) component:
  * This is used to initialize a memory range for use, as well as for the initial population of this region with data.
  * After the data is loaded, during normal database operations, the memory manager is mainly used to allocate memory for stack allocators and to commit them.
  * During the commit of the stack allocators, the memory manager will collect all released memory (the old versions of updated entities) and will track them for potential future reuse.
* Stack allocator (SA) component:
  * This is used during normal database operation, to allocate memory within a transaction from a large block that is managed through the stack allocator.
  * It supports rollback of the last allocations/deletions.
  * Its metadata is public, to allow it to be understood by the database serialization code.
  * Given that only a part of the memory managed by the stack allocator will be used by its corresponding transaction, the remainder will be reclaimed later, after the stack allocator data has been serialized to disk.
  * A stack allocator is allocated on the heap and is meant to be used by a single thread.

The MM is a server component. The SA can be initialized on both server or client. On client it can manage the memory assigned to the client session. On server it can be initialized to help read the memory written by a client.

### Operation details

* MM:
  * The MM keeps track of several important pieces of information. Its metadata is stored directly in the MM instance.
  * Freed memory blocks are tracked through `memory_record_t` objects (the method of this tracking is under development and has not been finalized).
  * Memory allocations are made from the start of the memory block, moving left->right if we imagine memory as increasing in address from left to right. Each allocated memory block has its size recorded right before the start of the block made available to users. This means that the size of a block is always available if we know its starting address.
* SA
  * The SA uses a metadata block to track its important information. This block is allocated at the end of the memory block managed by the SA.
  * SA allocations start from the beginning of the block and grow left->right.
  * For each allocation, the SA also writes a record after its metadata block, so these records get added right->left.
  * The metadata block together with the allocation records form a complete allocation record. The SA interface provides access to this data.

### Metadata

* MM
  * Currently, MM metadata tracks the following information:
    * The start of the main memory block that is available for allocations. This will shrink as more allocations are being made
    * List of freed memory blocks. The memory manager will attempt to allocate memory from these before using the main memory block.
* SA
  * The SA metadata tracks the following:
    * The count of allocations made so far.
    * The size of the first allocation.
    * The memory location where the next allocation can be made.
* MM memory allocation
  * Each block allocated by the MM is prefixed by a small metadata block that records its size. This allows us to find the size of an allocation given its starting offset.

### Access synchronization

The MM expects to be executed in a single process with its methods getting called by multiple threads.

Synchronization of MM operations is achieved via:
* atomic operations.
* mutexes.

### Challenges and future work

Here are some aspects that require future work:

* The tracking of freed memory blocks needs to be further improved.
  * We want to be able to detect adjacent freed blocks and combine them in larger blocks.
  * We also want to track blocks depending on their size, so that we don't have to inspect all blocks to find one of sufficient size.
  * The data structures used for this tracking need to permit efficient thread-safe operations (inserts, removes, lookups).
* Given the current pattern of operation, it is unlikely that the MM will be able to make an allocation from freed memory (because stack allocators will always request the same memory from which they'll usually release smaller blocks). The proposed solution for compaction is to provide an interface that iterates over data whose move would result in the most beneficial coallescing of freed blocks of memory. The system can then use this interface to reallocate memory for these blocks and perform a "fake update" that does not change the value of the data, but gets it placed at a new location. These "fake updates" should be treated specially by the transaction coordinator, to ensure that they do not prevent other transactions to complete.
