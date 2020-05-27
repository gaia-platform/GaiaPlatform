/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "flatbuffers/reflection.h"

#include "MonsterHelper.h"
#include "Utils.h"

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

uint8_t* AccessAndModifyEntity(uint8_t* buf)
{
    uint8_t* buffer_pointer = buf;

    auto monster = GetMonster(buffer_pointer);

    cout << "\nBefore change:" << endl;
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

    return buffer_pointer;
}

void PrintStructInformation(
    const Struct* struct_,
    const reflection::Object* type,
    int indentation_level = 1)
{
    string indentation(indentation_level * 2, ' ');

    auto fields = type->fields();
    for (size_t i = 0; i < fields->Length(); i++)
    {
        auto field = fields->Get(i);
        cout << indentation << "name=" << field->name()->c_str() << endl
            << indentation << "  type=" << reflection::EnumNameBaseType(field->type()->base_type())
            << ",\t id=" << field->id()
            << ",\t offset=" << field->offset()
            << endl;

        if (IsInteger(field->type()->base_type()))
        {
            const int64_t& field_value = GetAnyFieldI(*struct_, *field);
            cout << indentation << "  value=[" << field_value << "]" << endl;
        }
        else if (IsFloat(field->type()->base_type()))
        {
            const double& field_value = GetAnyFieldF(*struct_, *field);
            cout << indentation << "  value=[" << field_value << "]" << endl;
        }
        else
        {
            const string& field_value = GetAnyFieldS(*struct_, *field);
            cout << indentation << "  value=[" << field_value << "]" << endl;
        }
    }
}

void PrintTableInformation(
    const reflection::Schema* schema,
    const Table* table,
    const reflection::Object* type,
    int indentation_level = 1)
{
    string indentation(indentation_level * 2, ' ');

    auto fields = type->fields();
    for (size_t i = 0; i < fields->Length(); i++)
    {
        auto field = fields->Get(i);
        cout << indentation << "name=" << field->name()->c_str() << endl
            << indentation << "  type=" << reflection::EnumNameBaseType(field->type()->base_type())
            << ",\t id=" << field->id()
            << ",\t offset=" << field->offset()
            << endl;

        if (field->type()->base_type() == reflection::String)
        {
            const String* field_value = GetFieldS(*table, *field);
            if (field_value != nullptr)
            {
                cout << indentation << "  value=[" << field_value->c_str() << "]" << endl;
            }
        }
        else if (field->type()->base_type() == reflection::Bool
            || field->type()->base_type() == reflection::Byte
            || field->type()->base_type() == reflection::UType)
        {
            const int8_t& field_value = GetFieldI<int8_t>(*table, *field);
            cout << indentation << "  value=[" << (int)field_value << "]" << endl;
        }
        else if (field->type()->base_type() == reflection::Short)
        {
            const int16_t& field_value = GetFieldI<int16_t>(*table, *field);
            cout << indentation << "  value=[" << field_value << "]" << endl;
        }
        else if (field->type()->base_type() == reflection::Float)
        {
            const float& field_value = GetFieldF<float>(*table, *field);
            cout << indentation << "  value=[" << field_value << "]" << endl;
        }
        else if (field->type()->base_type() == reflection::Double)
        {
            const double& field_value = GetFieldF<double>(*table, *field);
            cout << indentation << "  value=[" << field_value << "]" << endl;
        }
        else if (field->type()->base_type() == reflection::Vector)
        {
            cout << indentation << "  element type=" << reflection::EnumNameBaseType(field->type()->element()) << endl;

            VectorOfAny* field_value = GetFieldAnyV(*table, *field);
            if (IsInteger(field->type()->element()))
            {
                for (size_t j = 0; j < field_value->size(); j++)
                {
                    int64_t value = GetAnyVectorElemI(field_value, field->type()->element(), j);
                    cout << indentation << "  element[" << j << "]=[" << value << "]" << endl;
                }
            }
            else if (field->type()->element() == reflection::Obj)
            {
                // Lookup type of object.
                auto types = schema->objects();
                const reflection::Object* obj_type = types->Get(field->type()->index());

                cout << indentation << "  element_object type=" << obj_type->name()->c_str()
                    << "\t is_struct=" << obj_type->is_struct() << endl;

                VectorOfAny* field_value = GetFieldAnyV(*table, *field);
                for (size_t j = 0; j < field_value->size(); j++)
                {
                    cout << indentation << "  element[" << j << "]:" << endl;

                    // Structs and Tables are handled differently.
                    if (obj_type->is_struct())
                    {
                        size_t obj_struct_size = GetTypeSizeInline(
                            field->type()->element(), field->type()->index(), *schema);

                        const Struct* obj_struct = GetAnyVectorElemAddressOf<const Struct>(
                            field_value, j, obj_struct_size);
                        PrintStructInformation(obj_struct, obj_type, indentation_level + 2);
                    }
                    else
                    {
                        const Table* obj_table = GetAnyVectorElemPointer<const Table>(field_value, j);
                        PrintTableInformation(schema, obj_table, obj_type, indentation_level + 2);
                    }
                }
            }
        }
        else if (field->type()->base_type() == reflection::Obj)
        {
            // Lookup type of object.
            auto types = schema->objects();
            const reflection::Object* obj_type = types->Get(field->type()->index());

            cout << indentation << "  object type=" << obj_type->name()->c_str()
                << "\t is_struct=" << obj_type->is_struct() << endl;

            // Structs and Tables are handled differently.
            if (obj_type->is_struct())
            {
                const Struct* obj_struct = GetFieldStruct(*table, *field);
                PrintStructInformation(obj_struct, obj_type, indentation_level + 1);
            }
            else
            {
                const Table* obj_table = GetFieldT(*table, *field);
                PrintTableInformation(schema, obj_table, obj_type, indentation_level + 1);
            }
        }
    }
}

void AccessEntityWithReflection(uint8_t* buf)
{
    uint8_t* buffer_pointer = buf;

    char* data = nullptr;
    int length = 0;

    cout << "\nLoading monster.bfbs..." << endl;
    LoadFileData("monster.bfbs", data, length);
    const reflection::Schema* schema = reflection::GetSchema(data);

    // List types.
    cout << "\nSchema contains types:" << endl;
    auto types = schema->objects();
    for (size_t i = 0; i < types->Length(); i++)
    {
        const reflection::Object* type = types->Get(i);
        cout << "  " << type->name()->c_str() << endl;
    }

    // List enums.
    cout << "\nSchema contains enums:" << endl;
    auto enums = schema->enums();
    for (size_t i = 0; i < enums->Length(); i++)
    {
        const reflection::Enum* enum_ = enums->Get(i);
        cout << "  " << enum_->name()->c_str() << endl;
    }

    // Get root table of serialized data.
    const Table* root_table = GetAnyRoot(buffer_pointer);

    // Go through fields of Monster.
    cout << "\nExplore Monster data using generic string field accessor:" << endl;
    auto monster_type = types->LookupByKey("Flatbuffers.Monster.Monster");
    auto fields = monster_type->fields();
    for (size_t i = 0; i < fields->Length(); i++)
    {
        auto field = fields->Get(i);
        cout << "  name=" << field->name()->c_str() << endl
            << "    type=" << reflection::EnumNameBaseType(field->type()->base_type())
            << ",\t id=" << field->id()
            << ",\t offset=" << field->offset()
            << endl;

        const string field_value = GetAnyFieldS(*root_table, *field, schema);
        cout << "    value=[" << field_value << "]" << endl;
    }

    // Go again through fields of Monster.
    cout << "\nExplore Monster data using specific field accessors:" << endl;
    PrintTableInformation(schema, root_table, monster_type);
}

void TestAccess()
{
    uint8_t* buf = nullptr;
    int size = 0;

    CreateEntity(buf, size);

    buf = AccessAndModifyEntity(buf);

    AccessEntityWithReflection(buf);
}
