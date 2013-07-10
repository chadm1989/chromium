#!/usr/bin/python
# Copyright (C) 2013 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Compile an .idl file to Blink V8 bindings (.h and .cpp files).

We are porting the IDL compiler from Perl to Python. The plan is as follows.
We will temporarily have two build flows (see ../derived_sources.gyp):
[1] Perl: deprecated_generate_bindings.pl, which calls:
    deprecated_idl_parser.pm => deprecated_code_generator_v8.pm
[2] Python: idl_compiler.py, which calls:
    IDL lexer => IDL parser => Python object builder =>
    interface dependency resolver => IDL semantic validator =>
    C++ code generator

We will move IDL files from the Perl build flow [1] to the Python build flow [2]
incrementally. See http://crbug.com/239771
"""
import optparse
import os
import shlex
import sys

import idl_reader
import code_generator_v8


def parse_options():
    parser = optparse.OptionParser()
    parser.add_option('--additional-idl-files')
    parser.add_option('--idl-attributes-file')
    parser.add_option('--include', dest='idl_directories', action='append')
    parser.add_option('--output-directory')
    parser.add_option('--interface-dependencies-file')
    parser.add_option('--verbose', action='store_true', default=False)
    parser.add_option('--write-file-only-if-changed', type='int')
    # ensure output comes last, so command line easy to parse via regexes
    parser.disable_interspersed_args()

    options, args = parser.parse_args()
    if options.output_directory is None:
        parser.error('Must specify output directory using --output-directory.')
    if options.additional_idl_files is not None:
        # additional_idl_files is passed as a string with varied (shell-style)
        # quoting, hence needs parsing.
        options.additional_idl_files = shlex.split(options.additional_idl_files)
    if len(args) != 1:
        parser.error('Must specify exactly 1 input file as argument, but %d given.' % len(args))
    options.idl_filename = os.path.realpath(args[0])
    return options


def main():
    options = parse_options()
    idl_filename = options.idl_filename
    basename = os.path.basename(idl_filename)
    interface_name, _ = os.path.splitext(basename)
    verbose = options.verbose
    if verbose:
        print idl_filename

    idl_definitions = idl_reader.read_idl_definitions(idl_filename, options.interface_dependencies_file, options.additional_idl_files)
    # FIXME: add parameters when add validator
    # idl_definitions = idl_reader.read_idl_interface(idl_filename, options.interface_dependencies_file, options.additional_idl_files, options.idl_attributes_file, verbose=verbose)
    if not idl_definitions:
        # We generate dummy .h and .cpp files just to tell build scripts
        # that outputs have been created.
        code_generator_v8.generate_dummy_header_and_cpp(interface_name, options.output_directory)
        return
    # FIXME: turn on code generator
    # Currently idl_definitions must be None (so dummy .h and .cpp files are
    # generated), as actual code generator not present yet.
    # code_generator_v8.write_interface(idl_definitions, interface_name, options.output_directory)
    raise RuntimeError('Stub: code generator not implemented yet')


if __name__ == '__main__':
    sys.exit(main())
