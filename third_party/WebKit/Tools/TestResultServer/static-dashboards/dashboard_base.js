// Copyright (C) 2012 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//         * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//         * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//         * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Keys in the JSON files.
var FAILURES_BY_TYPE_KEY = 'num_failures_by_type';
var FAILURE_MAP_KEY = 'failure_map';
var CHROME_REVISIONS_KEY = 'chromeRevision';
var BLINK_REVISIONS_KEY = 'blinkRevision';
var TIMESTAMPS_KEY = 'secondsSinceEpoch';
var BUILD_NUMBERS_KEY = 'buildNumbers';
var TESTS_KEY = 'tests';

// Failure types.
var PASS = 'PASS';
var NO_DATA = 'NO DATA';
var SKIP = 'SKIP';
var NOTRUN = 'NOTRUN';

var ONE_DAY_SECONDS = 60 * 60 * 24;
var ONE_WEEK_SECONDS = ONE_DAY_SECONDS * 7;

// Enum for indexing into the run-length encoded results in the JSON files.
// 0 is where the count is length is stored. 1 is the value.
var RLE = {
    LENGTH: 0,
    VALUE: 1
}

var _NON_FAILURE_TYPES = [PASS, NO_DATA, SKIP, NOTRUN];

function isFailingResult(failureMap, failureType)
{
    return _NON_FAILURE_TYPES.indexOf(failureMap[failureType]) == -1;
}

// Generic utility functions.
function $(id)
{
    return document.getElementById(id);
}

// Returns the name of the current group, or a valid (but perhaps arbitrary) default.
function currentBuilderGroupName()
{
    return g_history.crossDashboardState.group ||
        groupNamesForTestType(g_history.crossDashboardState.testType)[0];
}

function currentBuilderGroup()
{
    return builders.getBuilderGroup(currentBuilderGroupName(), g_history.crossDashboardState.testType);
}

function currentBuilders()
{
    return currentBuilderGroup().builders;
}

var g_resultsByBuilder = {};

// Create a new function with some of its arguements
// pre-filled.
// Taken from goog.partial in the Closure library.
// @param {Function} fn A function to partially apply.
// @param {...*} var_args Additional arguments that are partially
//         applied to fn.
// @return {!Function} A partially-applied form of the function bind() was
//         invoked as a method of.
function partial(fn, var_args)
{
    var args = Array.prototype.slice.call(arguments, 1);
    return function() {
        // Prepend the bound arguments to the current arguments.
        var newArgs = Array.prototype.slice.call(arguments);
        newArgs.unshift.apply(newArgs, args);
        return fn.apply(this, newArgs);
    };
};

// FIXME: This should move into a results json parsing file/class.
function getTotalTestCounts(failuresByType)
{
    var countData = {
        totalTests: [],
        totalFailingTests: []
    };

    for (var failureType in failuresByType) {
        var failures = failuresByType[failureType];
        failures.forEach(function(count, index) {
            if (!countData.totalTests[index]) {
                countData.totalTests[index] = 0;
                countData.totalFailingTests[index] = 0;
            }

            countData.totalTests[index] += count;
            if (failureType != PASS)
                countData.totalFailingTests[index] += count;
        });
    }
    return countData;
}

// FIXME: This should move into a results json parsing file/class.
function determineFlakiness(failureMap, results, resultsForTest)
{
    // FIXME: Ideally this heuristic would be a bit smarter and not consider
    // all passes, followed by a few consecutive failures, followed by all passes
    // to be flakiness since that's more likely the test actually failing for a
    // few runs due to a commit.
    var FAILURE_TYPES_TO_IGNORE = ['NOTRUN', 'NO DATA', 'SKIP'];
    var flipCount = 0;
    var mostRecentNonIgnorableFailureType;

    for (var i = 0; i < results.length; i++) {
        var result = results[i][RLE.VALUE];
        var failureType = failureMap[result];
        if (failureType != mostRecentNonIgnorableFailureType && FAILURE_TYPES_TO_IGNORE.indexOf(failureType) == -1) {
            if (mostRecentNonIgnorableFailureType)
                flipCount++;
            mostRecentNonIgnorableFailureType = failureType;
        }
    }

    resultsForTest.flipCount = flipCount;
    resultsForTest.isFlaky = flipCount > 1;
}
