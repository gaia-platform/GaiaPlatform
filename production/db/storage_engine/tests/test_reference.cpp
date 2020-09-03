/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_types.hpp"
#include "gtest/gtest.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;

constexpr gaia_type_t TYPE0 = 0;
constexpr gaia_type_t TYPE1 = 1;
constexpr gaia_type_t NON_EXISTENT_TYPE = 1001;

constexpr relation_offset_t PARENT_FIRST_CHILD_OFFSET = 0;
constexpr relation_offset_t CHILD_NEXT_CHILD_OFFSET = 0;
constexpr relation_offset_t CHILD_PARENT_OFFSET = 1;

/**
 * Facilitate the creation of relation objects and their insertion into
 * the registry.
 */
class relation_builder_t {
  public:
    static relation_builder_t one_to_one() {
        auto metadata = relation_builder_t();
        metadata.m_cardinality = cardinality_t::one;
        return metadata;
    }

    static relation_builder_t one_to_many() {
        auto metadata = relation_builder_t();
        metadata.m_cardinality = cardinality_t::many;
        return metadata;
    }

    relation_builder_t parent(gaia_type_t parent) {
        this->m_parent_type = parent;
        return *this;
    }

    relation_builder_t child(gaia_type_t child) {
        this->m_child_type = child;
        return *this;
    }

    // does not use the singleton instance to avoid strange test
    // behaviors
    void add_to_registry(type_registry_t &registry) {
        if (m_parent_type == INVALID_GAIA_TYPE) {
            throw invalid_argument("parent_type must be set");
        }

        if (m_child_type == INVALID_GAIA_TYPE) {
            throw invalid_argument("child_type must be set");
        }

        auto rel = make_shared<relation_t>(relation_t{
            .parent_type = this->m_parent_type,
            .child_type = this->m_child_type,
            .first_child = this->m_first_child,
            .next_child = this->m_next_child,
            .parent = this->m_parent,
            .cardinality = this->m_cardinality,
            .modality = this->m_modality});

        auto parent_meta = make_unique<type_metadata_t>(m_parent_type);
        parent_meta->add_parent_relation(m_first_child, rel);

        auto child_meta = make_unique<type_metadata_t>(m_child_type);
        child_meta->add_child_relation(m_next_child, rel);

        registry.add_metadata(m_parent_type, std::move(parent_meta));
        registry.add_metadata(m_child_type, std::move(child_meta));
    }

  private:
    relation_builder_t() = default;

    // mandatory values
    cardinality_t m_cardinality = cardinality_t::one;
    gaia_type_t m_parent_type = INVALID_GAIA_TYPE;
    gaia_type_t m_child_type = INVALID_GAIA_TYPE;

    // default values add methods for those.
    modality_t m_modality = modality_t::zero;
    relation_offset_t m_first_child = PARENT_FIRST_CHILD_OFFSET;
    relation_offset_t m_next_child = CHILD_NEXT_CHILD_OFFSET;
    relation_offset_t m_parent = CHILD_PARENT_OFFSET;
};

TEST(storage, registry_throws_exception_on_nonexistent_type) {
    type_registry_t test_registry;

    EXPECT_THROW(test_registry.get_metadata(NON_EXISTENT_TYPE), metadata_not_found);
}

TEST(storage, registry_throws_exception_on_duplicated_metadata) {
    type_registry_t test_registry;

    test_registry.add_metadata(TYPE0, make_unique<type_metadata_t>(TYPE0));

    EXPECT_THROW(test_registry.add_metadata(TYPE0, make_unique<type_metadata_t>(TYPE0)), duplicated_metadata);
}

TEST(storage, one_to_many_relation) {
    type_registry_t test_registry;

    relation_builder_t::one_to_many()
        .parent(TYPE0)
        .child(TYPE1)
        .add_to_registry(test_registry);

    auto parent = test_registry.get_metadata(TYPE0);
    auto child = test_registry.get_metadata(TYPE1);

    ASSERT_EQ(parent.get_type(), TYPE0);
    ASSERT_EQ(child.get_type(), TYPE1);

    auto parent_rel_opt = parent.find_parent_relation(PARENT_FIRST_CHILD_OFFSET);
    ASSERT_TRUE(parent_rel_opt.has_value());
    auto parent_rel = parent_rel_opt.value();

    auto child_rel_opt = child.find_child_relation(CHILD_NEXT_CHILD_OFFSET);
    ASSERT_TRUE(child_rel_opt.has_value());
    auto child_rel = child_rel_opt.value();

    // parent and child should be sharing the same relation
    // TODO these 2 should be the same but are somewhat different.
    // ASSERT_EQ(&parent_rel, &child_rel);

    ASSERT_EQ(parent_rel.parent_type, TYPE0);
    ASSERT_EQ(parent_rel.child_type, TYPE1);
    ASSERT_EQ(parent_rel.first_child, PARENT_FIRST_CHILD_OFFSET);
    ASSERT_EQ(parent_rel.next_child, CHILD_NEXT_CHILD_OFFSET);
    ASSERT_EQ(parent_rel.parent, CHILD_PARENT_OFFSET);
    ASSERT_EQ(parent_rel.modality, modality_t::zero);
    ASSERT_EQ(parent_rel.cardinality, cardinality_t::many);
}

TEST(storage, one_to_one) {
    type_registry_t test_registry;

    relation_builder_t::one_to_one()
        .parent(TYPE0)
        .child(TYPE1)
        .add_to_registry(test_registry);

    auto parent = test_registry.get_metadata(TYPE0);
    auto child = test_registry.get_metadata(TYPE1);

    ASSERT_EQ(parent.get_type(), TYPE0);
    ASSERT_EQ(child.get_type(), TYPE1);

    auto parent_rel_opt = parent.find_parent_relation(PARENT_FIRST_CHILD_OFFSET);
    ASSERT_TRUE(parent_rel_opt.has_value());
    auto parent_rel = parent_rel_opt.value();

    auto child_rel_opt = child.find_child_relation(CHILD_NEXT_CHILD_OFFSET);
    ASSERT_TRUE(child_rel_opt.has_value());
    auto child_rel = child_rel_opt.value();

    // parent and child should be sharing the same relation
    // TODO these 2 should be the same but are somewhat different.
    // ASSERT_EQ(&parent_rel, &child_rel);

    ASSERT_EQ(parent_rel.parent_type, TYPE0);
    ASSERT_EQ(parent_rel.child_type, TYPE1);
    ASSERT_EQ(parent_rel.first_child, PARENT_FIRST_CHILD_OFFSET);
    ASSERT_EQ(parent_rel.next_child, CHILD_NEXT_CHILD_OFFSET);
    ASSERT_EQ(parent_rel.parent, CHILD_PARENT_OFFSET);
    ASSERT_EQ(parent_rel.modality, modality_t::zero);
    ASSERT_EQ(parent_rel.cardinality, cardinality_t::one);
}