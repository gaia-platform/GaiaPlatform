#!/usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import argparse
import io
import importlib.util
import os
import re
import rosidl_parser.definition

def camel_to_snake_case(camel_str):
    return re.sub("[A-Z]", r"_\g<0>", camel_str).lower().lstrip('_')

class InterfaceField:
    def __init__(self, name, type):
        self.name = name.lstrip("_")
        self.type = type

    def __str__(self):
        if type(self.type) is rosidl_parser.definition.BasicType:
            return self.type.typename + " " + self.name
        elif type(self.type) is rosidl_parser.definition.NamespacedType:
            return self.type.namespaces[0] + "/" + self.type.name + " " + self.name
        else:
            return str(self.type) + " " + self.name

    def __hash__(self):
        return hash(self.type + self.name)

class InterfaceFile:
    def __init__(self, package, name, type="msg"):
        self.package = package
        self.name = name
        self.type = type

        module_name = "." + "_" + camel_to_snake_case(self.name)
        interface_module = importlib.import_module(
            module_name,
            self.package + "." + self.type
        )

        slots = getattr(interface_module, self.name).__slots__
        slot_types = getattr(interface_module, self.name).SLOT_TYPES
        self.fields = []

        for i in range(0, len(slots)):
            field = InterfaceField(slots[i], slot_types[i])
            self.fields.append(field)

    def __str__(self):
        text = self.package + " " + self.type + "/" + self.name + "." + self.type
        return text

    def __hash__(self):
        return hash(self.package + self.name + self.type)

def read_interface_file(manifest_line):
    (package, type_and_name) = manifest_line.split(" ", maxsplit=1)
    (type, file_name) = type_and_name.split("/", maxsplit=1)
    name = file_name.split(".", maxsplit=1)[0]
    interface_file = InterfaceFile(package, name, type)
    return interface_file

class TableField:
    gaia__basic_types = {
        "bool", "byte", "char", "float", "double", "int8", "uint8",
        "int16", "uint16", "int32", "uint32", "int64", "uint64", "string"
    }

    def __init__(self, name, type):
        self.name = name
        if type in TableField.gaia__basic_types:
            self.type = type


    def __str__(self):
        text = self.name + ": " + self.type
        return text

    def __hash__(self):
        return hash(self.type + self.name)

class Table:
    def __init__(self, name, fields):
        self.name = name
        self.fields = fields

    def __str__(self):
        text = self.name
        return text

    def __hash__(self):
        return hash(self.name)

class Schema:
    def __init__(self, name, interface_files):
        self.name = camel_to_snake_case(name)
        self.interface_files = interface_files

    def __str__(self):
        text = "~~ " + self.name + " ~~\n\n"

        for interface_file in self.interface_files:
            text += str(interface_file) + ":\n"

            for field in interface_file.fields:
                text += ">> " + str(field) + "\n"

            text += "\n"

        return text

parser = argparse.ArgumentParser()
parser.add_argument(
    "schema_name",
    help="Name of the DDL schema",
    type=str
)
parser.add_argument(
    "manifest_file_path",
    help="File path to the .txt manifest listing one interface file per line",
    type=str
)
args = parser.parse_args()

interface_files = []

with io.open(args.manifest_file_path, 'r') as manifest:
    lines = manifest.read().splitlines()
    manifest.close()

for line in lines:
    interface_file = read_interface_file(line)
    interface_files.append(interface_file)

schema = Schema(args.schema_name, interface_files)
print(schema)
