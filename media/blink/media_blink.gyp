# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    '../media_variables.gypi'
  ],
  'targets': [
    {
      # GN version: //media/blink
      'target_name': 'media_blink',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../cc/cc.gyp:cc',
        '../../cc/blink/cc_blink.gyp:cc_blink',
        '../../gpu/blink/gpu_blink.gyp:gpu_blink',
        '../../ui/gfx/gfx.gyp:gfx_geometry',
        '../../net/net.gyp:net',
        '../../skia/skia.gyp:skia',
        '../../third_party/WebKit/public/blink.gyp:blink',
        '../media.gyp:media',
        '../media.gyp:shared_memory_support',
        '../../url/url.gyp:url_lib',
      ],
      'defines': [
        'MEDIA_IMPLEMENTATION',
      ],
      # This sources list is duplicated in //media/blink/BUILD.gn
      'sources': [
        'active_loader.cc',
        'active_loader.h',
        'buffered_data_source.cc',
        'buffered_data_source.h',
        'buffered_data_source_host_impl.cc',
        'buffered_data_source_host_impl.h',
        'buffered_resource_loader.cc',
        'buffered_resource_loader.h',
        'cache_util.cc',
        'cache_util.h',
        'cdm_result_promise.h',
        'cdm_result_promise_helper.cc',
        'cdm_result_promise_helper.h',
        'cdm_session_adapter.cc',
        'cdm_session_adapter.h',
        'encrypted_media_player_support.cc',
        'encrypted_media_player_support.h',
        'key_system_config_selector.cc',
        'key_system_config_selector.h',
        'new_session_cdm_result_promise.cc',
        'new_session_cdm_result_promise.h',
        'texttrack_impl.cc',
        'texttrack_impl.h',
        'video_frame_compositor.cc',
        'video_frame_compositor.h',
        'webaudiosourceprovider_impl.cc',
        'webaudiosourceprovider_impl.h',
        'webcontentdecryptionmodule_impl.cc',
        'webcontentdecryptionmodule_impl.h',
        'webcontentdecryptionmoduleaccess_impl.cc',
        'webcontentdecryptionmoduleaccess_impl.h',
        'webcontentdecryptionmodulesession_impl.cc',
        'webcontentdecryptionmodulesession_impl.h',
        'webencryptedmediaclient_impl.cc',
        'webencryptedmediaclient_impl.h',
        'webinbandtexttrack_impl.cc',
        'webinbandtexttrack_impl.h',
        'webmediaplayer_delegate.h',
        'webmediaplayer_impl.cc',
        'webmediaplayer_impl.h',
        'webmediaplayer_params.cc',
        'webmediaplayer_params.h',
        'webmediaplayer_util.cc',
        'webmediaplayer_util.h',
        'webmediasource_impl.cc',
        'webmediasource_impl.h',
        'websourcebuffer_impl.cc',
        'websourcebuffer_impl.h',
      ],
      'conditions': [
        ['OS=="android" and media_use_ffmpeg==0', {
          'sources!': [
            'encrypted_media_player_support.cc',
            'encrypted_media_player_support.h',
            'webmediaplayer_impl.cc',
            'webmediaplayer_impl.h',
          ],
        }],
      ],
    },
    {
      'target_name': 'media_blink_unittests',
      'type': '<(gtest_target_type)',
      'dependencies': [
        'media_blink',
        '../media.gyp:media',
        '../media.gyp:media_test_support',
        '../../base/base.gyp:base',
        '../../base/base.gyp:test_support_base',
        '../../cc/cc.gyp:cc',
        '../../cc/blink/cc_blink.gyp:cc_blink',
        '../../gin/gin.gyp:gin',
        '../../net/net.gyp:net',
        '../../testing/gmock.gyp:gmock',
        '../../testing/gtest.gyp:gtest',
        '../../third_party/WebKit/public/blink.gyp:blink',
        '../../ui/gfx/gfx.gyp:gfx',
        '../../ui/gfx/gfx.gyp:gfx_geometry',
        '../../ui/gfx/gfx.gyp:gfx_test_support',
        '../../url/url.gyp:url_lib',
      ],
      'sources': [
        'buffered_data_source_host_impl_unittest.cc',
        'buffered_data_source_unittest.cc',
        'buffered_resource_loader_unittest.cc',
        'cache_util_unittest.cc',
        'key_system_config_selector_unittest.cc',
        'mock_webframeclient.h',
        'mock_weburlloader.cc',
        'mock_weburlloader.h',
        'run_all_unittests.cc',
        'test_response_generator.cc',
        'test_response_generator.h',
        'video_frame_compositor_unittest.cc',
        'webaudiosourceprovider_impl_unittest.cc',
      ],
    },
  ]
}
