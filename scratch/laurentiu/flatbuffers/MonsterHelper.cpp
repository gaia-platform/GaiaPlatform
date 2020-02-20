/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "MonsterHelper.h"

#include <iostream>
#include <fstream>

using namespace std;
using namespace flatbuffers;
using namespace Flatbuffers::Monster;

void DumpMonsterAttributes(const Monster* monster)
{
    auto hp = monster->hp();
    auto mana = monster->mana();
    auto name = monster->name()->c_str();
    auto color = monster->color();

    cout << "Monster attributes are: Mana = " << mana << ", HP = " << hp << ", Name = " << name << ", Color = " << color << "." << endl;

    auto pos = monster->pos();
    auto x = pos->x();
    auto y = pos->y();
    auto z = pos->z();

    cout << "  Position is: X = " << x << ", Y = " << y << ", Z = " << z << "." << endl;

    // A pointer to a `flatbuffers::Vector<>`.
    auto inv = monster->inventory();
    auto inv_len = inv->Length();
    cout << "  Inventory size is: " << inv_len << "." << endl;
    for (int i = 0; i < (int)inv_len; i++)
    {
        cout << "    Inventory item #" << (i + 1) << " = " << (int)(inv->Get(i)) << "." << endl;
    }

    // A pointer to a `flatbuffers::Vector<>`.
    auto weapons = monster->weapons();
    auto weapons_len = weapons->Length();
    cout << "  Weapons count is: " << weapons_len << "." << endl;
    for (int i = 0; i < (int)weapons_len; i++)
    {
        auto weapon = weapons->Get(i);
        cout << "    Weapon #" << (i + 1) << " name = " << weapon->name()->str() << "; damage = " << weapon->damage() << "." << endl;
    }

    auto union_type = monster->equipped_type();
    if (union_type == Equipment_Weapon)
    {
        // Requires `static_cast` to type `const Weapon*`.
        auto weapon = static_cast<const Weapon*>(monster->equipped()); 
        // "Axe"
        auto weapon_name = weapon->name()->str();
        // 5
        auto weapon_damage = weapon->damage();

        cout << "  Equipped weapon name = " << weapon_name << "; damage = " << weapon_damage << "." << endl;
    }

    auto path = monster->path();
    auto path_len = path->Length();
    cout << "  Path length is: " << path_len << "." << endl;
    for (int i = 0; i < (int)path_len; i++)
    {
        auto pathpoint = path->Get(i);
        cout << "    Pathpoint #" << (i + 1) << " X = " << pathpoint->x() << ", Y = " << pathpoint->y() << ", Z = " << pathpoint->z() << "." << endl;
    }
}
