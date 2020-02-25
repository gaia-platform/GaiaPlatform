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

stack_allocator_t::stack_allocator_t() : base_memory_manager_t()
{
    m_metadata = nullptr;
}

void stack_allocator_t::set_execution_flags(const execution_flags_t& executionFlags)
{
    m_execution_flags = executionFlags;
}

gaia::db::memory_manager::error_code_t stack_allocator_t::initialize(
    uint8_t* pBaseMemoryAddress,
    address_offset_t memoryOffset,
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

    if (memorySize < sizeof(stack_allocator_metadata_t))
    {
        return insufficient_memory_size;
    }

    // Save our parameters.
    m_base_memory_address = pBaseMemoryAddress;
    m_base_memory_offset = memoryOffset;
    m_total_memory_size = memorySize;   

    // Map the metadata information for quick reference.
    uint8_t* pMetadataAddress
        = m_base_memory_address + m_base_memory_offset + m_total_memory_size - sizeof(stack_allocator_metadata_t);
    m_metadata = reinterpret_cast<stack_allocator_metadata_t*>(pMetadataAddress);
    m_metadata->clear();
    m_metadata->next_allocation_offset = m_base_memory_offset;

    if (m_execution_flags.enable_console_output)
    {
        output_debugging_information("Initialize");
    }

    return success;
}

gaia::db::memory_manager::error_code_t stack_allocator_t::allocate(
    slot_id_t slotId,
    address_offset_t oldSlotOffset,
    size_t memorySize,
    address_offset_t& allocatedMemoryOffset) const
{
    allocatedMemoryOffset = 0;

    if (m_metadata == nullptr)
    {
        return not_initialized;
    }

    // A memorySize of 0 indicates a deletion - handle specially.
    error_code_t errorCode = validate_size(memorySize);
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
        sizeToAllocate += sizeof(memory_allocation_metadata_t);
    }

    address_offset_t nextAllocationOffset = m_metadata->next_allocation_offset;
    address_offset_t metadataOffset = get_offset(reinterpret_cast<uint8_t*>(m_metadata));

    // Now we can do a more accurate check about whether this allocation fits in our memory.
    if (nextAllocationOffset + sizeToAllocate
        > metadataOffset - (countAllocations + 1) * sizeof(stack_allocator_allocation_t))
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
            memory_allocation_metadata_t* pNextAllocationMetadata
                = reinterpret_cast<memory_allocation_metadata_t*>(pNextAllocationAddress);
            pNextAllocationMetadata->allocation_size = sizeToAllocate;
        }
    }

    m_metadata->count_allocations++;

    // Record allocation information.
    stack_allocator_allocation_t* pAllocationRecord = get_allocation_record(m_metadata->count_allocations);
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
            pAllocationRecord->memory_offset = nextAllocationOffset + sizeof(memory_allocation_metadata_t);
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

gaia::db::memory_manager::error_code_t stack_allocator_t::deallocate(slot_id_t slotId, address_offset_t slotOffset) const
{
    address_offset_t allocatedOffset;

    error_code_t errorCode = allocate(slotId, slotOffset, 0, allocatedOffset);

    retail_assert(allocatedOffset == 0, "Allocate(0) should have returned a 0 offset!");

    return errorCode;
}

gaia::db::memory_manager::error_code_t stack_allocator_t::deallocate(size_t countAllocationsToKeep) const
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

stack_allocator_metadata_t* stack_allocator_t::get_metadata() const
{
    return m_metadata;
}

size_t stack_allocator_t::get_allocation_count() const
{
    if (m_metadata == nullptr)
    {
        return 0;
    }

    return m_metadata->count_allocations;
}

stack_allocator_allocation_t* stack_allocator_t::get_allocation_record(size_t allocationNumber) const
{
    if (m_metadata == nullptr)
    {
        return nullptr;
    }

    if (allocationNumber == 0 || allocationNumber > m_metadata->count_allocations)
    {
        return nullptr;
    }

    address_offset_t metadataOffset = get_offset(reinterpret_cast<uint8_t*>(m_metadata));
    address_offset_t allocationRecordOffset = metadataOffset - allocationNumber * sizeof(stack_allocator_allocation_t);
    uint8_t* pAllocationRecordAddress = get_address(allocationRecordOffset);
    stack_allocator_allocation_t* pAllocationRecord = reinterpret_cast<stack_allocator_allocation_t*>(pAllocationRecordAddress);
    return pAllocationRecord;
}

address_offset_t stack_allocator_t::get_next_allocation_offset() const
{
    if (m_metadata == nullptr)
    {
        return 0;
    }

    size_t countAllocations = m_metadata->count_allocations;

    // Iterate over our allocation entries
    // and find out the offset at which we can next allocate memory.
    // We skip the first record because its size is stored differently.
    address_offset_t nextAllocationOffset = m_base_memory_offset + m_metadata->first_allocation_size;
    for (size_t allocationNumber = 2; allocationNumber <= countAllocations; allocationNumber++)
    {
        // Get allocation information object.
        stack_allocator_allocation_t* pAllocationRecord = get_allocation_record(allocationNumber);

        // Get actual allocation offset.
        address_offset_t allocationOffset = pAllocationRecord->memory_offset;

        // Skip allocation records that indicate deletes.
        if (allocationOffset == 0)
        {
            continue;
        }

        // Get allocation prefix pointer, to get the allocation size.
        memory_allocation_metadata_t* pAllocationMetadata = read_allocation_metadata(allocationOffset);

        // Add allocation size.
        nextAllocationOffset += pAllocationMetadata->allocation_size;
    }

    return nextAllocationOffset;
}

void stack_allocator_t::output_debugging_information(const string& contextDescription) const
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
