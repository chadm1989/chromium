#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Adaptor script called through build/isolate.gypi.

Creates a wrapping .isolate which 'includes' the original one, that can be
consumed by tools/swarming_client/isolate.py. Path variables are determined
based on the current working directory. The relative_cwd in the .isolated file
is determined based on the .isolate file that declare the 'command' variable to
be used so the wrapping .isolate doesn't affect this value.

This script loads build.ninja and processes it to determine all the executables
referenced by the isolated target. It adds them in the wrapping .isolate file.
"""

import StringIO
import glob
import logging
import os
import posixpath
import subprocess
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
SWARMING_CLIENT_DIR = os.path.join(TOOLS_DIR, 'swarming_client')
SRC_DIR = os.path.dirname(TOOLS_DIR)

sys.path.insert(0, SWARMING_CLIENT_DIR)

import isolate_format


def load_ninja_recursively(build_dir, ninja_path, build_steps):
  """Crudely extracts all the subninja and build referenced in ninja_path.

  In particular, it ignores rule and variable declarations. The goal is to be
  performant (well, as much as python can be performant) which is currently in
  the <200ms range for a complete chromium tree. As such the code is laid out
  for performance instead of readability.
  """
  logging.debug('Loading %s', ninja_path)
  try:
    with open(os.path.join(build_dir, ninja_path), 'rb') as f:
      line = None
      merge_line = ''
      subninja = []
      for line in f:
        line = line.rstrip()
        if not line:
          continue

        if line[-1] == '$':
          # The next line needs to be merged in.
          merge_line += line[:-1]
          continue

        if merge_line:
          line = merge_line + line
          merge_line = ''

        statement = line[:line.find(' ')]
        if statement == 'build':
          # Save the dependency list as a raw string. Only the lines needed will
          # be processed with raw_build_to_deps(). This saves a good 70ms of
          # processing time.
          build_target, dependencies = line[6:].split(': ', 1)
          # Interestingly, trying to be smart and only saving the build steps
          # with the intended extensions ('', '.stamp', '.so') slows down
          # parsing even if 90% of the build rules can be skipped.
          # On Windows, a single step may generate two target, so split items
          # accordingly. It has only been seen for .exe/.exe.pdb combos.
          for i in build_target.strip().split():
            build_steps[i] = dependencies
        elif statement == 'subninja':
          subninja.append(line[9:])
  except IOError:
    print >> sys.stderr, 'Failed to open %s' % ninja_path
    raise

  total = 1
  for rel_path in subninja:
    try:
      # Load each of the files referenced.
      # TODO(maruel): Skip the files known to not be needed. It saves an aweful
      # lot of processing time.
      total += load_ninja_recursively(build_dir, rel_path, build_steps)
    except IOError:
      print >> sys.stderr, '... as referenced by %s' % ninja_path
      raise
  return total


def load_ninja(build_dir):
  """Loads the tree of .ninja files in build_dir."""
  build_steps = {}
  total = load_ninja_recursively(build_dir, 'build.ninja', build_steps)
  logging.info('Loaded %d ninja files, %d build steps', total, len(build_steps))
  return build_steps


def using_blacklist(item):
  """Returns True if an item should be analyzed.

  Ignores many rules that are assumed to not depend on a dynamic library. If
  the assumption doesn't hold true anymore for a file format, remove it from
  this list. This is simply an optimization.
  """
  IGNORED = (
    '.a', '.cc', '.css', '.def', '.h', '.html', '.js', '.json', '.manifest',
    '.o', '.obj', '.pak', '.png', '.pdb', '.strings', '.txt',
  )
  # ninja files use native path format.
  ext = os.path.splitext(item)[1]
  if ext in IGNORED:
    return False
  # Special case Windows, keep .dll.lib but discard .lib.
  if item.endswith('.dll.lib'):
    return True
  if ext == '.lib':
    return False
  return item not in ('', '|', '||')


def raw_build_to_deps(item):
  """Converts a raw ninja build statement into the list of interesting
  dependencies.
  """
  # TODO(maruel): Use a whitelist instead? .stamp, .so.TOC, .dylib.TOC,
  # .dll.lib, .exe and empty.
  # The first item is the build rule, e.g. 'link', 'cxx', 'phony', etc.
  return filter(using_blacklist, item.split(' ')[1:])


def recurse(target, build_steps, rules_seen):
  """Recursively returns all the interesting dependencies for root_item."""
  out = []
  if rules_seen is None:
    rules_seen = set()
  if target in rules_seen:
    # TODO(maruel): Figure out how it happens.
    logging.warning('Circular dependency for %s!', target)
    return []
  rules_seen.add(target)
  try:
    dependencies = raw_build_to_deps(build_steps[target])
  except KeyError:
    logging.info('Failed to find a build step to generate: %s', target)
    return []
  logging.debug('recurse(%s) -> %s', target, dependencies)
  for dependency in dependencies:
    out.append(dependency)
    dependency_raw_dependencies = build_steps.get(dependency)
    if dependency_raw_dependencies:
      for i in raw_build_to_deps(dependency_raw_dependencies):
        out.extend(recurse(i, build_steps, rules_seen))
    else:
      logging.info('Failed to find a build step to generate: %s', dependency)
  return out


def post_process_deps(build_dir, dependencies):
  """Processes the dependency list with OS specific rules."""
  def filter_item(i):
    if i.endswith('.so.TOC'):
      # Remove only the suffix .TOC, not the .so!
      return i[:-4]
    if i.endswith('.dylib.TOC'):
      # Remove only the suffix .TOC, not the .dylib!
      return i[:-4]
    if i.endswith('.dll.lib'):
      # Remove only the suffix .lib, not the .dll!
      return i[:-4]
    return i

  # Check for execute access. This gets rid of all the phony rules.
  return [
    i for i in map(filter_item, dependencies)
    if os.access(os.path.join(build_dir, i), os.X_OK)
  ]


def create_wrapper(args, isolate_index, isolated_index):
  """Creates a wrapper .isolate that add dynamic libs.

  The original .isolate is not modified.
  """
  cwd = os.getcwd()
  isolate = args[isolate_index]
  # The code assumes the .isolate file is always specified path-less in cwd. Fix
  # if this assumption doesn't hold true.
  assert os.path.basename(isolate) == isolate, isolate

  # This will look like ../out/Debug. This is based against cwd. Note that this
  # must equal the value provided as PRODUCT_DIR.
  build_dir = os.path.dirname(args[isolated_index])

  # This will look like chrome/unit_tests.isolate. It is based against SRC_DIR.
  # It's used to calculate temp_isolate.
  src_isolate = os.path.relpath(os.path.join(cwd, isolate), SRC_DIR)

  # The wrapping .isolate. This will look like
  # ../out/Debug/gen/chrome/unit_tests.isolate.
  temp_isolate = os.path.join(build_dir, 'gen', src_isolate)
  temp_isolate_dir = os.path.dirname(temp_isolate)

  # Relative path between the new and old .isolate file.
  isolate_relpath = os.path.relpath(
      '.', temp_isolate_dir).replace(os.path.sep, '/')

  # It's a big assumption here that the name of the isolate file matches the
  # primary target. Fix accordingly if this doesn't hold true.
  target = isolate[:-len('.isolate')]
  build_steps = load_ninja(build_dir)
  binary_deps = post_process_deps(build_dir, recurse(target, build_steps, None))
  logging.debug(
      'Binary dependencies:%s', ''.join('\n  ' + i for i in binary_deps))

  # Now do actual wrapping .isolate.
  isolate_dict = {
    'includes': [
      posixpath.join(isolate_relpath, isolate),
    ],
    'variables': {
      # Will look like ['<(PRODUCT_DIR)/lib/flibuser_prefs.so'].
      isolate_format.KEY_TRACKED: sorted(
          '<(PRODUCT_DIR)/%s' % i.replace(os.path.sep, '/')
          for i in binary_deps),
    },
  }
  if not os.path.isdir(temp_isolate_dir):
    os.makedirs(temp_isolate_dir)
  comment = (
      '# Warning: this file was AUTOGENERATED.\n'
      '# DO NO EDIT.\n')
  out = StringIO.StringIO()
  isolate_format.print_all(comment, isolate_dict, out)
  isolate_content = out.getvalue()
  with open(temp_isolate, 'wb') as f:
    f.write(isolate_content)
  logging.info('Added %d dynamic libs', len(binary_deps))
  logging.debug('%s', isolate_content)
  args[isolate_index] = temp_isolate


def main():
  logging.basicConfig(level=logging.ERROR, format='%(levelname)7s %(message)s')
  args = sys.argv[1:]
  isolate = None
  isolated = None
  is_component = False
  for i, arg in enumerate(args):
    if arg == '--isolate':
      isolate = i + 1
    if arg == '--isolated':
      isolated = i + 1
    if arg == 'component=shared_library':
      is_component = True
  if isolate is None or isolated is None:
    print >> sys.stderr, 'Internal failure'
    return 1

  if is_component:
    create_wrapper(args, isolate, isolated)

  swarming_client = os.path.join(SRC_DIR, 'tools', 'swarming_client')
  sys.stdout.flush()
  result = subprocess.call(
      [sys.executable, os.path.join(swarming_client, 'isolate.py')] + args)
  return result


if __name__ == '__main__':
  sys.exit(main())
