/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "constants.hpp"
#include "retail_assert.hpp"
#include "stack_allocator.hpp"

using namespace gaia::common;
using namespace gaia::db::memory_manager;

CStackAllocator::CStackAllocator() : CBaseMemoryManager()
{
    m_metadata = nullptr;
}

void CStackAllocator::set_execution_flags(const ExecutionFlags& executionFlags)
{
    m_execution_flags = executionFlags;
}

EMemoryManagerErrorCode CStackAllocator::initialize(
    uint8_t* pBaseMemoryAddress,
    ADDRESS_OFFSET memoryOffset,
    size_t memorySize)
{
    if (pBaseMemoryAddress == nullptr || memoryOffset == 0 || memorySize == 0)
    {
        return invalid_argument_value;
    }

    if (!validate_address_alignment(pBaseMemoryAddress))
    {
        return memory_address_not_aligned;
    }

    if (!validate_offset_alignment(memoryOffset))
    {
        return memory_offset_not_aligned;
    }

    if (!validate_size_alignment(memorySize))
    {
        return memory_size_not_aligned;
    }

    if (memorySize < sizeof(StackAllocatorMetadata))
    {
        return insufficient_memory_size;
    }

    // Save our parameters.
    m_base_memory_address = pBaseMemoryAddress;
    m_base_memory_offset = memoryOffset;
    m_total_memory_size = memorySize;   

    // Map the metadata information for quick reference.
    uint8_t* pMetadataAddress
        = m_base_memory_address + m_base_memory_offset + m_total_memory_size - sizeof(StackAllocatorMetadata);
    m_metadata = reinterpret_cast<StackAllocatorMetadata*>(pMetadataAddress);
    m_metadata->clear();
    m_metadata->next_allocation_offset = m_base_memory_offset;

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("Initialize");
    }

    return success;
}

EMemoryManagerErrorCode CStackAllocator::allocate(
    SLOT_ID slotId,
    ADDRESS_OFFSET oldSlotOffset,
    size_t memorySize,
    ADDRESS_OFFSET& allocatedMemoryOffset) const
{
    allocatedMemoryOffset = 0;

    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    // A memorySize of 0 indicates a deletion - handle specially.
    EMemoryManagerErrorCode errorCode = validate_size(memorySize);
    if (memorySize != 0 && errorCode != success)
    {
        return errorCode;
    }

    if (!validate_offset_alignment(oldSlotOffset))
    {
        return memory_offset_not_aligned;
    }

    // For all but the first allocation, we will prefix a memory allocation metadata block.
    // The first allocation will eventually reuse the memory allocation metadata block
    // that is currently tracking the entire memory of the StackAllocator.
    size_t sizeToAllocate = memorySize;
    size_t countAllocations = m_metadata->count_allocations;
    if (sizeToAllocate > 0 && countAllocations > 0)
    {
        sizeToAllocate += sizeof(MemoryAllocationMetadata);
    }

    ADDRESS_OFFSET nextAllocationOffset = m_metadata->next_allocation_offset;
    ADDRESS_OFFSET metadataOffset = get_offset(reinterpret_cast<uint8_t*>(m_metadata));

    // Now we can do a more accurate check about whether this allocation fits in our memory.
    if (nextAllocationOffset + sizeToAllocate
        > metadataOffset - (countAllocations + 1) * sizeof(StackAllocatorAllocation))
    {
        return insufficient_memory_size;
    }

    if (sizeToAllocate > 0)
    {
        // Perform allocation.
        if (countAllocations == 0)
        {
            m_metadata->first_allocation_size = sizeToAllocate;
        }
        else
        {
            uint8_t* pNextAllocationAddress = get_address(nextAllocationOffset);
            MemoryAllocationMetadata* pNextAllocationMetadata
                = reinterpret_cast<MemoryAllocationMetadata*>(pNextAllocationAddress);
            pNextAllocationMetadata->allocation_size = sizeToAllocate;
        }
    }

    m_metadata->count_allocations++;

    // Record allocation information.
    StackAllocatorAllocation* pAllocationRecord = get_allocation_record(m_metadata->count_allocations);
    pAllocationRecord->clear();

    pAllocationRecord->slotId = slotId;
    pAllocationRecord->old_memory_offset = oldSlotOffset;

    // Set allocation offset.
    if (sizeToAllocate > 0)
    {
        if (countAllocations == 0)
        {
            pAllocationRecord->memory_offset = nextAllocationOffset;
            retail_assert(
                nextAllocationOffset == m_base_memory_offset,
                "For first allocation, the computed offset should be the base offset.");
        }
        else
        {
            pAllocationRecord->memory_offset = nextAllocationOffset + sizeof(MemoryAllocationMetadata);
        }
    }

    m_metadata->next_allocation_offset += sizeToAllocate;

    if (m_execution_flags.enable_console_output)
    {
        string context = (sizeToAllocate > 0) ? "Allocate" : "Delete";
        output_debugging_information(context);
    }

    allocatedMemoryOffset = pAllocationRecord->memory_offset;
    
    return success;
}

EMemoryManagerErrorCode CStackAllocator::deallocate(SLOT_ID slotId, ADDRESS_OFFSET slotOffset) const
{
    ADDRESS_OFFSET allocatedOffset;

    EMemoryManagerErrorCode errorCode = allocate(slotId, slotOffset, 0, allocatedOffset);

    retail_assert(allocatedOffset == 0, "Allocate(0) should have returned a 0 offset!");

    return errorCode;
}

EMemoryManagerErrorCode CStackAllocator::deallocate(size_t countAllocationsToKeep) const
{
    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    if (countAllocationsToKeep > m_metadata->count_allocations)
    {
        return allocation_count_too_large;
    }

    m_metadata->count_allocations = countAllocationsToKeep;

    // Reset first allocation size if that was deallocated as well.
    if (countAllocationsToKeep == 0)
    {
        m_metadata->first_allocation_size = 0;
    }

    // Recalculate the next offset at which we can allocate memory.
    m_metadata->next_allocation_offset = get_next_allocation_offset();

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("Deallocate");
    }

    return success;
}

StackAllocatorMetadata* CStackAllocator::get_metadata() const
{
    return m_metadata;
}

size_t CStackAllocator::get_allocation_count() const
{
    if (m_metadata == nullptr)
    {
        return 0;
    }

    return m_metadata->count_allocations;
}

StackAllocatorAllocation* CStackAllocator::get_allocation_record(size_t allocationNumber) const
{
    if (m_metadata == nullptr)
    {
        return nullptr;
    }

    if (allocationNumber == 0 || allocationNumber > m_metadata->count_allocations)
    {
        return nullptr;
    }

    ADDRESS_OFFSET metadataOffset = get_offset(reinterpret_cast<uint8_t*>(m_metadata));
    ADDRESS_OFFSET allocationRecordOffset = metadataOffset - allocationNumber * sizeof(StackAllocatorAllocation);
    uint8_t* pAllocationRecordAddress = get_address(allocationRecordOffset);
    StackAllocatorAllocation* pAllocationRecord = reinterpret_cast<StackAllocatorAllocation*>(pAllocationRecordAddress);
    return pAllocationRecord;
}

ADDRESS_OFFSET CStackAllocator::get_next_allocation_offset() const
{
    if (m_metadata == nullptr)
    {
        return 0;
    }

    size_t countAllocations = m_metadata->count_allocations;

    // Iterate over our allocation entries
    // and find out the offset at which we can next allocate memory.
    // We skip the first record because its size is stored differently.
    ADDRESS_OFFSET nextAllocationOffset = m_base_memory_offset + m_metadata->first_allocation_size;
    for (size_t allocationNumber = 2; allocationNumber <= countAllocations; allocationNumber++)
    {
        // Get allocation information object.
        StackAllocatorAllocation* pAllocationRecord = get_allocation_record(allocationNumber);

        // Get actual allocation offset.
        ADDRESS_OFFSET allocationOffset = pAllocationRecord->memory_offset;

        // Skip allocation records that indicate deletes.
        if (allocationOffset == 0)
        {
            continue;
        }

        // Get allocation prefix pointer, to get the allocation size.
        MemoryAllocationMetadata* pAllocationMetadata = read_allocation_metadata(allocationOffset);

        // Add allocation size.
        nextAllocationOffset += pAllocationMetadata->allocation_size;
    }

    return nextAllocationOffset;
}

void CStackAllocator::output_debugging_information(const string& contextDescription) const
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "  Stack allocator information for context: " << contextDescription << ":" << endl;

    if (m_metadata == nullptr)
    {
        cout << "    Stack allocator has not been initialized." << endl;
    }

    cout << "    Count allocations = " << m_metadata->count_allocations << endl;
    cout << "    First allocation size = " << m_metadata->first_allocation_size << endl;
    cout << "    Next allocation offset = " << m_metadata->next_allocation_offset << endl;
    cout << "    Serialization number = " << m_metadata->serialization_number << endl;
    cout << c_debug_output_separator_line_end << endl;
}
