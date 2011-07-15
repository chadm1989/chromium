#!/usr/bin/env python
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

"""A helper class for reading in and dealing with tests expectations
for layout tests.
"""

import itertools
import logging
import re

import webkitpy.thirdparty.simplejson as simplejson


_log = logging.getLogger(__name__)


# Test expectation and modifier constants.
# FIXME: range() starts with 0 which makes if expectation checks harder
# as PASS is 0.
(PASS, FAIL, TEXT, IMAGE, IMAGE_PLUS_TEXT, AUDIO, TIMEOUT, CRASH, SKIP, WONTFIX,
 SLOW, REBASELINE, MISSING, FLAKY, NOW, NONE) = range(16)

# Test expectation file update action constants
(NO_CHANGE, REMOVE_TEST, REMOVE_PLATFORM, ADD_PLATFORMS_EXCEPT_THIS) = range(4)


def result_was_expected(result, expected_results, test_needs_rebaselining, test_is_skipped):
    """Returns whether we got a result we were expecting.
    Args:
        result: actual result of a test execution
        expected_results: set of results listed in test_expectations
        test_needs_rebaselining: whether test was marked as REBASELINE
        test_is_skipped: whether test was marked as SKIP"""
    if result in expected_results:
        return True
    if result in (IMAGE, TEXT, IMAGE_PLUS_TEXT) and FAIL in expected_results:
        return True
    if result == MISSING and test_needs_rebaselining:
        return True
    if result == SKIP and test_is_skipped:
        return True
    return False


def remove_pixel_failures(expected_results):
    """Returns a copy of the expected results for a test, except that we
    drop any pixel failures and return the remaining expectations. For example,
    if we're not running pixel tests, then tests expected to fail as IMAGE
    will PASS."""
    expected_results = expected_results.copy()
    if IMAGE in expected_results:
        expected_results.remove(IMAGE)
        expected_results.add(PASS)
    if IMAGE_PLUS_TEXT in expected_results:
        expected_results.remove(IMAGE_PLUS_TEXT)
        expected_results.add(TEXT)
    return expected_results


def has_pixel_failures(actual_results):
    return IMAGE in actual_results or IMAGE_PLUS_TEXT in actual_results


# FIXME: This method is no longer used here in this module. Remove remaining callsite in manager.py and this method.
def strip_comments(line):
    """Strips comments from a line and return None if the line is empty
    or else the contents of line with leading and trailing spaces removed
    and all other whitespace collapsed"""

    commentIndex = line.find('//')
    if commentIndex is -1:
        commentIndex = len(line)

    line = re.sub(r'\s+', ' ', line[:commentIndex].strip())
    if line == '':
        return None
    else:
        return line


class ParseError(Exception):
    def __init__(self, fatal, errors):
        self.fatal = fatal
        self.errors = errors

    def __str__(self):
        return '\n'.join(map(str, self.errors))

    def __repr__(self):
        return 'ParseError(fatal=%s, errors=%s)' % (self.fatal, self.errors)


class ModifiersAndExpectations:
    """A holder for modifiers and expectations on a test that serializes to
    JSON."""

    def __init__(self, modifiers, expectations):
        self.modifiers = modifiers
        self.expectations = expectations


class ExpectationsJsonEncoder(simplejson.JSONEncoder):
    """JSON encoder that can handle ModifiersAndExpectations objects."""
    def default(self, obj):
        # A ModifiersAndExpectations object has two fields, each of which
        # is a dict. Since JSONEncoders handle all the builtin types directly,
        # the only time this routine should be called is on the top level
        # object (i.e., the encoder shouldn't recurse).
        assert isinstance(obj, ModifiersAndExpectations)
        return {"modifiers": obj.modifiers,
                "expectations": obj.expectations}


class TestExpectationSerializer:
    """Provides means of serializing TestExpectationLine instances."""
    @classmethod
    def to_string(cls, expectation):
        result = []
        if expectation.is_malformed():
            result.append(expectation.comment)
        else:
            if expectation.name is None:
                if expectation.comment is not None:
                    result.append('//')
                    result.append(expectation.comment)
            else:
                result.append("%s : %s = %s" % (" ".join(expectation.modifiers).upper(), expectation.name, " ".join(expectation.expectations).upper()))
                if expectation.comment is not None:
                    result.append(" //%s" % (expectation.comment))

        return ''.join(result)

    @classmethod
    def list_to_string(cls, expectations):
        return "\n".join([TestExpectationSerializer.to_string(expectation) for expectation in expectations])


class TestExpectationParser:
    """Provides parsing facilities for lines in the test_expectation.txt file."""

    def __init__(self, port, test_config, full_test_list, allow_rebaseline_modifier):
        self._port = port
        self._matcher = ModifierMatcher(test_config)
        self._full_test_list = full_test_list
        self._allow_rebaseline_modifier = allow_rebaseline_modifier

    def parse(self, expectation):
        expectation.num_matches = self._check_options(self._matcher, expectation)
        if expectation.num_matches == ModifierMatcher.NO_MATCH:
            return

        self._check_options_against_expectations(expectation)

        if self._check_path_does_not_exist(expectation):
            return

        if not self._full_test_list:
            expectation.matching_tests = [expectation.name]
        else:
            expectation.matching_tests = self._collect_matching_tests(expectation.name)

        expectation.parsed_modifiers = [modifier for modifier in expectation.modifiers if modifier in TestExpectations.MODIFIERS]
        self._parse_expectations(expectation)

    def _parse_expectations(self, expectation_line):
        result = set()
        for part in expectation_line.expectations:
            expectation = TestExpectations.expectation_from_string(part)
            if expectation is None:  # Careful, PASS is currently 0.
                expectation_line.errors.append('Unsupported expectation: %s' % part)
                continue
            result.add(expectation)
        expectation_line.parsed_expectations = result

    def _check_options(self, matcher, expectation):
        match_result = matcher.match(expectation)
        self._check_semantics(expectation)
        return match_result.num_matches

    def _check_semantics(self, expectation):
        has_wontfix = 'wontfix' in expectation.modifiers
        has_bug = False
        for opt in expectation.modifiers:
            if opt.startswith('bug'):
                has_bug = True
                if re.match('bug\d+', opt):
                    expectation.errors.append('BUG\d+ is not allowed, must be one of BUGCR\d+, BUGWK\d+, BUGV8_\d+, or a non-numeric bug identifier.')

        if not has_bug and not has_wontfix:
            expectation.warnings.append('Test lacks BUG modifier.')

        if self._allow_rebaseline_modifier and 'rebaseline' in expectation.modifiers:
            expectation.errors.append('REBASELINE should only be used for running rebaseline.py. Cannot be checked in.')

    def _check_options_against_expectations(self, expectation):
        if 'slow' in expectation.modifiers and 'timeout' in expectation.expectations:
            expectation.errors.append('A test can not be both SLOW and TIMEOUT. If it times out indefinitely, then it should be just TIMEOUT.')

    def _check_path_does_not_exist(self, expectation):
        # WebKit's way of skipping tests is to add a -disabled suffix.
        # So we should consider the path existing if the path or the
        # -disabled version exists.
        if (not self._port.test_exists(expectation.name)
            and not self._port.test_exists(expectation.name + '-disabled')):
            # Log a warning here since you hit this case any
            # time you update test_expectations.txt without syncing
            # the LayoutTests directory
            expectation.warnings.append('Path does not exist.')
            return True
        return False

    def _collect_matching_tests(self, test_list_path):
        """Convert the test specification to an absolute, normalized
        path and make sure directories end with the OS path separator."""
        # FIXME: full_test_list can quickly contain a big amount of
        # elements. We should consider at some point to use a more
        # efficient structure instead of a list. Maybe a dictionary of
        # lists to represent the tree of tests, leaves being test
        # files and nodes being categories.

        if self._port.test_isdir(test_list_path):
            # this is a test category, return all the tests of the category.
            test_list_path = self._port.normalize_test_name(test_list_path)

            return [test for test in self._full_test_list if test.startswith(test_list_path)]

        # this is a test file, do a quick check if it's in the
        # full test suite.
        result = []
        if test_list_path in self._full_test_list:
            result = [test_list_path, ]
        return result

    @classmethod
    def tokenize(cls, expectation_string):
        """Tokenizes a line from test_expectations.txt and returns an unparsed TestExpectationLine instance.

        The format of a test expectation line is:

        [[<modifiers>] : <name> = <expectations>][ //<comment>]

        Any errant whitespace is not preserved.

        """
        expectation = TestExpectationLine()
        comment_index = expectation_string.find("//")
        if comment_index == -1:
            comment_index = len(expectation_string)
        else:
            expectation.comment = expectation_string[comment_index + 2:]

        remaining_string = re.sub(r"\s+", " ", expectation_string[:comment_index].strip())
        if len(remaining_string) == 0:
            return expectation

        parts = remaining_string.split(':')
        if len(parts) != 2:
            expectation.errors.append("Missing a ':'" if len(parts) < 2 else "Extraneous ':'")
        else:
            test_and_expectation = parts[1].split('=')
            if len(test_and_expectation) != 2:
                expectation.errors.append("Missing expectations" if len(test_and_expectation) < 2 else "Extraneous '='")

        if expectation.is_malformed():
            expectation.comment = expectation_string
        else:
            expectation.modifiers = cls._split_space_separated(parts[0])
            expectation.name = test_and_expectation[0].strip()
            expectation.expectations = cls._split_space_separated(test_and_expectation[1])

        return expectation

    @classmethod
    def tokenize_list(cls, expectations_string):
        """Returns a list of TestExpectationLines, one for each line in expectations_string."""
        expectations = []
        for line in expectations_string.split("\n"):
            expectations.append(cls.tokenize(line))
        return expectations

    @classmethod
    def _split_space_separated(cls, space_separated_string):
        """Splits a space-separated string into an array."""
        # FIXME: Lower-casing is necessary to support legacy code. Need to eliminate.
        return [part.strip().lower() for part in space_separated_string.strip().split(' ')]


class TestExpectationLine:
    """Represents a line in test expectations file."""

    def __init__(self):
        """Initializes a blank-line equivalent of an expectation."""
        self.name = None
        self.modifiers = []
        self.parsed_modifiers = []
        self.expectations = []
        self.parsed_expectations = set()
        self.comment = None
        self.num_matches = ModifierMatcher.NO_MATCH
        self.matching_tests = []
        self.errors = []
        self.warnings = []

    def is_malformed(self):
        return len(self.errors) > 0

    def is_invalid(self):
        return self.is_malformed() or len(self.warnings) > 0

# FIXME: Refactor to use TestExpectationLine as data item.
# FIXME: Refactor API to be a proper CRUD.
class TestExpectationsModel:
    """Represents relational store of all expectations and provides CRUD semantics to manage it."""

    def __init__(self, port):
        self._port = port

        # Maps a test to its list of expectations.
        self._test_to_expectations = {}

        # Maps a test to its list of options (string values)
        self._test_to_options = {}

        # Maps a test to its list of modifiers: the constants associated with
        # the options minus any bug or platform strings
        self._test_to_modifiers = {}

        # Maps a test to the base path that it was listed with in the list and
        # the number of matches that base path had.
        self._test_list_paths = {}

        # List of tests that are in the overrides file (used for checking for
        # duplicates inside the overrides file itself). Note that just because
        # a test is in this set doesn't mean it's necessarily overridding a
        # expectation in the regular expectations; the test might not be
        # mentioned in the regular expectations file at all.
        self._overridding_tests = set()

        self._modifier_to_tests = self._dict_of_sets(TestExpectations.MODIFIERS)
        self._expectation_to_tests = self._dict_of_sets(TestExpectations.EXPECTATIONS)
        self._timeline_to_tests = self._dict_of_sets(TestExpectations.TIMELINES)
        self._result_type_to_tests = self._dict_of_sets(TestExpectations.RESULT_TYPES)

    def _dict_of_sets(self, strings_to_constants):
        """Takes a dict of strings->constants and returns a dict mapping
        each constant to an empty set."""
        d = {}
        for c in strings_to_constants.values():
            d[c] = set()
        return d

    def get_test_set(self, modifier, expectation=None, include_skips=True):
        if expectation is None:
            tests = self._modifier_to_tests[modifier]
        else:
            tests = (self._expectation_to_tests[expectation] &
                self._modifier_to_tests[modifier])

        if not include_skips:
            tests = tests - self.get_test_set(SKIP, expectation)

        return tests

    def get_tests_with_result_type(self, result_type):
        return self._result_type_to_tests[result_type]

    def get_tests_with_timeline(self, timeline):
        return self._timeline_to_tests[timeline]

    def get_options(self, test):
        """This returns the entire set of options for the given test
        (the modifiers plus the BUGXXXX identifier). This is used by the
        LTTF dashboard."""
        return self._test_to_options[test]

    def has_modifier(self, test, modifier):
        return test in self._modifier_to_tests[modifier]

    def has_test(self, test):
        return test in self._test_list_paths

    def get_expectations(self, test):
        return self._test_to_expectations[test]

    def add_tests(self, lineno, expectation, overrides_allowed):
        """Returns a list of errors, encountered while matching modifiers."""

        if expectation.is_invalid():
            return

        for test in expectation.matching_tests:
            if self._already_seen_better_match(test, expectation, lineno, overrides_allowed):
                continue

            self._clear_expectations_for_test(test, expectation.name)
            self._test_list_paths[test] = (self._port.normalize_test_name(expectation.name), expectation.num_matches, lineno)
            self.add_test(test, expectation, overrides_allowed)

    def add_test(self, test, expectation_line, overrides_allowed):
        """Sets the expected state for a given test.

        This routine assumes the test has not been added before. If it has,
        use _clear_expectations_for_test() to reset the state prior to
        calling this.

        Args:
          test: test to add
          expectation_line: expectation to add
          overrides_allowed: whether we're parsing the regular expectations
              or the overridding expectations"""
        self._test_to_expectations[test] = expectation_line.parsed_expectations
        for expectation in expectation_line.parsed_expectations:
            self._expectation_to_tests[expectation].add(test)

        self._test_to_options[test] = expectation_line.modifiers
        self._test_to_modifiers[test] = set()
        for modifier in expectation_line.parsed_modifiers:
            mod_value = TestExpectations.MODIFIERS[modifier]
            self._modifier_to_tests[mod_value].add(test)
            self._test_to_modifiers[test].add(mod_value)

        if 'wontfix' in expectation_line.parsed_modifiers:
            self._timeline_to_tests[WONTFIX].add(test)
        else:
            self._timeline_to_tests[NOW].add(test)

        if 'skip' in expectation_line.parsed_modifiers:
            self._result_type_to_tests[SKIP].add(test)
        elif expectation_line.parsed_expectations == set([PASS]):
            self._result_type_to_tests[PASS].add(test)
        elif len(expectation_line.parsed_expectations) > 1:
            self._result_type_to_tests[FLAKY].add(test)
        else:
            self._result_type_to_tests[FAIL].add(test)

        if overrides_allowed:
            self._overridding_tests.add(test)

    def _clear_expectations_for_test(self, test, test_list_path):
        """Remove prexisting expectations for this test.
        This happens if we are seeing a more precise path
        than a previous listing.
        """
        if self.has_test(test):
            self._test_to_expectations.pop(test, '')
            self._remove_from_sets(test, self._expectation_to_tests)
            self._remove_from_sets(test, self._modifier_to_tests)
            self._remove_from_sets(test, self._timeline_to_tests)
            self._remove_from_sets(test, self._result_type_to_tests)

        self._test_list_paths[test] = self._port.normalize_test_name(test_list_path)

    def _remove_from_sets(self, test, dict):
        """Removes the given test from the sets in the dictionary.

        Args:
          test: test to look for
          dict: dict of sets of files"""
        for set_of_tests in dict.itervalues():
            if test in set_of_tests:
                set_of_tests.remove(test)

    def _already_seen_better_match(self, test, expectation, lineno, overrides_allowed):
        """Returns whether we've seen a better match already in the file.

        Returns True if we've already seen a expectation.name that matches more of the test
            than this path does
        """
        # FIXME: See comment below about matching test configs and num_matches.
        if not self.has_test(test):
            # We've never seen this test before.
            return False

        prev_base_path, prev_num_matches, prev_lineno = self._test_list_paths[test]
        base_path = self._port.normalize_test_name(expectation.name)

        if len(prev_base_path) > len(base_path):
            # The previous path matched more of the test.
            return True

        if len(prev_base_path) < len(base_path):
            # This path matches more of the test.
            return False

        if overrides_allowed and test not in self._overridding_tests:
            # We have seen this path, but that's okay because it is
            # in the overrides and the earlier path was in the
            # expectations (not the overrides).
            return False

        # At this point we know we have seen a previous exact match on this
        # base path, so we need to check the two sets of modifiers.

        if overrides_allowed:
            expectation_source = "override"
        else:
            expectation_source = "expectation"

        # FIXME: This code was originally designed to allow lines that matched
        # more modifiers to override lines that matched fewer modifiers.
        # However, we currently view these as errors. If we decide to make
        # this policy permanent, we can probably simplify this code
        # and the ModifierMatcher code a fair amount.
        #
        # To use the "more modifiers wins" policy, change the errors for overrides
        # to be warnings and return False".

        if prev_num_matches == expectation.num_matches:
            expectation.errors.append('Duplicate or ambiguous %s.' % expectation_source)
            return True

        if prev_num_matches < expectation.num_matches:
            expectation.errors.append('More specific entry on line %d overrides line %d' % (lineno, prev_lineno))
            # FIXME: return False if we want more specific to win.
            return True

        expectation.errors.append('More specific entry on line %d overrides line %d' % (prev_lineno, lineno))
        return True


class TestExpectations:
    """Test expectations consist of lines with specifications of what
    to expect from layout test cases. The test cases can be directories
    in which case the expectations apply to all test cases in that
    directory and any subdirectory. The format is along the lines of:

      LayoutTests/fast/js/fixme.js = FAIL
      LayoutTests/fast/js/flaky.js = FAIL PASS
      LayoutTests/fast/js/crash.js = CRASH TIMEOUT FAIL PASS
      ...

    To add other options:
      SKIP : LayoutTests/fast/js/no-good.js = TIMEOUT PASS
      DEBUG : LayoutTests/fast/js/no-good.js = TIMEOUT PASS
      DEBUG SKIP : LayoutTests/fast/js/no-good.js = TIMEOUT PASS
      LINUX DEBUG SKIP : LayoutTests/fast/js/no-good.js = TIMEOUT PASS
      LINUX WIN : LayoutTests/fast/js/no-good.js = TIMEOUT PASS

    SKIP: Doesn't run the test.
    SLOW: The test takes a long time to run, but does not timeout indefinitely.
    WONTFIX: For tests that we never intend to pass on a given platform.

    Notes:
      -A test cannot be both SLOW and TIMEOUT
      -A test should only be one of IMAGE, TEXT, IMAGE+TEXT, AUDIO, or FAIL.
       FAIL is a legacy value that currently means either IMAGE,
       TEXT, or IMAGE+TEXT. Once we have finished migrating the expectations,
       we should change FAIL to have the meaning of IMAGE+TEXT and remove the
       IMAGE+TEXT identifier.
      -A test can be included twice, but not via the same path.
      -If a test is included twice, then the more precise path wins.
      -CRASH tests cannot be WONTFIX
    """

    TEST_LIST = "test_expectations.txt"

    EXPECTATIONS = {'pass': PASS,
                    'fail': FAIL,
                    'text': TEXT,
                    'image': IMAGE,
                    'image+text': IMAGE_PLUS_TEXT,
                    'audio': AUDIO,
                    'timeout': TIMEOUT,
                    'crash': CRASH,
                    'missing': MISSING}

    EXPECTATION_DESCRIPTIONS = {SKIP: ('skipped', 'skipped'),
                                PASS: ('pass', 'passes'),
                                FAIL: ('failure', 'failures'),
                                TEXT: ('text diff mismatch',
                                       'text diff mismatch'),
                                IMAGE: ('image mismatch', 'image mismatch'),
                                IMAGE_PLUS_TEXT: ('image and text mismatch',
                                                  'image and text mismatch'),
                                AUDIO: ('audio mismatch', 'audio mismatch'),
                                CRASH: ('DumpRenderTree crash',
                                        'DumpRenderTree crashes'),
                                TIMEOUT: ('test timed out', 'tests timed out'),
                                MISSING: ('no expected result found',
                                          'no expected results found')}

    EXPECTATION_ORDER = (PASS, CRASH, TIMEOUT, MISSING, IMAGE_PLUS_TEXT, TEXT, IMAGE, AUDIO, FAIL, SKIP)

    BUILD_TYPES = ('debug', 'release')

    MODIFIERS = {'skip': SKIP,
                 'wontfix': WONTFIX,
                 'slow': SLOW,
                 'rebaseline': REBASELINE,
                 'none': NONE}

    TIMELINES = {'wontfix': WONTFIX,
                 'now': NOW}

    RESULT_TYPES = {'skip': SKIP,
                    'pass': PASS,
                    'fail': FAIL,
                    'flaky': FLAKY}

    @classmethod
    def expectation_from_string(cls, string):
        assert(' ' not in string)  # This only handles one expectation at a time.
        return cls.EXPECTATIONS.get(string.lower())

    def __init__(self, port, tests, expectations,
                 test_config, is_lint_mode, overrides=None):
        """Loads and parses the test expectations given in the string.
        Args:
            port: handle to object containing platform-specific functionality
            tests: list of all of the test files
            expectations: test expectations as a string
            test_config: specific values to check against when
                parsing the file (usually port.test_config(),
                but may be different when linting or doing other things).
            is_lint_mode: If True, just parse the expectations string
                looking for errors.
            overrides: test expectations that are allowed to override any
                entries in |expectations|. This is used by callers
                that need to manage two sets of expectations (e.g., upstream
                and downstream expectations).
        """
        self._full_test_list = tests
        self._test_config = test_config
        self._is_lint_mode = is_lint_mode
        self._model = TestExpectationsModel(port)
        self._parser = TestExpectationParser(port, test_config, tests, is_lint_mode)

        # Maps relative test paths as listed in the expectations file to a
        # list of maps containing modifiers and expectations for each time
        # the test is listed in the expectations file. We use this to
        # keep a representation of the entire list of expectations, even
        # invalid ones.
        self._all_expectations = {}

        self._expectations = TestExpectationParser.tokenize_list(expectations)
        self._add_expectations(self._expectations, overrides_allowed=False)

        if overrides:
            overrides_expectations = TestExpectationParser.tokenize_list(overrides)
            self._add_expectations(overrides_expectations, overrides_allowed=True)
            self._expectations += overrides_expectations

        self._has_warnings = False
        self._report_errors()
        self._process_tests_without_expectations()

    # TODO(ojan): Allow for removing skipped tests when getting the list of
    # tests to run, but not when getting metrics.

    def get_rebaselining_failures(self):
        return (self._model.get_test_set(REBASELINE, FAIL) |
                self._model.get_test_set(REBASELINE, IMAGE) |
                self._model.get_test_set(REBASELINE, TEXT) |
                self._model.get_test_set(REBASELINE, IMAGE_PLUS_TEXT) |
                self._model.get_test_set(REBASELINE, AUDIO))

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def get_expectations(self, test):
        return self._model.get_expectations(test)

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def has_modifier(self, test, modifier):
        return self._model.has_modifier(test, modifier)

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def get_tests_with_result_type(self, result_type):
        return self._model.get_tests_with_result_type(result_type)

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def get_test_set(self, modifier, expectation=None, include_skips=True):
        return self._model.get_test_set(modifier, expectation, include_skips)

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def get_options(self, test):
        return self._model.get_options(test)

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def get_tests_with_timeline(self, timeline):
        return self._model.get_tests_with_timeline(timeline)

    def get_expectations_string(self, test):
        """Returns the expectatons for the given test as an uppercase string.
        If there are no expectations for the test, then "PASS" is returned."""
        expectations = self._model.get_expectations(test)
        retval = []

        for expectation in expectations:
            retval.append(self.expectation_to_string(expectation))

        return " ".join(retval)

    def expectation_to_string(self, expectation):
        """Return the uppercased string equivalent of a given expectation."""
        for item in self.EXPECTATIONS.items():
            if item[1] == expectation:
                return item[0].upper()
        raise ValueError(expectation)

    def matches_an_expected_result(self, test, result, pixel_tests_are_enabled):
        expected_results = self._model.get_expectations(test)
        if not pixel_tests_are_enabled:
            expected_results = remove_pixel_failures(expected_results)
        return result_was_expected(result,
                                   expected_results,
                                   self.is_rebaselining(test),
                                   self._model.has_modifier(test, SKIP))

    def is_rebaselining(self, test):
        return self._model.has_modifier(test, REBASELINE)

    def _report_errors(self):
        errors = []
        warnings = []
        for lineno, expectation in enumerate(self._expectations, start=1):
            for error in expectation.errors:
                errors.append("Line:%s %s %s" % (lineno, error, expectation.name if expectation.expectations else expectation.comment))
            for warning in expectation.warnings:
                warnings.append("Line:%s %s %s" % (lineno, warning, expectation.name if expectation.expectations else expectation.comment))

        if len(errors) or len(warnings):
            _log.error("FAILURES FOR %s" % str(self._test_config))

            for error in errors:
                _log.error(error)
            for warning in warnings:
                _log.error(warning)

            if len(errors):
                raise ParseError(fatal=True, errors=errors)
            if len(warnings):
                self._has_warnings = True
                if self._is_lint_mode:
                    raise ParseError(fatal=False, errors=warnings)

    def _process_tests_without_expectations(self):
        expectation = TestExpectationLine()
        expectation.parsed_expectations = set([PASS])
        if self._full_test_list:
            for test in self._full_test_list:
                if not self._model.has_test(test):
                    self._model.add_test(test, expectation, overrides_allowed=False)

    def get_expectations_json_for_all_platforms(self):
        # Specify separators in order to get compact encoding.
        return ExpectationsJsonEncoder(separators=(',', ':')).encode(
            self._all_expectations)

    def has_warnings(self):
        return self._has_warnings

    def remove_rebaselined_tests(self, tests):
        """Returns a copy of the expectations with the tests removed."""
        def without_rebaseline_modifier(expectation):
            return not (not expectation.is_malformed() and expectation.name in tests and "rebaseline" in expectation.modifiers)

        return TestExpectationSerializer.list_to_string(filter(without_rebaseline_modifier, self._expectations))

    def _add_to_all_expectations(self, test, options, expectations):
        if not test in self._all_expectations:
            self._all_expectations[test] = []
        self._all_expectations[test].append(
            ModifiersAndExpectations(options, expectations))

    def _add_expectations(self, expectation_list, overrides_allowed):
        for lineno, expectation in enumerate(expectation_list, start=1):
            expectations = expectation.expectations

            if not expectation.expectations:
                continue

            self._add_to_all_expectations(expectation.name,
                                            " ".join(expectation.modifiers).upper(),
                                            " ".join(expectation.expectations).upper())

            self._parser.parse(expectation)
            self._model.add_tests(lineno, expectation, overrides_allowed)


class ModifierMatchResult(object):
    def __init__(self, options):
        self.num_matches = ModifierMatcher.NO_MATCH
        self.options = options
        self.modifiers = []
        self._matched_regexes = set()
        self._matched_macros = set()


class ModifierMatcher(object):

    """
    This class manages the interpretation of the "modifiers" for a given
    line in the expectations file. Modifiers are the tokens that appear to the
    left of the colon on a line. For example, "BUG1234", "DEBUG", and "WIN" are
    all modifiers. This class gets what the valid modifiers are, and which
    modifiers are allowed to exist together on a line, from the
    TestConfiguration object that is passed in to the call.

    This class detects *intra*-line errors like unknown modifiers, but
    does not detect *inter*-line modifiers like duplicate expectations.

    More importantly, this class is also used to determine if a given line
    matches the port in question. Matches are ranked according to the number
    of modifiers that match on a line. A line with no modifiers matches
    everything and has a score of zero. A line with one modifier matches only
    ports that have that modifier and gets a score of 1, and so one. Ports
    that don't match at all get a score of -1.

    Given two lines in a file that apply to the same test, if both expectations
    match the current config, then the expectation is considered ambiguous,
    even if one expectation matches more of the config than the other. For
    example, in:

    BUG1 RELEASE : foo.html = FAIL
    BUG1 WIN RELEASE : foo.html = PASS
    BUG2 WIN : bar.html = FAIL
    BUG2 DEBUG : bar.html = PASS

    lines 1 and 2 would produce an error on a Win XP Release bot (the scores
    would be 1 and 2, respectively), and lines three and four would produce
    a duplicate expectation on a Win Debug bot since both the 'win' and the
    'debug' expectations would apply (both had scores of 1).

    In addition to the definitions of all of the modifiers, the class
    supports "macros" that are expanded prior to interpretation, and "ignore
    regexes" that can be used to skip over modifiers like the BUG* modifiers.
    """
    MACROS = {
        'mac': ['leopard', 'snowleopard'],
        'win': ['xp', 'vista', 'win7'],
        'linux': ['lucid'],
    }

    # We don't include the "none" modifier because it isn't actually legal.
    REGEXES_TO_IGNORE = (['bug\w+'] +
                         TestExpectations.MODIFIERS.keys()[:-1])
    DUPLICATE_REGEXES_ALLOWED = ['bug\w+']

    # Magic value returned when the options don't match.
    NO_MATCH = -1

    # FIXME: The code currently doesn't detect combinations of modifiers
    # that are syntactically valid but semantically invalid, like
    # 'MAC XP'. See ModifierMatchTest.test_invalid_combinations() in the
    # _unittest.py file.

    def __init__(self, test_config):
        """Initialize a ModifierMatcher argument with the TestConfiguration it
        should be matched against."""
        self.test_config = test_config
        self.allowed_configurations = test_config.all_test_configurations()
        self.macros = self.MACROS

        self.regexes_to_ignore = {}
        for regex_str in self.REGEXES_TO_IGNORE:
            self.regexes_to_ignore[regex_str] = re.compile(regex_str)

        # Keep a set of all of the legal modifiers for quick checking.
        self._all_modifiers = set()

        # Keep a dict mapping values back to their categories.
        self._categories_for_modifiers = {}
        for config in self.allowed_configurations:
            for category, modifier in config.items():
                self._categories_for_modifiers[modifier] = category
                self._all_modifiers.add(modifier)

    def match(self, expectation):
        """Checks a expectation.modifiers against the config set in the constructor.
        Options may be either actual modifier strings, "macro" strings
        that get expanded to a list of modifiers, or strings that are allowed
        to be ignored. All of the options must be passed in in lower case.

        Returns the number of matching categories, or NO_MATCH (-1) if it
        doesn't match or there were errors found. Matches are prioritized
        by the number of matching categories, because the more specific
        the options list, the more categories will match.

        The results of the most recent match are available in the 'options',
        'modifiers', 'num_matches', 'errors', and 'warnings' properties.
        """
        old_error_count = len(expectation.errors)
        result = ModifierMatchResult(expectation.modifiers)
        self._parse(expectation, result)
        if old_error_count != len(expectation.errors):
            return result
        self._count_matches(result)
        return result

    def _parse(self, expectation, result):
        # FIXME: Should we warn about lines having every value in a category?
        for option in result.options:
            self._parse_one(expectation, option, result)

    def _parse_one(self, expectation, option, result):
        if option in self._all_modifiers:
            self._add_modifier(expectation, option, result)
        elif option in self.macros:
            self._expand_macro(expectation, option, result)
        elif not self._matches_any_regex(expectation, option, result):
            expectation.errors.append("Unrecognized option '%s'" % option)

    def _add_modifier(self, expectation, option, result):
        if option in result.modifiers:
            expectation.errors.append("More than one '%s'" % option)
        else:
            result.modifiers.append(option)

    def _expand_macro(self, expectation, macro, result):
        if macro in result._matched_macros:
            expectation.errors.append("More than one '%s'" % macro)
            return

        mods = []
        for modifier in self.macros[macro]:
            if modifier in result.options:
                expectation.errors.append("Can't specify both modifier '%s' and macro '%s'" % (modifier, macro))
            else:
                mods.append(modifier)
        result._matched_macros.add(macro)
        result.modifiers.extend(mods)

    def _matches_any_regex(self, expectation, option, result):
        for regex_str, pattern in self.regexes_to_ignore.iteritems():
            if pattern.match(option):
                self._handle_regex_match(expectation, regex_str, result)
                return True
        return False

    def _handle_regex_match(self, expectation, regex_str, result):
        if (regex_str in result._matched_regexes and
            regex_str not in self.DUPLICATE_REGEXES_ALLOWED):
            expectation.errors.append("More than one option matching '%s'" %
                                 regex_str)
        else:
            result._matched_regexes.add(regex_str)

    def _count_matches(self, result):
        """Returns the number of modifiers that match the test config."""
        categorized_modifiers = self._group_by_category(result.modifiers)
        result.num_matches = 0
        for category, modifier in self.test_config.items():
            if category in categorized_modifiers:
                if modifier in categorized_modifiers[category]:
                    result.num_matches += 1
                else:
                    result.num_matches = self.NO_MATCH
                    return

    def _group_by_category(self, modifiers):
        # Returns a dict of category name -> list of modifiers.
        modifiers_by_category = {}
        for m in modifiers:
            modifiers_by_category.setdefault(self._category(m), []).append(m)
        return modifiers_by_category

    def _category(self, modifier):
        return self._categories_for_modifiers[modifier]
