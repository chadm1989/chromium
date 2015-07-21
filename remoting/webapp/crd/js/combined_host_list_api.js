// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * API implementation that combines two other implementations.
 */

/** @suppress {duplicate} */
var remoting = remoting || {};

(function() {

'use strict';

/**
 * Amount of time to wait for GCD results after legacy registry has
 * returned.
 */
var GCD_TIMEOUT_MS = 1000;

/**
 * @constructor
 * @param {!remoting.HostListApi} legacyImpl
 * @param {!remoting.HostListApi} gcdImpl
 * @implements {remoting.HostListApi}
 */
remoting.CombinedHostListApi = function(legacyImpl, gcdImpl) {
  /** @const {!remoting.HostListApi} */
  this.legacyImpl_ = legacyImpl;

  /** @const {!remoting.HostListApi} */
  this.gcdImpl_ = gcdImpl;

  /**
   * List of host IDs most recently retrieved from |legacyImpl_|.
   * @type {!Set<string>}
   */
  this.legacyIds_ = new Set();

  /**
   * List of host IDs most recently retrieved |gcdImpl_|.
   * @type {!Set<string>}
   */
  this.gcdIds_ = new Set();
};

/** @override */
remoting.CombinedHostListApi.prototype.register = function(
    hostName, publicKey, hostClientId) {
  var that = this;
  // First, register the new host with GCD, which will create a
  // service account and generate a host ID.
  return this.gcdImpl_.register(hostName, publicKey, hostClientId).then(
      function(gcdRegResult) {
        // After the GCD registration has been created, copy the
        // registration to the legacy directory so that clients not yet
        // upgraded to use GCD can see the new host.
        //
        // This is an ugly hack for multiple reasons:
        //
        // 1. It completely ignores |this.legacyImpl_|, complicating
        //    unit tests.
        //
        // 2. It relies on the fact that, when |hostClientId| is null,
        //    the legacy directory will "register" a host without
        //    creating a service account.  This is an obsolete feature
        //    of the legacy directory that is being revived for a new
        //    purpose.
        //
        // 3. It assumes the device ID generated by GCD is usable as a
        //    host ID by the legacy directory.  Fortunately both systems
        //    use UUIDs.
        return remoting.LegacyHostListApi.registerWithHostId(
            gcdRegResult.hostId, hostName, publicKey, null).then(
                function() {
                  // On success, return the result from GCD, ignoring
                  // the result returned by the legacy directory.
                  that.gcdIds_.add(gcdRegResult.hostId);
                  that.legacyIds_.add(gcdRegResult.hostId);
                  return gcdRegResult;
                },
                function(error) {
                  console.warn(
                      'Error copying host GCD host registration ' +
                      'to legacy directory: ' + error);
                  throw error;
                }
            );
      });
};

/** @override */
remoting.CombinedHostListApi.prototype.get = function() {
  // Fetch the host list from both directories and merge hosts that
  // have the same ID.
  var that = this;
  var legacyPromise = this.legacyImpl_.get();
  var gcdPromise = this.gcdImpl_.get();
  return legacyPromise.then(function(legacyHosts) {
    // If GCD is too slow, just act as if it had returned an empty
    // result set.
    var timeoutPromise = base.Promise.withTimeout(
        gcdPromise, GCD_TIMEOUT_MS, []);

    // Combine host information from both directories.  In the case of
    // conflicting information, prefer information from whichever
    // directory claims to have newer information.
    return timeoutPromise.then(function(gcdHosts) {
      // Update |that.gcdIds_| and |that.legacyIds_|.
      that.gcdIds_ = new Set();
      that.legacyIds_ = new Set();
      gcdHosts.forEach(function(host) {
        that.gcdIds_.add(host.hostId);
      });
      legacyHosts.forEach(function(host) {
        that.legacyIds_.add(host.hostId);
      });

      /**
       * A mapping from host IDs to the host data that will be
       * returned from this method.
       * @type {!Map<string,!remoting.Host>}
       */
      var hostMap = new Map();

      // Add legacy hosts to the output; some of these may be replaced
      // by GCD hosts.
      legacyHosts.forEach(function(host) {
        hostMap.set(host.hostId, host);
      });

      // Add GCD hosts to the output, possibly replacing some legacy
      // host data with newer data from GCD.
      gcdHosts.forEach(function(gcdHost) {
        var hostId = gcdHost.hostId;
        var legacyHost = hostMap.get(hostId);
        if (!legacyHost || legacyHost.updatedTime <= gcdHost.updatedTime) {
          hostMap.set(hostId, gcdHost);
        }
      });

      // Convert the result to an Array.
      // TODO(jrw): Use Array.from once it becomes available.
      var hosts = [];
      hostMap.forEach(function(host) {
        hosts.push(host);
      });
      return hosts;
    });
  });
};

/** @override */
remoting.CombinedHostListApi.prototype.put =
    function(hostId, hostName, hostPublicKey) {
  var legacyPromise = Promise.resolve();
  if (this.legacyIds_.has(hostId)) {
    legacyPromise = this.legacyImpl_.put(hostId, hostName, hostPublicKey);
  }
  var gcdPromise = Promise.resolve();
  if (this.gcdIds_.has(hostId)) {
    gcdPromise = this.gcdImpl_.put(hostId, hostName, hostPublicKey);
  }
  return legacyPromise.then(function() {
    // If GCD is too slow, just ignore it and return result from the
    // legacy directory.
    return base.Promise.withTimeout(
        gcdPromise, GCD_TIMEOUT_MS);
  });
};

/** @override */
remoting.CombinedHostListApi.prototype.remove = function(hostId) {
  var legacyPromise = Promise.resolve();
  if (this.legacyIds_.has(hostId)) {
    legacyPromise = this.legacyImpl_.remove(hostId);
  }
  var gcdPromise = Promise.resolve();
  if (this.gcdIds_.has(hostId)) {
    gcdPromise = this.gcdImpl_.remove(hostId);
  }
  return legacyPromise.then(function() {
    // If GCD is too slow, just ignore it and return result from the
    // legacy directory.
    return base.Promise.withTimeout(
        gcdPromise, GCD_TIMEOUT_MS);
  });
};

/** @override */
remoting.CombinedHostListApi.prototype.getSupportHost = function(supportId) {
  return this.legacyImpl_.getSupportHost(supportId);
};

})();
