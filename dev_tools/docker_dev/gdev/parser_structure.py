#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide a description of the structure to be constructed.
"""
from __future__ import annotations
from typing import FrozenSet, Set, Tuple
from importlib import import_module
from importlib.util import find_spec
from inspect import getdoc, isabstract, iscoroutinefunction
from argparse import ArgumentParser, REMAINDER
from dataclasses import dataclass
from importlib import import_module
from importlib.util import find_spec
from inspect import getdoc, isabstract, iscoroutinefunction
from pkgutil import iter_modules
from typing import FrozenSet, Sequence, Set, Tuple



@dataclass(frozen=True)
class ParserStructure:
    """
    Class to provide a description of the structure to be constructed.

    Note that the `gdev.cmd` is a `path` within the package hierarchy, not calling out to a gdev.cmd script.
    """

    command_parts: Tuple[str, ...]
    doc: str
    sub_parser_structures: FrozenSet[ParserStructure] = frozenset()

    def get_command_class(self) -> str:
        """
        Name of the class that contains the procedure to enact.
        """
        return ''.join([
            command_part.capitalize()
            for command_part in self.command_parts
            for command_part in command_part.split('_')
            if command_part
        ])

    def get_command_module(self) -> str:
        """
        Name of the module that contains the procedure to enact.
        """
        return '.'.join(['gdev.cmd', *self.command_parts])

    @classmethod
    def of_command_parts(cls, command_parts: Tuple[str, ...]) -> ParserStructure:
        """
        Create a parser structure out of the command parts.
        """

        module_name = '.'.join(['gdev.cmd', *command_parts])
        spec = find_spec(module_name)
        module = import_module(module_name)
        if spec.submodule_search_locations is None:
            command_class = ''.join([
                command_part.capitalize()
                for command_part in command_parts
                for command_part in command_part.split('_')
                if command_part
            ])
            doc = getdoc(module.__dict__[command_class]) or ''
            parser_structure = ParserStructure(command_parts=command_parts, doc=doc)
        else:
            doc = getdoc(module)
            sub_parser_structures: Set[ParserStructure] = set()
            for module in iter_modules(spec.submodule_search_locations):
                if not (sub_command := module.name).startswith('_'):
                    sub_parser_structures.add(
                        cls.of_command_parts(tuple([*command_parts, sub_command]))
                    )
            parser_structure = ParserStructure(
                command_parts=command_parts,
                doc=doc,
                sub_parser_structures=frozenset(sub_parser_structures)
            )

        return parser_structure
