/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <iostream>
#include <sstream>
#include <memory>

#include "constants.hpp"
#include "retail_assert.hpp"
#include "types.hpp"
#include "access_control.hpp"
#include "stack_allocator.hpp"
#include "memory_manager.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db::memory_manager;

memory_manager::memory_manager() : base_memory_manager()
{
    m_metadata = nullptr;

    // Sanity check.
    const size_t expectedMetadataSizeInBytes = 112;
    std::stringstream messageStream;

    messageStream
     << "Metadata information structure representation does not have the expected size on this system: "
     << sizeof(metadata) << "!";

    retail_assert(
        sizeof(metadata) == expectedMetadataSizeInBytes,
        messageStream.str());
}

void memory_manager::set_execution_flags(const execution_flags& executionFlags)
{
    m_execution_flags = executionFlags;
}

gaia::db::memory_manager::error_code memory_manager::manage(
    uint8_t* pMemoryAddress,
    size_t memorySize,
    size_t mainMemorySystemReservedSize,
    bool initialize)
{
    // Sanity checks.
    if (pMemoryAddress == nullptr || memorySize == 0)
    {
        return invalid_argument_value;
    }

    if (!validate_address_alignment(pMemoryAddress))
    {
        return memory_address_not_aligned;
    }

    if (!validate_size_alignment(memorySize))
    {
        return memory_size_not_aligned;
    }

    if (memorySize < sizeof(metadata) + mainMemorySystemReservedSize
        || sizeof(metadata) + mainMemorySystemReservedSize < mainMemorySystemReservedSize)
    {
        return insufficient_memory_size;
    }

    // Save our parameters.
    m_base_memory_address = pMemoryAddress;
    m_total_memory_size = memorySize;
    m_main_memory_system_reserved_size = mainMemorySystemReservedSize;

    // Map the metadata information for quick reference.
    m_metadata = reinterpret_cast<metadata*>(m_base_memory_address);

    // If necessary, initialize our metadata.
    if (initialize)
    {
        m_metadata->Clear(
            sizeof(metadata),
            m_total_memory_size);
    }

    if (m_execution_flags.enable_console_output)
    {
        cout << "  Configuration - mainMemorySystemReservedSize = " << m_main_memory_system_reserved_size << endl;
        cout << "  Configuration - enableExtraValidations = " << m_execution_flags.enable_extra_validations << endl;

        output_debugging_information("Manage");
    }

    // The master manager is the one that initializes the memory.
    m_is_master_manager = initialize;

    return success;
}

gaia::db::memory_manager::error_code memory_manager::allocate(
    size_t memorySize,
    ADDRESS_OFFSET& allocatedMemoryOffset) const
{
    allocatedMemoryOffset = 0;

    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    error_code errorCode = validate_size(memorySize);
    if (errorCode != success)
    {
        return errorCode;
    }

    size_t sizeToAllocate = memorySize + sizeof(memory_allocation_metadata);

    // First, attempt to reuse freed memory blocks, if possible.
    allocatedMemoryOffset = allocate_from_freed_memory(sizeToAllocate);

    // Otherwise, fall back to allocating from our main memory block.
    if (allocatedMemoryOffset == 0)
    {
        allocatedMemoryOffset = allocate_from_main_memory(sizeToAllocate);
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("Allocate");
    }

    if (allocatedMemoryOffset == 0)
    {
        return insufficient_memory_size;
    }

    return success;
}

gaia::db::memory_manager::error_code memory_manager::create_stack_allocator(
    size_t memorySize,
    stack_allocator*& pStackAllocator) const
{
    pStackAllocator = nullptr;

    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    ADDRESS_OFFSET memoryOffset = 0;
    error_code errorCode = allocate(memorySize, memoryOffset);
    if (errorCode != success)
    {
        return errorCode;
    }

    pStackAllocator = new stack_allocator();

    pStackAllocator->set_execution_flags(m_execution_flags);

    errorCode = pStackAllocator->initialize(m_base_memory_address, memoryOffset, memorySize);
    if (errorCode != success)
    {
        delete pStackAllocator;
        pStackAllocator = nullptr;
    }

    return errorCode;
}

gaia::db::memory_manager::error_code memory_manager::commit_stack_allocator(
    stack_allocator* pStackAllocator,
    SERIALIZATION_NUMBER serializationNumber) const
{
    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    if (pStackAllocator == nullptr)
    {
        return invalid_argument_value;
    }

    // Ensure that the stack allocator memory gets reclaimed.
    unique_ptr<stack_allocator> apStackAllocator(pStackAllocator);

    size_t countAllocations = pStackAllocator->get_allocation_count();

    // Get metadata record for the entire stack allocator memory block.
    memory_allocation_metadata* pFirstStackAllocationMetadata = read_allocation_metadata(pStackAllocator->m_base_memory_offset);
    ADDRESS_OFFSET firstStackAllocationMetadataOffset
        = get_offset(reinterpret_cast<uint8_t *>(pFirstStackAllocationMetadata));

    // Get metadata for the stack allocator.
    stack_allocator_metadata* pStackAllocatorMetadata = pStackAllocator->get_metadata();
    retail_assert(pStackAllocatorMetadata != nullptr, "An unexpected null metadata record was retrieved!");

    // Write serialization number.
    pStackAllocatorMetadata->serialization_number = serializationNumber;

    if (countAllocations == 0)
    {
        // Special case: all allocations have been reverted, so we need to mark the entire memory block as free.
        if (m_execution_flags.enable_extra_validations)
        {
            retail_assert(
                firstStackAllocationMetadataOffset
                == pStackAllocator->m_base_memory_offset - sizeof(memory_allocation_metadata),
                "Allocation metadata offset does not match manually computed size!");
            retail_assert(
                pFirstStackAllocationMetadata->allocation_size
                == pStackAllocator->m_total_memory_size + sizeof(memory_allocation_metadata),
                "Allocation metadata size does not match manually computed size!");
        }

        // Try to mark memory as free. This operation can only fail if we run out of memory.
        memory_record* pFreeMemoryRecord
            = get_free_memory_record(firstStackAllocationMetadataOffset, pFirstStackAllocationMetadata->allocation_size);
        if (pFreeMemoryRecord == nullptr)
        {
            return insufficient_memory_size;
        }
        else
        {
            insert_free_memory_record(pFreeMemoryRecord);
        }
    }
    else
    {
        // Iterate over all StackAllocator allocations and collect old memory offsets in free memory records.
        // However, we will not insert any of these records into the free memory list
        // until we know that our processing can no longer fail.
        unique_ptr<memory_record*[]> argFreeMemoryRecords(new memory_record*[countAllocations]());
        for (size_t allocationNumber = 1; allocationNumber <= countAllocations; allocationNumber++)
        {
            stack_allocator_allocation* pAllocationRecord = pStackAllocator->get_allocation_record(allocationNumber);
            retail_assert(pAllocationRecord != nullptr, "An unexpected null allocation record was retrieved!");

            if (pAllocationRecord->old_memory_offset != 0)
            {
                memory_allocation_metadata* pAllocationMetadata = read_allocation_metadata(pAllocationRecord->old_memory_offset);
                ADDRESS_OFFSET allocationMetadataOffset = get_offset(reinterpret_cast<uint8_t*>(pAllocationMetadata));

                // Mark memory block as free.
                // If we cannot do this, then we ran out of memory;
                // in that case we'll just reclaim all records we collected so far.
                memory_record* pFreeMemoryRecord
                    = get_free_memory_record(allocationMetadataOffset, pAllocationMetadata->allocation_size);
                if (pFreeMemoryRecord == nullptr)
                {
                    reclaim_records(argFreeMemoryRecords.get(), countAllocations);
                    return insufficient_memory_size;
                }
                else
                {
                    argFreeMemoryRecords[allocationNumber - 1] = pFreeMemoryRecord;
                }
                
            }
        }

        // Insert metadata block in the unserialized allocation list.
        // If we fail, reclaim all the records that we have collected so far.
        // But if we succeed, then we can insert all our collected records into the list of free memory records.
        error_code errorCode = track_stack_allocator_metadata_for_serialization(pStackAllocatorMetadata);
        if (errorCode != success)
        {
            reclaim_records(argFreeMemoryRecords.get(), countAllocations);
            return errorCode;
        }
        else
        {
            insert_free_memory_records(argFreeMemoryRecords.get(), countAllocations);
        }

        // Patch metadata information of the first allocation.
        // We will deallocate the unused memory after serialization,
        // because the stack allocator metadata is still needed until then
        // and is now tracked by the unserialized allocation list.
        pFirstStackAllocationMetadata->allocation_size
            = pStackAllocatorMetadata->first_allocation_size + sizeof(memory_allocation_metadata);
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("CommitStackAllocator");
        pStackAllocator->output_debugging_information("CommitStackAllocator");
    }

    return success;
}

gaia::db::memory_manager::error_code memory_manager::get_unserialized_allocations_list_head(memory_list_node*& pListHead) const
{
    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    // Serializing allocations should only be performed by the database engine
    // through its master manager instance.
    if (!m_is_master_manager)
    {
        return operation_available_only_to_master_manager;
    }

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("Getunserialized_allocations_list_head");
    }

    pListHead = &m_metadata->unserialized_allocations_list_head;

    return success;
}

gaia::db::memory_manager::error_code memory_manager::update_unserialized_allocations_list_head(
        ADDRESS_OFFSET nextUnserializedAllocationRecordOffset) const
{
    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    // We require an offset because the end of the list may not be safe to process,
    // due to other threads making insertions.
    if (nextUnserializedAllocationRecordOffset == 0)
    {
        return invalid_argument_value;
    }

    // Serializing allocations should only be performed by the database engine
    // through its master manager instance.
    if (!m_is_master_manager)
    {
        return operation_available_only_to_master_manager;
    }

    ADDRESS_OFFSET currentRecordOffset = m_metadata->unserialized_allocations_list_head.next;
    retail_assert(currentRecordOffset != 0, "Updateunserialized_allocations_list_head() was called on an empty list!");
    retail_assert(
        currentRecordOffset != nextUnserializedAllocationRecordOffset,
        "Updateunserialized_allocations_list_head() was called on an already updated list!");

    while (currentRecordOffset != nextUnserializedAllocationRecordOffset)
    {
        // Get the actual record.
        memory_record* pCurrentRecord = base_memory_manager::read_memory_record(currentRecordOffset);

        // Get the StackAllocator metadata.
        ADDRESS_OFFSET currentMetadataOffset = pCurrentRecord->memory_offset;
        uint8_t* pCurrentMetadataAddress = get_address(currentMetadataOffset);
        stack_allocator_metadata* pCurrentMetadata = reinterpret_cast<stack_allocator_metadata*>(pCurrentMetadataAddress);

        // Determine the boundaries of the memory block that we can free from the StackAllocator.
        ADDRESS_OFFSET startMemoryOffset = pCurrentMetadata->next_allocation_offset;
        ADDRESS_OFFSET endMemoryOffset = currentMetadataOffset + sizeof(stack_allocator_metadata);
        retail_assert(validate_offset(startMemoryOffset) == success, "Calculated start memory offset is invalid");
        retail_assert(validate_offset(endMemoryOffset) == success, "Calculated end memory offset is invalid");

        size_t memorySize = endMemoryOffset - startMemoryOffset;

        // Advance current record offset to next record, to prepare for the next iteration.
        currentRecordOffset = pCurrentRecord->next;

        // Also advance the list head to the next record.
        // This means that when the loop condition is reached,
        // the list head will already be updated to reference the next unserialized record.
        m_metadata->unserialized_allocations_list_head.next = currentRecordOffset;

        // We'll repurpose the current record as a free memory record,
        // so we'll update it to track the free memory block information.
        pCurrentRecord->memory_offset = startMemoryOffset;
        pCurrentRecord->memory_size = memorySize;
        pCurrentRecord->next = 0;

        // Insert the record in the free memory list.
        insert_free_memory_record(pCurrentRecord);
    }

    retail_assert(
        m_metadata->unserialized_allocations_list_head.next == nextUnserializedAllocationRecordOffset,
        "Updateunserialized_allocations_list_head() has not properly updated the list!");

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("Updateunserialized_allocations_list_head");
    }

    return success;
}

size_t memory_manager::get_main_memory_available_size(bool includeSystemReservedSize) const
{
    size_t availableSize = 0;

    // Ignore return value; we just want the available size.
    is_main_memory_exhausted(
        m_metadata->start_main_available_memory,
        m_metadata->lowest_metadata_memory_use,
        includeSystemReservedSize,
        availableSize);

    return availableSize;
}

bool memory_manager::is_main_memory_exhausted(
    ADDRESS_OFFSET startMemoryOffset,
    ADDRESS_OFFSET endMemoryOffset,
    bool includeSystemReservedSize) const
{
    size_t availableSize = 0;

    return is_main_memory_exhausted(
        startMemoryOffset,
        endMemoryOffset,
        includeSystemReservedSize,
        availableSize);
}

bool memory_manager::is_main_memory_exhausted(
    ADDRESS_OFFSET startMemoryOffset,
    ADDRESS_OFFSET endMemoryOffset,
    bool includeSystemReservedSize,
    size_t& availableSize) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    availableSize = 0;

    size_t reservedSize = includeSystemReservedSize ? 0 : m_main_memory_system_reserved_size;

    if (startMemoryOffset + reservedSize > endMemoryOffset
        || startMemoryOffset + reservedSize < startMemoryOffset)
    {
        return true;
    }

    availableSize = endMemoryOffset - startMemoryOffset - reservedSize;

    return false;
}

ADDRESS_OFFSET memory_manager::process_allocation(ADDRESS_OFFSET allocationOffset, size_t sizeToAllocate) const
{
    retail_assert(allocationOffset != 0, "ProcessAllocation() was called for an empty allocation!");

    // Write the allocation metadata.
    uint8_t* pAllocationMetadataAddress = get_address(allocationOffset);
    memory_allocation_metadata* pAllocationMetadata
        = reinterpret_cast<memory_allocation_metadata*>(pAllocationMetadataAddress);
    pAllocationMetadata->allocation_size = sizeToAllocate;

    // We return the offset past the metadata.
    allocationOffset += sizeof(memory_allocation_metadata);
    return allocationOffset;
}

ADDRESS_OFFSET memory_manager::allocate_from_main_memory(size_t sizeToAllocate) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    // Main memory allocations should not use the reserved space.
    bool includeSystemReservedSize = false;

    // If the allocation exhausts our memory, we cannot perform it.
    if (get_main_memory_available_size(includeSystemReservedSize) < sizeToAllocate)
    {
        return 0;
    }

    // Claim the space.
    ADDRESS_OFFSET oldStartMainAvailableMemory = __sync_fetch_and_add(
        &m_metadata->start_main_available_memory,
        sizeToAllocate);
    ADDRESS_OFFSET newStartMainAvailableMemory = oldStartMainAvailableMemory + sizeToAllocate;

    // Check again if our memory got exhausted by this allocation,
    // which can happen if someone else got the space before us.
    if (is_main_memory_exhausted(
        newStartMainAvailableMemory,
        m_metadata->lowest_metadata_memory_use,
        includeSystemReservedSize))
    {
        // We exhausted the main memory so we must undo our update.
        while (!__sync_bool_compare_and_swap(
            &m_metadata->start_main_available_memory,
            newStartMainAvailableMemory,
            oldStartMainAvailableMemory))
        {
            // A failure indicates that another thread has done the same.
            // Sleep to allow it to undo its change, so we can do the same.
            // This is safe because metadata allocations are never reverted
            // so all other subsequent memory allocations will also attempt to revert back.
            usleep(1);
        }

        return 0;
    }

    // Our allocation has succeeded.
    ADDRESS_OFFSET allocationOffset = oldStartMainAvailableMemory;
    ADDRESS_OFFSET adjustedAllocationOffset = process_allocation(allocationOffset, sizeToAllocate);

    if (m_execution_flags.enable_console_output)
    {
        cout << endl << "Allocated " << sizeToAllocate << " bytes at offset " << allocationOffset;
        cout << " from main memory." << endl;
    }

    return adjustedAllocationOffset;
}

ADDRESS_OFFSET memory_manager::allocate_from_freed_memory(size_t sizeToAllocate) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    iteration_context context;
    start(&m_metadata->free_memory_list_head, context);
    ADDRESS_OFFSET allocationOffset = 0;

    // Iterate over the free memory list and try to find a large enough block for this allocation.
    while (context.current_record != nullptr)
    {
        if (context.current_record->memory_size >= sizeToAllocate)
        {
            // We have 2 situations: the memory block is exactly our required size or it is larger.
            // In both cases, we need to check the size of the block again after acquiring the locks,
            // because another thread may have managed to update it before we could lock it. 
            if (context.current_record->memory_size == sizeToAllocate)
            {
                if (try_to_lock_access(context, access_lock_type::update_remove)
                    && context.auto_access_current_record.try_to_lock_access(access_lock_type::remove)
                    && context.current_record->memory_size == sizeToAllocate)
                {

                    retail_assert(
                        context.current_record->memory_size == sizeToAllocate,
                        "Internal error - we are attempting to allocate from a block that is too small!");

                    // Jolly good! We've found a memory block of exactly the size that we were looking for.
                    remove(context);

                    allocationOffset = context.current_record->memory_offset;

                    // Reclaim the memory record for later reuse;
                    insert_reclaimed_memory_record(context.current_record);

                    break;
                }
            }
            else
            {
                if (try_to_lock_access(context, access_lock_type::update)
                    && context.current_record->memory_size > sizeToAllocate)
                {
                    retail_assert(
                        context.current_record->memory_size > sizeToAllocate,
                        "Internal error - we are attempting to allocate from a block that is too small!");

                    allocationOffset = context.current_record->memory_offset;

                    // We have more space than we needed, so update the record
                    // to track the remaining space.
                    context.current_record->memory_offset += sizeToAllocate;
                    context.current_record->memory_size -= sizeToAllocate;

                    break;
                }
            }
        }

        move_next(context);
    }

    if (allocationOffset == 0)
    {
        return 0;
    }

    ADDRESS_OFFSET adjustedAllocationOffset = process_allocation(allocationOffset, sizeToAllocate);

    if (m_execution_flags.enable_console_output)
    {
        cout << endl << "Allocated " << sizeToAllocate << " bytes at offset " << allocationOffset;
        cout << " from freed memory." << endl;
    }

    return adjustedAllocationOffset;
}

memory_record* memory_manager::get_memory_record() const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    memory_record* pFreeMemoryRecord = get_reclaimed_memory_record();

    if (pFreeMemoryRecord == nullptr)
    {
        pFreeMemoryRecord = get_new_memory_record();
    }

    // The record should not indicate any access.
    retail_assert(
        pFreeMemoryRecord == nullptr
        || pFreeMemoryRecord->accessControl.readers_count == 0,
        "The readers count of a new memory record should be 0!");
    retail_assert(
        pFreeMemoryRecord == nullptr
        || pFreeMemoryRecord->accessControl.access_lock == access_lock_type::none,
        "The access lock of a new memory record should be none!");

    return pFreeMemoryRecord;
}

memory_record* memory_manager::get_new_memory_record() const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    // Metadata allocations can use the reserved space.
    bool includeSystemReservedSize = true;

    // If the allocation exhausts our memory, we cannot perform it.
    if (get_main_memory_available_size(includeSystemReservedSize) < sizeof(memory_record))
    {
        return nullptr;
    }

    // Claim the space.
    ADDRESS_OFFSET oldlowest_metadata_memory_use = __sync_fetch_and_sub(
        &m_metadata->lowest_metadata_memory_use,
        sizeof(memory_record));
    ADDRESS_OFFSET newlowest_metadata_memory_use = oldlowest_metadata_memory_use - sizeof(memory_record);

    // Check again if our memory got exhausted by this allocation,
    // which can happen if someone else got the space before us.
    if (is_main_memory_exhausted(
        m_metadata->start_main_available_memory,
        newlowest_metadata_memory_use,
        includeSystemReservedSize))
    {
        // We exhausted the main memory.
        // Attempting to revert the update to m_pMetadata->lowest_metadata_memory_use
        // can conflict with attempts to revert updates to m_pMetadata->start_main_available_memory,
        // so just leave it as is (yes, we'll potentially lose a record's chunk of memory!)
        // and fail the record allocation.
        return nullptr;
    }

    // Our allocation has succeeded.
    ADDRESS_OFFSET recordOffset = newlowest_metadata_memory_use;

    if (m_execution_flags.enable_console_output)
    {
        cout << endl << "Allocated offset " << recordOffset << " for a new memory record." << endl;
    }

    memory_record* pFreeMemoryRecord = base_memory_manager::read_memory_record(recordOffset);

    // This is uninitialized memory, so we need to explicitly clear it.
    pFreeMemoryRecord->clear();

    return pFreeMemoryRecord;
}

memory_record* memory_manager::get_reclaimed_memory_record() const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    iteration_context context;
    start(&m_metadata->reclaimed_records_list_head, context);
    memory_record* pReclaimedRecord = nullptr;

    // Iterate through the list of reclaimed records and attempt to extract one.
    while (context.current_record != nullptr)
    {
        // Retrieving a node from this list is going to be a small-effort initiative.
        // If we fail, we'll just allocate a new node, so eventually,
        // enough nodes will get inserted into this list that we'll succeed easily to remove one.
        //
        // We'll try to lock each node one after the other.
        if (try_to_lock_access(context, access_lock_type::update_remove)
            && context.auto_access_current_record.try_to_lock_access(access_lock_type::remove))
        {
            remove(context);

            pReclaimedRecord = context.current_record;

            break;
        }

        move_next(context);
    }

    if (pReclaimedRecord != nullptr
        && m_execution_flags.enable_console_output)
    {
        uint8_t* pReclaimedRecordAddress = reinterpret_cast<uint8_t*>(pReclaimedRecord);
        ADDRESS_OFFSET reclaimedRecordOffset = get_offset(pReclaimedRecordAddress);

        cout << endl << "Reclaimed memory record from offset " << reclaimedRecordOffset << "." << endl;
    }

    return pReclaimedRecord;
}

void memory_manager::insert_free_memory_record(memory_record* pFreeMemoryRecord) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");
    retail_assert(pFreeMemoryRecord != nullptr, "InsertFreeMemoryRecord() was called with a null parameter!");

    bool sortByOffset = true;
    insert_memory_record(&m_metadata->free_memory_list_head, pFreeMemoryRecord, sortByOffset);
}

void memory_manager::insert_reclaimed_memory_record(memory_record* pReclaimedMemoryRecord) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");
    retail_assert(pReclaimedMemoryRecord != nullptr, "InsertReclaimedMemoryRecord() was called with a null parameter!");

    iteration_context context;
    start(&m_metadata->reclaimed_records_list_head, context);

    // We'll keep trying to insert at the beginning of the list.
    while (true)
    {
        if (try_to_lock_access(context, access_lock_type::insert))
        {
            insert(context, pReclaimedMemoryRecord);

            return;
        }
    }
}

void memory_manager::insert_unserialized_allocations_record(memory_record* pUnserializedAllocationsRecord) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");
    retail_assert(pUnserializedAllocationsRecord != nullptr, "InsertUnserializedAllocationsRecord() was called with a null parameter!");

    // In this case we want to sort by the size value, which holds the serialization number.
    bool sortByOffset = false;
    insert_memory_record(&m_metadata->unserialized_allocations_list_head, pUnserializedAllocationsRecord, sortByOffset);
}

memory_record* memory_manager::get_free_memory_record(ADDRESS_OFFSET memoryOffset, size_t memorySize) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    memory_record* pFreeMemoryRecord = get_memory_record();

    if (pFreeMemoryRecord == nullptr)
    {
        return nullptr;
    }

    pFreeMemoryRecord->memory_offset = memoryOffset;
    pFreeMemoryRecord->memory_size = memorySize;

    return pFreeMemoryRecord;
}

void memory_manager::process_free_memory_records(memory_record** freeMemoryRecords, size_t size, bool markAsFree) const
{
    retail_assert(freeMemoryRecords != nullptr, "InsertFreeMemoryRecords() has been called with a null parameter!");

    for (size_t i = 0; i < size; i++)
    {
        if (freeMemoryRecords[i] != nullptr)
        {
            if (markAsFree)
            {
                insert_free_memory_record(freeMemoryRecords[i]);
            }
            else
            {
                insert_reclaimed_memory_record(freeMemoryRecords[i]);
            }
        }
    }
}

void memory_manager::insert_free_memory_records(memory_record** freeMemoryRecords, size_t size) const
{
    return process_free_memory_records(freeMemoryRecords, size, true);
}

void memory_manager::reclaim_records(memory_record** freeMemoryRecords, size_t size) const
{
    return process_free_memory_records(freeMemoryRecords, size, false);
}

gaia::db::memory_manager::error_code memory_manager::track_stack_allocator_metadata_for_serialization(
    stack_allocator_metadata* pStackAllocatorMetadata) const
{
    retail_assert(m_metadata != nullptr, "Memory manager has not been initialized!");

    memory_record* pMemoryRecord = get_memory_record();

    if (pMemoryRecord == nullptr)
    {
        return insufficient_memory_size;
    }

    uint8_t* pMetadataAddress = reinterpret_cast<uint8_t*>(pStackAllocatorMetadata);
    ADDRESS_OFFSET metadataOffset = get_offset(pMetadataAddress);

    pMemoryRecord->memory_offset = metadataOffset;

    // We don't need this field for its original purpose,
    // so we'll repurpose it to hold the serialization number,
    // to make it easier to check this value during list insertions.
    pMemoryRecord->memory_size = pStackAllocatorMetadata->serialization_number;

    insert_unserialized_allocations_record(pMemoryRecord);

    return success;
}

void memory_manager::output_debugging_information(const string& contextDescription) const
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "Debugging output for context: " << contextDescription << ":" << endl;

    if (m_metadata == nullptr)
    {
        cout << "  Memory manager has not been initialized." << endl;
    }

    cout << "  Main memory start = " << m_metadata->start_main_available_memory << endl;
    cout << "  Metadata start = " << m_metadata->lowest_metadata_memory_use << endl;
    cout << "  Available main memory including reserved space = " << get_main_memory_available_size(true) << endl;
    cout << "  Available main memory excluding reserved space = " << get_main_memory_available_size(false) << endl;
    cout << "  Free memory list: " << endl;
    output_list_content(m_metadata->free_memory_list_head);
    cout << "  Reclaimed records list: " << endl;
    output_list_content(m_metadata->reclaimed_records_list_head); 
    cout << "  Unserialized allocations list: " << endl;
    output_list_content(m_metadata->unserialized_allocations_list_head);
    cout << c_debug_output_separator_line_end << endl;
}

void memory_manager::output_list_content(memory_record listHead) const
{
    size_t recordCount = 0;
    ADDRESS_OFFSET currentRecordOffset = listHead.next;
    while (currentRecordOffset != 0)
    {
        recordCount++;

        memory_record* pCurrentRecord = read_memory_record(currentRecordOffset);

        cout << "    Record[" << recordCount << "] at offset " << currentRecordOffset << ":" << endl;
        cout << "      offset = " << pCurrentRecord->memory_offset;
        cout << " size = " << pCurrentRecord->memory_size;
        cout << " readersCount = " << pCurrentRecord->accessControl.readers_count;
        cout << " accessLock = " << pCurrentRecord->accessControl.access_lock;
        cout << " next = " << pCurrentRecord->next << endl;

        currentRecordOffset = pCurrentRecord->next;
    }

    cout << "    List contains " << recordCount << " records." << endl;
}
