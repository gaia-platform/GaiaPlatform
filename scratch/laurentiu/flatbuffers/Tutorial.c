/////////////////////////////////////////////
// This code is based on the flatbuffers tutorial
// at: https://google.github.io/flatbuffers/flatbuffers_guide_tutorial.html
/////////////////////////////////////////////

#include <stdio.h>

#include "generated/monster_builder.h"

// Convenient namespace macro to manage long namespace prefix.
#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(Flatbuffers_Monster, x)

// A helper to simplify creating vectors from C-arrays.
#define c_vec_len(V) (sizeof(V)/sizeof((V)[0]))

void RunTutorialCode();

int main(void)
{
    RunTutorialCode();

    return 0;
}

void SerializeEntity(uint8_t** buf, size_t* size)
{
    flatcc_builder_t builder, *B;
    B = &builder;

    // Initialize the builder object.
    flatcc_builder_init(B);

    flatbuffers_string_ref_t weapon_one_name = flatbuffers_string_create_str(B, "Sword");
    uint16_t weapon_one_damage = 3;
    flatbuffers_string_ref_t weapon_two_name = flatbuffers_string_create_str(B, "Axe");
    uint16_t weapon_two_damage = 5;
    ns(Weapon_ref_t) sword = ns(Weapon_create(B, weapon_one_name, weapon_one_damage));
    ns(Weapon_ref_t) axe = ns(Weapon_create(B, weapon_two_name, weapon_two_damage));

    // Serialize a name for our monster, called "Orc".
    // The _str suffix indicates the source is an ascii-z string.
    flatbuffers_string_ref_t name = flatbuffers_string_create_str(B, "Orc");

    // Create a `vector` representing the inventory of the Orc. Each number
    // could correspond to an item that can be claimed after he is slain.
    uint8_t treasure[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    flatbuffers_uint8_vec_ref_t inventory;
    // `c_vec_len` is the convenience macro we defined earlier.
    inventory = flatbuffers_uint8_vec_create(B, treasure, c_vec_len(treasure));

    // We use the internal builder stack to implement a dynamic vector.
    ns(Weapon_vec_start(B));
    ns(Weapon_vec_push(B, axe));
    ns(Weapon_vec_push(B, sword));
    ns(Weapon_vec_ref_t) weapons = ns(Weapon_vec_end(B));

    // Set his hit points to 300 and his mana to 150.
    uint16_t hp = 300;
    uint16_t mana = 150;

    // Define an equipment union. `create` calls in C has a single
    // argument for unions where C++ has both a type and a data argument.
    ns(Equipment_union_ref_t) equipped = ns(Equipment_as_Weapon(axe));

    ns(Vec3_t) pos = { 1.0f, 2.0f, 3.0f };
    ns(Vec3_t) pos2 = { 4.0f, 5.0f, 6.0f };

    // Build path
    ns(Vec3_vec_start(B));
    ns(Vec3_vec_push(B, &pos));
    ns(Vec3_vec_push(B, &pos2));
    ns(Vec3_vec_ref_t) path = ns(Vec3_vec_end(B));

    ns(Monster_create_as_root(
        B,
        &pos,
        mana,
        hp,
        name,
        inventory,
        ns(Color_Red),
        weapons,
        equipped,
        path));

    // Allocate and extract a readable buffer from internal builder heap.
    // The returned buffer must be deallocated using `free`.
    // NOTE: Finalizing the buffer does NOT change the builder, it
    // just creates a snapshot of the builder content.
    *buf = flatcc_builder_finalize_buffer(B, size);

    printf("Object was serialized to address: %x.\n", (uint)*buf);

    // Optionally reset builder to reuse builder without deallocating
    // internal stack and heap.
    flatcc_builder_reset(B);
    // build next buffer.
    // ...
    // Cleanup.
    flatcc_builder_clear(B);
}

void DeserializeEntity(uint8_t* buf)
{
    printf("Processing data at address: %x...\n\n", (uint)buf);

    // Note that we use the `table_t` suffix when reading a table object
    // as opposed to the `ref_t` suffix used during the construction of
    // the buffer.
    ns(Monster_table_t) monster = ns(Monster_as_root(buf));
    // Note: root object pointers are NOT the same as the `buffer` pointer.

    printf("Monster object address is: %x.\n", (uint)monster);

    uint16_t hp = ns(Monster_hp(monster));
    uint16_t mana = ns(Monster_mana(monster));
    flatbuffers_string_t name = ns(Monster_name(monster));

    printf("  HP = %d, Mana = %d, Name = %s.\n", hp, mana, name);

    ns(Vec3_struct_t) pos = ns(Monster_pos(monster));
    float x = ns(Vec3_x(pos));
    float y = ns(Vec3_y(pos));
    float z = ns(Vec3_z(pos));

    printf("  Position: X = %f, Y = %f, Z = %f.\n", x, y, z);

    // If `inv` hasn't been set, it will be null. It is valid get
    // the length of null which will be 0, useful for iteration.
    flatbuffers_uint8_vec_t inventory = ns(Monster_inventory(monster));
    size_t inv_len = flatbuffers_uint8_vec_len(inventory);

    printf("  Inventory length: %d.\n", (int)inv_len);

    char fifth_inventory_item = flatbuffers_uint8_vec_at(inventory, 4);

    printf("    Fifth inventory item: %d.\n", (int)fifth_inventory_item);

    ns(Weapon_vec_t) weapons = ns(Monster_weapons(monster));
    size_t weapons_len = ns(Weapon_vec_len(weapons));

    printf("  Weapons list length: %d.\n", (int)weapons_len);

    // We can use `const char *` instead of `flatbuffers_string_t`.
    const char *second_weapon_name = ns(Weapon_name(ns(Weapon_vec_at(weapons, 1))));
    uint16_t second_weapon_damage =  ns(Weapon_damage(ns(Weapon_vec_at(weapons, 1))));

    printf("    Second weapon: Name = %s, Damage = %d.\n", second_weapon_name, second_weapon_damage);

    // Access union type field.
    if (ns(Monster_equipped_type(monster)) == ns(Equipment_Weapon))
    {
        // Cast to appropriate type:
        // C allows for silent void pointer assignment, so we need no explicit cast.
        ns(Weapon_table_t) weapon = ns(Monster_equipped(monster));
        const char *weapon_name = ns(Weapon_name(weapon)); // "Axe"
        uint16_t weapon_damage = ns(Weapon_damage(weapon)); // 5

        printf("  Equipped weapon: Name = %s, Damage = %d.\n", weapon_name, weapon_damage);
    }

    ns(Vec3_vec_t) path = ns(Monster_path(monster));
    size_t path_len = ns(Vec3_vec_len(path));

    printf("  Path length: %d.\n", (int)path_len);
    ns(Vec3_struct_t) second_path_point = ns(Vec3_vec_at(path, 1));

    x = ns(Vec3_x(second_path_point));
    y = ns(Vec3_y(second_path_point));
    z = ns(Vec3_z(second_path_point));

    printf("    Second path position: X = %f, Y = %f, Z = %f.\n", x, y, z);
}

void RunTutorialCode()
{
    uint8_t* buf = 0;
    size_t size = 0;

    SerializeEntity(&buf, &size);

    DeserializeEntity(buf);

    // Release memory allocated for buf.
    free(buf);
}
