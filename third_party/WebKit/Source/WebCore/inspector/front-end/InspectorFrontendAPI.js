/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

 InspectorFrontendAPI = {
     isDebuggingEnabled: function()
     {
         return WebInspector.panels.scripts.debuggingEnabled;
     },

     setDebuggingEnabled: function(enabled)
     {
         if (enabled) {
             WebInspector.panels.scripts.enableDebugging();
             WebInspector.currentPanel = WebInspector.panels.scripts;
         } else
             WebInspector.panels.scripts.disableDebugging();
     },

     isJavaScriptProfilingEnabled: function()
     {
         return WebInspector.panels.scripts.profilingEnabled;
     },

     setJavaScriptProfilingEnabled: function(enabled)
     {
         if (enabled) {
             WebInspector.panels.profiles.enableProfiler();
             WebInspector.currentPanel = WebInspector.panels.profiles;
         } else
             WebInspector.panels.scripts.disableProfiler();
     },

     isTimelineProfilingEnabled: function()
     {
         return WebInspector.panels.timeline.timelineProfilingEnabled;
     },

     setTimelineProfilingEnabled: function(enabled)
     {
         WebInspector.panels.timeline.setTimelineProfilingEnabled(enabled);
     },

     isProfilingJavaScript: function()
     {
         return WebInspector.CPUProfileType.instance && WebInspector.CPUProfileType.instance.isRecordingProfile();
     },

     startProfilingJavaScript: function()
     {
         WebInspector.panels.profiles.enableProfiler();
         WebInspector.currentPanel = WebInspector.panels.profiles;
         if (WebInspector.CPUProfileType.instance)
             WebInspector.CPUProfileType.instance.startRecordingProfile();
     },

     stopProfilingJavaScript: function()
     {
         if (WebInspector.CPUProfileType.instance)
             WebInspector.CPUProfileType.instance.stopRecordingProfile();
         WebInspector.currentPanel = WebInspector.panels.profiles;
     },

     setAttachedWindow: function(attached)
     {
         WebInspector.attached = attached;
     },

     showConsole: function()
     {
         WebInspector.currentPanel = WebInspector.panels.console;
     }
 }
