# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A command to fetch new baselines from try jobs for a Rietveld issue.

This command interacts with the Rietveld API to get information about try jobs
with layout test results.
"""

import logging
import optparse

from webkitpy.common.checkout.scm.git import Git
from webkitpy.common.net.rietveld import latest_try_jobs
from webkitpy.common.net.web import Web
from webkitpy.layout_tests.models.test_expectations import BASELINE_SUFFIX_LIST
from webkitpy.tool.commands.rebaseline import AbstractParallelRebaselineCommand


_log = logging.getLogger(__name__)


class RebaselineFromTryJobs(AbstractParallelRebaselineCommand):
    name = "rebaseline-from-try-jobs"
    help_text = "Fetches new baselines from layout test runs on try bots."
    show_in_main_help = True

    def __init__(self):
        super(RebaselineFromTryJobs, self).__init__(options=[
            optparse.make_option(
                '--issue', type='int', default=None,
                help='Rietveld issue number; if none given, this will be obtained via `git cl issue`.'),
            optparse.make_option(
                '--dry-run', action='store_true', default=False,
                help='Dry run mode; list actions that would be performed but do not do anything.'),
            self.no_optimize_option,
            self.results_directory_option,
        ])
        self.web = Web()

    def execute(self, options, args, tool):
        issue_number = self._get_issue_number(options)
        if not issue_number:
            return
        if args:
            test_prefix_list = {}
            try_jobs = latest_try_jobs(issue_number, self._try_bots(), self.web)
            builders = [j.builder_name for j in try_jobs]
            for t in args:
                test_prefix_list[t] = {b: BASELINE_SUFFIX_LIST for b in builders}
        else:
            test_prefix_list = self._test_prefix_list(issue_number)
        self._log_test_prefix_list(test_prefix_list)

        if options.dry_run:
            return
        self._rebaseline(options, test_prefix_list, skip_checking_actual_results=True)

    def _get_issue_number(self, options):
        """Gets the Rietveld CL number from either |options| or from the current local branch."""
        if options.issue:
            return options.issue
        issue_number = self._git().get_issue_number()
        _log.debug('Issue number for current branch: %s' % issue_number)
        if not issue_number.isdigit():
            _log.error('No issue number given and no issue for current branch.')
            return None
        return int(issue_number)

    def _git(self):
        """Returns a Git instance; can be overridden for tests."""
        # Pass in a current working directory inside of the repo so
        # that this command can be called from outside of the repo.
        return Git(cwd=self._tool.filesystem.dirname(self._tool.path()))

    def _test_prefix_list(self, issue_number):
        """Returns a collection of test, builder and file extensions to get new baselines for."""
        builders_to_tests = self._builders_to_tests(issue_number)
        result = {}
        for builder, tests in builders_to_tests.iteritems():
            for test in tests:
                if test not in result:
                    result[test] = {}
                # TODO(qyearsley): Consider using TestExpectations.suffixes_for_test_result.
                result[test][builder] = BASELINE_SUFFIX_LIST
        return result

    def _builders_to_tests(self, issue_number):
        """Fetches a list of try bots, and for each, fetches tests with new baselines."""
        _log.debug('Getting results for Rietveld issue %d.' % issue_number)
        try_jobs = latest_try_jobs(issue_number, self._try_bots(), self.web)
        if not try_jobs:
            _log.debug('No try job results for builders in: %r.' % (self._try_bots(),))
        builders_to_tests = {}
        for job in try_jobs:
            test_results = self._unexpected_mismatch_results(job)
            builders_to_tests[job.builder_name] = sorted(r.test_name() for r in test_results)
        return builders_to_tests

    def _try_bots(self):
        """Retuns a collection of try bot builders to fetch results for."""
        return self._tool.builders.all_try_builder_names()

    def _unexpected_mismatch_results(self, try_job):
        """Fetches a list of LayoutTestResult objects for unexpected results with new baselines."""
        results_url = self._results_url(try_job.builder_name, try_job.build_number)
        builder = self._tool.buildbot.builder_with_name(try_job.builder_name)
        layout_test_results = builder.fetch_layout_test_results(results_url)
        if layout_test_results is None:
            _log.warning('Failed to request layout test results from "%s".', results_url)
            return []
        return layout_test_results.unexpected_mismatch_results()

    @staticmethod
    def _log_test_prefix_list(test_prefix_list):
        """Logs the tests to download new baselines for."""
        if not test_prefix_list:
            _log.info('No tests to rebaseline.')
            return
        _log.info('Tests to rebaseline:')
        for test, builders in test_prefix_list.iteritems():
            _log.info('  %s: %s', test, ', '.join(sorted(builders)))
