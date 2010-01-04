# Copyright (C) 2009 Google Inc. All rights reserved.
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
#
# WebKit's Python module for interacting with the Commit Queue status page.

from webkitpy.webkit_logging import log
from mechanize import Browser

# WebKit includes a built copy of BeautifulSoup in Scripts/webkitpy
# so this import should always succeed.
from .BeautifulSoup import BeautifulSoup

import urllib2


class StatusServer:
    default_host = "webkit-commit-queue.appspot.com"

    def __init__(self, host=default_host):
        self.set_host(host)
        self.browser = Browser()

    def set_host(self, host):
        self.host = host
        self.url = "http://%s" % self.host

    def results_url_for_status(self, status_id):
        return "%s/results/%s" % (self.url, status_id)

    def update_status(self, queue_name, status, patch=None, results_file=None):
        # During unit testing, host is None
        if not self.host:
            return

        log(status)
        update_status_url = "%s/update-status" % self.url
        self.browser.open(update_status_url)
        self.browser.select_form(name="update_status")
        self.browser['queue_name'] = queue_name
        if patch:
            if patch.get('bug_id'):
                self.browser['bug_id'] = str(patch['bug_id'])
            if patch.get('id'):
                self.browser['patch_id'] = str(patch['id'])
        self.browser['status'] = status
        if results_file:
            self.browser.add_file(results_file, "text/plain", "results.txt", 'results_file')
        response = self.browser.submit()
        return response.read() # This is the id of the newly created status object.

    def patch_status(self, queue_name, patch_id):
        update_status_url = "%s/patch-status/%s/%s" % (self.url, queue_name, patch_id)
        try:
            return urllib2.urlopen(update_status_url).read()
        except urllib2.HTTPError, e:
            if e.code == 404:
                return None
            raise e
