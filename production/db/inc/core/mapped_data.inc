/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename T_data>
void core_mapped_data_t<T_data>::clear()
{
    m_is_set = false;
    m_fd = -1;
    m_data = nullptr;
    m_mapped_data_size = 0;
}

template <typename T_data>
void core_mapped_data_t<T_data>::reset(core_mapped_data_t<T_data>& other)
{
    ASSERT_PRECONDITION(
        !m_is_set,
        "A set core_mapped_data_t instance should not take ownership of another instance!");
    ASSERT_PRECONDITION(
        other.m_is_set,
        "An unset core_mapped_data_t instance should not take ownership of another unset instance!");

    m_is_set = other.m_is_set;
    m_fd = other.m_fd;
    m_data = other.m_data;
    m_mapped_data_size = other.m_mapped_data_size;

    other.clear();
}

template <typename T_data>
void core_mapped_data_t<T_data>::close()
{
    close_internal();
}

template <typename T_data>
void core_mapped_data_t<T_data>::close_internal()
{
    common::unmap_fd_data(m_data, m_mapped_data_size);
    m_mapped_data_size = 0;

    common::close_fd(m_fd);

    m_is_set = false;
}

template <typename T_data>
T_data* core_mapped_data_t<T_data>::data()
{
    return m_data;
}

template <typename T_data>
int core_mapped_data_t<T_data>::fd()
{
    return m_fd;
}

template <typename T_data>
bool core_mapped_data_t<T_data>::is_set()
{
    return m_is_set;
}

template <typename T_data>
void mapped_data_t<T_data>::create(const char* name)
{
    ASSERT_PRECONDITION(
        !this->m_is_set,
        "Calling create() on an already set mapped_data_t instance!");

    this->m_fd = ::memfd_create(name, MFD_ALLOW_SEALING);
    if (this->m_fd == -1)
    {
        common::throw_system_error("memfd_create() failed in mapped_data_t::create()!");
    }

    this->m_mapped_data_size = sizeof(T_data);

    common::truncate_fd(this->m_fd, this->m_mapped_data_size);

    // Note that unless we supply the MAP_POPULATE flag to mmap(), only the
    // pages we actually use will ever be allocated. However, Linux may refuse
    // to allocate the virtual memory we requested if overcommit is disabled
    // (i.e., /proc/sys/vm/overcommit_memory = 2). Using MAP_NORESERVE (don't
    // reserve swap space) should ensure that overcommit always succeeds, unless
    // it is disabled. We may need to document the requirement that overcommit
    // is not disabled (i.e., /proc/sys/vm/overcommit_memory != 2).
    //
    // Alternatively, we could use the more tedious but reliable approach of
    // using mmap(PROT_NONE) and calling mprotect(PROT_READ|PROT_WRITE) on any
    // pages we need to use (this is analogous to VirtualAlloc(MEM_RESERVE)
    // followed by VirtualAlloc(MEM_COMMIT) in Windows).
    common::map_fd_data(
        this->m_data,
        this->m_mapped_data_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_NORESERVE,
        this->m_fd,
        0);

    this->m_is_set = true;
}

template <typename T_data>
void mapped_data_t<T_data>::open(int fd, bool manage_fd, bool is_shared)
{
    ASSERT_PRECONDITION(
        !this->m_is_set,
        "Calling open() on an already set mapped_data_t instance!");

    ASSERT_PRECONDITION(fd != -1, "mapped_data_t::open() was called with an invalid fd!");

    this->m_mapped_data_size = sizeof(T_data);

    if (manage_fd)
    {
        this->m_fd = fd;

        common::map_fd_data(
            this->m_data,
            this->m_mapped_data_size,
            PROT_READ | PROT_WRITE,
            (is_shared ? MAP_SHARED : MAP_PRIVATE) | MAP_NORESERVE,
            this->m_fd,
            0);

        common::close_fd(this->m_fd);
    }
    else
    {
        common::map_fd_data(
            this->m_data,
            this->m_mapped_data_size,
            PROT_READ | PROT_WRITE,
            (is_shared ? MAP_SHARED : MAP_PRIVATE) | MAP_NORESERVE,
            fd,
            0);
    }

    this->m_is_set = true;
}

template <typename T_data>
void mapped_data_t<T_data>::open_shared(int fd)
{
    bool manage_fd = false;
    bool is_shared = true;
    open(fd, manage_fd, is_shared);
}

void data_mapping_t::create(const char* server_instance_name) const
{
    ASSERT_PRECONDITION(
        server_instance_name != nullptr, "data_mapping_t::create() was called without a valid server instance name!");
    ASSERT_PRECONDITION(name_prefix != nullptr, "data_mapping_t::create() was called without a valid name prefix!");

    std::string mapping_name = std::string(name_prefix) + server_instance_name;
    mapped_data->create(mapping_name.c_str());
}

void data_mapping_t::open(int fd) const
{
    mapped_data->open_shared(fd);
}

void data_mapping_t::close() const
{
    mapped_data->close();
}

int data_mapping_t::fd() const
{
    return mapped_data->fd();
}

bool data_mapping_t::is_set() const
{
    return mapped_data->is_set();
}

void data_mapping_t::validate(
    const data_mapping_t mappings[], size_t count_mappings)
{
    ASSERT_INVARIANT(
        count_mappings == static_cast<size_t>(index_t::count_mappings),
        "Mappings count does not match expected value!");

    for (size_t i = 0; i < count_mappings; ++i)
    {
        ASSERT_INVARIANT(
            i == static_cast<size_t>(mappings[i].mapping_index),
            "Mapping definitions are not in the expected order!");
    }
}

void data_mapping_t::create(
    const data_mapping_t mappings[], const char* server_instance_name)
{
    size_t count_mappings = static_cast<size_t>(index_t::count_mappings);
    for (size_t i = 0; i < count_mappings; ++i)
    {
        mappings[i].create(server_instance_name);
    }
}

void data_mapping_t::close(
    const data_mapping_t mappings[])
{
    size_t count_mappings = static_cast<size_t>(index_t::count_mappings);
    for (size_t i = 0; i < count_mappings; ++i)
    {
        mappings[i].close();
    }
}

void data_mapping_t::collect_fds(
    const data_mapping_t mappings[], int fd_list[])
{
    size_t count_mappings = static_cast<size_t>(index_t::count_mappings);
    for (size_t i = 0; i < count_mappings; ++i)
    {
        fd_list[i] = mappings[i].fd();
    }
}
