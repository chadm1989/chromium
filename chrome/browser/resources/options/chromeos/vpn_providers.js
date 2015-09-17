// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A singleton that keeps track of the VPN providers enabled in
 * the primary user's profile.
 */

cr.define('options', function() {
  /**
   * @constructor
   */
  function VPNProviders() {
  }

  cr.addSingletonGetter(VPNProviders);

  VPNProviders.prototype = {
    /**
     * The VPN providers enabled in the primary user's profile. Each provider
     * has a name. Third-party VPN providers additionally have an extension ID.
     * @type {!Array<{name: string, extensionID: ?string}>}
     * @private
     */
    providers_: [],

    /**
     * Observers who will be called when the list of VPN providers changes.
     * @type {!Array<!function()>}
     */
    observers_: [],

    /**
     * The VPN providers enabled in the primary user's profile.
     * @type {!Array<{name: string, extensionID: ?string}>}
     */
    get providers() {
      return this.providers_;
    },
    set providers(providers) {
      this.providers_ = providers;
      for (var i = 0; i < this.observers_.length; ++i)
        this.observers_[i]();
    },

    /**
     * Adds an observer to be called when the list of VPN providers changes.
     * @param {!function()} observer The observer to add.
     * @private
     */
    addObserver_: function(observer) {
      this.observers_.push(observer);
    },
  };

  /**
   * Adds an observer to be called when the list of VPN providers changes. Note
   * that an observer may in turn call setProviders() but should be careful not
   * to get stuck in an infinite loop as every change to the list of VPN
   * providers will cause the observers to be called again.
   * @param {!function()} observer The observer to add.
   */
  VPNProviders.addObserver = function(observer) {
    VPNProviders.getInstance().addObserver_(observer);
  };

  /**
   * Returns the list of VPN providers enabled in the primary user's profile.
   * @return {!Array<{name: string, extensionID: ?string}>} The list of VPN
   *     providers enabled in the primary user's profile.
   */
  VPNProviders.getProviders = function() {
    return VPNProviders.getInstance().providers;
  };

  /**
   * Replaces the list of VPN providers enabled in the primary user's profile.
   * @param {!Array<{name: string, extensionID: ?string}>} providers The list
   *     of VPN providers enabled in the primary user's profile.
   */
  VPNProviders.setProviders = function(providers) {
    VPNProviders.getInstance().providers = providers;
  };

  // Export
  return {
    VPNProviders: VPNProviders
  };
});
