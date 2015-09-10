# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import threading

from pylib import constants

# TODO(jbudorick): Remove this once the blacklist is optional.
BLACKLIST_JSON = os.path.join(
    constants.DIR_SOURCE_ROOT,
    os.environ.get('CHROMIUM_OUT_DIR', 'out'),
    'bad_devices.json')

class Blacklist(object):

  def __init__(self, path):
    self._blacklist_lock = threading.RLock()
    self._path = path

  def Read(self):
    """Reads the blacklist from the blacklist file.

    Returns:
      A list containing bad devices.
    """
    with self._blacklist_lock:
      if not os.path.exists(self._path):
        return []

      with open(self._path, 'r') as f:
        return json.load(f)

  def Write(self, blacklist):
    """Writes the provided blacklist to the blacklist file.

    Args:
      blacklist: list of bad devices to write to the blacklist file.
    """
    with self._blacklist_lock:
      with open(self._path, 'w') as f:
        json.dump(list(set(blacklist)), f)

  def Extend(self, devices):
    """Adds devices to blacklist file.

    Args:
      devices: list of bad devices to be added to the blacklist file.
    """
    logging.info('Adding %s to blacklist %s', ','.join(devices), self._path)
    with self._blacklist_lock:
      blacklist = self.Read()
      blacklist.extend(devices)
      self.Write(blacklist)

  def Reset(self):
    """Erases the blacklist file if it exists."""
    logging.info('Resetting blacklist %s', self._path)
    with self._blacklist_lock:
      if os.path.exists(self._path):
        os.remove(self._path)

