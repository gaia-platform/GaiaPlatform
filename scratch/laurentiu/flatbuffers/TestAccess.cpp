/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "MonsterHelper.h"

#include <iostream>

using namespace std;
using namespace flatbuffers;
using namespace Flatbuffers::Monster;

void TestAccess();

int main(void)
{
    TestAccess();

    return 0;
}

void CreateEntity(uint8_t*& buf, int& size)
{
    FlatBufferBuilder builder(1024);

    builder.ForceDefaults(true);

    auto position = Vec3(1.0f, 2.0f, 3.0f);

    vector<uint8_t> inventory {2, 4, 6, 8};

    auto weapon_one_name = builder.CreateString("Sword");
    short weapon_one_damage = 3;
    auto weapon_two_name = builder.CreateString("Axe");
    short weapon_two_damage = 5;
    auto sword = CreateWeapon(builder, weapon_one_name, weapon_one_damage);
    auto axe = CreateWeapon(builder, weapon_two_name, weapon_two_damage);
    vector<Offset<Weapon>> weapons;
    weapons.push_back(sword);
    weapons.push_back(axe);

    vector<Vec3> path {Vec3(1.0f, 2.0f, 3.0f), Vec3(4.0f, 5.0f, 6.0f)};

    auto monster = CreateMonsterDirect(
        builder,
        &position,
        150,
        300,
        "Orc",
        &inventory,
        Color_Red,
        &weapons,
        Equipment_Weapon,
        axe.Union(),
        &path);

    builder.Finish(monster);

    // This must be called after `Finish()`.
    buf = builder.GetBufferPointer();
    // Returns the size of the buffer that
    // `GetBufferPointer()` points to.
    size = builder.GetSize();

    cout << "Monster buffer size is: " << size << "." << endl << endl;
}

void AccessEntity(uint8_t* buf)
{

    uint8_t* buffer_pointer = buf;

    auto monster = GetMonster(buffer_pointer);

    cout << "Before change:" << endl;
    DumpMonsterAttributes(monster);

    // The following code will work if we change Monster inheritance from Table to be public!
    // cout << endl << "Mana = " << monster->GetField<int16_t>(Monster::VT_MANA, 0) << endl;
    // cout << "HP = " << monster->GetField<int16_t>(Monster::VT_HP, 0) << endl;
    // cout << "Color = " << (int)monster->GetField<int8_t>(Monster::VT_COLOR, 0) << endl;

    // The following code does not work at all, even with public inheritance from Table.
    // monster->SetField<int16_t>(Monster::VT_MANA, 500, 0);
    // monster->SetField<int16_t>(Monster::VT_HP, 250, 0);
    // monster->SetField<int8_t>(Monster::VT_COLOR, 0, 0);

    MonsterT* monsterobj = monster->UnPack();

    cout << endl << "Monster object name is: " << monsterobj->name << "." << endl << endl;
    monsterobj->name = "Darkling";

    FlatBufferBuilder fbb;
    fbb.Finish(Monster::Pack(fbb, monsterobj));

    buffer_pointer = fbb.GetBufferPointer();
    monster = GetMonster(buffer_pointer);

    cout << "After change:" << endl;
    DumpMonsterAttributes(monster);
}

void TestAccess()
{
    uint8_t* buf = nullptr;
    int size = 0;
    
    CreateEntity(buf, size);

    AccessEntity(buf);
}
