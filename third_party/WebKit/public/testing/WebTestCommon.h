/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebTestCommon_h
#define WebTestCommon_h

// -----------------------------------------------------------------------------
// Default configuration

#if !defined(WEBTESTRUNNER_IMPLEMENTATION)
#define WEBTESTRUNNER_IMPLEMENTATION 0
#endif

// -----------------------------------------------------------------------------
// Exported symbols need to be annotated with WEBTESTRUNNER_EXPORT

#if defined(WEBTESTRUNNER_DLL)

#if defined(WIN32)
#if WEBTESTRUNNER_IMPLEMENTATION
#define WEBTESTRUNNER_EXPORT __declspec(dllexport)
#else
#define WEBTESTRUNNER_EXPORT __declspec(dllimport)
#endif

#else // defined(WIN32)

#if WEBTESTRUNNER_IMPLEMENTATION
#define WEBTESTRUNNER_EXPORT __attribute__((visibility("default")))
#else
#define WEBTESTRUNNER_EXPORT
#endif

#endif

#else // defined(WEBTESTRUNNER_DLL)

#define WEBTESTRUNNER_EXPORT

#endif

#endif // WebTestCommon_h
