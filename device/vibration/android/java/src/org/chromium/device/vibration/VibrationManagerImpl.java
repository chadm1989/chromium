// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.vibration;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.os.Vibrator;
import android.util.Log;

import org.chromium.mojo.system.MojoException;
import org.chromium.mojom.device.VibrationManager;

/**
 * Android implementation of the vibration manager service defined in
 * device/vibration/vibration_manager.mojom.
 */
public class VibrationManagerImpl implements VibrationManager {
    private static final String TAG = "VibrationManagerImpl";

    private static final long MINIMUM_VIBRATION_DURATION_MS = 1; // 1 millisecond
    private static final long MAXIMUM_VIBRATION_DURATION_MS = 10000; // 10 seconds

    private final AudioManager mAudioManager;
    private final Vibrator mVibrator;
    private final boolean mHasVibratePermission;

    public VibrationManagerImpl(Context context) {
        mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        mVibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        mHasVibratePermission =
                context.checkCallingOrSelfPermission(android.Manifest.permission.VIBRATE)
                == PackageManager.PERMISSION_GRANTED;
        if (!mHasVibratePermission) {
            Log.w(TAG, "Failed to use vibrate API, requires VIBRATE permission.");
        }
    }

    @Override
    public void close() {}

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void vibrate(long milliseconds) {
        // Though the Blink implementation already sanitizes vibration times, don't
        // trust any values passed from the client.
        long sanitizedMilliseconds = Math.max(MINIMUM_VIBRATION_DURATION_MS,
                Math.min(milliseconds, MAXIMUM_VIBRATION_DURATION_MS));

        if (mAudioManager.getRingerMode() != AudioManager.RINGER_MODE_SILENT
                && mHasVibratePermission) {
            mVibrator.vibrate(sanitizedMilliseconds);
        }
    }

    @Override
    public void cancel() {
        if (mHasVibratePermission) mVibrator.cancel();
    }
}
