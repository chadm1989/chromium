// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_INTERNAL_CHROME_BROWSER_WEB_WEB_JS_TEST_H_
#define IOS_INTERNAL_CHROME_BROWSER_WEB_WEB_JS_TEST_H_

#import <Foundation/Foundation.h>

#import "base/mac/bundle_locations.h"
#import "base/mac/scoped_nsobject.h"
#include "testing/gtest_mac.h"

class GURL;

namespace web {

// Base fixture mixin for testing JavaScripts.
template <class WebTestT>
class WebJsTest : public WebTestT {
 public:
  WebJsTest(NSArray* java_script_paths)
      : java_script_paths_([java_script_paths copy]) {}

 protected:
  // Loads |html| and inject JavaScripts at |javaScriptPaths_|.
  void LoadHtmlAndInject(NSString* html) {
    WebTestT::LoadHtml(html);
    Inject();
  }

  // Returns a NSString representation of the JavaScript's evaluation results;
  // the JavaScript is passed in as a |format| and its arguments.
  NSString* EvaluateJavaScriptWithFormat(NSString* format, ...)
      __attribute__((format(__NSString__, 2, 3)));

  // Helper method that EXPECTs the |java_script| evaluation results on each
  // element obtained by scripts in |get_element_javas_cripts|; the expected
  // result is the corresponding entry in |expected_results|.
  void EvaluateJavaScriptOnElementsAndCheck(NSString* java_script,
                                            NSArray* get_element_java_scripts,
                                            NSArray* expected_results);

  // Helper method that EXPECTs the |java_script| evaluation results on each
  // element obtained by JavaScripts in |get_element_java_scripts|. The
  // expected results are boolean and are true only for elements in
  // |get_element_java_scripts_expecting_true| which is subset of
  // |get_element_java_scripts|.
  void EvaluateBooleanJavaScriptOnElementsAndCheck(
      NSString* java_script,
      NSArray* get_element_java_scripts,
      NSArray* get_element_java_scripts_expecting_true);

 private:
  // Injects JavaScript at |java_script_paths_|.
  void Inject();

  base::scoped_nsobject<NSArray> java_script_paths_;
};

template <class WebTestT>
void WebJsTest<WebTestT>::Inject() {
  // Main web injection should have occured.
  ASSERT_NSEQ(@"object",
              WebTestT::EvaluateJavaScriptAsString(@"typeof __gCrWeb"));

  for (NSString* java_script_path in java_script_paths_.get()) {
    NSString* path =
        [base::mac::FrameworkBundle() pathForResource:java_script_path
                                               ofType:@"js"];
    WebTestT::EvaluateJavaScriptAsString([NSString
        stringWithContentsOfFile:path
                        encoding:NSUTF8StringEncoding
                           error:nil]);
  }
}

template <class WebTestT>
NSString* WebJsTest<WebTestT>::EvaluateJavaScriptWithFormat(NSString* format,
                                                            ...) {
  va_list args;
  va_start(args, format);
  base::scoped_nsobject<NSString> java_script(
      [[NSString alloc] initWithFormat:format arguments:args]);
  va_end(args);

  return WebTestT::EvaluateJavaScriptAsString(java_script);
}

template <class WebTestT>
void WebJsTest<WebTestT>::EvaluateJavaScriptOnElementsAndCheck(
    NSString* java_script,
    NSArray* get_element_java_scripts,
    NSArray* expected_results) {
  for (NSUInteger i = 0; i < get_element_java_scripts.count; ++i) {
    EXPECT_NSEQ(
        expected_results[i],
        EvaluateJavaScriptWithFormat(java_script, get_element_java_scripts[i]));
  }
}

template <class WebTestT>
void WebJsTest<WebTestT>::EvaluateBooleanJavaScriptOnElementsAndCheck(
    NSString* java_script,
    NSArray* get_element_java_scripts,
    NSArray* get_element_java_scripts_expecting_true) {
  for (NSUInteger index = 0; index < get_element_java_scripts.count; ++index) {
    NSString* get_element_java_script = get_element_java_scripts[index];
    NSString* expected = [get_element_java_scripts_expecting_true
                             containsObject:get_element_java_script]
                             ? @"true"
                             : @"false";
    EXPECT_NSEQ(expected, EvaluateJavaScriptWithFormat(java_script,
                                                       get_element_java_script))
        << [NSString stringWithFormat:@"%@ on %@ should return %@", java_script,
                                      get_element_java_script, expected];
  }
}

}  // namespace web

#endif  // IOS_INTERNAL_CHROME_BROWSER_WEB_WEB_JS_TEST_H_
