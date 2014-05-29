// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains global constant variables used by the application.

// Heart beat message header.
var HEART_BEAT_HEADER = 'HEARTBEAT';

// Default key used to encrypt many media files used in browser tests.
var KEY = new Uint8Array([0xeb, 0xdd, 0x62, 0xf1, 0x68, 0x14, 0xd2, 0x7b,
                          0x68, 0xef, 0x12, 0x2a, 0xfc, 0xe4, 0xae, 0x3c]);

var DEFAULT_LICENSE_SERVER = document.location.origin + '/license_server';

var DEFAULT_MEDIA_FILE = 'http://shadi.kir/alcatraz/Chrome_44-enc_av.webm';

// Key ID used for init data.
var KEY_ID = '0123456789012345';

// Available EME key systems to use.
var PREFIXED_CLEARKEY = 'webkit-org.w3.clearkey';
var CLEARKEY = 'org.w3.clearkey';
var EXTERNAL_CLEARKEY = 'org.chromium.externalclearkey';
var WIDEVINE_KEYSYSTEM = 'com.widevine.alpha';

// Key system name:value map to show on the document page.
var KEY_SYSTEMS = {
  'Widevine': WIDEVINE_KEYSYSTEM,
  'Clearkey': CLEARKEY,
  'External Clearkey': EXTERNAL_CLEARKEY
};

// General WebM and MP4 name:content_type map to show on the document page.
var MEDIA_TYPES = {
  'WebM - Audio Video': 'video/webm; codecs="vorbis, vp8"',
  'WebM - Video Only': 'video/webm; codecs="vp8"',
  'WebM - Audio Only': 'video/webm; codecs="vorbis"',
  'MP4 - Video Only': 'video/mp4; codecs="avc1.4D4041"',
  'MP4 - Audio Only': 'audio/mp4; codecs="mp4a.40.2"'
};

// Update the EME versions list by checking runtime support by the browser.
var EME_VERSIONS_OPTIONS = {};
var video = document.createElement('video');
if (video.webkitAddKey)
  EME_VERSIONS_OPTIONS['Prefixed EME (v 0.1b)'] = 'true';
if (video.setMediaKeys)
  EME_VERSIONS_OPTIONS['Unprefixed EME (Working draft)'] = 'false';

// Global document elements ID's.
var VIDEO_ELEMENT_ID = 'video';
var MEDIA_FILE_ELEMENT_ID = 'mediaFile';
var LICENSE_SERVER_ELEMENT_ID = 'licenseServer';
var KEYSYSTEM_ELEMENT_ID = 'keySystemList';
var MEDIA_TYPE_ELEMENT_ID = 'mediaTypeList';
var USE_SRC_ELEMENT_ID = 'useSRC';
var USE_PREFIXED_EME_ID = 'usePrefixedEME';

// These variables get updated every second, so better to have global pointers.
var decodedFPSElement = document.getElementById('decodedFPS');
var droppedFPSElement = document.getElementById('droppedFPS');
var droppedFramesElement = document.getElementById('droppedFrames');
var docLogs = document.getElementById('logs');
