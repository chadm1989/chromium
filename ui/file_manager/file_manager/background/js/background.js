// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Type of a Files.app's instance launch.
 * @enum {number}
 */
var LaunchType = {
  ALWAYS_CREATE: 0,
  FOCUS_ANY_OR_CREATE: 1,
  FOCUS_SAME_OR_CREATE: 2
};

/**
 * Root class of the background page.
 * @constructor
 * @extends {BackgroundBase}
 * @struct
 */
function FileBrowserBackground() {
  BackgroundBase.call(this);

  /**
   * Synchronous queue for asynchronous calls.
   * @type {!AsyncUtil.Queue}
   */
  this.queue = new AsyncUtil.Queue();

  /**
   * Progress center of the background page.
   * @type {!ProgressCenter}
   */
  this.progressCenter = new ProgressCenter();

  /**
   * File operation manager.
   * @type {!FileOperationManager}
   */
  this.fileOperationManager = new FileOperationManager();

  /**
   * Class providing loading of import history, used in
   * cloud import.
   *
   * @type {!importer.HistoryLoader}
   */
  this.historyLoader = new importer.RuntimeHistoryLoader();

  /**
   * Event handler for progress center.
   * @private {!FileOperationHandler}
   */
  this.fileOperationHandler_ = new FileOperationHandler(this);

  /**
   * Event handler for C++ sides notifications.
   * @private {!DeviceHandler}
   */
  this.deviceHandler_ = new DeviceHandler();

  // Handle device navigation requests.
  this.deviceHandler_.addEventListener(
      DeviceHandler.VOLUME_NAVIGATION_REQUESTED,
      this.handleViewEvent_.bind(this));

  /**
   * Drive sync handler.
   * @type {!DriveSyncHandler}
   */
  this.driveSyncHandler = new DriveSyncHandler(this.progressCenter);
  this.driveSyncHandler.addEventListener(
      DriveSyncHandler.COMPLETED_EVENT,
      function() { this.tryClose(); }.bind(this));

  /**
   * Provides support for scaning media devices as part of Cloud Import.
   * @type {!importer.MediaScanner}
   */
  this.mediaScanner = new importer.DefaultMediaScanner(
      importer.createMetadataHashcode,
      this.historyLoader,
      importer.DefaultDirectoryWatcher.create);

  /**
   * Handles importing of user media (e.g. photos, videos) from removable
   * devices.
   * @type {!importer.MediaImportHandler}
   */
  this.mediaImportHandler =
      new importer.MediaImportHandler(
          this.progressCenter,
          this.historyLoader);

  /**
   * Promise of string data.
   * @type {!Promise}
   */
  this.stringDataPromise = new Promise(function(fulfill) {
    chrome.fileManagerPrivate.getStrings(fulfill);
  });

  /**
   * String assets.
   * @type {Object<string, string>}
   */
  this.stringData = null;

  /**
   * Callback list to be invoked after initialization.
   * It turns to null after initialization.
   *
   * @private {!Array<function()>}
   */
  this.initializeCallbacks_ = [];

  /**
   * Last time when the background page can close.
   *
   * @private {?number}
   */
  this.lastTimeCanClose_ = null;

  // Initialize handlers.
  chrome.fileBrowserHandler.onExecute.addListener(this.onExecute_.bind(this));
  chrome.app.runtime.onLaunched.addListener(this.onLaunched_.bind(this));
  chrome.app.runtime.onRestarted.addListener(this.onRestarted_.bind(this));
  chrome.contextMenus.onClicked.addListener(
      this.onContextMenuClicked_.bind(this));

  this.queue.run(function(callback) {
    this.stringDataPromise.then(function(strings) {
      // Init string data.
      this.stringData = strings;
      loadTimeData.data = strings;

      // Init context menu.
      this.initContextMenu_();

      callback();
    }.bind(this)).catch(function(error) {
      console.error(error.stack || error);
      callback();
    });
  }.bind(this));
}

/**
 * A number of delay milliseconds from the first call of tryClose to the actual
 * close action.
 * @const {number}
 * @private
 */
FileBrowserBackground.CLOSE_DELAY_MS_ = 5000;

FileBrowserBackground.prototype.__proto__ = BackgroundBase.prototype;

/**
 * Register callback to be invoked after initialization.
 * If the initialization is already done, the callback is invoked immediately.
 *
 * @param {function()} callback Initialize callback to be registered.
 */
FileBrowserBackground.prototype.ready = function(callback) {
  this.stringDataPromise.then(callback);
};

/**
 * Checks the current condition of background page.
 * @return {boolean} True if the background page is closable, false if not.
 */
FileBrowserBackground.prototype.canClose = function() {
  // If the file operation is going, the background page cannot close.
  if (this.fileOperationManager.hasQueuedTasks() ||
      this.driveSyncHandler.syncing) {
    this.lastTimeCanClose_ = null;
    return false;
  }

  var views = chrome.extension.getViews();
  var closing = false;
  for (var i = 0; i < views.length; i++) {
    // If the window that is not the background page itself and it is not
    // closing, the background page cannot close.
    if (views[i] !== window && !views[i].closing) {
      this.lastTimeCanClose_ = null;
      return false;
    }
    closing = closing || views[i].closing;
  }

  // If some windows are closing, or the background page can close but could not
  // 5 seconds ago, We need more time for sure.
  if (closing ||
      this.lastTimeCanClose_ === null ||
      (Date.now() - this.lastTimeCanClose_ <
           FileBrowserBackground.CLOSE_DELAY_MS_)) {
    if (this.lastTimeCanClose_ === null)
      this.lastTimeCanClose_ = Date.now();
    setTimeout(this.tryClose.bind(this), FileBrowserBackground.CLOSE_DELAY_MS_);
    return false;
  }

  // Otherwise we can close the background page.
  return true;
};

/**
 * Opens the volume root (or opt directoryPath) in main UI.
 *
 * @param {!Event} event An event with the volumeId or
 *     devicePath.
 * @private
 */
FileBrowserBackground.prototype.handleViewEvent_ =
    function(event) {
  VolumeManager.getInstance()
      .then(
          /**
           * Retrieves the root file entry of the volume on the requested
           * device.
           * @param {!VolumeManager} volumeManager
           */
          function(volumeManager) {
            if (event.devicePath) {
              var volume = volumeManager.volumeInfoList.findByDevicePath(
                  event.devicePath);
              if (volume) {
                this.navigateToVolume_(volume);
              } else {
                console.error('Got view event with invalid volume id.');
              }
            } else if (event.volumeId) {
              volumeManager.volumeInfoList.whenVolumeInfoReady(event.volumeId)
                  .then(
                      function(volume) {
                        this.navigateToVolume_(volume, event.filePath);
                      }.bind(this))
                  .catch(
                      function(e) {
                        console.error(
                            'Unable to find volume for id: ' + event.volumeId +
                            '. Error: ' + e.message);
                      });
            } else {
              console.error('Got view event with no actionable destination.');
            }
          }.bind(this));
};

/**
 * Opens the volume root (or opt directoryPath) in main UI.
 *
 * @param {!VolumeInfo} volume
 * @param {string=} opt_directoryPath Optional directory path to be opened.
 * @private
 */
FileBrowserBackground.prototype.navigateToVolume_ =
    function(volume, opt_directoryPath) {
  volume.resolveDisplayRoot()
      .then(
          /**
           * If a path was specified, retrieve that directory entry,
           * otherwise just return the unmodified root entry.
           * @param {DirectoryEntry} root
           * @return {!Promise<DirectoryEntry>}
           */
          function(root) {
            if (opt_directoryPath) {
              return new Promise(
                  root.getDirectory.bind(
                      root, opt_directoryPath, {create: false}));
            } else {
              return Promise.resolve(root);
            }
          })
      .then(
          /**
           * Launches app opened on {@code directory}.
           * @param {DirectoryEntry} directory
           */
          function(directory) {
            launchFileManager(
                {currentDirectoryURL: directory.toURL()},
                /* App ID */ undefined,
                LaunchType.FOCUS_SAME_OR_CREATE);
          });
};

/**
 * Prefix for the file manager window ID.
 * @type {string}
 * @const
 */
var FILES_ID_PREFIX = 'files#';

/**
 * Regexp matching a file manager window ID.
 * @type {RegExp}
 * @const
 */
var FILES_ID_PATTERN = new RegExp('^' + FILES_ID_PREFIX + '(\\d*)$');

/**
 * Prefix for the dialog ID.
 * @type {string}
 * @const
 */
var DIALOG_ID_PREFIX = 'dialog#';

/**
 * Value of the next file manager window ID.
 * @type {number}
 */
var nextFileManagerWindowID = 0;

/**
 * Value of the next file manager dialog ID.
 * @type {number}
 */
var nextFileManagerDialogID = 0;

/**
 * File manager window create options.
 * @type {Object}
 * @const
 */
var FILE_MANAGER_WINDOW_CREATE_OPTIONS = Object.freeze({
  bounds: Object.freeze({
    left: Math.round(window.screen.availWidth * 0.1),
    top: Math.round(window.screen.availHeight * 0.1),
    width: Math.round(window.screen.availWidth * 0.8),
    height: Math.round(window.screen.availHeight * 0.8)
  }),
  minWidth: 480,
  minHeight: 300,
  hidden: true
});

/**
 * @param {Object=} opt_appState App state.
 * @param {number=} opt_id Window id.
 * @param {LaunchType=} opt_type Launch type. Default: ALWAYS_CREATE.
 * @param {function(string)=} opt_callback Completion callback with the App ID.
 */
function launchFileManager(opt_appState, opt_id, opt_type, opt_callback) {
  var type = opt_type || LaunchType.ALWAYS_CREATE;
  opt_appState =
      /**
       * @type {(undefined|
       *         {currentDirectoryURL: (string|undefined),
       *          selectionURL: (string|undefined),
       *          displayedId: (string|undefined)})}
       */
      (opt_appState);

  // Wait until all windows are created.
  window.background.queue.run(function(onTaskCompleted) {
    // Check if there is already a window with the same URL. If so, then
    // reuse it instead of opening a new one.
    if (type == LaunchType.FOCUS_SAME_OR_CREATE ||
        type == LaunchType.FOCUS_ANY_OR_CREATE) {
      if (opt_appState) {
        for (var key in window.background.appWindows) {
          if (!key.match(FILES_ID_PATTERN))
            continue;

          var contentWindow = window.background.appWindows[key].contentWindow;
          if (!contentWindow.appState)
            continue;

          // Different current directories.
          if (opt_appState.currentDirectoryURL !==
                  contentWindow.appState.currentDirectoryURL) {
            continue;
          }

          // Selection URL specified, and it is different.
          if (opt_appState.selectionURL &&
                  opt_appState.selectionURL !==
                  contentWindow.appState.selectionURL) {
            continue;
          }

          AppWindowWrapper.focusOnDesktop(
              window.background.appWindows[key], opt_appState.displayedId);
          if (opt_callback)
            opt_callback(key);
          onTaskCompleted();
          return;
        }
      }
    }

    // Focus any window if none is focused. Try restored first.
    if (type == LaunchType.FOCUS_ANY_OR_CREATE) {
      // If there is already a focused window, then finish.
      for (var key in window.background.appWindows) {
        if (!key.match(FILES_ID_PATTERN))
          continue;

        // The isFocused() method should always be available, but in case
        // Files.app's failed on some error, wrap it with try catch.
        try {
          if (window.background.appWindows[key].contentWindow.isFocused()) {
            if (opt_callback)
              opt_callback(key);
            onTaskCompleted();
            return;
          }
        } catch (e) {
          console.error(e.message);
        }
      }
      // Try to focus the first non-minimized window.
      for (var key in window.background.appWindows) {
        if (!key.match(FILES_ID_PATTERN))
          continue;

        if (!window.background.appWindows[key].isMinimized()) {
          AppWindowWrapper.focusOnDesktop(
              window.background.appWindows[key],
              (opt_appState || {}).displayedId);
          if (opt_callback)
            opt_callback(key);
          onTaskCompleted();
          return;
        }
      }
      // Restore and focus any window.
      for (var key in window.background.appWindows) {
        if (!key.match(FILES_ID_PATTERN))
          continue;

        AppWindowWrapper.focusOnDesktop(
            window.background.appWindows[key],
            (opt_appState || {}).displayedId);
        if (opt_callback)
          opt_callback(key);
        onTaskCompleted();
        return;
      }
    }

    // Create a new instance in case of ALWAYS_CREATE type, or as a fallback
    // for other types.

    var id = opt_id || nextFileManagerWindowID;
    nextFileManagerWindowID = Math.max(nextFileManagerWindowID, id + 1);
    var appId = FILES_ID_PREFIX + id;

    var appWindow = new AppWindowWrapper(
        'main.html',
        appId,
        FILE_MANAGER_WINDOW_CREATE_OPTIONS);
    appWindow.launch(opt_appState || {}, false, function() {
      AppWindowWrapper.focusOnDesktop(
          appWindow.rawAppWindow, (opt_appState || {}).displayedId);
      if (opt_callback)
        opt_callback(appId);
      onTaskCompleted();
    });
  });
}

/**
 * Registers dialog window to the background page.
 *
 * @param {!Window} dialogWindow Window of the dialog.
 */
function registerDialog(dialogWindow) {
  var id = DIALOG_ID_PREFIX + (nextFileManagerDialogID++);
  window.background.dialogs[id] = dialogWindow;
  dialogWindow.addEventListener('pagehide', function() {
    delete window.background.dialogs[id];
  });
}

/**
 * Executes a file browser task.
 *
 * @param {string} action Task id.
 * @param {Object} details Details object.
 * @private
 */
FileBrowserBackground.prototype.onExecute_ = function(action, details) {
  var appState = {
    params: {action: action},
    // It is not allowed to call getParent() here, since there may be
    // no permissions to access it at this stage. Therefore we are passing
    // the selectionURL only, and the currentDirectory will be resolved
    // later.
    selectionURL: details.entries[0].toURL()
  };

  // Every other action opens a Files app window.
  // For mounted devices just focus any Files.app window. The mounted
  // volume will appear on the navigation list.
  launchFileManager(
      appState,
      /* App ID */ undefined,
      LaunchType.FOCUS_SAME_OR_CREATE);
};

/**
 * Launches the app.
 * @private
 */
FileBrowserBackground.prototype.onLaunched_ = function() {
  if (nextFileManagerWindowID == 0) {
    // The app just launched. Remove window state records that are not needed
    // any more.
    chrome.storage.local.get(function(items) {
      for (var key in items) {
        if (items.hasOwnProperty(key)) {
          if (key.match(FILES_ID_PATTERN))
            chrome.storage.local.remove(key);
        }
      }
    });
  }
  launchFileManager(null, undefined, LaunchType.FOCUS_ANY_OR_CREATE);
};

/**
 * Restarted the app, restore windows.
 * @private
 */
FileBrowserBackground.prototype.onRestarted_ = function() {
  // Reopen file manager windows.
  chrome.storage.local.get(function(items) {
    for (var key in items) {
      if (items.hasOwnProperty(key)) {
        var match = key.match(FILES_ID_PATTERN);
        if (match) {
          var id = Number(match[1]);
          try {
            var appState = /** @type {Object} */ (JSON.parse(items[key]));
            launchFileManager(appState, id);
          } catch (e) {
            console.error('Corrupt launch data for ' + id);
          }
        }
      }
    }
  });
};

/**
 * Handles clicks on a custom item on the launcher context menu.
 * @param {!Object} info Event details.
 * @private
 */
FileBrowserBackground.prototype.onContextMenuClicked_ = function(info) {
  if (info.menuItemId == 'new-window') {
    // Find the focused window (if any) and use it's current url for the
    // new window. If not found, then launch with the default url.
    for (var key in window.background.appWindows) {
      try {
        if (window.background.appWindows[key].contentWindow.isFocused()) {
          var appState = {
            // Do not clone the selection url, only the current directory.
            currentDirectoryURL: window.background.appWindows[key].
                contentWindow.appState.currentDirectoryURL
          };
          launchFileManager(appState);
          return;
        }
      } catch (ignore) {
        // The isFocused method may not be defined during initialization.
        // Therefore, wrapped with a try-catch block.
      }
    }

    // Launch with the default URL.
    launchFileManager();
  }
};

/**
 * Initializes the context menu. Recreates if already exists.
 * @private
 */
FileBrowserBackground.prototype.initContextMenu_ = function() {
  try {
    // According to the spec [1], the callback is optional. But no callback
    // causes an error for some reason, so we call it with null-callback to
    // prevent the error. http://crbug.com/353877
    // - [1] https://developer.chrome.com/extensions/contextMenus#method-remove
    chrome.contextMenus.remove('new-window', function() {});
  } catch (ignore) {
    // There is no way to detect if the context menu is already added, therefore
    // try to recreate it every time.
  }
  chrome.contextMenus.create({
    id: 'new-window',
    contexts: ['launcher'],
    title: str('NEW_WINDOW_BUTTON_LABEL')
  });
};

/**
 * Singleton instance of Background.
 * @type {FileBrowserBackground}
 */
window.background = new FileBrowserBackground();
