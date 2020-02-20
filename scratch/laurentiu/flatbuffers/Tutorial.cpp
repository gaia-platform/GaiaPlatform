/////////////////////////////////////////////
// This code is based on the flatbuffers tutorial
// at: https://google.github.io/flatbuffers/flatbuffers_guide_tutorial.html
/////////////////////////////////////////////

#include "Utils.h"
#include "MonsterHelper.h"

#include <iostream>

using namespace std;
using namespace flatbuffers;
using namespace Flatbuffers::Monster;

void RunTutorialCode();

int main(void)
{
    RunTutorialCode();

    return 0;
}

void SerializeEntity(uint8_t*& buf, int& size)
{
    cout << endl << ">>> Part 1: Create and serialize a monster object. <<<" << endl << endl;

    // Create a `FlatBufferBuilder`, which will be used to create our
    // monsters' FlatBuffers.
    FlatBufferBuilder builder(1024);

    builder.ForceDefaults(true);

    // Create the position struct
    auto position = Vec3(1.0f, 2.0f, 3.0f);
    // Set his hit points to 300 and his mana to 150.
    int mana = 150;
    int hp = 300;
    // Serialize a name for our monster, called "Orc".
    auto name = builder.CreateString("Orc                 ");

    // Create a `vector` representing the inventory of the Orc. Each number
    // could correspond to an item that can be claimed after he is slain.
    unsigned char treasure[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto inventory = builder.CreateVector(treasure, 10);

    cout << "Created orc inventory: @" << inventory.o << "." << endl;

    auto weapon_one_name = builder.CreateString("Sword");
    short weapon_one_damage = 3;
    auto weapon_two_name = builder.CreateString("Axe");
    short weapon_two_damage = 5;
    // Use the `CreateWeapon` shortcut to create Weapons with all the fields set.
    auto sword = CreateWeapon(builder, weapon_one_name, weapon_one_damage);
    auto axe = CreateWeapon(builder, weapon_two_name, weapon_two_damage);

    cout << "Created sword: @" << sword.o << " and axe: @" << axe.o << "." << endl;

    // Place the weapons into a `vector`, then convert that into a FlatBuffer `vector`.
    vector<Offset<Weapon>> weapons_vector;
    weapons_vector.push_back(sword);
    weapons_vector.push_back(axe);
    auto weapons = builder.CreateVector(weapons_vector);

    cout << "Created orc weapons: @" << weapons.o << "." << endl;

    Vec3 points[] = {Vec3(1.0f, 2.0f, 3.0f), Vec3(4.0f, 5.0f, 6.0f)};
    auto path = builder.CreateVectorOfStructs(points, 2);

    cout << "Created orc path: @" << path.o << "." << endl;

    MonsterBuilder monster_builder(builder);
    monster_builder.add_pos(&position);
    monster_builder.add_mana(mana);
    monster_builder.add_hp(hp);
    monster_builder.add_name(name);
    monster_builder.add_inventory(inventory);
    monster_builder.add_color(Color_Red);
    monster_builder.add_weapons(weapons);
    monster_builder.add_equipped_type(Equipment_Weapon);
    monster_builder.add_equipped(axe.Union());
    monster_builder.add_path(path);
    auto orc = monster_builder.Finish();

    cout << "Created orc monster: @" << orc.o << "." << endl;

    // Call `Finish()` to instruct the builder that this monster is complete.
    builder.Finish(orc);

    // This must be called after `Finish()`.
    buf = builder.GetBufferPointer();
    // Returns the size of the buffer that
    // `GetBufferPointer()` points to.
    size = builder.GetSize();

    cout << "Monster buffer size is: " << size << "." << endl;
}

void DeserializeEntity(uint8_t* buf)
{
    cout << endl << ">>> Part 2: Deserialize a monster object loaded from memory. <<<" << endl << endl;

    uint8_t* buffer_pointer = buf;

    // Get a pointer to the root object inside the buffer.
    auto monster = GetMonster(buffer_pointer);
    // `monster` is of type `Monster *`.
    // Note: root object pointers are NOT the same as `buffer_pointer`.
    // `GetMonster` is a convenience function that calls `GetRoot<Monster>`,
    // the latter is also available for non-root types.

    DumpMonsterAttributes(monster);
}

void ModifyMutableEntity(uint8_t* buf)
{
    cout << endl << ">>> Part 3a: Modify a monster object through mutable interface. <<<" << endl << endl;

    uint8_t* buffer_pointer = buf;

    auto monster = GetMutableMonster(buffer_pointer);

    monster->mutable_name()->Mutate(3, 0);

    cout << "Before change:" << endl;
    DumpMonsterAttributes(monster);

    monster->mutate_mana(200);
    monster->mutate_hp(120);
    monster->mutable_pos()->mutate_z(4);
    monster->mutable_inventory()->Mutate(0, 1);

    // We can only modify a string in-place; we cannot extend its value.
    monster->mutable_name()->Mutate(0, 'G');
    monster->mutable_name()->Mutate(1, 'o');
    monster->mutable_name()->Mutate(2, 'b');
    monster->mutable_name()->Mutate(3, 'l');
    monster->mutable_name()->Mutate(4, 'i');
    monster->mutable_name()->Mutate(5, 'n');
    monster->mutable_name()->Mutate(6, 0);

    cout << endl << "After change:" << endl;
    DumpMonsterAttributes(monster);
}

void ModifyObjectEntity(uint8_t* buf)
{
    cout << endl << ">>> Part 3b: Modify a monster object through object API. <<<" << endl << endl;

    uint8_t* buffer_pointer = buf;

    auto monster = GetMonster(buffer_pointer);

    cout << "Before change:" << endl;
    DumpMonsterAttributes(monster);

    // Autogenerated class from table Monster.
    MonsterT monsterobj;

    // Deserialize from buffer into object.
    monster->UnPackTo(&monsterobj);

    cout << endl << "Name of unpacked monster object = " << monsterobj.name << "." << endl;

    // Update object directly like a C++ class instance.
    monsterobj.mana = 10;
    monsterobj.hp = 25;
    monsterobj.color = Color_Green;
    monsterobj.name = "Weakling";  // Change the name.

    // Serialize into new flatbuffer.
    FlatBufferBuilder fbb;
    fbb.Finish(Monster::Pack(fbb, &monsterobj));

    buffer_pointer = fbb.GetBufferPointer();
    monster = GetMonster(buffer_pointer);

    cout << endl << "After change:" << endl;
    DumpMonsterAttributes(monster);
}

void LoadEntity()
{
    cout << endl << ">>> Part 4: Deserialize a monster object loaded from a file. <<<" << endl << endl;

    char* data = nullptr;
    int length = 0;

    LoadFileData("./monster_data.bin", data, length);

    cout << "File size is: " << length << "." << endl << endl;

    auto monster = GetMonster(data);

    DumpMonsterAttributes(monster);

    delete[] data;
}

void RunTutorialCode()
{
    uint8_t* buf = nullptr;
    int size = 0;
    
    SerializeEntity(buf, size);

    DeserializeEntity(buf);

    ModifyMutableEntity(buf);
    ModifyObjectEntity(buf);

    LoadEntity();
}
