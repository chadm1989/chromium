// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.content.DialogInterface;
import android.test.suitebuilder.annotation.MediumTest;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.autofill.AutofillTestHelper;
import org.chromium.chrome.browser.autofill.PersonalDataManager.AutofillProfile;
import org.chromium.chrome.browser.autofill.PersonalDataManager.CreditCard;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * A payment integration test for a merchant that does not require shipping address.
 */
public class PaymentRequestNoShippingTest extends PaymentRequestTestBase {
    public PaymentRequestNoShippingTest() {
        super("payment_request_no_shipping_test.html");
    }

    @Override
    public void onMainActivityStarted()
            throws InterruptedException, ExecutionException, TimeoutException {
        AutofillTestHelper helper = new AutofillTestHelper();
        String billingAddressId = helper.setProfile(new AutofillProfile("", "https://example.com",
                true, "Jon Doe", "Google", "340 Main St", "CA", "Los Angeles", "", "90291", "",
                "US", "310-310-6000", "jon.doe@gmail.com", "en-US"));
        helper.setCreditCard(new CreditCard("", "https://example.com", true, true, "Jon Doe",
                "4111111111111111", "1111", "12", "2050", "visa", R.drawable.pr_visa,
                billingAddressId));
    }

    @MediumTest
    public void testCloseDialog() throws InterruptedException, ExecutionException,
            TimeoutException {
        triggerUIAndWait(mReadyToClose);
        clickAndWait(R.id.close_button, mDismissed);
        expectResultContains(new String[] {"Request cancelled"});
    }

    @MediumTest
    public void testEditAndCloseDialog() throws InterruptedException, ExecutionException,
            TimeoutException {
        triggerUIAndWait(mReadyForInput);
        clickAndWait(R.id.button_secondary, mReadyToClose);
        clickAndWait(R.id.close_button, mDismissed);
        expectResultContains(new String[] {"Request cancelled"});
    }

    @MediumTest
    public void testEditAndCancelDialog() throws InterruptedException, ExecutionException,
            TimeoutException {
        triggerUIAndWait(mReadyForInput);
        clickAndWait(R.id.button_secondary, mReadyForInput);
        clickAndWait(R.id.button_secondary, mDismissed);
        expectResultContains(new String[] {"Request cancelled"});
    }

    @MediumTest
    public void testPay() throws InterruptedException, ExecutionException, TimeoutException {
        triggerUIAndWait(mReadyToPay);
        clickAndWait(R.id.button_primary, mReadyForUnmaskInput);
        setTextInCardUnmaskDialogAndWait(R.id.card_unmask_input, "123", mReadyToUnmask);
        clickCardUnmaskButtonAndWait(DialogInterface.BUTTON_POSITIVE, mDismissed);
        expectResultContains(new String[] {"Jon Doe", "4111111111111111", "12", "2050", "visa",
                "123"});
    }

    @MediumTest
    public void testCancelUnmaskAndRetry()
            throws InterruptedException, ExecutionException, TimeoutException {
        triggerUIAndWait(mReadyToPay);
        clickAndWait(R.id.button_primary, mReadyForUnmaskInput);
        clickCardUnmaskButtonAndWait(DialogInterface.BUTTON_NEGATIVE, mReadyToPay);
        clickAndWait(R.id.button_primary, mReadyForUnmaskInput);
        setTextInCardUnmaskDialogAndWait(R.id.card_unmask_input, "123", mReadyToUnmask);
        clickCardUnmaskButtonAndWait(DialogInterface.BUTTON_POSITIVE, mDismissed);
        expectResultContains(new String[] {"Jon Doe", "4111111111111111", "12", "2050", "visa",
                "123"});
    }
}
