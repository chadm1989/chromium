// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var expectedDevices = [{
  'vendor': 'Vendor 1',
  'model': 'Model 1',
  'capacity': 1 << 20,
  'storageUnitId': '/test/id/1',
  }, {
  'vendor': 'Vendor 2',
  'model': 'Model 2',
  'capacity': 1 << 22,
  'storageUnitId': '/test/id/2',
  }];


function testDeviceList() {
  chrome.imageWriterPrivate.listRemovableStorageDevices(
          chrome.test.callback(listRemovableDevicesCallback));
}

function listRemovableDevicesCallback(deviceList) {
  deviceList.sort(function (a, b) {
    if (a.storageUnitId > b.storageUnitId) return 1;
    if (a.storageUnitId < b.storageUnitId) return -1;
    return 0;
  });

  chrome.test.assertEq(2, deviceList.length);

  deviceList.forEach(function (dev, i) {
    var expected = expectedDevices[i];
    chrome.test.assertEq(expected.vendor, dev.vendor);
    chrome.test.assertEq(expected.model, dev.model);
    chrome.test.assertEq(expected.capacity, dev.capacity);
    chrome.test.assertEq(expected.storageUnitId, dev.storageUnitId);
  });
}

chrome.test.runTests([testDeviceList])
