/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __WebKitAvailability__
#define __WebKitAvailability__

/* The structure of this header is based on AvailabilityMacros.h.  The major difference is that the availability
   macros are defined in terms of WebKit version numbers rather than Mac OS X system version numbers, as WebKit
   releases span multiple versions of Mac OS X.
*/

#define WEBKIT_VERSION_1_0    0x0100
#define WEBKIT_VERSION_1_1    0x0110
#define WEBKIT_VERSION_1_2    0x0120
#define WEBKIT_VERSION_1_3    0x0130
#define WEBKIT_VERSION_2_0    0x0200
#define WEBKIT_VERSION_3_0    0x0300
#define WEBKIT_VERSION_3_1    0x0310
#define WEBKIT_VERSION_LATEST 0x9999

#ifdef __APPLE__
#import <AvailabilityMacros.h>
#else
// For non-Mac platforms, require the newest version.
#define WEBKIT_VERSION_MIN_REQUIRED WEBKIT_VERSION_LATEST
/*
 * only certain compilers support __attribute__((deprecated))
 */
#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
    #define DEPRECATED_ATTRIBUTE __attribute__((deprecated))
#else
    #define DEPRECATED_ATTRIBUTE
#endif
#endif

/* The versions of GCC that shipped with Xcode prior to 3.0 (GCC build number < 5400) did not support attributes on methods
   declared in a category.  If we are building with one of these versions, we need to omit the attribute.  We achieve this
   by wrapping the annotation in WEBKIT_CATEGORY_METHOD_ANNOTATION, which will remove the annotation when an old version
   of GCC is in use and will otherwise expand to the annotation.
*/
#if defined(__APPLE_CC__) && __APPLE_CC__ < 5400
    #define WEBKIT_CATEGORY_METHOD_ANNOTATION(ANNOTATION)
#else
    #define WEBKIT_CATEGORY_METHOD_ANNOTATION(ANNOTATION) ANNOTATION
#endif


/* If minimum WebKit version is not specified, assume the version that shipped with the target Mac OS X version */
#ifndef WEBKIT_VERSION_MIN_REQUIRED
    #if !defined(MAC_OS_X_VERSION_10_2) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_2
        #error WebKit was not available prior to Mac OS X 10.2
    #elif !defined(MAC_OS_X_VERSION_10_3) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3
        /* WebKit 1.0 is the only version available on Mac OS X 10.2. */
        #define WEBKIT_VERSION_MIN_REQUIRED WEBKIT_VERSION_1_0
    #elif !defined(MAC_OS_X_VERSION_10_4) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
        /* WebKit 1.1 is the version that shipped on Mac OS X 10.3. */
        #define WEBKIT_VERSION_MIN_REQUIRED WEBKIT_VERSION_1_1
    #elif !defined(MAC_OS_X_VERSION_10_5) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
        /* WebKit 2.0 is the version that shipped on Mac OS X 10.4. */
        #define WEBKIT_VERSION_MIN_REQUIRED WEBKIT_VERSION_2_0
    #elif !defined(MAC_OS_X_VERSION_10_6) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_6
        /* WebKit 3.0 is the version that shipped on Mac OS X 10.5. */
        #define WEBKIT_VERSION_MIN_REQUIRED WEBKIT_VERSION_3_0
    #else
        #define WEBKIT_VERSION_MIN_REQUIRED WEBKIT_VERSION_LATEST
    #endif
#endif


/* If maximum WebKit version is not specified, assume largerof(latest, minimum) */
#ifndef WEBKIT_VERSION_MAX_ALLOWED
    #if WEBKIT_VERSION_MIN_REQUIRED > WEBKIT_VERSION_LATEST
        #define WEBKIT_VERSION_MAX_ALLOWED WEBKIT_VERSION_MIN_REQUIRED
    #else
        #define WEBKIT_VERSION_MAX_ALLOWED WEBKIT_VERSION_LATEST
    #endif
#endif


/* Sanity check the configured values */
#if WEBKIT_VERSION_MAX_ALLOWED < WEBKIT_VERSION_MIN_REQUIRED
    #error WEBKIT_VERSION_MAX_ALLOWED must be >= WEBKIT_VERSION_MIN_REQUIRED
#endif
#if WEBKIT_VERSION_MIN_REQUIRED < WEBKIT_VERSION_1_0
    #error WEBKIT_VERSION_MIN_REQUIRED must be >= WEBKIT_VERSION_1_0
#endif






/*
 * AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER
 * 
 * Used on functions introduced in WebKit 1.0
 */
#define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER

/*
 * AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED
 * 
 * Used on functions introduced in WebKit 1.0,
 * and deprecated in WebKit 1.0
 */
#define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE

/*
 * DEPRECATED_IN_WEBKIT_VERSION_1_0_AND_LATER
 * 
 * Used on types deprecated in WebKit 1.0 
 */
#define DEPRECATED_IN_WEBKIT_VERSION_1_0_AND_LATER     DEPRECATED_ATTRIBUTE






/*
 * AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER
 * 
 * Used on declarations introduced in WebKit 1.1
 */
#if WEBKIT_VERSION_MAX_ALLOWED < WEBKIT_VERSION_1_1
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif WEBKIT_VERSION_MIN_REQUIRED < WEBKIT_VERSION_1_1
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED
 * 
 * Used on declarations introduced in WebKit 1.1, 
 * and deprecated in WebKit 1.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_1
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED    AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_1
 * 
 * Used on declarations introduced in WebKit 1.0, 
 * but later deprecated in WebKit 1.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_1
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_1    AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER
#endif

/*
 * DEPRECATED_IN_WEBKIT_VERSION_1_1_AND_LATER
 * 
 * Used on types deprecated in WebKit 1.1 
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_1
    #define DEPRECATED_IN_WEBKIT_VERSION_1_1_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_WEBKIT_VERSION_1_1_AND_LATER
#endif






/*
 * AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER
 * 
 * Used on declarations introduced in WebKit 1.2 
 */
#if WEBKIT_VERSION_MAX_ALLOWED < WEBKIT_VERSION_1_2
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif WEBKIT_VERSION_MIN_REQUIRED < WEBKIT_VERSION_1_2
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED
 * 
 * Used on declarations introduced in WebKit 1.2, 
 * and deprecated in WebKit 1.2
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_2
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED    AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_2
 * 
 * Used on declarations introduced in WebKit 1.0, 
 * but later deprecated in WebKit 1.2
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_2
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_2    AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_2
 * 
 * Used on declarations introduced in WebKit 1.1, 
 * but later deprecated in WebKit 1.2
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_2
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_2    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_2    AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER
#endif

/*
 * DEPRECATED_IN_WEBKIT_VERSION_1_2_AND_LATER
 * 
 * Used on types deprecated in WebKit 1.2
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_2
    #define DEPRECATED_IN_WEBKIT_VERSION_1_2_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_WEBKIT_VERSION_1_2_AND_LATER
#endif






/*
 * AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER
 * 
 * Used on declarations introduced in WebKit 1.3 
 */
#if WEBKIT_VERSION_MAX_ALLOWED < WEBKIT_VERSION_1_3
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif WEBKIT_VERSION_MIN_REQUIRED < WEBKIT_VERSION_1_3
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED
 * 
 * Used on declarations introduced in WebKit 1.3, 
 * and deprecated in WebKit 1.3
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_3
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED    AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_3
 * 
 * Used on declarations introduced in WebKit 1.0, 
 * but later deprecated in WebKit 1.3
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_3
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_3    AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_3
 * 
 * Used on declarations introduced in WebKit 1.1, 
 * but later deprecated in WebKit 1.3
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_3
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_3    AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_3
 * 
 * Used on declarations introduced in WebKit 1.2, 
 * but later deprecated in WebKit 1.3
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_3
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_3    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_1_3    AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER
#endif

/*
 * DEPRECATED_IN_WEBKIT_VERSION_1_3_AND_LATER
 * 
 * Used on types deprecated in WebKit 1.3 
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_1_3
    #define DEPRECATED_IN_WEBKIT_VERSION_1_3_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_WEBKIT_VERSION_1_3_AND_LATER
#endif






/*
 * AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER
 * 
 * Used on declarations introduced in WebKit 2.0 
 */
#if WEBKIT_VERSION_MAX_ALLOWED < WEBKIT_VERSION_2_0
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif WEBKIT_VERSION_MIN_REQUIRED < WEBKIT_VERSION_2_0
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED
 * 
 * Used on declarations introduced in WebKit 2.0, 
 * and deprecated in WebKit 2.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_2_0
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED    AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0
 * 
 * Used on declarations introduced in WebKit 1.0, 
 * but later deprecated in WebKit 2.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_2_0
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0    AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0
 * 
 * Used on declarations introduced in WebKit 1.1, 
 * but later deprecated in WebKit 2.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_2_0
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0    AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0
 * 
 * Used on declarations introduced in WebKit 1.2, 
 * but later deprecated in WebKit 2.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_2_0
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0    AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0
 * 
 * Used on declarations introduced in WebKit 1.3, 
 * but later deprecated in WebKit 2.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_2_0
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_2_0    AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER
#endif

/*
 * DEPRECATED_IN_WEBKIT_VERSION_2_0_AND_LATER
 * 
 * Used on types deprecated in WebKit 2.0 
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_2_0
    #define DEPRECATED_IN_WEBKIT_VERSION_2_0_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_WEBKIT_VERSION_2_0_AND_LATER
#endif






/*
 * AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER
 * 
 * Used on declarations introduced in WebKit 3.0 
 */
#if WEBKIT_VERSION_MAX_ALLOWED < WEBKIT_VERSION_3_0
    #define AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif WEBKIT_VERSION_MIN_REQUIRED < WEBKIT_VERSION_3_0
    #define AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED
 * 
 * Used on declarations introduced in WebKit 3.0, 
 * and deprecated in WebKit 3.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_0
    #define AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED    AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0
 * 
 * Used on declarations introduced in WebKit 1.0, 
 * but later deprecated in WebKit 3.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_0
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0    AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0
 * 
 * Used on declarations introduced in WebKit 1.1, 
 * but later deprecated in WebKit 3.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_0
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0    AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0
 * 
 * Used on declarations introduced in WebKit 1.2, 
 * but later deprecated in WebKit 3.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_0
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0    AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0
 * 
 * Used on declarations introduced in WebKit 1.3, 
 * but later deprecated in WebKit 3.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_0
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0    AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0
 * 
 * Used on declarations introduced in WebKit 2.0, 
 * but later deprecated in WebKit 3.0
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_0
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_0    AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER
#endif

/*
 * DEPRECATED_IN_WEBKIT_VERSION_3_0_AND_LATER
 * 
 * Used on types deprecated in WebKit 3.0 
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_0
    #define DEPRECATED_IN_WEBKIT_VERSION_3_0_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_WEBKIT_VERSION_3_0_AND_LATER
#endif






/*
 * AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER
 * 
 * Used on declarations introduced in WebKit 3.1
 */
#if WEBKIT_VERSION_MAX_ALLOWED < WEBKIT_VERSION_3_1
    #define AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER     UNAVAILABLE_ATTRIBUTE
#elif WEBKIT_VERSION_MIN_REQUIRED < WEBKIT_VERSION_3_1
    #define AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER_BUT_DEPRECATED
 * 
 * Used on declarations introduced in WebKit 3.1, 
 * and deprecated in WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_1
    #define AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER_BUT_DEPRECATED    AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 1.0, 
 * but later deprecated in WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_1
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 1.1, 
 * but later deprecated in WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_1
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 1.2, 
 * but later deprecated in WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_1
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 1.3, 
 * but later deprecated in WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_1
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 2.0, 
 * but later deprecated in WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_1
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 3.0, 
 * but later deprecated in WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_1
    #define AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED_IN_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER
#endif

/*
 * DEPRECATED_IN_WEBKIT_VERSION_3_1_AND_LATER
 * 
 * Used on types deprecated in WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_3_1
    #define DEPRECATED_IN_WEBKIT_VERSION_3_1_AND_LATER    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_IN_WEBKIT_VERSION_3_1_AND_LATER
#endif






/*
 * AVAILABLE_AFTER_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced after WebKit 3.1
 */
#if WEBKIT_VERSION_MAX_ALLOWED < WEBKIT_VERSION_LATEST
    #define AVAILABLE_AFTER_WEBKIT_VERSION_3_1     UNAVAILABLE_ATTRIBUTE
#elif WEBKIT_VERSION_MIN_REQUIRED < WEBKIT_VERSION_LATEST
    #define AVAILABLE_AFTER_WEBKIT_VERSION_3_1     WEAK_IMPORT_ATTRIBUTE
#else
    #define AVAILABLE_AFTER_WEBKIT_VERSION_3_1
#endif

/*
 * AVAILABLE_AFTER_WEBKIT_VERSION_3_1_BUT_DEPRECATED
 * 
 * Used on declarations introduced after WebKit 3.1, 
 * and deprecated after WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_LATEST
    #define AVAILABLE_AFTER_WEBKIT_VERSION_3_1_BUT_DEPRECATED    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_AFTER_WEBKIT_VERSION_3_1_BUT_DEPRECATED    AVAILABLE_AFTER_WEBKIT_VERSION_3_1
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 1.0, 
 * but later deprecated after WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_LATEST
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_1_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 1.1, 
 * but later deprecated after WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_LATEST
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_1_1_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 1.2, 
 * but later deprecated after WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_LATEST
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_1_2_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 1.3, 
 * but later deprecated after WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_LATEST
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 2.0, 
 * but later deprecated after WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_LATEST
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_2_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 3.0, 
 * but later deprecated after WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_LATEST
    #define AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER
#endif

/*
 * AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1
 * 
 * Used on declarations introduced in WebKit 3.1, 
 * but later deprecated after WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_LATEST
    #define AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_3_1    AVAILABLE_WEBKIT_VERSION_3_1_AND_LATER
#endif

/*
 * DEPRECATED_AFTER_WEBKIT_VERSION_3_1
 * 
 * Used on types deprecated after WebKit 3.1
 */
#if WEBKIT_VERSION_MIN_REQUIRED >= WEBKIT_VERSION_LATEST
    #define DEPRECATED_AFTER_WEBKIT_VERSION_3_1    DEPRECATED_ATTRIBUTE
#else
    #define DEPRECATED_AFTER_WEBKIT_VERSION_3_1
#endif


#endif /* __WebKitAvailability__ */
