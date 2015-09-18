/* Copyright 2015 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

/**
 * @fileoverview
 * 'cr-settings-prefs' models Chrome settings and preferences, listening for
 * changes to Chrome prefs whitelisted in chrome.settingsPrivate.
 * When changing prefs in this element's 'prefs' property via the UI, this
 * element tries to set those preferences in Chrome. Whether or not the calls to
 * settingsPrivate.setPref succeed, 'prefs' is eventually consistent with the
 * Chrome pref store.
 *
 * Example:
 *
 *    <cr-settings-prefs prefs="{{prefs}}"></cr-settings-prefs>
 *    <cr-settings-a11y-page prefs="{{prefs}}"></cr-settings-a11y-page>
 *
 * @group Chrome Settings Elements
 * @element cr-settings-prefs
 */

(function() {
  'use strict';

  /**
   * Checks whether two values are recursively equal. Only compares serializable
   * data (primitives, serializable arrays and serializable objects).
   * @param {*} val1 Value to compare.
   * @param {*} val2 Value to compare with val1.
   * @return {boolean} True if the values are recursively equal.
   */
  function deepEqual(val1, val2) {
    if (val1 === val2)
      return true;

    if (Array.isArray(val1) || Array.isArray(val2)) {
      if (!Array.isArray(val1) || !Array.isArray(val2))
        return false;
      return arraysEqual(/** @type {!Array} */(val1),
                         /** @type {!Array} */(val2));
    }

    if (val1 instanceof Object && val2 instanceof Object)
      return objectsEqual(val1, val2);

    return false;
  }

  /**
   * @param {!Array} arr1
   * @param {!Array} arr2
   * @return {boolean} True if the arrays are recursively equal.
   */
  function arraysEqual(arr1, arr2) {
    if (arr1.length != arr2.length)
      return false;

    for (var i = 0; i < arr1.length; i++) {
      if (!deepEqual(arr1[i], arr2[i]))
        return false;
    }

    return true;
  }

  /**
   * @param {!Object} obj1
   * @param {!Object} obj2
   * @return {boolean} True if the objects are recursively equal.
   */
  function objectsEqual(obj1, obj2) {
    var keys1 = Object.keys(obj1);
    var keys2 = Object.keys(obj2);
    if (keys1.length != keys2.length)
      return false;

    for (var i = 0; i < keys1.length; i++) {
      var key = keys1[i];
      if (!deepEqual(obj1[key], obj2[key]))
        return false;
    }

    return true;
  }

  /**
   * Returns a recursive copy of the value.
   * @param {*} val Value to copy. Should be a primitive or only contain
   *     serializable data (primitives, serializable arrays and
   *     serializable objects).
   * @return {*} A deep copy of the value.
   */
  function deepCopy(val) {
    if (!(val instanceof Object))
      return val;
    return Array.isArray(val) ? deepCopyArray(/** @type {!Array} */(val)) :
        deepCopyObject(val);
  };

  /**
   * @param {!Array} arr
   * @return {!Array} Deep copy of the array.
   */
  function deepCopyArray(arr) {
    var copy = [];
    for (var i = 0; i < arr.length; i++)
      copy.push(deepCopy(arr[i]));
    return copy;
  }

  /**
   * @param {!Object} obj
   * @return {!Object} Deep copy of the object.
   */
  function deepCopyObject(obj) {
    var copy = {};
    var keys = Object.keys(obj);
    for (var i = 0; i < keys.length; i++) {
      var key = keys[i];
      copy[key] = deepCopy(obj[key]);
    }
    return copy;
  }

  Polymer({
    is: 'cr-settings-prefs',

    properties: {
      /**
       * Object containing all preferences, for use by Polymer controls.
       */
      prefs: {
        type: Object,
        notify: true,
      },

      /**
       * Map of pref keys to values representing the state of the Chrome
       * pref store as of the last update from the API.
       * @type {Object<*>}
       * @private
       */
      lastPrefValues_: {
        type: Object,
        value: function() { return {}; },
      },
    },

    observers: [
      'prefsChanged_(prefs.*)',
    ],

    settingsApi_: chrome.settingsPrivate,

    /** @override */
    ready: function() {
      // Set window.mockApi to pass a custom settings API, i.e. for tests.
      // TODO(michaelpg): don't use a global.
      if (window.mockApi)
        this.settingsApi_ = window.mockApi;
      CrSettingsPrefs.isInitialized = false;

      this.settingsApi_.onPrefsChanged.addListener(
          this.onSettingsPrivatePrefsChanged_.bind(this));
      this.settingsApi_.getAllPrefs(
          this.onSettingsPrivatePrefsFetched_.bind(this));
    },

    /**
     * Polymer callback for changes to this.prefs.
     * @param {!{path: string, value: *}} change
     * @private
     */
    prefsChanged_: function(change) {
      if (!CrSettingsPrefs.isInitialized)
        return;

      var key = this.getPrefKeyFromPath_(change.path);
      var prefStoreValue = this.lastPrefValues_[key];

      var prefObj = /** @type {chrome.settingsPrivate.PrefObject} */(
          this.get(key, this.prefs));

      // If settingsPrivate already has this value, do nothing. (Otherwise,
      // a change event from settingsPrivate could make us call
      // settingsPrivate.setPref and potentially trigger an IPC loop.)
      if (deepEqual(prefStoreValue, prefObj.value))
        return;

      this.settingsApi_.setPref(
          key,
          prefObj.value,
          /* pageId */ '',
          /* callback */ this.setPrefCallback_.bind(this, key));
    },

    /**
     * Called when prefs in the underlying Chrome pref store are changed.
     * @param {!Array<!chrome.settingsPrivate.PrefObject>} prefs
     *     The prefs that changed.
     * @private
     */
    onSettingsPrivatePrefsChanged_: function(prefs) {
      if (CrSettingsPrefs.isInitialized)
        this.updatePrefs_(prefs);
    },

    /**
     * Called when prefs are fetched from settingsPrivate.
     * @param {!Array<!chrome.settingsPrivate.PrefObject>} prefs
     * @private
     */
    onSettingsPrivatePrefsFetched_: function(prefs) {
      this.updatePrefs_(prefs);

      CrSettingsPrefs.isInitialized = true;
      document.dispatchEvent(new Event(CrSettingsPrefs.INITIALIZED));
    },

    /**
     * Checks the result of calling settingsPrivate.setPref.
     * @param {string} key The key used in the call to setPref.
     * @param {boolean} success True if setting the pref succeeded.
     * @private
     */
    setPrefCallback_: function(key, success) {
      if (success)
        return;

      // Get the current pref value from chrome.settingsPrivate to ensure the
      // UI stays up to date.
      this.settingsApi_.getPref(key, function(pref) {
        this.updatePrefs_([pref]);
      }.bind(this));
    },

    /**
     * Updates the prefs model with the given prefs.
     * @param {!Array<!chrome.settingsPrivate.PrefObject>} newPrefs
     * @private
     */
    updatePrefs_: function(newPrefs) {
      // Use the existing prefs object or create it.
      var prefs = this.prefs || {};
      newPrefs.forEach(function(newPrefObj) {
        // Use the PrefObject from settingsPrivate to create a copy in
        // lastPrefValues_ at the pref's key.
        this.lastPrefValues_[newPrefObj.key] = deepCopy(newPrefObj.value);

        // Add the pref to |prefs|.
        cr.exportPath(newPrefObj.key, newPrefObj, prefs);
        // If this.prefs already exists, notify listeners of the change.
        if (prefs == this.prefs)
          this.notifyPath('prefs.' + newPrefObj.key, newPrefObj);
      }, this);
      if (!this.prefs)
        this.prefs = prefs;
    },

    /**
     * Given a 'property-changed' path, returns the key of the preference the
     * path refers to. E.g., if the path of the changed property is
     * 'prefs.search.suggest_enabled.value', the key of the pref that changed is
     * 'search.suggest_enabled'.
     * @param {string} path
     * @return {string}
     * @private
     */
    getPrefKeyFromPath_: function(path) {
      // Skip the first token, which refers to the member variable (this.prefs).
      var parts = path.split('.');
      assert(parts.shift() == 'prefs');

      for (let i = 1; i <= parts.length; i++) {
        let key = parts.slice(0, i).join('.');
        // The lastPrefValues_ keys match the pref keys.
        if (this.lastPrefValues_.hasOwnProperty(key))
          return key;
      }
      return '';
    },
  });
})();
