# production/db/memory_manager
This is a folder for the database engine memory manager code.

## Design/implementation notes

### Use cases

The Memory Manager contains two main components: the memmory manager itself and a stack allocator:

* Memory manager (MM) component:
  * This is used to initialize a memory range for use, as well as for the initial population of this region with data.
  * After the data is loaded, during normal database operations, the memory manager is mainly used to produce stack allocators and to commit them.
  * During the commit of the stack allocators, the memory manager will track their metadata, to enabled the database serialization component to later write the changes to disk. Also, the memory manager will collect all released memory (storing the old versions of updated entities) and will track them for potential future reuse.
  * After the serialization component flushes to disk a set of stack allocators (in the order of their respective serialization numbers), it will call the manager to let it know that it can now reclaim the memory released by these stack allocators (their unuse memory plus their metadata information).
* Stack allocator (SA) component:
  * This is used during normal database operation, to allocate memory within a transaction from a large block that is managed through the stack allocator.
  * It supports rollback of the last allocations/deletions.
  * Its metadata is public, to allow it to be understood by the database serialization code.
  * Given that only a part of the memory managed by the stack allocator will be used by its corresponding transaction, the remainder will be reclaimed later, after the stack allocator data has been serialized to disk.
  * A StackAllocator is allocated on the heap and is meant to be used by a single thread.

In addition to this, there is one more important operation concept: there has to be 1 Master (Primary) Memory Manager operated by the database engine and all other memory managers would be slaves (secondaries). The main difference is that only the Master is used for serialization. The slaves are only used for memory allocation and memory access, but serialization can only happen in the database.

### Operation details

* MM:
  * The MM keeps track of several important pieces of information in a metadata block that is allocated at the start of the large memory block that it manages.
  * For tracking additional data, the MM will allocate MemoryRecord entities from the end of the managed memory block and it will chain them in different lists. These allocations move right->left.
  * Memory allocations are made right after the metadata and move left->right.
  * The space between these 2 sets of allocations represents the remaining main memory block.
* SA
  * The SA also uses a metadata block to track its important information. Unlike the MM, this block is allocated at the end of the memory block managed by the SA.
  * SA allocations start from the beginning of the block and grow left->right.
  * For each allocation, the SA also writes a record after its metadata block, so these get added right->left.

### Metadata

* MM
  * Currently, MM metadata tracks the following information:
    * The edges of the remaining main block of memory that sits between the previously allocated blocks and all the allocated MemoryRecords (which never get deallocated - they just get recycled).
    * Three lists of MemoryRecords:
      * List of freed memory blocks. The memory manager will attempt to allocate memory from these before using the main memory block.
      * List of unserialized SA allocations. This is kept for the database engine serialization code.
      * List of reclaimed MemoryRecords. These are never released; instead they get added to this list for potential future reuse.
* SA
  * The SA metadata tracks the following:
    * The count of allocations made so far.
    * The size of the first allocation.
    * The memory location where the next allocation can be made.
    * The serialization number assigned to the transaction that used this SA.
* MM memory allocation
  * Each block allocated by the MM is prefixed by a small metadata block that records its size. This allows us to find the size of an allocation given its starting offset.

### Access synchronization

The MM expects that multiple instances of it execute in parallel, not just in multiple threads, but in multiple processes as well.

Synchronization of MM operations is achieved via:
* atomic operations.
* reference counts.
* exclusive locks - held for as short a time as possible.

Linked list operations raise interesting problems due to the fact that nodes can be removed and inserted in other lists after a thread has read them. To prevent such issues, remove operations do not complete the removal of a node until all readers have abandoned it. Also, any list modification (insert/update/remove) requires exclusive access to be gained on the previous node (and in the case of remove, on the current node as well). Requesting this exclusive access, however, is delayed until the last moment. Traversals of lists, in particular, do not require exclusive locks and only rely on reference counts. The downside is that, in some special cases, a traversal may need to be restarted - for example if we found that the node that we wanted to get exclusive access to is already being removed by another thread.

### Challenges and future work

Here are some aspects that require future work:

* All the MM list operations require unit testing.
* Given the current pattern of operation, it is unlikely that the MM will be able to make an allocation from freed memory. There will be a need of a free memory compaction operation, but such an operation is inherently challenging because of the multi-process access to memory - we need an additional interface with the database engine to let it know that memory has been moved and that it should reflect those moves.
  * Perhaps after serialization, when we reclaim most of a stack allocator's memory, we can look at compacting its allocated part?
