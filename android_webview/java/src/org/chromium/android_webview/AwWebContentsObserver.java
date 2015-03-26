// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import org.chromium.android_webview.AwContents.VisualStateCallback;
import org.chromium.base.ThreadUtils;
import org.chromium.content_public.browser.NavigationEntry;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContentsObserver;
import org.chromium.net.NetError;
import org.chromium.ui.base.PageTransition;

import java.lang.ref.WeakReference;

/**
 * Routes notifications from WebContents to AwContentsClient and other listeners.
 */
public class AwWebContentsObserver extends WebContentsObserver {
    // TODO(tobiasjs) similarly to WebContentsObserver.mWebContents, mAwContents
    // needs to be a WeakReference, which suggests that there exists a strong
    // reference to an AwWebContentsObserver instance. This is not intentional,
    // and should be found and cleaned up.
    private final WeakReference<AwContents> mAwContents;
    private final WeakReference<AwContentsClient> mAwContentsClient;
    private boolean mStartedNonApiProvisionalLoadInMainFrame = false;

    public AwWebContentsObserver(
            WebContents webContents, AwContents awContents, AwContentsClient awContentsClient) {
        super(webContents);
        mAwContents = new WeakReference<>(awContents);
        mAwContentsClient = new WeakReference<>(awContentsClient);
    }

    boolean hasStartedNonApiProvisionalLoadInMainFrame() {
        return mStartedNonApiProvisionalLoadInMainFrame;
    }

    @Override
    public void didFinishLoad(long frameId, String validatedUrl, boolean isMainFrame) {
        AwContentsClient client = mAwContentsClient.get();
        if (client == null) return;
        String unreachableWebDataUrl = AwContentsStatics.getUnreachableWebDataUrl();
        boolean isErrorUrl =
                unreachableWebDataUrl != null && unreachableWebDataUrl.equals(validatedUrl);
        if (isMainFrame && !isErrorUrl) {
            client.onPageFinished(validatedUrl);
        }
    }

    @Override
    public void didFailLoad(boolean isProvisionalLoad,
            boolean isMainFrame, int errorCode, String description, String failingUrl) {
        AwContentsClient client = mAwContentsClient.get();
        if (client == null) return;
        String unreachableWebDataUrl = AwContentsStatics.getUnreachableWebDataUrl();
        boolean isErrorUrl =
                unreachableWebDataUrl != null && unreachableWebDataUrl.equals(failingUrl);
        if (isMainFrame && !isErrorUrl && errorCode == NetError.ERR_ABORTED) {
            // Need to call onPageFinished for backwards compatibility with the classic webview.
            // See also AwContents.IoThreadClientImpl.onReceivedError.
            client.onPageFinished(failingUrl);
        }
    }

    @Override
    public void didNavigateMainFrame(final String url, String baseUrl,
            boolean isNavigationToDifferentPage, boolean isFragmentNavigation, int statusCode) {
        // Only invoke the onPageCommitVisible callback when navigating to a different page,
        // but not when navigating to a different fragment within the same page.
        if (isNavigationToDifferentPage) {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    AwContents awContents = mAwContents.get();
                    if (awContents != null) {
                        awContents.insertVisualStateCallback(0, new VisualStateCallback() {
                            @Override
                            public void onComplete(long requestId) {
                                AwContentsClient client = mAwContentsClient.get();
                                if (client == null) return;
                                client.onPageCommitVisible(url);
                            }
                        });
                    }
                }
            });
        }

        // This is here to emulate the Classic WebView firing onPageFinished for main frame
        // navigations where only the hash fragment changes.
        if (isFragmentNavigation) {
            AwContentsClient client = mAwContentsClient.get();
            if (client == null) return;
            client.onPageFinished(url);
        }
    }

    @Override
    public void didNavigateAnyFrame(String url, String baseUrl, boolean isReload) {
        final AwContentsClient client = mAwContentsClient.get();
        if (client == null) return;
        client.doUpdateVisitedHistory(url, isReload);
    }

    @Override
    public void didStartProvisionalLoadForFrame(
            long frameId,
            long parentFrameId,
            boolean isMainFrame,
            String validatedUrl,
            boolean isErrorPage,
            boolean isIframeSrcdoc) {
        if (!isMainFrame) return;
        AwContents awContents = mAwContents.get();
        if (awContents != null) {
            NavigationEntry pendingEntry = awContents.getNavigationController().getPendingEntry();
            if (pendingEntry != null
                    && (pendingEntry.getTransition() & PageTransition.FROM_API) == 0) {
                mStartedNonApiProvisionalLoadInMainFrame = true;
            }
        }
    }
}
