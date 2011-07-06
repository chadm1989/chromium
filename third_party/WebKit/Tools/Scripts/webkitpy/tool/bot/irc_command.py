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

import itertools
import random
from webkitpy.common.config import irc as config_irc

from webkitpy.common.config import urls
from webkitpy.common.config.committers import CommitterList
from webkitpy.common.checkout.changelog import parse_bug_id
from webkitpy.common.system.executive import ScriptError
from webkitpy.tool.bot.queueengine import TerminateQueue
from webkitpy.tool.grammar import join_with_separators


def _post_error_and_check_for_bug_url(tool, nicks_string, exception):
    tool.irc().post("%s" % exception)
    bug_id = parse_bug_id(exception.output)
    if bug_id:
        bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
        tool.irc().post("%s: Ugg...  Might have created %s" % (nicks_string, bug_url))


# FIXME: Merge with Command?
class IRCCommand(object):
    def execute(self, nick, args, tool, sheriff):
        raise NotImplementedError, "subclasses must implement"


class LastGreenRevision(IRCCommand):
    def execute(self, nick, args, tool, sheriff):
        return "%s: %s" % (nick,
            urls.view_revision_url(tool.buildbot.last_green_revision()))


class Restart(IRCCommand):
    def execute(self, nick, args, tool, sheriff):
        tool.irc().post("Restarting...")
        raise TerminateQueue()


class Rollout(IRCCommand):
    def _parse_args(self, args):
        if not args:
            return (None, None)

        # the first argument must be a revision number
        first_revision = args[0].lstrip("r")
        if not first_revision.isdigit():
            return (None, None)

        parsing_revision = True
        svn_revision_list = [int(first_revision)]
        rollout_reason = []
        for arg in args[1:]:
            if arg.lstrip("r").isdigit() and parsing_revision:
                svn_revision_list.append(int(arg.lstrip("r")))
            else:
                parsing_revision = False
                rollout_reason.append(arg)

        rollout_reason = " ".join(rollout_reason)
        return svn_revision_list, rollout_reason

    def _responsible_nicknames_from_revisions(self, tool, sheriff, svn_revision_list):
        commit_infos = map(tool.checkout().commit_info_for_revision, svn_revision_list)
        nickname_lists = map(sheriff.responsible_nicknames_from_commit_info, commit_infos)
        return sorted(set(itertools.chain(*nickname_lists)))

    def _nicks_string(self, tool, sheriff, requester_nick, svn_revision_list):
        # FIXME: _parse_args guarentees that our svn_revision_list is all numbers.
        # However, it's possible our checkout will not include one of the revisions,
        # so we may need to catch exceptions from commit_info_for_revision here.
        target_nicks = [requester_nick] + self._responsible_nicknames_from_revisions(tool, sheriff, svn_revision_list)
        return ", ".join(target_nicks)

    def execute(self, nick, args, tool, sheriff):
        svn_revision_list, rollout_reason = self._parse_args(args)

        if (not svn_revision_list or not rollout_reason):
            # return is equivalent to an irc().post(), but makes for easier unit testing.
            return "%s: Usage: SVN_REVISION [SVN_REVISIONS] REASON" % nick

        # FIXME: IRCCommand should bind to a tool and have a self._tool like Command objects do.
        # Likewise we should probably have a self._sheriff.
        nicks_string = self._nicks_string(tool, sheriff, nick, svn_revision_list)

        revision_urls_string = join_with_separators([urls.view_revision_url(revision) for revision in svn_revision_list])
        tool.irc().post("%s: Preparing rollout for %s..." % (nicks_string, revision_urls_string))

        try:
            complete_reason = "%s (Requested by %s on %s)." % (
                rollout_reason, nick, config_irc.channel)
            bug_id = sheriff.post_rollout_patch(svn_revision_list, complete_reason)
            bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
            tool.irc().post("%s: Created rollout: %s" % (nicks_string, bug_url))
        except ScriptError, e:
            tool.irc().post("%s: Failed to create rollout patch:" % nicks_string)
            _post_error_and_check_for_bug_url(tool, nicks_string, e)


class RollChromiumDEPS(IRCCommand):
    def _parse_args(self, args):
        if not args:
            return
        revision = args[0].lstrip("r")
        if not revision.isdigit():
            return
        return revision

    def execute(self, nick, args, tool, sheriff):
        revision = self._parse_args(args)

        roll_target = "r%s" % revision if revision else "last-known good revision"
        tool.irc().post("%s: Rolling Chromium DEPS to %s" % (nick, roll_target))

        try:
            bug_id = sheriff.post_chromium_deps_roll(revision)
            bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
            tool.irc().post("%s: Created DEPS roll: %s" % (nick, bug_url))
        except ScriptError, e:
            match = re.search(r"Current Chromium DEPS revision \d+ is newer than \d+\.", e.output)
            if match:
                tool.irc().post("%s: %s" % (nick, match.group(0)))
                return
            tool.irc().post("%s: Failed to create DEPS roll:" % nick)
            _post_error_and_check_for_bug_url(tool, nick, e)


class Help(IRCCommand):
    def execute(self, nick, args, tool, sheriff):
        return "%s: Available commands: %s" % (nick, ", ".join(commands.keys()))


class Hi(IRCCommand):
    def execute(self, nick, args, tool, sheriff):
        quips = tool.bugs.quips()
        quips.append('"Only you can prevent forest fires." -- Smokey the Bear')
        return random.choice(quips)


class Whois(IRCCommand):
    def execute(self, nick, args, tool, sheriff):
        if len(args) != 1:
            return "%s: Usage: BUGZILLA_EMAIL" % nick
        email = args[0]
        # FIXME: We should get the ContributorList off the tool somewhere.
        committer = CommitterList().contributor_by_email(email)
        if not committer:
            return "%s: Sorry, I don't know %s. Maybe you could introduce me?" % (nick, email)
        if not committer.irc_nickname:
            return "%s: %s hasn't told me their nick. Boo hoo :-(" % (nick, email)
        return "%s: %s is %s. Why do you ask?" % (nick, email, committer.irc_nickname)


class Eliza(IRCCommand):
    therapist = None

    def __init__(self):
        if not self.therapist:
            import webkitpy.thirdparty.autoinstalled.eliza as eliza
            Eliza.therapist = eliza.eliza()

    def execute(self, nick, args, tool, sheriff):
        return "%s: %s" % (nick, self.therapist.respond(" ".join(args)))


class CreateBug(IRCCommand):
    def execute(self, nick, args, tool, sheriff):
        if not args:
            return "%s: Usage: BUG_TITLE" % nick

        bug_title = " ".join(args)
        bug_description = "%s\nRequested by %s on %s." % (bug_title, nick, config_irc.channel)

        # There happens to be a committers list hung off of Bugzilla, so
        # re-using that one makes things easiest for now.
        requester = tool.bugs.committers.contributor_by_irc_nickname(nick)
        requester_email = requester.bugzilla_email() if requester else None

        try:
            bug_id = tool.bugs.create_bug(bug_title, bug_description, cc=requester_email, assignee=requester_email)
            bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
            return "%s: Created bug: %s" % (nick, bug_url)
        except Exception, e:
            return "%s: Failed to create bug:\n%s" % (nick, e)


# FIXME: Lame.  We should have an auto-registering CommandCenter.
commands = {
    "help": Help,
    "hi": Hi,
    "last-green-revision": LastGreenRevision,
    "restart": Restart,
    "rollout": Rollout,
    "whois": Whois,
    "create-bug": CreateBug,
    "roll-chromium-deps": RollChromiumDEPS,
}
