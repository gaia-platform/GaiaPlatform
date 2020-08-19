#!/usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

import argparse
import io
import json
import os

def filename(filepath):
    return os.path.basename(filepath)

class Schema:
    def __init__(self, name):
        self.name = name
        self.tables = set()

    def __str__(self):
        text = "namespace " + self.name + ";\n\n"
        for table in self.tables:
            text += str(table) + "\n\n"
        return text

    def add_table(self, table):
        self.tables.add(table)

class Table:
    valid_types = {
        "bool", "byte", "char", "float32", "float64", "int8", "uint8",
        "int16", "uint16", "int32", "uint32", "int64", "uint64", "string"
    }

    def __init__(self, data):
        self.name = data["pkg"] + "__" + data["name"]
        self.fields = {}
        self.references = {}

        for field_name, field_type in data["fields"].items():
            if field_type in Table.valid_types:
                self.fields[field_name] = field_type

        for field_name, nested_type in data["nested_fields"].items():
            self.references[field_name] = nested_type["pkg"] + "__" \
                + nested_type["type"]

    def __str__(self):
        text = "create table " + self.name + " (\n"

        for field_name, field_type in self.fields.items():
            text += "    " + field_name + ": " + field_type + ",\n"

        for ref_name, ref_table in self.references.items():
            text += "    " + ref_name + " references " + ref_table + ",\n"

        text = text.rstrip(",\n")
        text += "\n);"
        return text

    def __hash__(self):
        return hash(self.name)

parser = argparse.ArgumentParser()
parser.add_argument(
    "manifest_path",
    help="File path to manifest.txt, listing JSON files generated from ROS2 messages",
    type=str
)
args = parser.parse_args()

schema = Schema("ros_msgs")

with io.open(args.manifest_path, 'r') as manifest:
    json_files = manifest.read().splitlines()
    manifest.close()

for json_path in json_files:
    json_filename = filename(json_path)

    with io.open(json_path, 'r') as json_file:
        json_data = json.load(json_file)
        json_file.close()

    table = Table(json_data)
    schema.add_table(table)

print(str(schema))
