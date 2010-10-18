# Copyright (C) 2010 Google Inc. All rights reserved.
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


import re


class Queue(object):

    # Eventually the list of queues may be stored in the data store.
    _all_queue_names = [
        "commit-queue",
        "style-queue",
        "chromium-ews",
        "qt-ews",
        "gtk-ews",
        "mac-ews",
        "win-ews",
        "efl-ews",
    ]

    def __init__(self, name):
        assert(name in self._all_queue_names)
        self._name = name

    @classmethod
    def queue_with_name(cls, queue_name):
        if queue_name not in cls._all_queue_names:
            return None
        return Queue(queue_name)

    @classmethod
    def all(cls):
        return [Queue(name) for name in cls._all_queue_names]

    def name(self):
        return self._name

    def _caplitalize_after_dash(self, string):
        return "-".join([word[0].upper() + word[1:] for word in string.split("-")])

    # For use in status bubbles or table headers
    def short_name(self):
        # HACK: chromium-ews is incorrectly named.
        short_name = self._name.replace("chromium-ews", "Cr-Linux-ews")
        short_name = short_name.replace("-ews", "")
        short_name = short_name.replace("-queue", "")
        return self._caplitalize_after_dash(short_name.capitalize())

    def display_name(self):
        # HACK: chromium-ews is incorrectly named.
        display_name = self._name.replace("chromium-ews", "cr-linux-ews")

        display_name = display_name.replace("-", " ")
        display_name = display_name.replace("cr", "chromium")
        display_name = display_name.title()
        display_name = display_name.replace("Ews", "EWS")
        return display_name

    _dash_regexp = re.compile("-")

    def name_with_underscores(self):
        return self._dash_regexp.sub("_", self._name)

    def is_ews(self):
        return self._name.endswith("-ews")
