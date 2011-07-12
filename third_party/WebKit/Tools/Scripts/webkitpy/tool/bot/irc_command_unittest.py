# Copyright (c) 2010 Google Inc. All rights reserved.
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

import unittest

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.tool.bot.irc_command import *
from webkitpy.tool.mocktool import MockTool, MockExecutive


class IRCCommandTest(unittest.TestCase):
    def test_eliza(self):
        eliza = Eliza()
        eliza.execute("tom", "hi", None, None)
        eliza.execute("tom", "bye", None, None)

    def test_whois(self):
        whois = Whois()
        self.assertEquals("tom: Usage: BUGZILLA_EMAIL",
                          whois.execute("tom", [], None, None))
        self.assertEquals("tom: Usage: BUGZILLA_EMAIL",
                          whois.execute("tom", ["Adam", "Barth"], None, None))
        self.assertEquals("tom: Sorry, I don't know unknown@example.com. Maybe you could introduce me?",
                          whois.execute("tom", ["unknown@example.com"], None, None))
        self.assertEquals("tom: tonyg@chromium.org is tonyg-cr. Why do you ask?",
                          whois.execute("tom", ["tonyg@chromium.org"], None, None))
        self.assertEquals("tom: vicki@apple.com hasn't told me their nick. Boo hoo :-(",
                          whois.execute("tom", ["vicki@apple.com"], None, None))

    def test_create_bug(self):
        create_bug = CreateBug()
        self.assertEquals("tom: Usage: BUG_TITLE",
                          create_bug.execute("tom", [], None, None))

        example_args = ["sherrif-bot", "should", "have", "a", "create-bug", "command"]
        tool = MockTool()

        # MockBugzilla has a create_bug, but it logs to stderr, this avoids any logging.
        tool.bugs.create_bug = lambda a, b, cc=None, assignee=None: 78
        self.assertEquals("tom: Created bug: http://example.com/78",
                          create_bug.execute("tom", example_args, tool, None))

        def mock_create_bug(title, description, cc=None, assignee=None):
            raise Exception("Exception from bugzilla!")
        tool.bugs.create_bug = mock_create_bug
        self.assertEquals("tom: Failed to create bug:\nException from bugzilla!",
                          create_bug.execute("tom", example_args, tool, None))

    def test_roll_chromium_deps(self):
        roll = RollChromiumDEPS()
        self.assertEquals(None, roll._parse_args([]))
        self.assertEquals("1234", roll._parse_args(["1234"]))

    def test_rollout_updates_working_copy(self):
        rollout = Rollout()
        tool = MockTool()
        tool.executive = MockExecutive(should_log=True)
        expected_stderr = "MOCK run_and_throw_if_fail: ['mock-update-webkit']\n"
        OutputCapture().assert_outputs(self, rollout._update_working_copy, [tool], expected_stderr=expected_stderr)

    def test_rollout(self):
        rollout = Rollout()
        self.assertEquals(([1234], "testing foo"),
                          rollout._parse_args(["1234", "testing", "foo"]))

        # Test invalid argument parsing:
        self.assertEquals((None, None), rollout._parse_args([]))
        self.assertEquals((None, None), rollout._parse_args(["--bar", "1234"]))

        # Invalid arguments result in the USAGE message.
        self.assertEquals("tom: Usage: SVN_REVISION [SVN_REVISIONS] REASON",
                          rollout.execute("tom", [], None, None))

        # FIXME: We need a better way to test IRCCommands which call tool.irc().post()
