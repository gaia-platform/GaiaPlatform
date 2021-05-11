/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

bool gaia_ptr_t::operator==(const gaia_ptr_t& other) const
{
    return m_locator == other.m_locator;
}

bool gaia_ptr_t::operator==(const std::nullptr_t) const
{
    return to_ptr() == nullptr;
}

bool gaia_ptr_t::operator!=(const std::nullptr_t) const
{
    return to_ptr() != nullptr;
}

gaia_ptr_t::operator bool() const
{
    return to_ptr() != nullptr;
}

gaia_ptr_t gaia_ptr_t::operator++()
{
    if (m_locator)
    {
        find_next(to_ptr()->type);
    }
    return *this;
}

gaia_ptr_t gaia_ptr_t::open(
    common::gaia_id_t id)
{
    return gaia_ptr_t(id);
}

bool gaia_ptr_t::is_null() const
{
    return m_locator == 0;
}

common::gaia_id_t gaia_ptr_t::id() const
{
    return to_ptr()->id;
}

common::gaia_type_t gaia_ptr_t::type() const
{
    return to_ptr()->type;
}

char* gaia_ptr_t::data() const
{
    return data_size() ? const_cast<char*>(to_ptr()->data()) : nullptr;
}

size_t gaia_ptr_t::data_size() const
{
    size_t total_len = to_ptr()->payload_size;
    size_t refs_len = to_ptr()->num_references * sizeof(common::gaia_id_t);
    size_t data_size = total_len - refs_len;
    return data_size;
}

common::gaia_id_t* gaia_ptr_t::references() const
{
    return const_cast<common::gaia_id_t*>(to_ptr()->references());
}

size_t gaia_ptr_t::num_references() const
{
    return to_ptr()->num_references;
}

bool gaia_ptr_t::is(common::gaia_type_t type) const
{
    return to_ptr() && to_ptr()->type == type;
}

// "function with deduced return type cannot be used before it is defined".
// The function must be defined in the same translation unit where it is used,
// and the only way to guarantee that for our clients is to define it in the
// header file itself.
auto gaia_ptr_t::find_all_iter(
    common::gaia_type_t type,
    std::function<bool(gaia_ptr_t)> user_predicate)
{
    // Get the gaia_id generator and wrap it in a gaia_ptr_t generator.
    std::function<std::optional<common::gaia_id_t>()> id_generator = get_id_generator_for_type(type);
    std::function<std::optional<gaia_ptr_t>()> gaia_ptr_generator = [id_generator]() -> std::optional<gaia_ptr_t> {
        std::optional<common::gaia_id_t> id_opt = id_generator();
        if (id_opt)
        {
            return gaia_ptr_t::open(*id_opt);
        }
        return std::nullopt;
    };
    // We need to construct an iterator from this generator rather than
    // directly constructing a range from the generator, because we need
    // to filter out values corresponding to deleted objects, and we can
    // do that only by supplying a predicate to the iterator.
    // REVIEW: this can filter out objects that do not exist in the client view,
    // but it cannot return objects that only exist in the client view.
    // That will require merging the client's transaction log.
    std::function<bool(gaia_ptr_t)> gaia_ptr_predicate = [user_predicate](gaia_ptr_t ptr) {
        return !ptr.is_null() && user_predicate(ptr);
    };
    auto gaia_ptr_iterator = gaia::common::iterators::generator_iterator_t(
        gaia_ptr_generator,
        gaia_ptr_predicate);
    return gaia_ptr_iterator;
}

// "function with deduced return type cannot be used before it is defined".
// The function must be defined in the same translation unit where it is used,
// and the only way to guarantee that for our clients is to define it in the
// header file itself.
auto gaia_ptr_t::find_all_range(
    common::gaia_type_t type,
    std::function<bool(gaia_ptr_t)> user_predicate)
{
    return gaia::common::iterators::range(find_all_iter(type, user_predicate));
}
