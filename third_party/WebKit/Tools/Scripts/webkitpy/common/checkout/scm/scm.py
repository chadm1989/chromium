# Copyright (c) 2009, Google Inc. All rights reserved.
# Copyright (c) 2009 Apple Inc. All rights reserved.
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
# Python module for interacting with an SCM system (like SVN or Git)

import logging
import os
import re
import sys
import shutil

from webkitpy.common.system.deprecated_logging import error, log
from webkitpy.common.system.executive import Executive, ScriptError
from webkitpy.common.system import ospath


class CheckoutNeedsUpdate(ScriptError):
    def __init__(self, script_args, exit_code, output, cwd):
        ScriptError.__init__(self, script_args=script_args, exit_code=exit_code, output=output, cwd=cwd)


# FIXME: Should be moved onto SCM
def commit_error_handler(error):
    if re.search("resource out of date", error.output):
        raise CheckoutNeedsUpdate(script_args=error.script_args, exit_code=error.exit_code, output=error.output, cwd=error.cwd)
    Executive.default_error_handler(error)


class AuthenticationError(Exception):
    def __init__(self, server_host, prompt_for_password=False):
        self.server_host = server_host
        self.prompt_for_password = prompt_for_password



# SCM methods are expected to return paths relative to self.checkout_root.
class SCM:
    def __init__(self, cwd, executive=None):
        self.cwd = cwd
        self.checkout_root = self.find_checkout_root(self.cwd)
        self.dryrun = False
        self._executive = executive or Executive()

    # A wrapper used by subclasses to create processes.
    def run(self, args, cwd=None, input=None, error_handler=None, return_exit_code=False, return_stderr=True, decode_output=True):
        # FIXME: We should set cwd appropriately.
        return self._executive.run_command(args,
                           cwd=cwd,
                           input=input,
                           error_handler=error_handler,
                           return_exit_code=return_exit_code,
                           return_stderr=return_stderr,
                           decode_output=decode_output)

    # SCM always returns repository relative path, but sometimes we need
    # absolute paths to pass to rm, etc.
    def absolute_path(self, repository_relative_path):
        return os.path.join(self.checkout_root, repository_relative_path)

    # FIXME: This belongs in Checkout, not SCM.
    def scripts_directory(self):
        return os.path.join(self.checkout_root, "Tools", "Scripts")

    # FIXME: This belongs in Checkout, not SCM.
    def script_path(self, script_name):
        return os.path.join(self.scripts_directory(), script_name)

    def ensure_clean_working_directory(self, force_clean):
        if self.working_directory_is_clean():
            return
        if not force_clean:
            # FIXME: Shouldn't this use cwd=self.checkout_root?
            print self.run(self.status_command(), error_handler=Executive.ignore_error)
            raise ScriptError(message="Working directory has modifications, pass --force-clean or --no-clean to continue.")
        log("Cleaning working directory")
        self.clean_working_directory()

    def ensure_no_local_commits(self, force):
        if not self.supports_local_commits():
            return
        commits = self.local_commits()
        if not len(commits):
            return
        if not force:
            error("Working directory has local commits, pass --force-clean to continue.")
        self.discard_local_commits()

    def run_status_and_extract_filenames(self, status_command, status_regexp):
        filenames = []
        # We run with cwd=self.checkout_root so that returned-paths are root-relative.
        for line in self.run(status_command, cwd=self.checkout_root).splitlines():
            match = re.search(status_regexp, line)
            if not match:
                continue
            # status = match.group('status')
            filename = match.group('filename')
            filenames.append(filename)
        return filenames

    def strip_r_from_svn_revision(self, svn_revision):
        match = re.match("^r(?P<svn_revision>\d+)", unicode(svn_revision))
        if (match):
            return match.group('svn_revision')
        return svn_revision

    def svn_revision_from_commit_text(self, commit_text):
        match = re.search(self.commit_success_regexp(), commit_text, re.MULTILINE)
        return match.group('svn_revision')

    @staticmethod
    def _subclass_must_implement():
        raise NotImplementedError("subclasses must implement")

    @staticmethod
    def in_working_directory(path):
        SCM._subclass_must_implement()

    @staticmethod
    def find_checkout_root(path):
        SCM._subclass_must_implement()

    @staticmethod
    def commit_success_regexp():
        SCM._subclass_must_implement()

    def working_directory_is_clean(self):
        self._subclass_must_implement()

    def clean_working_directory(self):
        self._subclass_must_implement()

    def status_command(self):
        self._subclass_must_implement()

    def add(self, path, return_exit_code=False):
        self._subclass_must_implement()

    def delete(self, path):
        self._subclass_must_implement()

    def exists(self, path):
        self._subclass_must_implement()

    def changed_files(self, git_commit=None):
        self._subclass_must_implement()

    def changed_files_for_revision(self, revision):
        self._subclass_must_implement()

    def revisions_changing_file(self, path, limit=5):
        self._subclass_must_implement()

    def added_files(self):
        self._subclass_must_implement()

    def conflicted_files(self):
        self._subclass_must_implement()

    def display_name(self):
        self._subclass_must_implement()

    def head_svn_revision(self):
        self._subclass_must_implement()

    def create_patch(self, git_commit=None, changed_files=None):
        self._subclass_must_implement()

    def committer_email_for_revision(self, revision):
        self._subclass_must_implement()

    def contents_at_revision(self, path, revision):
        self._subclass_must_implement()

    def diff_for_revision(self, revision):
        self._subclass_must_implement()

    def diff_for_file(self, path, log=None):
        self._subclass_must_implement()

    def show_head(self, path):
        self._subclass_must_implement()

    def apply_reverse_diff(self, revision):
        self._subclass_must_implement()

    def revert_files(self, file_paths):
        self._subclass_must_implement()

    def commit_with_message(self, message, username=None, password=None, git_commit=None, force_squash=False, changed_files=None):
        self._subclass_must_implement()

    def svn_commit_log(self, svn_revision):
        self._subclass_must_implement()

    def last_svn_commit_log(self):
        self._subclass_must_implement()

    # Subclasses must indicate if they support local commits,
    # but the SCM baseclass will only call local_commits methods when this is true.
    @staticmethod
    def supports_local_commits():
        SCM._subclass_must_implement()

    def remote_merge_base():
        SCM._subclass_must_implement()

    def commit_locally_with_message(self, message):
        error("Your source control manager does not support local commits.")

    def discard_local_commits(self):
        pass

    def local_commits(self):
        return []
