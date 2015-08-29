# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      # GN version: //components/sync_driver
      'target_name': 'sync_driver',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../net/net.gyp:net',
        '../sync/sync.gyp:sync',
        '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation',
        '../third_party/cacheinvalidation/cacheinvalidation.gyp:cacheinvalidation_proto_cpp',
        '../ui/gfx/gfx.gyp:gfx',
        'favicon_core',
        'history_core_browser',
        'invalidation_public',
        'os_crypt',
        'signin_core_browser',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        # Note: file list duplicated in GN build.
        'sync_driver/backend_data_type_configurer.cc',
        'sync_driver/backend_data_type_configurer.h',
        'sync_driver/backend_migrator.cc',
        'sync_driver/backend_migrator.h',
        'sync_driver/change_processor.cc',
        'sync_driver/change_processor.h',
        'sync_driver/data_type_controller.cc',
        'sync_driver/data_type_controller.h',
        'sync_driver/data_type_encryption_handler.cc',
        'sync_driver/data_type_encryption_handler.h',
        'sync_driver/data_type_error_handler.h',
        'sync_driver/data_type_manager.cc',
        'sync_driver/data_type_manager.h',
        'sync_driver/data_type_manager_impl.cc',
        'sync_driver/data_type_manager_impl.h',
        'sync_driver/data_type_manager_observer.h',
        'sync_driver/data_type_status_table.cc',
        'sync_driver/data_type_status_table.h',
        'sync_driver/device_info.cc',
        'sync_driver/device_info.h',
        'sync_driver/device_info_data_type_controller.cc',
        'sync_driver/device_info_data_type_controller.h',
        'sync_driver/device_info_sync_service.cc',
        'sync_driver/device_info_sync_service.h',
        'sync_driver/device_info_tracker.h',
        'sync_driver/favicon_cache.cc',
        'sync_driver/favicon_cache.h',
        'sync_driver/frontend_data_type_controller.cc',
        'sync_driver/frontend_data_type_controller.h',
        'sync_driver/generic_change_processor.cc',
        'sync_driver/generic_change_processor.h',
        'sync_driver/generic_change_processor_factory.cc',
        'sync_driver/generic_change_processor_factory.h',
        'sync_driver/glue/synced_session.cc',
        'sync_driver/glue/synced_session.h',
        'sync_driver/glue/typed_url_model_associator.cc',
        'sync_driver/glue/typed_url_model_associator.h',
        'sync_driver/invalidation_adapter.cc',
        'sync_driver/invalidation_adapter.h',
        'sync_driver/invalidation_helper.cc',
        'sync_driver/invalidation_helper.h',
        'sync_driver/local_device_info_provider.h',
        'sync_driver/model_association_manager.cc',
        'sync_driver/model_association_manager.h',
        'sync_driver/model_associator.h',
        'sync_driver/non_blocking_data_type_controller.cc',
        'sync_driver/non_blocking_data_type_controller.h',
        'sync_driver/non_blocking_data_type_manager.cc',
        'sync_driver/non_blocking_data_type_manager.h',
        'sync_driver/non_ui_data_type_controller.cc',
        'sync_driver/non_ui_data_type_controller.h',
        'sync_driver/open_tabs_ui_delegate.cc',
        'sync_driver/open_tabs_ui_delegate.h',
        'sync_driver/pref_names.cc',
        'sync_driver/pref_names.h',
        'sync_driver/profile_sync_auth_provider.cc',
        'sync_driver/profile_sync_auth_provider.h',
        'sync_driver/protocol_event_observer.cc',
        'sync_driver/protocol_event_observer.h',
        'sync_driver/proxy_data_type_controller.cc',
        'sync_driver/proxy_data_type_controller.h',
        'sync_driver/shared_change_processor.cc',
        'sync_driver/shared_change_processor.h',
        'sync_driver/shared_change_processor_ref.cc',
        'sync_driver/shared_change_processor_ref.h',
        'sync_driver/sync_api_component_factory.h',
        'sync_driver/sync_client.cc',
        'sync_driver/sync_client.h',
        'sync_driver/sync_error_controller.cc',
        'sync_driver/sync_error_controller.h',
        'sync_driver/sync_frontend.cc',
        'sync_driver/sync_frontend.h',
        'sync_driver/sync_prefs.cc',
        'sync_driver/sync_prefs.h',
        'sync_driver/sync_service.h',
        'sync_driver/sync_service_observer.cc',
        'sync_driver/sync_service_observer.h',
        'sync_driver/sync_service_utils.cc',
        'sync_driver/sync_service_utils.h',
        'sync_driver/sync_stopped_reporter.cc',
        'sync_driver/sync_stopped_reporter.h',
        'sync_driver/system_encryptor.cc',
        'sync_driver/system_encryptor.h',
        'sync_driver/tab_node_pool.cc',
        'sync_driver/tab_node_pool.h',
        'sync_driver/ui_data_type_controller.cc',
        'sync_driver/ui_data_type_controller.h',
        'sync_driver/user_selectable_sync_type.h',
      ],
      'conditions': [
        ['OS!="ios"', {
          'dependencies': [
            'sessions_content',
          ],
        }, {  # OS==ios
          'dependencies': [
            'sessions_ios',
          ],
        }],
        ['configuration_policy==1', {
          'dependencies': [
            'policy',
          ],
          'sources': [
            'sync_driver/sync_policy_handler.cc',
            'sync_driver/sync_policy_handler.h',
          ],
        }],
      ],
    },
    {
      'target_name': 'sync_driver_test_support',
      'type': 'static_library',
      'dependencies': [
        'sync_driver',
        '../base/base.gyp:base',
        '../sync/sync.gyp:sync',
        '../sync/sync.gyp:test_support_sync_internal_api',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'sync_driver/change_processor_mock.cc',
        'sync_driver/change_processor_mock.h',
        'sync_driver/data_type_controller_mock.cc',
        'sync_driver/data_type_controller_mock.h',
        'sync_driver/data_type_error_handler_mock.cc',
        'sync_driver/data_type_error_handler_mock.h',
        'sync_driver/data_type_manager_mock.cc',
        'sync_driver/data_type_manager_mock.h',
        'sync_driver/fake_data_type_controller.cc',
        'sync_driver/fake_data_type_controller.h',
        'sync_driver/fake_generic_change_processor.cc',
        'sync_driver/fake_generic_change_processor.h',
        'sync_driver/fake_sync_client.cc',
        'sync_driver/fake_sync_client.h',
        'sync_driver/fake_sync_service.cc',
        'sync_driver/fake_sync_service.h',
        'sync_driver/frontend_data_type_controller_mock.cc',
        'sync_driver/frontend_data_type_controller_mock.h',
        'sync_driver/local_device_info_provider_mock.cc',
        'sync_driver/local_device_info_provider_mock.h',
        'sync_driver/model_associator_mock.cc',
        'sync_driver/model_associator_mock.h',
        'sync_driver/non_ui_data_type_controller_mock.cc',
        'sync_driver/non_ui_data_type_controller_mock.h',
      ],
    },
  ],
}
