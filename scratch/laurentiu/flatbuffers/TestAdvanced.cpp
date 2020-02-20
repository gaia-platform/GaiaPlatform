/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "Utils.h"
#include "MonsterHelper.h"

#include <iostream>
#include <iomanip>

using namespace std;
using namespace flatbuffers;
using namespace Flatbuffers::Monster;

void TestAdvanced();

int main(void)
{
    TestAdvanced();

    return 0;
}

void DumpFileContents(char* data)
{
    auto monster = GetMonster(data);

    cout << "Starting monster attributes:" << endl;
    DumpMonsterAttributes(monster);
}

void TestRegularInterface(char* data, int size)
{
    cout << endl << ">>> TestRegularInterface. <<<" << endl;

    cout << "Dumping raw data content:" << endl;
    for (int offset = 0; offset < size; offset += sizeof(int16_t))
    {
        int16_t value = *((int16_t*)(data + offset));

        if (offset % 6 == 0)
        {
            cout << endl << "Offset " << offset << ":\t";
        }

        cout << "0x" << setfill('0') << setw(8) << hex << (int)value;
        cout << " (" << setfill(' ') << setw(5) << dec << (int)value << ")  ";
    }
    cout << endl << endl;

    cout << "VTable offset values are:" << endl;
    cout << "  VT_POS = " << (int)Monster::VT_POS << endl;
    cout << "  VT_MANA = " << (int)Monster::VT_MANA << endl;
    cout << "  VT_HP = " << (int)Monster::VT_HP << endl;
    cout << "  VT_NAME = " << (int)Monster::VT_NAME << endl;
    cout << "  VT_INVENTORY = " << (int)Monster::VT_INVENTORY << endl;
    cout << "  VT_COLOR = " << (int)Monster::VT_COLOR << endl;
    cout << "  VT_WEAPONS = " << (int)Monster::VT_WEAPONS << endl;
    cout << "  VT_EQUIPPED_TYPE = " << (int)Monster::VT_EQUIPPED_TYPE << endl;
    cout << "  VT_EQUIPPED = " << (int)Monster::VT_EQUIPPED << endl;
    cout << "  VT_PATH = " << (int)Monster::VT_PATH << endl;

    cout << endl << "Parsing data using offset values:" << endl;

    // See https://google.github.io/flatbuffers/md__internals.html for additional information.
    uoffset_t offsetMainTable = *((uoffset_t*)data);
    cout << "  The offset of the main table is: " << offsetMainTable << "." << endl;

    soffset_t relativeOffsetMainTableVtable = *((soffset_t*)(data + offsetMainTable));
    cout << "    The relative offset of the main table vtable is: " << relativeOffsetMainTableVtable << "." << endl;

    uoffset_t offsetMainTableVtable = offsetMainTable - relativeOffsetMainTableVtable;
    cout << "    The offset of the main table vtable is: " << offsetMainTableVtable << "." << endl;

    voffset_t sizeVtable = *((voffset_t*)(data + offsetMainTableVtable));
    cout << "    The size of the main table vtable is: " << sizeVtable << "." << endl;

    voffset_t sizeObject = *((voffset_t*)(data + offsetMainTableVtable + sizeof(voffset_t)));
    cout << "    The size of the main object inline data is: " << sizeObject << "." << endl;

    // Read pos.
    voffset_t offsetPos = *((voffset_t*)(data + offsetMainTableVtable + Monster::VT_POS));
    cout << "  The offset of the POS field is: " << offsetPos << "." << endl;
    if (offsetPos != 0)
    {
        Vec3 pos = *((Vec3*)(data + offsetMainTable + offsetPos));
        cout << "    POS is: X = " << pos.x() << ", Y = " << pos.y() << ", Z = " << pos.z() << "." << endl;
    }

    // Read mana.
    voffset_t offsetMana = *((voffset_t*)(data + offsetMainTableVtable + Monster::VT_MANA));
    cout << "  The offset of the MANA field is: " << offsetMana << "." << endl;
    if (offsetMana != 0)
    {
        int16_t mana = *((int16_t*)(data + offsetMainTable + offsetMana));
        cout << "    MANA is: " << mana << "." << endl;
    }

    // Read HP.
    voffset_t offsetHp = *((voffset_t*)(data + offsetMainTableVtable + Monster::VT_HP));
    cout << "  The offset of the HP field is: " << offsetHp << "." << endl;
    if (offsetHp == 0)
    {
        cout << "    HP was set to the default value (100)." << endl;
    }

    // Read name.
    voffset_t offsetNameOffset = *((voffset_t*)(data + offsetMainTableVtable + Monster::VT_NAME));
    cout << "  The offset of the NAME field offset is: " << offsetNameOffset << "." << endl;

    uoffset_t relativeOffsetName = *((uoffset_t*)(data + offsetMainTable + offsetNameOffset));
    cout << "    The relative offset of the NAME field is: " << relativeOffsetName << "." << endl;

    uoffset_t offsetName = offsetNameOffset + relativeOffsetName;
    uint32_t nameLength = *((uint32_t*)(data + offsetMainTable + offsetName));
    cout << "    NAME length is: " << nameLength << "." << endl;
    char* name = (char*)(data + offsetMainTable + offsetName + sizeof(uint32_t));
    cout << "    NAME is: " << name << "." << endl;

    // Read inventory.
    voffset_t offsetInventoryOffset = *((voffset_t*)(data + offsetMainTableVtable + Monster::VT_INVENTORY));
    cout << "  The offset of the INVENTORY field offset is: " << offsetInventoryOffset << "." << endl;

    uoffset_t relativeOffsetInventory = *((uoffset_t*)(data + offsetMainTable + offsetInventoryOffset));
    cout << "    The relative offset of the INVENTORY field is: " << relativeOffsetInventory << "." << endl;

    uoffset_t offsetInventory = offsetInventoryOffset + relativeOffsetInventory;
    uint32_t inventoryLength = *((uint32_t*)(data + offsetMainTable + offsetInventory));
    cout << "    INVENTORY length is: " << inventoryLength << "." << endl;
    for (int i = 0; i < (int)inventoryLength; i++)
    {
        int8_t item = *(int8_t*)(data + offsetMainTable + offsetInventory + sizeof(uint32_t) + i * sizeof(int8_t));
        cout << "      INVENTORY item #" << (i + 1) << " is: " << (int)item << "." << endl;
    } 

    // Read color.
    voffset_t offsetColor = *((voffset_t*)(data + offsetMainTableVtable + Monster::VT_COLOR));
    cout << "  The offset of the COLOR field is: " << offsetColor << "." << endl;
    if (offsetColor != 0)
    {
        int8_t color = *((int8_t*)(data + offsetMainTable + offsetColor));
        cout << "    COLOR is: " << (int)color << "." << endl;
    }

    // Read weapons.
    voffset_t offsetWeaponsOffset = *((voffset_t*)(data + offsetMainTableVtable + Monster::VT_WEAPONS));
    cout << "  The offset of the WEAPONS field offset is: " << offsetWeaponsOffset << "." << endl;

    uoffset_t relativeOffsetWeapons = *((uoffset_t*)(data + offsetMainTable + offsetWeaponsOffset));
    cout << "    The relative offset of the WEAPONS field is: " << relativeOffsetWeapons << "." << endl;

    uoffset_t offsetWeapons = offsetWeaponsOffset + relativeOffsetWeapons;
    uint32_t weaponsLength = *((uint32_t*)(data + offsetMainTable + offsetWeapons));
    cout << "    WEAPONS length is: " << weaponsLength << "." << endl;
    for (int i = 0; i < (int)weaponsLength; i++)
    {
        uoffset_t offsetWeapon = offsetMainTable + offsetWeapons + sizeof(uint32_t) + i * sizeof(uoffset_t);
        cout << "      Offset weapon #" << (i + 1) << " is: " << offsetWeapon << "." << endl;

        uoffset_t relativeOffsetWeaponTable = *((uoffset_t*)(data + offsetWeapon));
        cout << "        The relative offset of the weapon table is: " << relativeOffsetWeaponTable << "." << endl;

        uoffset_t offsetWeaponTable = offsetWeapon + relativeOffsetWeaponTable;
        cout << "        The offset of the weapon table is: " << offsetWeaponTable << "." << endl;

        soffset_t relativeOffsetWeaponTableVtable = *((soffset_t*)(data + offsetWeaponTable));
        cout << "        The relative offset of the weapon table vtable is: " << relativeOffsetWeaponTableVtable << "." << endl;

        uoffset_t offsetWeaponTableVtable = offsetWeaponTable - relativeOffsetWeaponTableVtable;
        cout << "        The offset of the weapon table vtable is: " << offsetWeaponTableVtable << "." << endl;

        voffset_t sizeWeaponVtable = *((voffset_t*)(data + offsetWeaponTableVtable));
        cout << "          The size of the main table vtable is: " << sizeWeaponVtable << "." << endl;

        voffset_t sizeWeaponObject = *((voffset_t*)(data + offsetWeaponTableVtable + sizeof(voffset_t)));
        cout << "          The size of the weapon object inline data is: " << sizeWeaponObject << "." << endl;

        voffset_t offsetWeaponNameOffset = *((voffset_t*)(data + offsetWeaponTableVtable + Weapon::VT_NAME));
        cout << "        The offset of the weapon name field offset is: " << offsetWeaponNameOffset << "." << endl;

        uoffset_t relativeOffsetWeaponName = *((uoffset_t*)(data + offsetWeaponTable + offsetWeaponNameOffset));
        cout << "          The relative offset of the weapon name field is: " << relativeOffsetWeaponName << "." << endl;

        uoffset_t offsetWeaponName = offsetWeaponNameOffset + relativeOffsetWeaponName;
        uint32_t weaponNameLength = *((uint32_t*)(data + offsetWeaponTable + offsetWeaponName));
        cout << "          Weapon name length is: " << weaponNameLength << "." << endl;
        char* weaponName = (char*)(data + offsetWeaponTable + offsetWeaponName + sizeof(uint32_t));
        cout << "          Weapon name is: " << weaponName << "." << endl;

        voffset_t offsetWeaponDamage = *((voffset_t*)(data + offsetWeaponTableVtable + Weapon::VT_DAMAGE));
        cout << "        The offset of the weapon damage field is: " << offsetWeaponDamage << "." << endl;

        int16_t weaponDamage = *((int16_t*)(data + offsetWeaponTable + offsetWeaponDamage));
        cout << "          Weapon damage is: " << weaponDamage << "." << endl;
    }

    // Read equipped type.
    voffset_t offsetEquippedType = *((voffset_t*)(data + offsetMainTableVtable + Monster::VT_EQUIPPED_TYPE));
    cout << "  The offset of the EQUIPPED_TYPE field is: " << offsetEquippedType << "." << endl;
    if (offsetEquippedType != 0)
    {
        int8_t color = *((int8_t*)(data + offsetMainTable + offsetEquippedType));
        cout << "    EQUIPPED_TYPE is: " << (int)color << "." << endl;
    }

    // Read equipped.
    voffset_t offsetEquippedOffset = *((voffset_t*)(data + offsetMainTableVtable + Monster::VT_EQUIPPED));
    cout << "  The offset of the EQUIPPED field offset is: " << offsetEquippedOffset << "." << endl;

    {
        uoffset_t relativeOffsetWeaponTable = *((uoffset_t*)(data + offsetMainTable + offsetEquippedOffset));
        cout << "        The relative offset of the weapon table is: " << relativeOffsetWeaponTable << "." << endl;

        uoffset_t offsetWeaponTable = offsetMainTable + offsetEquippedOffset + relativeOffsetWeaponTable;
        cout << "        The offset of the weapon table is: " << offsetWeaponTable << "." << endl;

        soffset_t relativeOffsetWeaponTableVtable = *((soffset_t*)(data + offsetWeaponTable));
        cout << "        The relative offset of the weapon table vtable is: " << relativeOffsetWeaponTableVtable << "." << endl;

        uoffset_t offsetWeaponTableVtable = offsetWeaponTable - relativeOffsetWeaponTableVtable;
        cout << "        The offset of the weapon table vtable is: " << offsetWeaponTableVtable << "." << endl;

        voffset_t sizeWeaponVtable = *((voffset_t*)(data + offsetWeaponTableVtable));
        cout << "          The size of the main table vtable is: " << sizeWeaponVtable << "." << endl;

        voffset_t sizeWeaponObject = *((voffset_t*)(data + offsetWeaponTableVtable + sizeof(voffset_t)));
        cout << "          The size of the weapon object inline data is: " << sizeWeaponObject << "." << endl;

        voffset_t offsetWeaponNameOffset = *((voffset_t*)(data + offsetWeaponTableVtable + Weapon::VT_NAME));
        cout << "        The offset of the weapon name field offset is: " << offsetWeaponNameOffset << "." << endl;

        uoffset_t relativeOffsetWeaponName = *((uoffset_t*)(data + offsetWeaponTable + offsetWeaponNameOffset));
        cout << "          The relative offset of the weapon name field is: " << relativeOffsetWeaponName << "." << endl;

        uoffset_t offsetWeaponName = offsetWeaponNameOffset + relativeOffsetWeaponName;
        uint32_t weaponNameLength = *((uint32_t*)(data + offsetWeaponTable + offsetWeaponName));
        cout << "          Weapon name length is: " << weaponNameLength << "." << endl;
        char* weaponName = (char*)(data + offsetWeaponTable + offsetWeaponName + sizeof(uint32_t));
        cout << "          Weapon name is: " << weaponName << "." << endl;

        voffset_t offsetWeaponDamage = *((voffset_t*)(data + offsetWeaponTableVtable + Weapon::VT_DAMAGE));
        cout << "        The offset of the weapon damage field is: " << offsetWeaponDamage << "." << endl;

        int16_t weaponDamage = *((int16_t*)(data + offsetWeaponTable + offsetWeaponDamage));
        cout << "          Weapon damage is: " << weaponDamage << "." << endl;
    }

    // Read path.
    voffset_t offsetPathOffset = *((voffset_t*)(data + offsetMainTableVtable + Monster::VT_PATH));
    cout << "  The offset of the PATH field offset is: " << offsetPathOffset << "." << endl;

    uoffset_t relativeOffsetPath = *((uoffset_t*)(data + offsetMainTable + offsetPathOffset));
    cout << "    The relative offset of the PATH field is: " << relativeOffsetPath << "." << endl;

    uoffset_t offsetPath = offsetPathOffset + relativeOffsetPath;
    uint32_t pathLength = *((uint32_t*)(data + offsetMainTable + offsetPath));
    cout << "    PATH length is: " << pathLength << "." << endl;
    for (int i = 0; i < (int)pathLength; i++)
    {
        Vec3 pos = *((Vec3*)(data + offsetMainTable + offsetPath + sizeof(uint32_t) + i * sizeof(Vec3)));
        cout << "      Position #" << (i + 1) << " is: X = " << pos.x() << ", Y = " << pos.y() << ", Z = " << pos.z() << "." << endl;
    }
}

void TestAdvanced()
{
    char* data = nullptr;
    int size = 0;

    LoadFileData("monster_data.bin", data, size);
    cout << "File size is: " << size << "." << endl;
    DumpFileContents(data);

    TestRegularInterface(data, size);
}
