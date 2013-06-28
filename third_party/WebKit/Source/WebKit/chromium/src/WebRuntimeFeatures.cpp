/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebRuntimeFeatures.h"

#include "RuntimeEnabledFeatures.h"
#include "WebMediaPlayerClientImpl.h"
#include "modules/websockets/WebSocket.h"

using namespace WebCore;

namespace WebKit {

void WebRuntimeFeatures::enableStableFeatures(bool enable)
{
    RuntimeEnabledFeatures::setStableFeaturesEnabled(enable);
    // FIXME: enableMediaPlayer does not use RuntimeEnabledFeatures
    // and does not belong as part of WebRuntimeFeatures.
    enableMediaPlayer(enable);
}

void WebRuntimeFeatures::enableExperimentalFeatures(bool enable)
{
    RuntimeEnabledFeatures::setExperimentalFeaturesEnabled(enable);
}

void WebRuntimeFeatures::enableTestOnlyFeatures(bool enable)
{
    RuntimeEnabledFeatures::setTestFeaturesEnabled(enable);
}

void WebRuntimeFeatures::enableApplicationCache(bool enable)
{
    RuntimeEnabledFeatures::setApplicationCacheEnabled(enable);
}

bool WebRuntimeFeatures::isApplicationCacheEnabled()
{
    return RuntimeEnabledFeatures::applicationCacheEnabled();
}

void WebRuntimeFeatures::enableCanvasPath(bool enable)
{
    RuntimeEnabledFeatures::setCanvasPathEnabled(enable);
}

bool WebRuntimeFeatures::isCanvasPathEnabled()
{
    return RuntimeEnabledFeatures::canvasPathEnabled();
}

void WebRuntimeFeatures::enableCSSCompositing(bool enable)
{
    RuntimeEnabledFeatures::setCSSCompositingEnabled(enable);
}

bool WebRuntimeFeatures::isCSSCompositingEnabled()
{
    return RuntimeEnabledFeatures::cssCompositingEnabled();
}

void WebRuntimeFeatures::enableCSSExclusions(bool enable)
{
    RuntimeEnabledFeatures::setCSSExclusionsEnabled(enable);
}

bool WebRuntimeFeatures::isCSSExclusionsEnabled()
{
    return RuntimeEnabledFeatures::cssExclusionsEnabled();
}

void WebRuntimeFeatures::enableCSSGridLayout(bool enable)
{
    RuntimeEnabledFeatures::setCSSGridLayoutEnabled(enable);
}

bool WebRuntimeFeatures::isCSSGridLayoutEnabled()
{
    return RuntimeEnabledFeatures::cssGridLayoutEnabled();
}

void WebRuntimeFeatures::enableCSSRegions(bool enable)
{
    RuntimeEnabledFeatures::setCSSRegionsEnabled(enable);
}

bool WebRuntimeFeatures::isCSSRegionsEnabled()
{
    return RuntimeEnabledFeatures::cssRegionsEnabled();
}

void WebRuntimeFeatures::enableCSSTouchAction(bool enable)
{
    RuntimeEnabledFeatures::setCSSTouchActionEnabled(enable);
}

bool WebRuntimeFeatures::isCSSTouchActionEnabled()
{
    return RuntimeEnabledFeatures::cssTouchActionEnabled();
}

void WebRuntimeFeatures::enableCustomDOMElements(bool enable)
{
    RuntimeEnabledFeatures::setCustomDOMElementsEnabled(enable);
}

bool WebRuntimeFeatures::isCustomDOMElementsEnabled()
{
    return RuntimeEnabledFeatures::customDOMElementsEnabled();
}

void WebRuntimeFeatures::enableDatabase(bool enable)
{
    RuntimeEnabledFeatures::setDatabaseEnabled(enable);
}

bool WebRuntimeFeatures::isDatabaseEnabled()
{
    return RuntimeEnabledFeatures::databaseEnabled();
}

void WebRuntimeFeatures::enableDeviceMotion(bool enable)
{
    RuntimeEnabledFeatures::setDeviceMotionEnabled(enable);
}

bool WebRuntimeFeatures::isDeviceMotionEnabled()
{
    return RuntimeEnabledFeatures::deviceMotionEnabled();
}

void WebRuntimeFeatures::enableDeviceOrientation(bool enable)
{
    RuntimeEnabledFeatures::setDeviceOrientationEnabled(enable);
}

bool WebRuntimeFeatures::isDeviceOrientationEnabled()
{
    return RuntimeEnabledFeatures::deviceOrientationEnabled();
}

void WebRuntimeFeatures::enableDialogElement(bool enable)
{
    RuntimeEnabledFeatures::setDialogElementEnabled(enable);
}

bool WebRuntimeFeatures::isDialogElementEnabled()
{
    return RuntimeEnabledFeatures::dialogElementEnabled();
}

void WebRuntimeFeatures::enableDirectoryUpload(bool enable)
{
    RuntimeEnabledFeatures::setDirectoryUploadEnabled(enable);
}

bool WebRuntimeFeatures::isDirectoryUploadEnabled()
{
    return RuntimeEnabledFeatures::directoryUploadEnabled();
}

void WebRuntimeFeatures::enableEncryptedMedia(bool enable)
{
    RuntimeEnabledFeatures::setEncryptedMediaEnabled(enable);
    // FIXME: Hack to allow MediaKeyError to be enabled for either version.
    RuntimeEnabledFeatures::setEncryptedMediaAnyVersionEnabled(
        RuntimeEnabledFeatures::encryptedMediaEnabled()
        || RuntimeEnabledFeatures::legacyEncryptedMediaEnabled());
}

bool WebRuntimeFeatures::isEncryptedMediaEnabled()
{
    return RuntimeEnabledFeatures::encryptedMediaEnabled();
}

void WebRuntimeFeatures::enableLegacyEncryptedMedia(bool enable)
{
    RuntimeEnabledFeatures::setLegacyEncryptedMediaEnabled(enable);
    // FIXME: Hack to allow MediaKeyError to be enabled for either version.
    RuntimeEnabledFeatures::setEncryptedMediaAnyVersionEnabled(
        RuntimeEnabledFeatures::encryptedMediaEnabled()
        || RuntimeEnabledFeatures::legacyEncryptedMediaEnabled());
}

bool WebRuntimeFeatures::isLegacyEncryptedMediaEnabled()
{
    return RuntimeEnabledFeatures::legacyEncryptedMediaEnabled();
}

void WebRuntimeFeatures::enableExperimentalCanvasFeatures(bool enable)
{
    RuntimeEnabledFeatures::setExperimentalCanvasFeaturesEnabled(enable);
}

bool WebRuntimeFeatures::isExperimentalCanvasFeaturesEnabled()
{
    return RuntimeEnabledFeatures::experimentalCanvasFeaturesEnabled();
}

void WebRuntimeFeatures::enableExperimentalContentSecurityPolicyFeatures(bool enable)
{
    RuntimeEnabledFeatures::setExperimentalContentSecurityPolicyFeaturesEnabled(enable);
}

bool WebRuntimeFeatures::isExperimentalContentSecurityPolicyFeaturesEnabled()
{
    return RuntimeEnabledFeatures::experimentalContentSecurityPolicyFeaturesEnabled();
}

void WebRuntimeFeatures::enableExperimentalShadowDOM(bool enable)
{
    RuntimeEnabledFeatures::setExperimentalShadowDOMEnabled(enable);
}

bool WebRuntimeFeatures::isExperimentalShadowDOMEnabled()
{
    return RuntimeEnabledFeatures::experimentalShadowDOMEnabled();
}

void WebRuntimeFeatures::enableExperimentalWebSocket(bool enable)
{
    // Do nothing. This flag will be deleted.
}

bool WebRuntimeFeatures::isExperimentalWebSocketEnabled()
{
    // This flag will be deleted.
    return false;
}

void WebRuntimeFeatures::enableFileSystem(bool enable)
{
    RuntimeEnabledFeatures::setFileSystemEnabled(enable);
}

bool WebRuntimeFeatures::isFileSystemEnabled()
{
    return RuntimeEnabledFeatures::fileSystemEnabled();
}

void WebRuntimeFeatures::enableFontLoadEvents(bool enable)
{
    RuntimeEnabledFeatures::setFontLoadEventsEnabled(enable);
}

bool WebRuntimeFeatures::isFontLoadEventsEnabled()
{
    return RuntimeEnabledFeatures::fontLoadEventsEnabled();
}

void WebRuntimeFeatures::enableFullscreen(bool enable)
{
    RuntimeEnabledFeatures::setFullscreenEnabled(enable);
}

bool WebRuntimeFeatures::isFullscreenEnabled()
{
    return RuntimeEnabledFeatures::fullscreenEnabled();
}

void WebRuntimeFeatures::enableGamepad(bool enable)
{
    RuntimeEnabledFeatures::setGamepadEnabled(enable);
}

bool WebRuntimeFeatures::isGamepadEnabled()
{
    return RuntimeEnabledFeatures::gamepadEnabled();
}

void WebRuntimeFeatures::enableGeolocation(bool enable)
{
    RuntimeEnabledFeatures::setGeolocationEnabled(enable);
}

bool WebRuntimeFeatures::isGeolocationEnabled()
{
    return RuntimeEnabledFeatures::geolocationEnabled();
}

void WebRuntimeFeatures::enableIMEAPI(bool enable)
{
    RuntimeEnabledFeatures::setIMEAPIEnabled(enable);
}

bool WebRuntimeFeatures::isIMEAPIEnabled()
{
    return RuntimeEnabledFeatures::imeAPIEnabled();
}

void WebRuntimeFeatures::enableIndexedDB(bool enable)
{
    RuntimeEnabledFeatures::setIndexedDBEnabled(enable);
}

bool WebRuntimeFeatures::isIndexedDBEnabled()
{
    return RuntimeEnabledFeatures::indexedDBEnabled();
}

void WebRuntimeFeatures::enableInputTypeWeek(bool enable)
{
    RuntimeEnabledFeatures::setInputTypeWeekEnabled(enable);
}

bool WebRuntimeFeatures::isInputTypeWeekEnabled()
{
    return RuntimeEnabledFeatures::inputTypeWeekEnabled();
}

void WebRuntimeFeatures::enableJavaScriptI18NAPI(bool enable)
{
    RuntimeEnabledFeatures::setJavaScriptI18NAPIEnabled(enable);
}

bool WebRuntimeFeatures::isJavaScriptI18NAPIEnabled()
{
    return RuntimeEnabledFeatures::javaScriptI18NAPIEnabled();
}

void WebRuntimeFeatures::enableLazyLayout(bool enable)
{
    RuntimeEnabledFeatures::setLazyLayoutEnabled(enable);
}

bool WebRuntimeFeatures::isLazyLayoutEnabled()
{
    return RuntimeEnabledFeatures::lazyLayoutEnabled();
}

void WebRuntimeFeatures::enableLocalStorage(bool enable)
{
    RuntimeEnabledFeatures::setLocalStorageEnabled(enable);
}

bool WebRuntimeFeatures::isLocalStorageEnabled()
{
    return RuntimeEnabledFeatures::localStorageEnabled();
}

void WebRuntimeFeatures::enableMediaPlayer(bool enable)
{
    RuntimeEnabledFeatures::setMediaEnabled(enable);
}

bool WebRuntimeFeatures::isMediaPlayerEnabled()
{
    return RuntimeEnabledFeatures::mediaEnabled();
}

void WebRuntimeFeatures::enableMediaSource(bool enable)
{
    RuntimeEnabledFeatures::setMediaSourceEnabled(enable);
}

bool WebRuntimeFeatures::isMediaSourceEnabled()
{
    return RuntimeEnabledFeatures::mediaSourceEnabled();
}

void WebRuntimeFeatures::enableWebKitMediaSource(bool enable)
{
    RuntimeEnabledFeatures::setWebKitMediaSourceEnabled(enable);
}

bool WebRuntimeFeatures::isWebKitMediaSourceEnabled()
{
    return RuntimeEnabledFeatures::webKitMediaSourceEnabled();
}

void WebRuntimeFeatures::enableMediaStream(bool enable)
{
    RuntimeEnabledFeatures::setMediaStreamEnabled(enable);
}

bool WebRuntimeFeatures::isMediaStreamEnabled()
{
    return RuntimeEnabledFeatures::mediaStreamEnabled();
}

void WebRuntimeFeatures::enableNotifications(bool enable)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    RuntimeEnabledFeatures::setNotificationsEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isNotificationsEnabled()
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    return RuntimeEnabledFeatures::notificationsEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enablePagePopup(bool enable)
{
    RuntimeEnabledFeatures::setPagePopupEnabled(enable);
}

bool WebRuntimeFeatures::isPagePopupEnabled()
{
    return RuntimeEnabledFeatures::pagePopupEnabled();
}

void WebRuntimeFeatures::enableParseSVGAsHTML(bool enable)
{
    RuntimeEnabledFeatures::setParseSVGAsHTMLEnabled(enable);
}

bool WebRuntimeFeatures::isParseSVGAsHTMLEnabled()
{
    return RuntimeEnabledFeatures::parseSVGAsHTMLEnabled();
}

void WebRuntimeFeatures::enablePeerConnection(bool enable)
{
    RuntimeEnabledFeatures::setPeerConnectionEnabled(enable);
}

bool WebRuntimeFeatures::isPeerConnectionEnabled()
{
    return RuntimeEnabledFeatures::peerConnectionEnabled();
}

void WebRuntimeFeatures::enableQuota(bool enable)
{
    RuntimeEnabledFeatures::setQuotaEnabled(enable);
}

bool WebRuntimeFeatures::isQuotaEnabled()
{
    return RuntimeEnabledFeatures::quotaEnabled();
}

void WebRuntimeFeatures::enableRequestAutocomplete(bool enable)
{
    RuntimeEnabledFeatures::setRequestAutocompleteEnabled(enable);
}

bool WebRuntimeFeatures::isRequestAutocompleteEnabled()
{
    return RuntimeEnabledFeatures::requestAutocompleteEnabled();
}

void WebRuntimeFeatures::enableScriptedSpeech(bool enable)
{
    RuntimeEnabledFeatures::setScriptedSpeechEnabled(enable);
}

bool WebRuntimeFeatures::isScriptedSpeechEnabled()
{
    return RuntimeEnabledFeatures::scriptedSpeechEnabled();
}

void WebRuntimeFeatures::enableSeamlessIFrames(bool enable)
{
    RuntimeEnabledFeatures::setSeamlessIFramesEnabled(enable);
}

bool WebRuntimeFeatures::isSeamlessIFramesEnabled()
{
    return RuntimeEnabledFeatures::seamlessIFramesEnabled();
}

void WebRuntimeFeatures::enableSessionStorage(bool enable)
{
    RuntimeEnabledFeatures::setSessionStorageEnabled(enable);
}

bool WebRuntimeFeatures::isSessionStorageEnabled()
{
    return RuntimeEnabledFeatures::sessionStorageEnabled();
}

void WebRuntimeFeatures::enableSpeechInput(bool enable)
{
    RuntimeEnabledFeatures::setSpeechInputEnabled(enable);
}

bool WebRuntimeFeatures::isSpeechInputEnabled()
{
    return RuntimeEnabledFeatures::speechInputEnabled();
}

void WebRuntimeFeatures::enableSpeechSynthesis(bool enable)
{
    RuntimeEnabledFeatures::setSpeechSynthesisEnabled(enable);
}

bool WebRuntimeFeatures::isSpeechSynthesisEnabled()
{
    return RuntimeEnabledFeatures::speechSynthesisEnabled();
}

void WebRuntimeFeatures::enableStyleScoped(bool enable)
{
    RuntimeEnabledFeatures::setStyleScopedEnabled(enable);
}

bool WebRuntimeFeatures::isStyleScopedEnabled()
{
    return RuntimeEnabledFeatures::styleScopedEnabled();
}

void WebRuntimeFeatures::enableTouch(bool enable)
{
    RuntimeEnabledFeatures::setTouchEnabled(enable);
}

bool WebRuntimeFeatures::isTouchEnabled()
{
    return RuntimeEnabledFeatures::touchEnabled();
}

void WebRuntimeFeatures::enableVideoTrack(bool enable)
{
    RuntimeEnabledFeatures::setVideoTrackEnabled(enable);
}

bool WebRuntimeFeatures::isVideoTrackEnabled()
{
    return RuntimeEnabledFeatures::videoTrackEnabled();
}

void WebRuntimeFeatures::enableWebAudio(bool enable)
{
    RuntimeEnabledFeatures::setWebAudioEnabled(enable);
}

bool WebRuntimeFeatures::isWebAudioEnabled()
{
    return RuntimeEnabledFeatures::webAudioEnabled();
}

void WebRuntimeFeatures::enableWebGLDraftExtensions(bool enable)
{
    RuntimeEnabledFeatures::setWebGLDraftExtensionsEnabled(enable);
}

bool WebRuntimeFeatures::isWebGLDraftExtensionsEnabled()
{
    return RuntimeEnabledFeatures::webGLDraftExtensionsEnabled();
}

void WebRuntimeFeatures::enableWebMIDI(bool enable)
{
    return RuntimeEnabledFeatures::setWebMIDIEnabled(enable);
}

bool WebRuntimeFeatures::isWebMIDIEnabled()
{
    return RuntimeEnabledFeatures::webMIDIEnabled();
}

void WebRuntimeFeatures::enableDataListElement(bool enable)
{
    RuntimeEnabledFeatures::setDataListElementEnabled(enable);
}

bool WebRuntimeFeatures::isDataListElementEnabled()
{
    return RuntimeEnabledFeatures::dataListElementEnabled();
}

void WebRuntimeFeatures::enableInputTypeColor(bool enable)
{
    RuntimeEnabledFeatures::setInputTypeColorEnabled(enable);
}

bool WebRuntimeFeatures::isInputTypeColorEnabled()
{
    return RuntimeEnabledFeatures::inputTypeColorEnabled();
}

void WebRuntimeFeatures::enableInputModeAttribute(bool enable)
{
    RuntimeEnabledFeatures::setInputModeAttributeEnabled(enable);
}

bool WebRuntimeFeatures::isInputModeAttributeEnabled()
{
    return RuntimeEnabledFeatures::inputModeAttributeEnabled();
}

} // namespace WebKit
