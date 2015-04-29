// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var util = {};
var embedder = {};
embedder.baseGuestURL = '';
embedder.emptyGuestURL = '';
embedder.windowOpenGuestURL = '';
embedder.noReferrerGuestURL = '';
embedder.redirectGuestURL = '';
embedder.redirectGuestURLDest = '';
embedder.closeSocketURL = '';
embedder.tests = {};

var request_to_comm_channel_1 = 'connect';
var request_to_comm_channel_2 = 'connect_request';
var response_from_comm_channel_1 = 'connected';
var response_from_comm_channel_2 = 'connected_response';

embedder.setUp_ = function(config) {
  if (!config || !config.testServer) {
    return;
  }
  embedder.baseGuestURL = 'http://localhost:' + config.testServer.port;
  embedder.emptyGuestURL = embedder.baseGuestURL +
      '/extensions/platform_apps/web_view/shim/empty_guest.html';
  embedder.windowOpenGuestURL = embedder.baseGuestURL +
      '/extensions/platform_apps/web_view/shim/guest.html';
  embedder.windowOpenGuestFromSameURL = embedder.baseGuestURL +
      '/extensions/platform_apps/web_view/shim/guest_from_opener.html';
  embedder.noReferrerGuestURL = embedder.baseGuestURL +
      '/extensions/platform_apps/web_view/shim/guest_noreferrer.html';
  embedder.detectUserAgentURL = embedder.baseGuestURL + '/detect-user-agent';
  embedder.redirectGuestURL = embedder.baseGuestURL + '/server-redirect';
  embedder.redirectGuestURLDest = embedder.baseGuestURL +
      '/extensions/platform_apps/web_view/shim/guest_redirect.html';
  embedder.closeSocketURL = embedder.baseGuestURL + '/close-socket';
  embedder.testImageBaseURL = embedder.baseGuestURL +
      '/extensions/platform_apps/web_view/shim/';
  embedder.virtualURL = 'http://virtualurl/';
  embedder.pluginURL = embedder.baseGuestURL +
      '/extensions/platform_apps/web_view/shim/embed.html';
};

window.runTest = function(testName) {
  if (!embedder.test.testList[testName]) {
    console.log('Incorrect testName: ' + testName);
    embedder.test.fail();
    return;
  }

  // Run the test.
  embedder.test.testList[testName]();
};

var LOG = function(msg) {
  window.console.log(msg);
};

// Creates a <webview> tag in document.body and returns the reference to it.
// It also sets a dummy src. The dummy src is significant because this makes
// sure that the <object> shim is created (asynchronously at this point) for the
// <webview> tag. This makes the <webview> tag ready for add/removeEventListener
// calls.
util.createWebViewTagInDOM = function(partitionName) {
  var webview = document.createElement('webview');
  webview.style.width = '300px';
  webview.style.height = '200px';
  var urlDummy = 'data:text/html,<body>Initial dummy guest</body>';
  webview.setAttribute('src', urlDummy);
  webview.setAttribute('partition', partitionName);
  document.body.appendChild(webview);
  return webview;
};

embedder.test = {};
embedder.test.succeed = function() {
  chrome.test.sendMessage('TEST_PASSED');
};

embedder.test.fail = function() {
  chrome.test.sendMessage('TEST_FAILED');
};

embedder.test.assertEq = function(a, b) {
  if (a != b) {
    console.log('assertion failed: ' + a + ' != ' + b);
    embedder.test.fail();
  }
};

embedder.test.assertTrue = function(condition) {
  if (!condition) {
    console.log('assertion failed: true != ' + condition);
    embedder.test.fail();
  }
};

embedder.test.assertFalse = function(condition) {
  if (condition) {
    console.log('assertion failed: false != ' + condition);
    embedder.test.fail();
  }
};

// Tests begin.

// This test verifies that the allowtransparency property is interpreted as true
// if it exists (regardless of its value), and can be removed by setting it to
// to anything false.
function testAllowTransparencyAttribute() {
  var webview = document.createElement('webview');
  webview.src = 'data:text/html,webview test';
  embedder.test.assertFalse(webview.hasAttribute('allowtransparency'));
  embedder.test.assertFalse(webview.allowtransparency);
  webview.allowtransparency = true;

  webview.addEventListener('loadstop', function(e) {
    embedder.test.assertTrue(webview.hasAttribute('allowtransparency'));
    embedder.test.assertTrue(webview.allowtransparency);
    webview.allowtransparency = false;
    embedder.test.assertFalse(webview.hasAttribute('allowtransparency'));
    embedder.test.assertFalse(webview.allowtransparency);
    webview.allowtransparency = '';
    embedder.test.assertFalse(webview.hasAttribute('allowtransparency'));
    embedder.test.assertFalse(webview.allowtransparency);
    webview.allowtransparency = 'some string';
    embedder.test.assertTrue(webview.hasAttribute('allowtransparency'));
    embedder.test.assertTrue(webview.allowtransparency);
    embedder.test.succeed();
  });

  document.body.appendChild(webview);
}

// This test verifies that a lengthy page with autosize enabled will report
// the correct height in the sizechanged event.
function testAutosizeHeight() {
  var webview = document.createElement('webview');

  webview.autosize = true;
  webview.minwidth = 200;
  webview.maxwidth = 210;
  webview.minheight = 40;
  webview.maxheight = 200;

  var step = 1;
  webview.addEventListener('sizechanged', function(e) {
    switch (step) {
      case 1:
        embedder.test.assertEq(200, e.newHeight);
        // Change the maxheight to verify that we see the change.
        webview.maxheight = 50;
        break;
      case 2:
        embedder.test.assertEq(200, e.oldHeight);
        embedder.test.assertEq(50, e.newHeight);
        embedder.test.succeed();
        break;
      default:
        window.console.log('Unexpected sizechanged event, step = ' + step);
        embedder.test.fail();
        break;
    }
    ++step;
  });

  webview.src = 'data:text/html,' +
                'a<br/>b<br/>c<br/>d<br/>e<br/>f<br/>' +
                'a<br/>b<br/>c<br/>d<br/>e<br/>f<br/>' +
                'a<br/>b<br/>c<br/>d<br/>e<br/>f<br/>' +
                'a<br/>b<br/>c<br/>d<br/>e<br/>f<br/>' +
                'a<br/>b<br/>c<br/>d<br/>e<br/>f<br/>';
  document.body.appendChild(webview);
}

// This test verifies that if a browser plugin is in autosize mode before
// navigation then the guest starts auto-sized.
function testAutosizeBeforeNavigation() {
  var webview = document.createElement('webview');

  webview.setAttribute('autosize', 'true');
  webview.setAttribute('minwidth', 200);
  webview.setAttribute('maxwidth', 210);
  webview.setAttribute('minheight', 100);
  webview.setAttribute('maxheight', 110);

  webview.addEventListener('sizechanged', function(e) {
    // The old size should be the default size of the webview, which at the time
    // of writing this comment is 300 x 300, but is not important to this test
    // and could change in the future, so it is not checked here.
    embedder.test.assertTrue(e.newWidth >= 200 && e.newWidth <= 210);
    embedder.test.assertTrue(e.newHeight >= 100 && e.newHeight <= 110);
    embedder.test.succeed();
  });

  webview.setAttribute('src', 'data:text/html,webview test sizechanged event');
  document.body.appendChild(webview);
}

// Makes sure 'sizechanged' event is fired only if autosize attribute is
// specified.
// After loading <webview> without autosize attribute and a size, say size1,
// we set autosize attribute and new min size with size2. We would get (only
// one) sizechanged event with size1 as old size and size2 as new size.
function testAutosizeAfterNavigation() {
  var webview = document.createElement('webview');

  var step = 1;
  var autosizeWidth = -1;
  var autosizeHeight = -1;
  var sizeChangeHandler = function(e) {
    switch (step) {
      case 1:
        // This would be triggered after we set autosize attribute.
        embedder.test.assertEq(50, e.oldWidth);
        embedder.test.assertEq(100, e.oldHeight);
        embedder.test.assertTrue(e.newWidth >= 60 && e.newWidth <= 70);
        embedder.test.assertTrue(e.newHeight >= 110 && e.newHeight <= 120);

        // Remove autosize attribute and expect webview to retain the same size.
        autosizeWidth = e.newWidth;
        autosizeHeight = e.newHeight;
        webview.removeAttribute('autosize');
        break;
      case 2:
        // Expect the autosized size.
        embedder.test.assertEq(autosizeWidth, e.newWidth);
        embedder.test.assertEq(autosizeHeight, e.newHeight);

        embedder.test.succeed();
        break;
      default:
        window.console.log('Unexpected sizechanged event, step = ' + step);
        embedder.test.fail();
        break;
    }

    ++step;
  };

  webview.addEventListener('sizechanged', sizeChangeHandler);

  webview.addEventListener('loadstop', function(e) {
    webview.setAttribute('autosize', true);
    webview.setAttribute('minwidth', 60);
    webview.setAttribute('maxwidth', 70);
    webview.setAttribute('minheight', 110);
    webview.setAttribute('maxheight', 120);
  });

  webview.style.width = '50px';
  webview.style.height = '100px';
  webview.setAttribute('src', 'data:text/html,webview test sizechanged event');
  document.body.appendChild(webview);
}

// This test verifies that autosize works when some of the parameters are unset.
function testAutosizeWithPartialAttributes() {
  window.console.log('testAutosizeWithPartialAttributes');
  var webview = document.createElement('webview');

  var step = 1;
  var sizeChangeHandler = function(e) {
    window.console.log('sizeChangeHandler, new: ' +
                       e.newWidth + ' X ' + e.newHeight);
    switch (step) {
      case 1:
        // Expect 300x200.
        embedder.test.assertEq(300, e.newWidth);
        embedder.test.assertEq(200, e.newHeight);

        // Change the min size to cause a relayout.
        webview.minwidth = 500;
        break;
      case 2:
        embedder.test.assertTrue(e.newWidth >= webview.minwidth);
        embedder.test.assertTrue(e.newWidth <= webview.maxwidth);

        // Tests when minwidth > maxwidth, minwidth = maxwidth.
        // i.e. minwidth is essentially 700.
        webview.minwidth = 800;
        break;
      case 3:
        // Expect 700X?
        embedder.test.assertEq(700, e.newWidth);
        embedder.test.assertTrue(e.newHeight >= 200);
        embedder.test.assertTrue(e.newHeight <= 600);

        embedder.test.succeed();
        break;
      default:
        window.console.log('Unexpected sizechanged event, step = ' + step);
        embedder.test.fail();
        break;
    }

    ++step;
  };

  webview.addEventListener('sizechanged', sizeChangeHandler);

  webview.addEventListener('loadstop', function(e) {
    webview.minwidth = 300;
    webview.maxwidth = 700;
    webview.minheight = 200;
    webview.maxheight = 600;
    webview.autosize = true;
  });

  webview.style.width = '640px';
  webview.style.height = '480px';
  webview.setAttribute('src', 'data:text/html,webview check autosize');
  document.body.appendChild(webview);
}

// This test verifies that all autosize attributes can be removed
// without crashing the plugin, or throwing errors.
function testAutosizeRemoveAttributes() {
  var webview = document.createElement('webview');

  var step = 1;
  var sizeChangeHandler = function(e) {
    switch (step) {
      case 1:
        // This is the sizechanged event for autosize.

        // Remove attributes.
        webview.removeAttribute('minwidth');
        webview.removeAttribute('maxwidth');
        webview.removeAttribute('minheight');
        webview.removeAttribute('maxheight');
        webview.removeAttribute('autosize');

        // We'd get one more sizechanged event after we turn off
        // autosize.
        webview.style.width = '500px';
        webview.style.height = '500px';
        break;
      case 2:
        embedder.test.succeed();
        break;
    }

    ++step;
  };

  webview.addEventListener('loadstop', function(e) {
    webview.minwidth = 300;
    webview.maxwidth = 700;
    webview.minheight = 600;
    webview.maxheight = 400;
    webview.autosize = true;
  });

  webview.addEventListener('sizechanged', sizeChangeHandler);

  webview.style.width = '640px';
  webview.style.height = '480px';
  webview.setAttribute('src', 'data:text/html,webview check autosize');
  document.body.appendChild(webview);
}

function testAPIMethodExistence() {
  var apiMethodsToCheck = [
    'back',
    'canGoBack',
    'canGoForward',
    'forward',
    'getProcessId',
    'go',
    'reload',
    'stop',
    'terminate'
  ];
  var webview = document.createElement('webview');
  webview.setAttribute('partition', arguments.callee.name);
  webview.addEventListener('loadstop', function(e) {
    for (var i = 0; i < apiMethodsToCheck.length; ++i) {
      embedder.test.assertEq('function',
                           typeof webview[apiMethodsToCheck[i]]);
    }

    // Check contentWindow.
    embedder.test.assertEq('object', typeof webview.contentWindow);
    embedder.test.assertEq('function',
                         typeof webview.contentWindow.postMessage);
    embedder.test.succeed();
  });
  webview.setAttribute('src', 'data:text/html,webview check api');
  document.body.appendChild(webview);
}

// This test verifies that the loadstop event fires when loading a webview
// accessible resource from a partition that is privileged.
function testChromeExtensionURL() {
  var localResource = chrome.runtime.getURL('guest_with_inline_script.html');
  var webview = document.createElement('webview');
  // foobar is a privileged partition according to the manifest file.
  webview.partition = 'foobar';
  webview.addEventListener('loadabort', function(e) {
    embedder.test.fail();
  });
  webview.addEventListener('loadstop', function(e) {
    embedder.test.succeed();
  });
  webview.setAttribute('src', localResource);
  document.body.appendChild(webview);
}

// This test verifies that the loadstop event fires when loading a webview
// accessible resource from a partition that is privileged if the src URL
// is not fully qualified.
function testChromeExtensionRelativePath() {
  var webview = document.createElement('webview');
  // foobar is a privileged partition according to the manifest file.
  webview.partition = 'foobar';
  webview.addEventListener('loadabort', function(e) {
    embedder.test.fail();
  });
  webview.addEventListener('loadstop', function(e) {
    embedder.test.succeed();
  });
  webview.setAttribute('src', 'guest_with_inline_script.html');
  document.body.appendChild(webview);
}

// Tests that a <webview> that starts with "display: none" style loads
// properly.
function testDisplayNoneWebviewLoad() {
  var webview = document.createElement('webview');
  var visible = false;
  webview.style.display = 'none';
  // foobar is a privileged partition according to the manifest file.
  webview.partition = 'foobar';
  webview.addEventListener('loadabort', function(e) {
    embedder.test.fail();
  });
  webview.addEventListener('loadstop', function(e) {
    embedder.test.assertTrue(visible);
    embedder.test.succeed();
  });
  // Set the .src while we are "display: none".
  webview.setAttribute('src', 'about:blank');
  document.body.appendChild(webview);

  setTimeout(function() {
    visible = true;
    // This should trigger loadstop.
    webview.style.display = '';
  }, 0);
}

function testDisplayNoneWebviewRemoveChild() {
  var webview = document.createElement('webview');
  var visibleAndInDOM = false;
  webview.style.display = 'none';
  // foobar is a privileged partition according to the manifest file.
  webview.partition = 'foobar';
  webview.addEventListener('loadabort', function(e) {
    embedder.test.fail();
  });
  webview.addEventListener('loadstop', function(e) {
    embedder.test.assertTrue(visibleAndInDOM);
    embedder.test.succeed();
  });
  // Set the .src while we are "display: none".
  webview.setAttribute('src', 'about:blank');
  document.body.appendChild(webview);

  setTimeout(function() {
    webview.parentNode.removeChild(webview);
    webview.style.display = '';
    visibleAndInDOM = true;
    // This should trigger loadstop.
    document.body.appendChild(webview);
  }, 0);
}

// Makes sure inline scripts works inside guest that was loaded from
// accessible_resources.
function testInlineScriptFromAccessibleResources() {
  var webview = document.createElement('webview');
  // foobar is a privileged partition according to the manifest file.
  webview.partition = 'foobar';
  webview.addEventListener('loadabort', function(e) {
    embedder.test.fail();
  });
  webview.addEventListener('consolemessage', function(e) {
    window.console.log('consolemessage: ' + e.message);
    if (e.message == 'guest_with_inline_script.html: Inline script ran') {
      embedder.test.succeed();
    }
  });
  webview.setAttribute('src', 'guest_with_inline_script.html');
  document.body.appendChild(webview);
}

// This tests verifies that webview fires a loadabort event instead of crashing
// the browser if we attempt to navigate to a chrome-extension: URL with an
// extension ID that does not exist.
function testInvalidChromeExtensionURL() {
  var invalidResource = 'chrome-extension://abc123/guest.html';
  var webview = document.createElement('webview');
  // foobar is a privileged partition according to the manifest file.
  webview.partition = 'foobar';
  webview.addEventListener('loadabort', function(e) {
    embedder.test.succeed();
  });
  webview.setAttribute('src', invalidResource);
  document.body.appendChild(webview);
}

function testWebRequestAPIExistence() {
  var apiPropertiesToCheck = [
    // Declarative WebRequest API.
    'onMessage',
    'onRequest',
    // WebRequest API.
    'onBeforeRequest',
    'onBeforeSendHeaders',
    'onSendHeaders',
    'onHeadersReceived',
    'onAuthRequired',
    'onBeforeRedirect',
    'onResponseStarted',
    'onCompleted',
    'onErrorOccurred'
  ];
  var webview = document.createElement('webview');
  webview.setAttribute('partition', arguments.callee.name);
  webview.addEventListener('loadstop', function(e) {
    for (var i = 0; i < apiPropertiesToCheck.length; ++i) {
      embedder.test.assertEq('object',
                             typeof webview.request[apiPropertiesToCheck[i]]);
      embedder.test.assertEq(
          'function',
          typeof webview.request[apiPropertiesToCheck[i]].addListener);
      embedder.test.assertEq(
          'function',
          typeof webview.request[apiPropertiesToCheck[i]].addRules);
      embedder.test.assertEq(
          'function',
          typeof webview.request[apiPropertiesToCheck[i]].getRules);
      embedder.test.assertEq(
          'function',
          typeof webview.request[apiPropertiesToCheck[i]].removeRules);
    }

    // Try to overwrite webview.request, shall not succeed.
    webview.request = '123';
    embedder.test.assertTrue(typeof webview.request !== 'string');

    embedder.test.succeed();
  });
  webview.setAttribute('src', 'data:text/html,webview check api');
  document.body.appendChild(webview);
}

// This test verifies that the loadstart, loadstop, and exit events fire as
// expected.
function testEventName() {
  var webview = document.createElement('webview');
  webview.setAttribute('partition', arguments.callee.name);

  webview.addEventListener('loadstart', function(evt) {
    embedder.test.assertEq('loadstart', evt.type);
  });

  webview.addEventListener('loadstop', function(evt) {
    embedder.test.assertEq('loadstop', evt.type);
    webview.terminate();
  });

  webview.addEventListener('exit', function(evt) {
    embedder.test.assertEq('exit', evt.type);
    embedder.test.succeed();
  });

  webview.setAttribute('src', 'data:text/html,trigger navigation');
  document.body.appendChild(webview);
}

function testOnEventProperties() {
  var sequence = ['first', 'second', 'third', 'fourth'];
  var webview = document.createElement('webview');
  function createHandler(id) {
    return function(e) {
      embedder.test.assertEq(id, sequence.shift());
    };
  }

  webview.addEventListener('loadstart', createHandler('first'));
  webview.addEventListener('loadstart', createHandler('second'));
  webview.onloadstart = createHandler('third');
  webview.addEventListener('loadstart', createHandler('fourth'));
  webview.addEventListener('loadstop', function(evt) {
    embedder.test.assertEq(0, sequence.length);

    // Test that setting another 'onloadstart' handler replaces the previous
    // handler.
    sequence = ['first', 'second', 'fourth'];
    webview.onloadstart = function() {
      embedder.test.assertEq(0, sequence.length);
      embedder.test.succeed();
    };

    webview.setAttribute('src', 'data:text/html,next navigation');
  });

  webview.setAttribute('src', 'data:text/html,trigger navigation');
  document.body.appendChild(webview);
}

// Tests that the 'loadprogress' event is triggered correctly.
function testLoadProgressEvent() {
  var webview = document.createElement('webview');
  var progress = 0;

  webview.addEventListener('loadstop', function(evt) {
    embedder.test.assertEq(1, progress);
    embedder.test.succeed();
  });

  webview.addEventListener('loadprogress', function(evt) {
    progress = evt.progress;
  });

  webview.setAttribute('src', 'data:text/html,trigger navigation');
  document.body.appendChild(webview);
}

// This test registers two listeners on an event (loadcommit) and removes
// the <webview> tag when the first listener fires.
// Current expected behavior is that the second event listener will still
// fire without crashing.
function testDestroyOnEventListener() {
  var webview = document.createElement('webview');
  var url = 'data:text/html,<body>Destroy test</body>';

  var loadCommitCount = 0;
  function loadCommitCommon(e) {
    embedder.test.assertEq('loadcommit', e.type);
    if (url != e.url)
      return;
    ++loadCommitCount;
    if (loadCommitCount == 2) {
      // Pass in a timeout so that we can catch if any additional loadcommit
      // occurs.
      setTimeout(function() {
        embedder.test.succeed();
      }, 0);
    } else if (loadCommitCount > 2) {
      embedder.test.fail();
    }
  };

  // The test starts from here, by setting the src to |url|.
  webview.addEventListener('loadcommit', function(e) {
    window.console.log('loadcommit1');
    webview.parentNode.removeChild(webview);
    loadCommitCommon(e);
  });
  webview.addEventListener('loadcommit', function(e) {
    window.console.log('loadcommit2');
    loadCommitCommon(e);
  });
  webview.setAttribute('src', url);
  document.body.appendChild(webview);
}

// This test registers two event listeners on a same event (loadcommit).
// Each of the listener tries to change some properties on the event param,
// which should not be possible.
function testCannotMutateEventName() {
  var webview = document.createElement('webview');
  var url = 'data:text/html,<body>Two</body>';

  var loadCommitACalled = false;
  var loadCommitBCalled = false;

  var maybeFinishTest = function(e) {
    if (loadCommitACalled && loadCommitBCalled) {
      embedder.test.assertEq('loadcommit', e.type);
      embedder.test.succeed();
    }
  };

  var onLoadCommitA = function(e) {
    if (e.url == url) {
      embedder.test.assertEq('loadcommit', e.type);
      embedder.test.assertTrue(e.isTopLevel);
      embedder.test.assertFalse(loadCommitACalled);
      loadCommitACalled = true;
      // Try mucking with properities inside |e|.
      e.type = 'modified';
      maybeFinishTest(e);
    }
  };
  var onLoadCommitB = function(e) {
    if (e.url == url) {
      embedder.test.assertEq('loadcommit', e.type);
      embedder.test.assertTrue(e.isTopLevel);
      embedder.test.assertFalse(loadCommitBCalled);
      loadCommitBCalled = true;
      // Try mucking with properities inside |e|.
      e.type = 'modified';
      maybeFinishTest(e);
    }
  };

  // The test starts from here, by setting the src to |url|. Event
  // listener registration works because we already have a (dummy) src set
  // on the <webview> tag.
  webview.addEventListener('loadcommit', onLoadCommitA);
  webview.addEventListener('loadcommit', onLoadCommitB);
  webview.setAttribute('src', url);
  document.body.appendChild(webview);
}

// This test verifies that the partion attribute cannot be changed after the src
// has been set.
function testPartitionChangeAfterNavigation() {
  var webview = document.createElement('webview');
  var partitionAttribute = arguments.callee.name;
  webview.setAttribute('partition', partitionAttribute);

  var loadstopHandler = function(e) {
    webview.partition = 'illegal';
    embedder.test.assertEq(partitionAttribute, webview.partition);
    embedder.test.succeed();
  };
  webview.addEventListener('loadstop', loadstopHandler);

  document.body.appendChild(webview);
  webview.setAttribute('src', 'data:text/html,trigger navigation');
}

// This test verifies that removing partition attribute after navigation does
// not work, i.e. the partition remains the same.
function testPartitionRemovalAfterNavigationFails() {
  var webview = document.createElement('webview');
  document.body.appendChild(webview);

  var partition = 'testme';
  webview.setAttribute('partition', partition);

  var loadstopHandler = function(e) {
    window.console.log('webview.loadstop');
    // Removing after navigation should not change the partition.
    webview.removeAttribute('partition');
    embedder.test.assertEq('testme', webview.partition);
    embedder.test.succeed();
  };
  webview.addEventListener('loadstop', loadstopHandler);

  webview.setAttribute('src', 'data:text/html,<html><body>guest</body></html>');
}

// This test verifies that a content script will be injected to the webview when
// the webview is navigated to a page that matches the URL pattern defined in
// the content sript.
function testAddContentScript() {
  var webview = document.createElement('webview');

  console.log('Step 1: call <webview>.addContentScripts.');
  webview.addContentScripts(
      [{"name": 'myrule',
        "matches": ['http://*/extensions/*'],
        "js": ['inject_comm_channel.js'],
        "run_at": 'document_start'}]);

  webview.addEventListener('loadstop', function() {
    var msg = [request_to_comm_channel_1];
    webview.contentWindow.postMessage(JSON.stringify(msg), '*');
  });

  window.addEventListener('message', function(e) {
    var data = JSON.parse(e.data);
    if (data == response_from_comm_channel_1) {
      console.log(
          'Step 2: A communication channel has been established with webview.');
      embedder.test.succeed();
      return;
    }
    console.log('Unexpected message: \'' + data[0]  + '\'');
    embedder.test.fail();
  });

  webview.src = embedder.emptyGuestURL;
  document.body.appendChild(webview);
}

// Adds two content scripts with the same URL pattern to <webview> at the same
// time. This test verifies that both scripts are injected when the <webview>
// navigates to a URL that matches the URL pattern.
function testAddMultipleContentScripts() {
  var webview = document.createElement('webview');

  console.log('Step 1: call <webview>.addContentScripts(myrule1 & myrule2)');
  webview.addContentScripts(
      [{"name": 'myrule1',
        "matches": ['http://*/extensions/*'],
        "js": ['inject_comm_channel.js'],
        "run_at": 'document_start'},
       {"name": 'myrule2',
        "matches": ['http://*/extensions/*'],
        "js": ['inject_comm_channel_2.js'],
        "run_at": 'document_start'}]);

  webview.addEventListener('loadstop', function() {
    var msg1 = [request_to_comm_channel_1];
    webview.contentWindow.postMessage(JSON.stringify(msg1), '*');
    var msg2 = [request_to_comm_channel_2];
    webview.contentWindow.postMessage(JSON.stringify(msg2), '*');
  });

  var response_1 = false;
  var response_2 = false;
  window.addEventListener('message', function(e) {
    var data = JSON.parse(e.data);
    if (data == response_from_comm_channel_1) {
      console.log(
          'Step 2: A communication channel has been established with webview.');
      response_1 = true;
      if (response_1 && response_2)
        embedder.test.succeed();
      return;
    } else if (data == response_from_comm_channel_2) {
      console.log(
          'Step 3: A communication channel has been established with webview.');
      response_2 = true;
      if (response_1 && response_2)
        embedder.test.succeed();
      return;
    }
    console.log('Unexpected message: \'' + data[0]  + '\'');
    embedder.test.fail();
  });

  webview.src = embedder.emptyGuestURL;
  document.body.appendChild(webview);
}

// Adds a content script to <webview> and navigates. After seeing the script is
// injected, we add another content script with the same name to the <webview>.
// This test verifies that the second script will replace the first one and be
// injected after navigating the <webview>. Meanwhile, the <webview> shouldn't
// get any message from the first script anymore.
function testAddContentScriptWithSameNameShouldOverwriteTheExistingOne() {
  var webview = document.createElement('webview');

  console.log('Step 1: call <webview>.addContentScripts(myrule1)');
  webview.addContentScripts(
      [{"name": 'myrule1',
        "matches": ['http://*/extensions/*'],
        "js": ['inject_comm_channel.js'],
        "run_at": 'document_start'}]);
  var connect_script_1 = true;
  var connect_script_2 = false;

  webview.addEventListener('loadstop', function() {
    if (connect_script_1) {
      var msg1 = [request_to_comm_channel_1];
      webview.contentWindow.postMessage(JSON.stringify(msg1), '*');
      connect_script_1 = false;
    }
    if (connect_script_2) {
      var msg2 = [request_to_comm_channel_2];
      webview.contentWindow.postMessage(JSON.stringify(msg2), '*');
      connect_script_2 = false;
    }
  });

  var should_get_response_from_script_1 = true;
  window.addEventListener('message', function(e) {
    var data = JSON.parse(e.data);
    if (data == response_from_comm_channel_1) {
      if (should_get_response_from_script_1) {
        console.log(
            'Step 2: A communication channel has been established with webview.'
            );
        webview.addContentScripts(
            [{"name": 'myrule1',
              "matches": ['http://*/extensions/*'],
              "js": ['inject_comm_channel_2.js'],
              "run_at": 'document_start'}]);
        connect_script_2 = true;
        should_get_response_from_script_1 = false;
        webview.src = embedder.emptyGuestURL;
      } else {
        embedder.test.fail();
      }
      return;
    } else if (data == response_from_comm_channel_2) {
      console.log(
          'Step 3: Another communication channel has been established ' +
          'with webview.');
      setTimeout(function() {
        embedder.test.succeed();
      }, 0);
      return;
    }
    console.log('Unexpected message: \'' + data[0]  + '\'');
    embedder.test.fail();
  });

  webview.src = embedder.emptyGuestURL;
  document.body.appendChild(webview);
}

// There are two <webview>s are added to the DOM, and we add a content script
// to one of them. This test verifies that the script won't be injected in
// the other <webview>.
function testAddContentScriptToOneWebViewShouldNotInjectToTheOtherWebView() {
  var webview1 = document.createElement('webview');
  var webview2 = document.createElement('webview');

  console.log('Step 1: call <webview1>.addContentScripts.');
  webview1.addContentScripts(
      [{"name": 'myrule',
        "matches": ['http://*/extensions/*'],
        "js": ['inject_comm_channel.js'],
        "run_at": 'document_start'}]);

  webview2.addEventListener('loadstop', function() {
    console.log('Step 2: webview2 requests to build communication channel.');
    var msg = [request_to_comm_channel_1];
    webview2.contentWindow.postMessage(JSON.stringify(msg), '*');
    setTimeout(function() {
      embedder.test.succeed();
    }, 0);
  });

  window.addEventListener('message', function(e) {
    var data = JSON.parse(e.data);
    if (data == response_from_comm_channel_1) {
      embedder.test.fail();
      return;
    }
    console.log('Unexpected message: \'' + data[0]  + '\'');
    embedder.test.fail();
  });

  webview1.src = embedder.emptyGuestURL;
  webview2.src = embedder.emptyGuestURL;
  document.body.appendChild(webview1);
  document.body.appendChild(webview2);
}


// Adds a content script to <webview> and navigates to a URL that matches the
// URL pattern defined in the script. After the first navigation, we remove this
// script from the <webview> and navigates to the same URL. This test verifies
// taht the script is injected during the first navigation, but isn't injected
// after removing it.
function testAddAndRemoveContentScripts() {
  var webview = document.createElement('webview');

  console.log('Step 1: call <webview>.addContentScripts.');
  webview.addContentScripts(
      [{"name": 'myrule',
        "matches": ['http://*/extensions/*'],
        "js": ['inject_comm_channel.js'],
        "run_at": 'document_start'}]);

  var count = 0;
  webview.addEventListener('loadstop', function() {
    if (count == 0) {
      console.log('Step 2: post message to build connect.');
      var msg = [request_to_comm_channel_1];
      webview.contentWindow.postMessage(JSON.stringify(msg), '*');
      ++count;
    } else if (count == 1) {
      console.log(
          'Step 4: call <webview>.removeContentScripts and navigate.');
      webview.removeContentScripts();
      webview.src = embedder.emptyGuestURL;
      ++count;
    } else if (count == 2) {
      console.log('Step 5: post message to build connect again.');
      var msg = [request_to_comm_channel_1];
      webview.contentWindow.postMessage(JSON.stringify(msg), '*');
      setTimeout(function() {
        embedder.test.succeed();
      }, 0);
    }
  });

  var replyCount = 0;
  window.addEventListener('message', function(e) {
    var data = JSON.parse(e.data);
    if (data[0] == response_from_comm_channel_1) {
      console.log(
          'Step 3: A communication channel has been established with webview.');
      if (replyCount == 0) {
        webview.setAttribute('src', 'about:blank');
        ++replyCount;
        return;
      } else if (replyCount == 1) {
        embedder.test.fail();
        return;
      }
    }
    console.log('Unexpected message: \'' + data[0]  + '\'');
    embedder.test.fail();
  });

  webview.src = embedder.emptyGuestURL;
  document.body.appendChild(webview);
}

// This test verifies that the addContentScripts API works with the new window
// API.
function testAddContentScriptsWithNewWindowAPI() {
  var webview = document.createElement('webview');

  var newwebview;
  webview.addEventListener('newwindow', function(e) {
    e.preventDefault();
    newwebview = document.createElement('webview');

    console.log('Step 2: call newwebview.addContentScripts.');
    newwebview.addContentScripts(
        [{"name": 'myrule',
          "matches": ['http://*/extensions/*'],
          "js": ['inject_comm_channel.js'],
          "run_at": 'document_start'}]);

    newwebview.addEventListener('loadstop', function(evt) {
      var msg = [request_to_comm_channel_1];
      console.log('Step 4: new webview postmessage to build communication ' +
          'channel.');
      newwebview.contentWindow.postMessage(JSON.stringify(msg), '*');
    });

    document.body.appendChild(newwebview);
    // attach the new window to the new <webview>.
    console.log('Step 3: attaches the new webview.');
    e.window.attach(newwebview);
  });

  window.addEventListener('message', function(e) {
    var data = JSON.parse(e.data);
    if (data == response_from_comm_channel_1 &&
        e.source == newwebview.contentWindow) {
      console.log('Step 5: a communication channel has been established ' +
          'with the new webview.');
      embedder.test.succeed();
      return;
    } else {
      embedder.test.fail();
      return;
    }
    console.log('unexpected message: \'' + data[0]  + '\'');
    embedder.test.fail();
  });

  console.log('Step 1: navigates the webview to window open guest URL.');
  webview.setAttribute('src', embedder.windowOpenGuestFromSameURL);
  document.body.appendChild(webview);
}

// Adds a content script to <webview>. This test verifies that the script is
// injected after terminate and reload <webview>.
function testContentScriptIsInjectedAfterTerminateAndReloadWebView() {
  var webview = document.createElement('webview');

  console.log('Step 1: call <webview>.addContentScripts.');
  webview.addContentScripts(
      [{"name": 'myrule',
        "matches": ['http://*/extensions/*'],
        "js": ['inject_comm_channel.js'],
        "run_at": 'document_start'}]);

  var count = 0;
  webview.addEventListener('loadstop', function() {
    if (count == 0) {
      console.log('Step 2: call webview.terminate().');
      webview.terminate();
      ++count;
      return;
    } else if (count == 1) {
      console.log('Step 4: postMessage to build communication.');
      var msg = [request_to_comm_channel_1];
      webview.contentWindow.postMessage(JSON.stringify(msg), '*');
      ++count;
    }
  });

  webview.addEventListener('exit', function() {
    console.log('Step 3: call webview.reload().');
    webview.reload();
  });

  window.addEventListener('message', function(e) {
    var data = JSON.parse(e.data);
    if (data == response_from_comm_channel_1) {
      console.log(
          'Step 5: A communication channel has been established with webview.');
      embedder.test.succeed();
      return;
    }
    console.log('Unexpected message: \'' + data[0]  + '\'');
    embedder.test.fail();
  });

  webview.src = embedder.emptyGuestURL;
  document.body.appendChild(webview);
}

// This test verifies the content script won't be removed when the guest is
// destroyed, i.e., removed <webview> from the DOM.
function testContentScriptExistsAsLongAsWebViewTagExists() {
  var webview = document.createElement('webview');

  console.log('Step 1: call <webview>.addContentScripts.');
  webview.addContentScripts(
      [{"name": 'myrule',
        "matches": ['http://*/extensions/*'],
        "js": ['simple_script.js'],
        "run_at": 'document_end'}]);

  var count = 0;
  webview.addEventListener('loadstop', function() {
    if (count == 0) {
       console.log('Step 2: check the result of content script injected.');
      webview.executeScript({
        code: 'document.body.style.backgroundColor;'
      }, function(results) {
        embedder.test.assertEq(1, results.length);
        embedder.test.assertEq('red', results[0]);

        console.log('Step 3: remove webview from the DOM.');
        document.body.removeChild(webview);

        console.log('Step 4: add webview back to the DOM.');
        document.body.appendChild(webview);
        ++count;
      });
    } else if (count == 1) {
      webview.executeScript({
        code: 'document.body.style.backgroundColor;'
      }, function(results) {
        console.log('Step 5: check the result of content script injected' +
            ' again.');
        embedder.test.assertEq(1, results.length);
        embedder.test.assertEq('red', results[0]);
        embedder.test.succeed();
      });
    }
  });

  webview.src = embedder.emptyGuestURL;
  document.body.appendChild(webview);
}

function testAddContentScriptWithCode() {
  var webview = document.createElement('webview');

  console.log('Step 1: call <webview>.addContentScripts.');
  webview.addContentScripts(
      [{"name": 'myrule',
        "matches": ['http://*/extensions/*'],
        "code": 'document.body.style.backgroundColor = \'red\';',
        "run_at": 'document_end'}]);

  webview.addEventListener('loadstop', function() {
    console.log('Step 2: call webview.executeScript() to check result.')
    webview.executeScript({
      code: 'document.body.style.backgroundColor;'},
      function(results) {
        embedder.test.assertEq(1, results.length);
        embedder.test.assertEq('red', results[0]);
        embedder.test.succeed();
    });
  });

  webview.src = embedder.emptyGuestURL;
  document.body.appendChild(webview);
}

function testExecuteScriptFail() {
  var webview = document.createElement('webview');
  document.body.appendChild(webview);
  setTimeout(function() {
    webview.executeScript(
        {code:'document.body.style.backgroundColor = "red";'},
        function(results) {
          embedder.test.fail();
        });
    setTimeout(function() {
      embedder.test.succeed();
    }, 0);
  }, 0);
}

function testExecuteScript() {
  var webview = document.createElement('webview');
  webview.setAttribute('partition', arguments.callee.name);
  webview.addEventListener('loadstop', function() {
    webview.executeScript(
      {code:'document.body.style.backgroundColor = "red";'},
      function(results) {
        embedder.test.assertEq(1, results.length);
        embedder.test.assertEq('red', results[0]);
        embedder.test.succeed();
      });
  });
  webview.setAttribute('src', 'data:text/html,trigger navigation');
  document.body.appendChild(webview);
}

// This test verifies that the call to executeScript will fail and return null
// if the webview has been navigated between the time the call was made and the
// time it arrives in the guest process.
function testExecuteScriptIsAbortedWhenWebViewSourceIsChanged() {
  var webview = document.createElement('webview');
  webview.addEventListener('loadstop', function onLoadStop(e) {
    window.console.log('2. Inject script to trigger a guest-initiated ' +
        'navigation.');
    var navUrl = 'data:text/html,trigger nav';
    webview.executeScript({
      code: 'window.location.href = "' + navUrl + '";'
    });

    window.console.log('3. Listening for the load that will be started as a ' +
        'result of 2.');
    webview.addEventListener('loadstart', function onLoadStart(e) {
      embedder.test.assertEq('about:blank', webview.src);
      window.console.log('4. Attempting to inject script into about:blank. ' +
          'This is expected to fail.');
      webview.executeScript(
        { code: 'document.body.style.backgroundColor = "red";' },
        function(results) {
          window.console.log(
              '5. Verify that executeScript has, indeed, failed.');
          embedder.test.assertEq(null, results);
          embedder.test.assertEq(navUrl, webview.src);
          embedder.test.succeed();
        }
      );
      webview.removeEventListener('loadstart', onLoadStart);
    });
    webview.removeEventListener('loadstop', onLoadStop);
  });

  window.console.log('1. Performing initial navigation.');
  webview.setAttribute('src', 'about:blank');
  document.body.appendChild(webview);
}

// This test calls terminate() on guest after it has already been
// terminated. This makes sure we ignore the call gracefully.
function testTerminateAfterExit() {
  var webview = document.createElement('webview');
  webview.setAttribute('partition', arguments.callee.name);
  var loadstopSucceedsTest = false;
  webview.addEventListener('loadstop', function(evt) {
    embedder.test.assertEq('loadstop', evt.type);
    if (loadstopSucceedsTest) {
      embedder.test.succeed();
      return;
    }

    webview.terminate();
  });

  webview.addEventListener('exit', function(evt) {
    embedder.test.assertEq('exit', evt.type);
    // Call terminate again.
    webview.terminate();
    // Load another page. The test would pass when loadstop is called on
    // this second page. This would hopefully catch if call to
    // webview.terminate() caused a browser crash.
    setTimeout(function() {
      loadstopSucceedsTest = true;
      webview.setAttribute('src', 'data:text/html,test second page');
    }, 0);
  });

  webview.setAttribute('src', 'data:text/html,test terminate() crash.');
  document.body.appendChild(webview);
}

// This test verifies that multiple consecutive changes to the <webview> src
// attribute will cause a navigation.
function testNavOnConsecutiveSrcAttributeChanges() {
  var testPage1 = 'data:text/html,test page 1';
  var testPage2 = 'data:text/html,test page 2';
  var testPage3 = 'data:text/html,test page 3';
  var webview = new WebView();
  webview.partition = arguments.callee.name;
  var loadCommitCount = 0;
  webview.addEventListener('loadcommit', function(e) {
    if (e.url == testPage3) {
      embedder.test.succeed();
    }
    loadCommitCount++;
    if (loadCommitCount > 3) {
      embedder.test.fail();
    }
  });
  document.body.appendChild(webview);
  webview.src = testPage1;
  webview.src = testPage2;
  webview.src = testPage3;
}

// This test verifies that we can set the <webview> src multiple times and the
// changes will cause a navigation.
function testNavOnSrcAttributeChange() {
  var testPage1 = 'data:text/html,test page 1';
  var testPage2 = 'data:text/html,test page 2';
  var testPage3 = 'data:text/html,test page 3';
  var tests = [testPage1, testPage2, testPage3];
  var webview = new WebView();
  webview.partition = arguments.callee.name;
  var loadCommitCount = 0;
  webview.addEventListener('loadcommit', function(evt) {
    var success = tests.indexOf(evt.url) > -1;
    embedder.test.assertTrue(success);
    ++loadCommitCount;
    if (loadCommitCount == tests.length) {
      embedder.test.succeed();
    } else if (loadCommitCount > tests.length) {
      embedder.test.fail();
    } else {
      webview.src = tests[loadCommitCount];
    }
  });
  webview.src = tests[0];
  document.body.appendChild(webview);
}

// This test verifies that assigning the src attribute the same value it had
// prior to a crash spawns off a new guest process.
function testAssignSrcAfterCrash() {
  var webview = document.createElement('webview');
  webview.setAttribute('partition', arguments.callee.name);
  var terminated = false;
  webview.addEventListener('loadstop', function(evt) {
    if (!terminated) {
      webview.terminate();
      return;
    }
    // The guest has recovered after being terminated.
    embedder.test.succeed();
  });
  webview.addEventListener('exit', function(evt) {
    terminated = true;
    webview.setAttribute('src', 'data:text/html,test page');
  });
  webview.setAttribute('src', 'data:text/html,test page');
  document.body.appendChild(webview);
}

// This test verifies that <webview> reloads the page if the src attribute is
// assigned the same value.
function testReassignSrcAttribute() {
  var dataUrl = 'data:text/html,test page';
  var webview = new WebView();
  webview.partition = arguments.callee.name;

  var loadStopCount = 0;
  webview.addEventListener('loadstop', function(evt) {
    embedder.test.assertEq(dataUrl, webview.getAttribute('src'));
    ++loadStopCount;
    console.log('[' + loadStopCount + '] loadstop called');
    if (loadStopCount == 3) {
      embedder.test.succeed();
    } else if (loadStopCount > 3) {
      embedder.test.fail();
    } else {
      webview.src = dataUrl;
    }
  });
  webview.src = dataUrl;
  document.body.appendChild(webview);
}

// This test verifies that <webview> restores the src attribute if it is
// removed after navigation.
function testRemoveSrcAttribute() {
  var dataUrl = 'data:text/html,test page';
  var webview = document.createElement('webview');
  webview.setAttribute('partition', arguments.callee.name);
  var terminated = false;
  webview.addEventListener('loadstop', function(evt) {
    webview.removeAttribute('src');
    setTimeout(function() {
      embedder.test.assertEq(dataUrl, webview.getAttribute('src'));
      embedder.test.succeed();
    }, 0);
  });
  webview.setAttribute('src', dataUrl);
  document.body.appendChild(webview);
}

function testPluginLoadInternalResource() {
  var first = document.createElement('webview');
  first.addEventListener('loadabort', function(e) {
    var second = document.createElement('webview');
    second.addEventListener('permissionrequest', function(e) {
      e.preventDefault();
      embedder.test.assertEq('loadplugin', e.permission);
      embedder.test.succeed();
    });
    e.preventDefault();
    second.partition = 'foobar';
    second.setAttribute('src', 'test.pdf');
    document.body.appendChild(second);
  });
  first.setAttribute('src', 'test.pdf');
  document.body.appendChild(first);
}

function testPluginLoadPermission() {
  var pluginIdentifier = 'unknown platform';
  if (navigator.platform.match(/linux/i))
    pluginIdentifier = 'libppapi_tests.so';
  else if (navigator.platform.match(/win32/i))
    pluginIdentifier = 'ppapi_tests.dll';
  else if (navigator.platform.match(/win64/i))
    pluginIdentifier = 'ppapi_tests.dll';
  else if (navigator.platform.match(/mac/i))
    pluginIdentifier = 'ppapi_tests.plugin';

  var webview = document.createElement('webview');
  webview.addEventListener('permissionrequest', function(e) {
    e.preventDefault();
    embedder.test.assertEq('loadplugin', e.permission);
    embedder.test.assertEq(pluginIdentifier, e.name);
    embedder.test.assertEq(pluginIdentifier, e.identifier);
    embedder.test.assertEq('function', typeof e.request.allow);
    embedder.test.assertEq('function', typeof e.request.deny);
    embedder.test.succeed();
  });
  webview.setAttribute('src', 'data:text/html,<body>' +
                              '<embed type="application/x-ppapi-tests">' +
                              '</embed></body>');
  document.body.appendChild(webview);
}

// This test verifies that new window attachment functions as expected.
function testNewWindow() {
  var webview = document.createElement('webview');
  webview.addEventListener('newwindow', function(e) {
    e.preventDefault();
    var newwebview = document.createElement('webview');
    newwebview.addEventListener('loadstop', function(evt) {
      // If the new window finishes loading, the test is successful.
      embedder.test.succeed();
    });
    document.body.appendChild(newwebview);
    // Attach the new window to the new <webview>.
    e.window.attach(newwebview);
  });
  webview.setAttribute('src', embedder.windowOpenGuestURL);
  document.body.appendChild(webview);
}

// This test verifies "first-call-wins" semantics. That is, the first call
// to perform an action on the new window takes the action and all
// subsequent calls throw an exception.
function testNewWindowTwoListeners() {
  var webview = document.createElement('webview');
  var error = false;
  webview.addEventListener('newwindow', function(e) {
    e.preventDefault();
    var newwebview = document.createElement('webview');
    document.body.appendChild(newwebview);
    try {
      e.window.attach(newwebview);
    } catch (err) {
      embedder.test.fail();
    }
  });
  webview.addEventListener('newwindow', function(e) {
    e.preventDefault();
    try {
      e.window.discard();
    } catch (err) {
      embedder.test.succeed();
    }
  });
  webview.setAttribute('src', embedder.windowOpenGuestURL);
  document.body.appendChild(webview);
}

// This test verifies that the attach can be called inline without
// preventing default.
function testNewWindowNoPreventDefault() {
  var webview = document.createElement('webview');
  webview.addEventListener('newwindow', function(e) {
    var newwebview = document.createElement('webview');
    document.body.appendChild(newwebview);
    // Attach the new window to the new <webview>.
    try {
      e.window.attach(newwebview);
      embedder.test.succeed();
    } catch (err) {
      embedder.test.fail();
    }
  });
  webview.setAttribute('src', embedder.windowOpenGuestURL);
  document.body.appendChild(webview);
}

function testNewWindowNoReferrerLink() {
  var webview = document.createElement('webview');
  webview.addEventListener('newwindow', function(e) {
    e.preventDefault();
    var newwebview = document.createElement('webview');
    newwebview.addEventListener('loadstop', function(evt) {
      // If the new window finishes loading, the test is successful.
      embedder.test.succeed();
    });
    document.body.appendChild(newwebview);
    // Attach the new window to the new <webview>.
    e.window.attach(newwebview);
  });
  webview.setAttribute('src', embedder.noReferrerGuestURL);
  document.body.appendChild(webview);
}

// This test verifies that the load event fires when the a new page is
// loaded.
// TODO(fsamuel): Add a test to verify that subframe loads within a guest
// do not fire the 'contentload' event.
function testContentLoadEvent() {
  var webview = document.createElement('webview');
  webview.addEventListener('contentload', function(e) {
    embedder.test.succeed();
  });
  webview.setAttribute('src', 'data:text/html,trigger navigation');
  document.body.appendChild(webview);
}

// This test verifies that the load event fires when the a new page is
// loaded even if the <webview> is set to display:none.
function testContentLoadEventWithDisplayNone() {
  var webview = document.createElement('webview');
  webview.style.display = 'none';
  webview.addEventListener('contentload', function(e) {
    embedder.test.succeed();
  });
  webview.setAttribute('src', 'data:text/html,trigger navigation');
  document.body.appendChild(webview);
}

// This test verifies that the WebRequest API onBeforeRequest event fires on
// webview.
function testWebRequestAPI() {
  var webview = new WebView();
  webview.request.onBeforeRequest.addListener(function(e) {
    embedder.test.succeed();
  }, { urls: ['<all_urls>']}) ;
  webview.src = embedder.windowOpenGuestURL;
  document.body.appendChild(webview);
}

// This test verifies that the WebRequest API onBeforeSendHeaders event fires on
// webview and supports headers. This tests verifies that we can modify HTTP
// headers via the WebRequest API and those modified headers will be sent to the
// HTTP server.
function testWebRequestAPIWithHeaders() {
  var webview = new WebView();
  var requestFilter = {
    urls: ['<all_urls>']
  };
  var extraInfoSpec = ['requestHeaders', 'blocking'];
  webview.request.onBeforeSendHeaders.addListener(function(details) {
    var headers = details.requestHeaders;
    for( var i = 0, l = headers.length; i < l; ++i ) {
      if (headers[i].name == 'User-Agent') {
        headers[i].value = 'foobar';
        break;
      }
    }
    var blockingResponse = {};
    blockingResponse.requestHeaders = headers;
    return blockingResponse;
  }, requestFilter, extraInfoSpec);

  var loadstartCalled = false;
  webview.addEventListener('loadstart', function(e) {
    embedder.test.assertTrue(e.isTopLevel);
    embedder.test.assertEq(embedder.detectUserAgentURL, e.url);
    loadstartCalled = true;
  });

  webview.addEventListener('loadredirect', function(e) {
    embedder.test.assertTrue(e.isTopLevel);
    embedder.test.assertEq(embedder.detectUserAgentURL,
        e.oldUrl.replace('127.0.0.1', 'localhost'));
    embedder.test.assertEq(embedder.redirectGuestURLDest,
        e.newUrl.replace('127.0.0.1', 'localhost'));
    if (loadstartCalled) {
      embedder.test.succeed();
    } else {
      embedder.test.fail();
    }
  });
  webview.src = embedder.detectUserAgentURL;
  document.body.appendChild(webview);
}

// This test verifies that the basic use cases of the declarative WebRequest API
// work as expected. This test demonstrates that rules can be added prior to
// navigation and attachment.
// 1. It adds a rule to block URLs that contain guest.
// 2. It attempts to navigate to a guest.html page.
// 3. It detects the appropriate loadabort message.
// 4. It removes the rule blocking the page and reloads.
// 5. The page loads successfully.
function testDeclarativeWebRequestAPI() {
  var step = 1;
  var webview = new WebView();
  var rule = {
    conditions: [
      new chrome.webViewRequest.RequestMatcher(
        {
          url: { urlContains: 'guest' }
        }
      )
    ],
    actions: [
      new chrome.webViewRequest.CancelRequest()
    ]
  };
  webview.request.onRequest.addRules([rule]);
  webview.addEventListener('loadabort', function(e) {
    embedder.test.assertEq(1, step);
    embedder.test.assertEq('ERR_BLOCKED_BY_CLIENT', e.reason);
    step = 2;
    webview.request.onRequest.removeRules();
    webview.reload();
  });
  webview.addEventListener('loadstop', function(e) {
    embedder.test.assertEq(2, step);
    embedder.test.succeed();
  });
  webview.src = embedder.emptyGuestURL;
  document.body.appendChild(webview);
}

function testDeclarativeWebRequestAPISendMessage() {
  var webview = new WebView();
  window.console.log(embedder.emptyGuestURL);
  var rule = {
    conditions: [
      new chrome.webViewRequest.RequestMatcher(
        {
          url: { urlContains: 'guest' }
        }
      )
    ],
    actions: [
      new chrome.webViewRequest.SendMessageToExtension({ message: 'bleep' })
    ]
  };
  webview.request.onRequest.addRules([rule]);
  webview.request.onMessage.addListener(function(e) {
    embedder.test.assertEq('bleep', e.message);
    embedder.test.succeed();
  });
  webview.src = embedder.emptyGuestURL;
  document.body.appendChild(webview);
}

// This test verifies that setting a <webview>'s style.display = 'block' does
// not throw and attach error.
function testDisplayBlock() {
  var webview = new WebView();
  webview.onloadstop = function(e) {
    LOG('webview.onloadstop');
    window.console.error = function() {
      // If we see an error, that means attach failed.
      embedder.test.fail();
    };
    webview.style.display = 'block';
    embedder.test.assertTrue(webview.getProcessId() > 0);

    webview.onloadstop = function(e) {
      LOG('Second webview.onloadstop');
      embedder.test.succeed();
    };
    webview.src = 'data:text/html,<body>Second load</body>';
  }
  webview.src = 'about:blank';
  document.body.appendChild(webview);
}

// This test verifies that the WebRequest API onBeforeRequest event fires on
// clients*.google.com URLs.
function testWebRequestAPIGoogleProperty() {
  var webview = new WebView();
  webview.request.onBeforeRequest.addListener(function(e) {
    embedder.test.succeed();
    return {cancel: true};
  }, { urls: ['<all_urls>']}, ['blocking']) ;
  webview.src = 'http://clients6.google.com';
  document.body.appendChild(webview);
}

// This test verifies that the WebRequest event listener for onBeforeRequest
// survives reparenting of the <webview>.
function testWebRequestListenerSurvivesReparenting() {
  var webview = new WebView();
  var count = 0;
  webview.request.onBeforeRequest.addListener(function(e) {
    if (++count == 2) {
      embedder.test.succeed();
    }
  }, { urls: ['<all_urls>']});
  var onLoadStop =  function(e) {
    webview.removeEventListener('loadstop', onLoadStop);
    webview.parentNode.removeChild(webview);
    var container = document.getElementById('object-container');
    if (!container) {
      embedder.test.fail('Container for object not found.');
      return;
    }
    container.appendChild(webview);
  };
  webview.addEventListener('loadstop', onLoadStop);
  webview.src = embedder.emptyGuestURL;
  document.body.appendChild(webview);
}

// This test verifies that getProcessId is defined and returns a non-zero
// value corresponding to the processId of the guest process.
function testGetProcessId() {
  var webview = document.createElement('webview');
  webview.setAttribute('src', 'data:text/html,trigger navigation');
  var firstLoad = function() {
    webview.removeEventListener('loadstop', firstLoad);
    embedder.test.assertTrue(webview.getProcessId() > 0);
    embedder.test.succeed();
  };
  webview.addEventListener('loadstop', firstLoad);
  document.body.appendChild(webview);
}

function testHiddenBeforeNavigation() {
  var webview = document.createElement('webview');
  webview.style.visibility = 'hidden';

  var postMessageHandler = function(e) {
    var data = JSON.parse(e.data);
    window.removeEventListener('message', postMessageHandler);
    if (data[0] == 'visibilityState-response') {
      embedder.test.assertEq('hidden', data[1]);
      embedder.test.succeed();
    } else {
      LOG('Unexpected message: ' + data);
      embedder.test.fail();
    }
  };

  webview.addEventListener('loadstop', function(e) {
    LOG('webview.loadstop');
    window.addEventListener('message', postMessageHandler);
    webview.addEventListener('consolemessage', function(e) {
      LOG('g: ' + e.message);
    });

    webview.executeScript(
      {file: 'inject_hidden_test.js'},
      function(results) {
        if (!results || !results.length) {
          LOG('Failed to inject script: inject_hidden_test.js');
          embedder.test.fail();
          return;
        }

        LOG('script injection success');
        webview.contentWindow.postMessage(
            JSON.stringify(['visibilityState-request']), '*');
      });
  });

  webview.setAttribute('src', 'data:text/html,<html><body></body></html>');
  document.body.appendChild(webview);
}

// This test verifies that the loadstart event fires at the beginning of a load
// and the loadredirect event fires when a redirect occurs.
function testLoadStartLoadRedirect() {
  var webview = document.createElement('webview');
  var loadstartCalled = false;
  webview.setAttribute('src', embedder.redirectGuestURL);
  webview.addEventListener('loadstart', function(e) {
    embedder.test.assertTrue(e.isTopLevel);
    embedder.test.assertEq(embedder.redirectGuestURL, e.url);
    loadstartCalled = true;
  });
  webview.addEventListener('loadredirect', function(e) {
    embedder.test.assertTrue(e.isTopLevel);
    embedder.test.assertEq(embedder.redirectGuestURL,
        e.oldUrl.replace('127.0.0.1', 'localhost'));
    embedder.test.assertEq(embedder.redirectGuestURLDest,
        e.newUrl.replace('127.0.0.1', 'localhost'));
    if (loadstartCalled) {
      embedder.test.succeed();
    } else {
      embedder.test.fail();
    }
  });
  document.body.appendChild(webview);
}

// This test verifies that the loadabort event fires when loading a webview
// accessible resource from a partition that is not privileged.
function testLoadAbortChromeExtensionURLWrongPartition() {
  var localResource = chrome.runtime.getURL('guest.html');
  var webview = document.createElement('webview');
  webview.addEventListener('loadabort', function(e) {
    embedder.test.assertEq(-109, e.code);
    embedder.test.assertEq('ERR_ADDRESS_UNREACHABLE', e.reason);
    embedder.test.succeed();
  });
  webview.addEventListener('loadstop', function(e) {
    embedder.test.fail();
  });
  webview.setAttribute('src', localResource);
  document.body.appendChild(webview);
}

// This test verifies that the loadabort event fires as expected and with the
// appropriate fields when an empty response is returned.
function testLoadAbortEmptyResponse() {
  var webview = document.createElement('webview');
  webview.addEventListener('loadabort', function(e) {
    embedder.test.assertEq(-324, e.code);
    embedder.test.assertEq('ERR_EMPTY_RESPONSE', e.reason);
    embedder.test.succeed();
  });
  webview.setAttribute('src', embedder.closeSocketURL);
  document.body.appendChild(webview);
}

// This test verifies that the loadabort event fires as expected when an illegal
// chrome URL is provided.
function testLoadAbortIllegalChromeURL() {
  var webview = document.createElement('webview');
  webview.addEventListener('loadabort', function(e) {
    embedder.test.assertEq(-3, e.code);
    embedder.test.assertEq('ERR_ABORTED', e.reason);
  });
  webview.addEventListener('loadstop', function(e)  {
    embedder.test.assertEq('about:blank', webview.src);
    embedder.test.succeed();
  });
  webview.src = 'chrome://newtab';
  document.body.appendChild(webview);
}

function testLoadAbortIllegalFileURL() {
  var webview = document.createElement('webview');
  webview.addEventListener('loadabort', function(e) {
    embedder.test.assertEq(-3, e.code);
    embedder.test.assertEq('ERR_ABORTED', e.reason);
  });
  webview.addEventListener('loadstop', function(e) {
    embedder.test.assertEq('about:blank', webview.src);
    embedder.test.succeed();
  });
  webview.src = 'file://foo';
  document.body.appendChild(webview);
}

function testLoadAbortIllegalJavaScriptURL() {
  var webview = document.createElement('webview');
  webview.addEventListener('loadabort', function(e) {
    embedder.test.assertEq(-3, e.code);
    embedder.test.assertEq('ERR_ABORTED', e.reason);
  });
  webview.addEventListener('loadstop', function(e) {
    embedder.test.assertEq('about:blank', webview.src);
    embedder.test.succeed();
  });
  webview.setAttribute('src', 'javascript:void(document.bgColor="#0000FF")');
  document.body.appendChild(webview);
}

// Verifies that navigating to invalid URL (e.g. 'http:') doesn't cause a crash.
function testLoadAbortInvalidNavigation() {
  var webview = document.createElement('webview');
  webview.addEventListener('loadabort', function(e) {
    embedder.test.assertEq(-3, e.code);
    embedder.test.assertEq('ERR_ABORTED', e.reason);
    embedder.test.assertEq('', e.url);
  });
  webview.addEventListener('loadstop', function(e) {
    embedder.test.assertEq('about:blank', webview.src);
    embedder.test.succeed();
  });
  webview.addEventListener('exit', function(e) {
    // We should not crash.
    embedder.test.fail();
  });
  webview.src = 'http:';
  document.body.appendChild(webview);
}

// Verifies that navigation to a URL that is valid but not web-safe or
// pseudo-scheme fires loadabort and doesn't cause a crash.
function testLoadAbortNonWebSafeScheme() {
  var webview = document.createElement('webview');
  var chromeGuestURL = 'chrome-guest://abc123/';
  webview.addEventListener('loadabort', function(e) {
    embedder.test.assertEq(-3, e.code);
    embedder.test.assertEq('ERR_ABORTED', e.reason);
    embedder.test.assertEq(chromeGuestURL, e.url);
  });
  webview.addEventListener('loadstop', function(e) {
    embedder.test.assertEq('about:blank', webview.src);
    embedder.test.succeed();
  });
  webview.addEventListener('exit', function(e) {
    // We should not crash.
    embedder.test.fail();
  });
  webview.src = chromeGuestURL;
  document.body.appendChild(webview);
};

// This test verifies that the reload method on webview functions as expected.
function testReload() {
  var triggerNavUrl = 'data:text/html,trigger navigation';
  var webview = document.createElement('webview');

  var loadCommitCount = 0;
  webview.addEventListener('loadstop', function(e) {
    if (loadCommitCount < 2) {
      webview.reload();
    } else if (loadCommitCount == 2) {
      embedder.test.succeed();
    } else {
      embedder.test.fail();
    }
  });
  webview.addEventListener('loadcommit', function(e) {
    embedder.test.assertEq(triggerNavUrl, e.url);
    embedder.test.assertTrue(e.isTopLevel);
    loadCommitCount++;
  });

  webview.setAttribute('src', triggerNavUrl);
  document.body.appendChild(webview);
}

// This test verifies that the reload method on webview functions as expected.
function testReloadAfterTerminate() {
  var triggerNavUrl = 'data:text/html,trigger navigation';
  var webview = document.createElement('webview');

  var step = 1;
  webview.addEventListener('loadstop', function(e) {
    switch (step) {
      case 1:
        webview.terminate();
        break;
      case 2:
        setTimeout(function() { embedder.test.succeed(); }, 0);
        break;
      default:
        window.console.log('Unexpected loadstop event, step = ' + step);
        embedder.test.fail();
        break;
    }
    ++step;
  });

  webview.addEventListener('exit', function(e) {
    // Trigger a focus state change of the guest to test for
    // http://crbug.com/413874.
    webview.blur();
    webview.focus();
    setTimeout(function() { webview.reload(); }, 0);
  });

  webview.src = triggerNavUrl;
  document.body.appendChild(webview);
}

// This test verifies that a <webview> is torn down gracefully when removed from
// the DOM on exit.

window.removeWebviewOnExitDoCrash = null;

function testRemoveWebviewOnExit() {
  var triggerNavUrl = 'data:text/html,trigger navigation';
  var webview = document.createElement('webview');

  webview.addEventListener('loadstop', function(e) {
    chrome.test.sendMessage('guest-loaded');
  });

  window.removeWebviewOnExitDoCrash = function() {
    webview.terminate();
  };

  webview.addEventListener('exit', function(e) {
    // We expected to be killed.
    if (e.reason != 'killed') {
      console.log('EXPECTED TO BE KILLED!');
      return;
    }
    webview.parentNode.removeChild(webview);
  });

  // Trigger a navigation to create a guest process.
  webview.setAttribute('src', embedder.emptyGuestURL);
  document.body.appendChild(webview);
}

function testRemoveWebviewAfterNavigation() {
  var webview = new WebView();
  document.body.appendChild(webview);
  webview.src = 'data:text/html,trigger navigation';
  document.body.removeChild(webview);
  setTimeout(function() {
    embedder.test.succeed();
  }, 0);
}

function testNavigationToExternalProtocol() {
  var webview = document.createElement('webview');
  webview.addEventListener('loadstop', function(e) {
    webview.addEventListener('loadabort', function(e) {
      embedder.test.assertEq('ERR_UNKNOWN_URL_SCHEME', e.reason);
      embedder.test.succeed();
    });
    webview.executeScript({
      code: 'window.location.href = "tel:+12223334444";'
    }, function(results) {});
  });
  webview.setAttribute('src', 'data:text/html,navigate to external protocol');
  document.body.appendChild(webview);
}

// This test ensures if the guest isn't there and we resize the guest (from JS),
// it remembers the size correctly.
function testNavigateAfterResize() {
  var webview = new WebView();

  var postMessageHandler = function(e) {
    var data = JSON.parse(e.data);
    LOG('postMessageHandler: ' + data);
    webview.removeEventListener('message', postMessageHandler);
    if (data[0] == 'dimension-response') {
      var actualWidth = data[1];
      var actualHeight = data[2];
      LOG('actualWidth: ' + actualWidth + ', actualHeight: ' + actualHeight);
      embedder.test.assertEq(100, actualWidth);
      embedder.test.assertEq(125, actualHeight);
      embedder.test.succeed();
    }
  };
  window.addEventListener('message', postMessageHandler);

  webview.addEventListener('consolemessage', function(e) {
    LOG('guest log: ' + e.message);
  });

  webview.addEventListener('loadstop', function(e) {
    webview.executeScript(
      {file: 'navigate_after_resize.js'},
      function(results) {
        if (!results || !results.length) {
          LOG('Failed to inject navigate_after_resize.js');
          embedder.test.fail();
          return;
        }
        LOG('Inject success: navigate_after_resize.js');
        var msg = ['dimension-request'];
        webview.contentWindow.postMessage(JSON.stringify(msg), '*');
      });
  });

  // First set size.
  webview.style.width = '100px';
  webview.style.height = '125px';

  // Then navigate.
  webview.src = 'about:blank';
  document.body.appendChild(webview);
}

function testResizeWebviewResizesContent() {
  var webview = new WebView();
  webview.src = 'about:blank';
  webview.addEventListener('loadstop', function(e) {
    webview.executeScript(
      {file: 'inject_resize_test.js'},
      function(results) {
        window.console.log('The resize test has been injected into webview.');
      }
    );
    webview.executeScript(
      {file: 'inject_comm_channel.js'},
      function(results) {
        window.console.log('The guest script for a two-way comm channel has ' +
            'been injected into webview.');
        // Establish a communication channel with the guest.
        var msg = ['connect'];
        webview.contentWindow.postMessage(JSON.stringify(msg), '*');
      }
    );
  });
  window.addEventListener('message', function(e) {
    var data = JSON.parse(e.data);
    if (data[0] == 'connected') {
      console.log('A communication channel has been established with webview.');
      console.log('Resizing <webview> width from 300px to 400px.');
      webview.style.width = '400px';
      return;
    }
    if (data[0] == 'resize') {
      var width = data[1];
      var height = data[2];
      embedder.test.assertEq(400, width);
      embedder.test.assertEq(300, height);
      embedder.test.succeed();
      return;
    }
    console.log('Unexpected message: \'' + data[0]  + '\'');
    embedder.test.fail();
  });
  document.body.appendChild(webview);
}

function testResizeWebviewWithDisplayNoneResizesContent() {
  var webview = new WebView();
  webview.src = 'about:blank';
  var loadStopCalled = false;
  webview.addEventListener('loadstop', function listener(e) {
    if (loadStopCalled) {
      window.console.log('webview is unexpectedly reloading.');
      embedder.test.fail();
      return;
    }
    loadStopCalled = true;
    webview.executeScript(
      {file: 'inject_resize_test.js'},
      function(results) {
        if (!results || !results.length) {
          embedder.test.fail();
          return;
        }
        window.console.log('The resize test has been injected into webview.');
      }
    );
    webview.executeScript(
      {file: 'inject_comm_channel.js'},
      function(results) {
        if (!results || !results.length) {
          embedder.test.fail();
          return;
        }
        window.console.log('The guest script for a two-way comm channel has ' +
            'been injected into webview.');
        // Establish a communication channel with the guest.
        var msg = ['connect'];
        webview.contentWindow.postMessage(JSON.stringify(msg), '*');
      }
    );
  });
  window.addEventListener('message', function(e) {
    var data = JSON.parse(e.data);
    if (data[0] == 'connected') {
      console.log('A communication channel has been established with webview.');
      console.log('Resizing <webview> width from 300px to 400px.');
      webview.style.display = 'none';
      window.setTimeout(function() {
        webview.style.width = '400px';
        window.setTimeout(function() {
          webview.style.display = 'block';
        }, 10);
      }, 10);
      return;
    }
    if (data[0] == 'resize') {
      var width = data[1];
      var height = data[2];
      embedder.test.assertEq(400, width);
      embedder.test.assertEq(300, height);
      embedder.test.succeed();
      return;
    }
    window.console.log('Unexpected message: \'' + data[0]  + '\'');
    embedder.test.fail();
  });
  document.body.appendChild(webview);
}

function testPostMessageCommChannel() {
  var webview = new WebView();
  // Run this test with display:none to verify that postMessage works correctly.
  webview.style.display = 'none';
  webview.src = 'about:blank';
  webview.addEventListener('loadstop', function(e) {
    window.console.log('loadstop');
    webview.executeScript(
      {file: 'inject_comm_channel.js'},
      function(results) {
        window.console.log('The guest script for a two-way comm channel has ' +
            'been injected into webview.');
        // Establish a communication channel with the guest.
        var msg = ['connect'];
        webview.contentWindow.postMessage(JSON.stringify(msg), '*');
      }
    );
  });
  webview.addEventListener('consolemessage', function(e) {
    window.console.log('Guest: "' + e.message + '"');
  });
  window.addEventListener('message', function(e) {
    var data = JSON.parse(e.data);
    if (data[0] == 'connected') {
      console.log('A communication channel has been established with webview.');
      embedder.test.succeed();
      return;
    }
    console.log('Unexpected message: \'' + data[0]  + '\'');
    embedder.test.fail();
  });
  document.body.appendChild(webview);
}

function testScreenshotCapture() {
  var webview = document.createElement('webview');

  webview.addEventListener('loadstop', function(e) {
    webview.captureVisibleRegion(null, function(dataUrl) {
      // 100x100 red box.
      var expectedUrl = 'data:image/jpeg;base64,/9j/4AAQSkZJRgABAQAAAQABAAD/' +
          '2wBDAAMCAgMCAgMDAwMEAwMEBQgFBQQEBQoHBwYIDAoMDAsKCwsNDhIQDQ4RDgsLE' +
          'BYQERMUFRUVDA8XGBYUGBIUFRT/2wBDAQMEBAUEBQkFBQkUDQsNFBQUFBQUFBQUFB' +
          'QUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBT/wAARCABkAGQ' +
          'DASIAAhEBAxEB/8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAA' +
          'AgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM' +
          '2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3' +
          'R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8j' +
          'JytLT1NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAA' +
          'AAAAAAECAwQFBgcICQoL/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBU' +
          'QdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChYkNOEl8RcYGRomJygpKjU2Nzg5OkNERU' +
          'ZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOEhYaHiImKkpOUlZaXmJmaoqO' +
          'kpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09fb3' +
          '+Pn6/9oADAMBAAIRAxEAPwD50ooor8MP9UwooooAKKKKACiiigAooooAKKKKACiii' +
          'gAooooAKKKKACiiigAooooAKKKKACiiigAooooAKKKKACiiigAooooAKKKKACiiig' +
          'AooooAKKKKACiiigAooooAKKKKACiiigAooooAKKKKACiiigAooooAKKKKACiiigA' +
          'ooooAKKKKACiiigAooooAKKKKACiiigAooooAKKKKACiiigAooooAKKKKACiiigAo' +
          'oooAKKKKACiiigAooooAKKKKACiiigD/2Q==';
      embedder.test.assertEq(expectedUrl, dataUrl);
      embedder.test.succeed();
    });
  });

  webview.style.width = '100px';
  webview.style.height = '100px';
  webview.setAttribute('src',
      'data:text/html,<body style="background-color: red"></body>');
  document.body.appendChild(webview);
}

function testZoomAPI() {
  var webview = new WebView();
  webview.src = 'about:blank';
  webview.addEventListener('loadstop', function(e) {
    // getZoom() should work initially.
    webview.getZoom(function(zoomFactor) {
      embedder.test.assertEq(zoomFactor, 1);
    });

    // Two consecutive calls to getZoom() should return the same result.
    var zoomFactor1;
    webview.getZoom(function(zoomFactor) {
      zoomFactor1 = zoomFactor;
    });
    webview.getZoom(function(zoomFactor) {
      embedder.test.assertEq(zoomFactor1, zoomFactor);
    });

    // Test setZoom()'s callback.
    var callbackTest = false;
    webview.setZoom(0.95, function() {
      callbackTest = true;
    });

    // getZoom() should return the same zoom factor as is set in setZoom().
    webview.setZoom(1.53);
    webview.getZoom(function(zoomFactor) {
      embedder.test.assertEq(zoomFactor, 1.53);
    });
    webview.setZoom(0.835847);
    webview.getZoom(function(zoomFactor) {
      embedder.test.assertEq(zoomFactor, 0.835847);
    });
    webview.setZoom(0.3795);
    webview.getZoom(function(zoomFactor) {
      embedder.test.assertEq(zoomFactor, 0.3795);
    });

    // setZoom() should really zoom the page (thus changing window.innerWidth).
    webview.setZoom(0.45, function() {
      webview.executeScript({code: 'window.innerWidth'},
        function(result) {
          var width1 = result[0];
          webview.setZoom(1.836);
          webview.executeScript({code: 'window.innerWidth'},
            function(result) {
              var width2 = result[0];
              embedder.test.assertTrue(width2 < width1);
              webview.setZoom(0.73);
              webview.executeScript({code: 'window.innerWidth'},
                function(result) {
                  var width3 = result[0];
                  embedder.test.assertTrue(width3 < width1);
                  embedder.test.assertTrue(width2 < width3);

                  // Test the onzoomchange event.
                  webview.addEventListener('zoomchange', function(e) {
                    embedder.test.assertEq(event.oldZoomFactor, 0.73);
                    embedder.test.assertEq(event.newZoomFactor, 0.25325);

                    embedder.test.assertTrue(callbackTest);

                    embedder.test.succeed();
                  });
                  webview.setZoom(0.25325);
                }
              );
            }
          );
        }
      );
    });
  });
  document.body.appendChild(webview);
};

function testFindAPI() {
  var webview = new WebView();
  webview.src = 'data:text/html,Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br><br>' +
      '<a href="about:blank">Click here!</a>';

  var loadstopListener2 = function(e) {
    embedder.test.assertEq(webview.src, "about:blank");
    embedder.test.succeed();
  }

  var loadstopListener1 = function(e) {
    // Test find results.
    webview.find("dog", {}, function(results) {
      callbackTest = true;
      embedder.test.assertEq(results.numberOfMatches, 100);
      embedder.test.assertTrue(results.selectionRect.width > 0);
      embedder.test.assertTrue(results.selectionRect.height > 0);

      // Test finding next active matches.
      webview.find("dog");
      webview.find("dog");
      webview.find("dog");
      webview.find("dog");
      webview.find("dog", {}, function(results) {
        embedder.test.assertEq(results.activeMatchOrdinal, 6);
        webview.find("dog", {backward: true});
        webview.find("dog", {backward: true}, function(results) {
          // Test the |backward| find option.
          embedder.test.assertEq(results.activeMatchOrdinal, 4);

          // Test the |matchCase| find option.
          webview.find("Dog", {matchCase: true}, function(results) {
            embedder.test.assertEq(results.numberOfMatches, 40);

            // Test canceling find requests.
            webview.find("dog");
            webview.stopFinding();
            webview.find("dog");
            webview.find("cat");

            // Test find results when looking for something that isn't there.
            webview.find("fish", {}, function(results) {
              embedder.test.assertEq(results.numberOfMatches, 0);
              embedder.test.assertEq(results.activeMatchOrdinal, 0);
              embedder.test.assertEq(results.selectionRect.left, 0);
              embedder.test.assertEq(results.selectionRect.top, 0);
              embedder.test.assertEq(results.selectionRect.width, 0);
              embedder.test.assertEq(results.selectionRect.height, 0);

              // Test following a link with stopFinding().
              webview.removeEventListener('loadstop', loadstopListener1);
              webview.addEventListener('loadstop', loadstopListener2);
              webview.find("click here!", {}, function() {
                webview.stopFinding("activate");
              });
            });
          });
        });
      });
    });
  };

  webview.addEventListener('loadstop', loadstopListener1);
  document.body.appendChild(webview);
};

function testFindAPI_findupdate() {
  var webview = new WebView();
  webview.src = 'data:text/html,Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br>' +
      'Dog dog dog Dog dog dogcatDog dogDogdog.<br><br>' +
      '<a href="about:blank">Click here!</a>';
  var canceledTest = false;
  webview.addEventListener('loadstop', function(e) {
    // Test the |findupdate| event.
    webview.addEventListener('findupdate', function(e) {
      if (e.activeMatchOrdinal > 0) {
        // embedder.test.assertTrue(e.numberOfMatches >= e.activeMatchOrdinal)
        // This currently fails because of http://crbug.com/342445 .
        embedder.test.assertTrue(e.selectionRect.width > 0);
        embedder.test.assertTrue(e.selectionRect.height > 0);
      }

      if (e.finalUpdate) {
        if (e.canceled) {
          canceledTest = true;
        } else {
          embedder.test.assertEq(e.searchText, "dog");
          embedder.test.assertEq(e.numberOfMatches, 100);
          embedder.test.assertEq(e.activeMatchOrdinal, 1);
          embedder.test.assertTrue(canceledTest);
          embedder.test.succeed();
        }
      }
    });
    wv.find("dog");
    wv.find("cat");
    wv.find("dog");
  });

  document.body.appendChild(webview);
};

function testLoadDataAPI() {
  var webview = new WebView();
  webview.src = 'about:blank';

  var loadstopListener2 = function(e) {
    // Test the virtual URL.
    embedder.test.assertEq(webview.src, embedder.virtualURL);

    // Test that the image was loaded from the right source.
    webview.executeScript(
        {code: "document.querySelector('img').src"}, function(e) {
          embedder.test.assertEq(e, embedder.testImageBaseURL + "test.bmp");

          // Test that insertCSS works (executeScript already works to reach
          // this point).
          webview.insertCSS({code: ''}, function() {
            embedder.test.succeed();
          });
        });
  };

  var loadstopListener1 = function(e) {
    webview.removeEventListener('loadstop', loadstopListener1);
    webview.addEventListener('loadstop', loadstopListener2);

    // Load a data URL containing a relatively linked image, with the
    // image's base URL specified, and a virtual URL provided.
    webview.loadDataWithBaseUrl("data:text/html;base64,PGh0bWw+CiAgVGhpcyBpcy" +
        "BhIHRlc3QuPGJyPgogIDxpbWcgc3JjPSJ0ZXN0LmJtcCI+PGJyPgo8L2h0bWw+Cg==",
                                embedder.testImageBaseURL,
                                embedder.virtualURL);
  };

  webview.addEventListener('loadstop', loadstopListener1);
  document.body.appendChild(webview);
};

// Test that the resize events fire with the correct values, and in the
// correct order, when resizing occurs.
function testResizeEvents() {
  var webview = new WebView();
  webview.src = 'about:blank';
  webview.style.width = '600px';
  webview.style.height = '400px';

  var checkSizes = function(e) {
    embedder.test.assertEq(e.oldWidth, 600)
    embedder.test.assertEq(e.oldHeight, 400)
    embedder.test.assertEq(e.newWidth, 500)
    embedder.test.assertEq(e.newHeight, 400)
  }

  var resizeListener = function(e) {
    webview.onresize = null;
    webview.oncontentresize = contentResizeListener;

    console.log('onresize');
    checkSizes(e);
  };

  var contentResizeListener = function(e) {
    webview.oncontentresize = null;

    console.log('oncontentresize');
    checkSizes(e);
    embedder.test.succeed();
  };

  var loadstopListener = function(e) {
    webview.removeEventListener('loadstop', loadstopListener);
    webview.onresize = resizeListener;

    console.log('Resizing <webview> width from 600px to 500px.');
    webview.style.width = '500px';
  };

  webview.addEventListener('loadstop', loadstopListener);
  document.body.appendChild(webview);
};

function testPerOriginZoomMode() {
  var webview1 = new WebView();
  var webview2 = new WebView();
  webview1.src = 'about:blank';
  webview2.src = 'about:blank';

  webview1.addEventListener('loadstop', function(e) {
    document.body.appendChild(webview2);
  });
  webview2.addEventListener('loadstop', function(e) {
    webview1.getZoomMode(function(zoomMode) {
      // Check that |webview1| is in 'per-origin' mode and zoom it. Check that
      // both webviews zoomed.
      embedder.test.assertEq(zoomMode, 'per-origin');
      webview1.setZoom(3.14, function() {
        webview1.getZoom(function(zoom) {
          embedder.test.assertEq(zoom, 3.14);
          webview2.getZoom(function(zoom) {
            embedder.test.assertEq(zoom, 3.14);
            embedder.test.succeed();
          });
        });
      });
    });
  });

  document.body.appendChild(webview1);
}

function testPerViewZoomMode() {
  var webview1 = new WebView();
  var webview2 = new WebView();
  webview1.src = 'about:blank';
  webview2.src = 'about:blank';

  webview1.addEventListener('loadstop', function(e) {
    document.body.appendChild(webview2);
  });
  webview2.addEventListener('loadstop', function(e) {
    // Set |webview2| to 'per-view' mode and zoom it. Make sure that the
    // zoom did not affect |webview1|.
    webview2.setZoomMode('per-view', function() {
      webview2.getZoomMode(function(zoomMode) {
        embedder.test.assertEq(zoomMode, 'per-view');
        webview2.setZoom(0.45, function() {
          webview1.getZoom(function(zoom) {
            embedder.test.assertFalse(zoom == 0.45);
            webview2.getZoom(function(zoom) {
              embedder.test.assertEq(zoom, 0.45);
              embedder.test.succeed();
            });
          });
        });
      });
    });
  });

  document.body.appendChild(webview1);
}

function testDisabledZoomMode() {
  var webview = new WebView();
  webview.src = 'about:blank';

  var zoomchanged = false;
  var zoomchangeListener = function(e) {
    embedder.test.assertEq(e.newZoomFactor, 1);
    zoomchanged = true;
  };

  webview.addEventListener('loadstop', function(e) {
    // Set |webview| to 'disabled' mode and check that
    // zooming is actually disabled. Also check that the
    // "zoomchange" event pick up changes from changing the
    // zoom mode.
    webview.addEventListener('zoomchange', zoomchangeListener);
    webview.setZoomMode('disabled', function() {
      webview.getZoomMode(function(zoomMode) {
        embedder.test.assertEq(zoomMode, 'disabled');
        webview.removeEventListener('zoomchange', zoomchangeListener);
        webview.setZoom(1.39, function() {
          webview.getZoom(function(zoom) {
            embedder.test.assertEq(zoom, 1);
            embedder.test.assertTrue(zoomchanged);
            embedder.test.succeed();
          });
        });
      });
    });
  });

  document.body.appendChild(webview);
}

function testZoomBeforeNavigation() {
  var webview = new WebView();

  webview.addEventListener('loadstop', function(e) {
    // Check that the zoom state persisted.
    webview.getZoom(function(zoomFactor) {
      embedder.test.assertEq(zoomFactor, 3.14);
      embedder.test.succeed();
    });
  });

  // Set the zoom before the first navigation.
  webview.setZoom(3.14);

  webview.src = 'about:blank';
  document.body.appendChild(webview);
}

function testPlugin() {
  var webview = document.createElement('webview');
  webview.setAttribute('src', embedder.pluginURL);
  webview.addEventListener('loadstop', function(e) {
    // Not crashing means success.
    embedder.test.succeed();
  });
  document.body.appendChild(webview);
}

embedder.test.testList = {
  'testAllowTransparencyAttribute': testAllowTransparencyAttribute,
  'testAutosizeHeight': testAutosizeHeight,
  'testAutosizeAfterNavigation': testAutosizeAfterNavigation,
  'testAutosizeBeforeNavigation': testAutosizeBeforeNavigation,
  'testAutosizeRemoveAttributes': testAutosizeRemoveAttributes,
  'testAutosizeWithPartialAttributes': testAutosizeWithPartialAttributes,
  'testAPIMethodExistence': testAPIMethodExistence,
  'testChromeExtensionURL': testChromeExtensionURL,
  'testChromeExtensionRelativePath': testChromeExtensionRelativePath,
  'testDisplayNoneWebviewLoad': testDisplayNoneWebviewLoad,
  'testDisplayNoneWebviewRemoveChild': testDisplayNoneWebviewRemoveChild,
  'testInlineScriptFromAccessibleResources':
      testInlineScriptFromAccessibleResources,
  'testInvalidChromeExtensionURL': testInvalidChromeExtensionURL,
  'testWebRequestAPIExistence': testWebRequestAPIExistence,
  'testEventName': testEventName,
  'testOnEventProperties': testOnEventProperties,
  'testLoadProgressEvent': testLoadProgressEvent,
  'testDestroyOnEventListener': testDestroyOnEventListener,
  'testCannotMutateEventName': testCannotMutateEventName,
  'testPartitionChangeAfterNavigation': testPartitionChangeAfterNavigation,
  'testPartitionRemovalAfterNavigationFails':
      testPartitionRemovalAfterNavigationFails,
  'testAddContentScript': testAddContentScript,
  'testAddMultipleContentScripts': testAddMultipleContentScripts,
  'testAddContentScriptWithSameNameShouldOverwriteTheExistingOne':
      testAddContentScriptWithSameNameShouldOverwriteTheExistingOne,
  'testAddContentScriptToOneWebViewShouldNotInjectToTheOtherWebView':
      testAddContentScriptToOneWebViewShouldNotInjectToTheOtherWebView,
  'testAddAndRemoveContentScripts': testAddAndRemoveContentScripts,
  'testAddContentScriptsWithNewWindowAPI':
      testAddContentScriptsWithNewWindowAPI,
  'testContentScriptIsInjectedAfterTerminateAndReloadWebView':
      testContentScriptIsInjectedAfterTerminateAndReloadWebView,
  'testContentScriptExistsAsLongAsWebViewTagExists':
      testContentScriptExistsAsLongAsWebViewTagExists,
  'testAddContentScriptWithCode': testAddContentScriptWithCode,
  'testExecuteScriptFail': testExecuteScriptFail,
  'testExecuteScript': testExecuteScript,
  'testExecuteScriptIsAbortedWhenWebViewSourceIsChanged':
      testExecuteScriptIsAbortedWhenWebViewSourceIsChanged,
  'testTerminateAfterExit': testTerminateAfterExit,
  'testAssignSrcAfterCrash': testAssignSrcAfterCrash,
  'testNavOnConsecutiveSrcAttributeChanges':
      testNavOnConsecutiveSrcAttributeChanges,
  'testNavOnSrcAttributeChange': testNavOnSrcAttributeChange,
  'testReassignSrcAttribute': testReassignSrcAttribute,
  'testRemoveSrcAttribute': testRemoveSrcAttribute,
  'testPluginLoadInternalResource': testPluginLoadInternalResource,
  'testPluginLoadPermission': testPluginLoadPermission,
  'testNewWindow': testNewWindow,
  'testNewWindowTwoListeners': testNewWindowTwoListeners,
  'testNewWindowNoPreventDefault': testNewWindowNoPreventDefault,
  'testNewWindowNoReferrerLink': testNewWindowNoReferrerLink,
  'testContentLoadEvent': testContentLoadEvent,
  'testContentLoadEventWithDisplayNone': testContentLoadEventWithDisplayNone,
  'testDeclarativeWebRequestAPI': testDeclarativeWebRequestAPI,
  'testDeclarativeWebRequestAPISendMessage':
      testDeclarativeWebRequestAPISendMessage,
  'testDisplayBlock': testDisplayBlock,
  'testWebRequestAPI': testWebRequestAPI,
  'testWebRequestAPIWithHeaders': testWebRequestAPIWithHeaders,
  'testWebRequestAPIGoogleProperty': testWebRequestAPIGoogleProperty,
  'testWebRequestListenerSurvivesReparenting':
      testWebRequestListenerSurvivesReparenting,
  'testGetProcessId': testGetProcessId,
  'testHiddenBeforeNavigation': testHiddenBeforeNavigation,
  'testLoadStartLoadRedirect': testLoadStartLoadRedirect,
  'testLoadAbortChromeExtensionURLWrongPartition':
      testLoadAbortChromeExtensionURLWrongPartition,
  'testLoadAbortEmptyResponse': testLoadAbortEmptyResponse,
  'testLoadAbortIllegalChromeURL': testLoadAbortIllegalChromeURL,
  'testLoadAbortIllegalFileURL': testLoadAbortIllegalFileURL,
  'testLoadAbortIllegalJavaScriptURL': testLoadAbortIllegalJavaScriptURL,
  'testLoadAbortInvalidNavigation': testLoadAbortInvalidNavigation,
  'testLoadAbortNonWebSafeScheme': testLoadAbortNonWebSafeScheme,
  'testNavigateAfterResize': testNavigateAfterResize,
  'testNavigationToExternalProtocol': testNavigationToExternalProtocol,
  'testReload': testReload,
  'testReloadAfterTerminate': testReloadAfterTerminate,
  'testRemoveWebviewOnExit': testRemoveWebviewOnExit,
  'testRemoveWebviewAfterNavigation': testRemoveWebviewAfterNavigation,
  'testResizeWebviewResizesContent': testResizeWebviewResizesContent,
  'testResizeWebviewWithDisplayNoneResizesContent':
      testResizeWebviewWithDisplayNoneResizesContent,
  'testPostMessageCommChannel': testPostMessageCommChannel,
  'testScreenshotCapture' : testScreenshotCapture,
  'testZoomAPI' : testZoomAPI,
  'testFindAPI': testFindAPI,
  'testFindAPI_findupdate': testFindAPI,
  'testLoadDataAPI': testLoadDataAPI,
  'testResizeEvents': testResizeEvents,
  'testPerOriginZoomMode': testPerOriginZoomMode,
  'testPerViewZoomMode': testPerViewZoomMode,
  'testDisabledZoomMode': testDisabledZoomMode,
  'testZoomBeforeNavigation': testZoomBeforeNavigation,
  'testPlugin': testPlugin,
};

onload = function() {
  chrome.test.getConfig(function(config) {
    embedder.setUp_(config);
    chrome.test.sendMessage("Launched");
  });
};
