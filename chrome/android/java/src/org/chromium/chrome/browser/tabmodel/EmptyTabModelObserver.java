// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tabmodel;

import org.chromium.chrome.browser.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModel.TabSelectionType;

/**
 * An empty base implementation of the TabModelObserver interface.
 */
public class EmptyTabModelObserver implements TabModelObserver {

    @Override
    public void didSelectTab(Tab tab, TabSelectionType type, int lastId) {
    }

    @Override
    public void willCloseTab(Tab tab, boolean animate) {
    }

    @Override
    public void didCloseTab(Tab tab) {
    }

    @Override
    public void willAddTab(Tab tab, TabLaunchType type) {
    }

    @Override
    public void didAddTab(Tab tab, TabLaunchType type) {
    }

    @Override
    public void didMoveTab(Tab tab, int newIndex, int curIndex) {
    }

    @Override
    public void tabPendingClosure(Tab tab) {
    }

    @Override
    public void tabClosureUndone(Tab tab) {
    }

    @Override
    public void tabClosureCommitted(Tab tab) {
    }

}
