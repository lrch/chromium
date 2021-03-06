// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.sync;

import android.accounts.Account;
import android.content.Context;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.sync.AndroidSyncSettings.AndroidSyncSettingsObserver;
import org.chromium.sync.AndroidSyncSettings.CachedAccountSyncSettings;
import org.chromium.sync.signin.ChromeSigninController;
import org.chromium.sync.test.util.MockSyncContentResolverDelegate;

/**
 * Tests for AndroidSyncSettings.
 */
public class AndroidSyncSettingsTest extends InstrumentationTestCase {

    private static class CountingMockSyncContentResolverDelegate
            extends MockSyncContentResolverDelegate {
        private int mGetMasterSyncAutomaticallyCalls;
        private int mGetSyncAutomaticallyCalls;
        private int mGetIsSyncableCalls;
        private int mSetIsSyncableCalls;
        private int mSetSyncAutomaticallyCalls;

        @Override
        public boolean getMasterSyncAutomatically() {
            mGetMasterSyncAutomaticallyCalls++;
            return super.getMasterSyncAutomatically();
        }

        @Override
        public boolean getSyncAutomatically(Account account, String authority) {
            mGetSyncAutomaticallyCalls++;
            return super.getSyncAutomatically(account, authority);
        }

        @Override
        public int getIsSyncable(Account account, String authority) {
            mGetIsSyncableCalls++;
            return super.getIsSyncable(account, authority);
        }

        @Override
        public void setIsSyncable(Account account, String authority, int syncable) {
            mSetIsSyncableCalls++;
            super.setIsSyncable(account, authority, syncable);
        }

        @Override
        public void setSyncAutomatically(Account account, String authority, boolean sync) {
            mSetSyncAutomaticallyCalls++;
            super.setSyncAutomatically(account, authority, sync);
        }
    }

    private static class CountingCachedAccountSyncSettings extends CachedAccountSyncSettings {
        private int mUpdateSyncSettingsForAccountInternalCalls;
        private int mSetIsSyncableInternalCalls;
        private int mSetSyncAutomaticallyInternalCalls;

        public CountingCachedAccountSyncSettings(String contractAuthority,
                MockSyncContentResolverDelegate contentResolverWrapper) {
            super(contractAuthority, contentResolverWrapper);
        }

        @Override
        protected void updateSyncSettingsForAccountInternal(Account account) {
            mUpdateSyncSettingsForAccountInternalCalls++;
            super.updateSyncSettingsForAccountInternal(account);
        }

        @Override
        protected void setIsSyncableInternal(Account account) {
            mSetIsSyncableInternalCalls++;
            super.setIsSyncableInternal(account);
        }

        @Override
        protected void setSyncAutomaticallyInternal(Account account, boolean value) {
            mSetSyncAutomaticallyInternalCalls++;
            super.setSyncAutomaticallyInternal(account, value);
        }

        public void resetCount() {
            mUpdateSyncSettingsForAccountInternalCalls = 0;
        }
    }

    private static class MockSyncSettingsObserver implements AndroidSyncSettingsObserver {
        private boolean mReceivedNotification;

        public void clearNotification() {
            mReceivedNotification = false;
        }

        public boolean didReceiveNotification() {
            return mReceivedNotification;
        }

        @Override
        public void androidSyncSettingsChanged() {
            mReceivedNotification = true;
        }
    }

    private AndroidSyncSettings mAndroid;
    private CountingMockSyncContentResolverDelegate mSyncContentResolverDelegate;
    private String mAuthority;
    private Account mTestAccount;
    private Account mAlternateTestAccount;
    private CountingCachedAccountSyncSettings mCachedAccountSyncSettings;
    private MockSyncSettingsObserver mSyncSettingsObserver;

    @Override
    protected void setUp() throws Exception {
        mSyncContentResolverDelegate = new CountingMockSyncContentResolverDelegate();
        Context context = getInstrumentation().getTargetContext();
        mCachedAccountSyncSettings = new CountingCachedAccountSyncSettings(
                context.getPackageName(), mSyncContentResolverDelegate);
        AndroidSyncSettings.overrideAndroidSyncSettingsForTests(
                context, mSyncContentResolverDelegate, mCachedAccountSyncSettings);
        mAndroid = AndroidSyncSettings.get(getInstrumentation().getTargetContext());
        // Need to set the signed in account name to ensure that sync settings notifications
        // update the right account.
        ChromeSigninController.get(
                getInstrumentation().getTargetContext()).setSignedInAccountName(
                        "account@example.com");
        mAuthority = AndroidSyncSettings.get(getInstrumentation().getTargetContext())
                .getContractAuthority();
        mTestAccount = new Account("account@example.com", "com.google");
        mAlternateTestAccount = new Account("alternateAccount@example.com", "com.google");

        mSyncSettingsObserver = new MockSyncSettingsObserver();
        mAndroid.registerObserver(mSyncSettingsObserver);

        super.setUp();
    }

    @SmallTest
    @Feature({"Sync"})
    public void testToggleMasterSyncAutomaticallyFromSettings() throws InterruptedException {
        mSyncContentResolverDelegate.setMasterSyncAutomatically(true);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertTrue("master sync should be set", mAndroid.isMasterSyncEnabled());

        mSyncContentResolverDelegate.setMasterSyncAutomatically(false);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertFalse("master sync should be unset", mAndroid.isMasterSyncEnabled());
    }

    @SmallTest
    @Feature({"Sync"})
    public void testToggleAccountSyncFromSettings() throws InterruptedException {
        // Turn on syncability.
        mSyncContentResolverDelegate.setMasterSyncAutomatically(true);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();

        // First sync
        mSyncContentResolverDelegate.setIsSyncable(mTestAccount, mAuthority, 1);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        mSyncContentResolverDelegate.setSyncAutomatically(mTestAccount, mAuthority, true);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertTrue("sync should be set", mAndroid.isSyncEnabled(mTestAccount));
        assertTrue("sync should be set for chrome app",
                mAndroid.isChromeSyncEnabled(mTestAccount));

        // Disable sync automatically for the app
        mSyncContentResolverDelegate.setSyncAutomatically(mTestAccount, mAuthority, false);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertFalse("sync should be unset", mAndroid.isSyncEnabled(mTestAccount));
        assertFalse("sync should be unset for chrome app",
                mAndroid.isChromeSyncEnabled(mTestAccount));

        // Re-enable sync
        mSyncContentResolverDelegate.setSyncAutomatically(mTestAccount, mAuthority, true);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertTrue("sync should be re-enabled", mAndroid.isSyncEnabled(mTestAccount));
        assertTrue("sync should be unset for chrome app",
                mAndroid.isChromeSyncEnabled(mTestAccount));

        // Disabled from master sync
        mSyncContentResolverDelegate.setMasterSyncAutomatically(false);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertFalse("sync should be disabled due to master sync",
                mAndroid.isSyncEnabled(mTestAccount));
        assertTrue("sync should be set for chrome app",
                mAndroid.isChromeSyncEnabled(mTestAccount));
    }

    @SmallTest
    @Feature({"Sync"})
    public void testToggleAccountSyncFromApplication() throws InterruptedException {
        // Turn on syncability.
        mSyncContentResolverDelegate.setMasterSyncAutomatically(true);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAndroid.enableChromeSync(mTestAccount);
            }
        });
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertTrue("account should be synced", mAndroid.isSyncEnabled(mTestAccount));

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAndroid.disableChromeSync(mTestAccount);
            }
        });
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertFalse("account should not be synced", mAndroid.isSyncEnabled(mTestAccount));
    }

    @SmallTest
    @Feature({"Sync"})
    public void testToggleSyncabilityForMultipleAccounts() throws InterruptedException {
        // Turn on syncability.
        mSyncContentResolverDelegate.setMasterSyncAutomatically(true);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAndroid.enableChromeSync(mTestAccount);
            }
        });
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertTrue("account should be synced", mAndroid.isSyncEnabled(mTestAccount));

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAndroid.enableChromeSync(mAlternateTestAccount);
            }
        });
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertTrue("alternate account should be synced",
                mAndroid.isSyncEnabled(mAlternateTestAccount));

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAndroid.disableChromeSync(mAlternateTestAccount);
            }
        });
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertFalse("alternate account should not be synced",
                mAndroid.isSyncEnabled(mAlternateTestAccount));
        assertTrue("account should still be synced", mAndroid.isSyncEnabled(mTestAccount));

        // Ensure we don't erroneously re-use cached data.
        assertFalse("null account should not be synced", mAndroid.isSyncEnabled(null));
    }

    @SmallTest
    @Feature({"Sync"})
    public void testSyncSettingsCaching() throws InterruptedException {
        // Turn on syncability.
        mSyncContentResolverDelegate.setMasterSyncAutomatically(true);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mAndroid.enableChromeSync(mTestAccount);
            }
        });
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();
        assertTrue("account should be synced", mAndroid.isSyncEnabled(mTestAccount));

        int masterSyncAutomaticallyCalls =
                mSyncContentResolverDelegate.mGetMasterSyncAutomaticallyCalls;
        int isSyncableCalls = mSyncContentResolverDelegate.mGetIsSyncableCalls;
        int getSyncAutomaticallyAcalls = mSyncContentResolverDelegate.mGetSyncAutomaticallyCalls;

        // Do a bunch of reads.
        mAndroid.isMasterSyncEnabled();
        mAndroid.isSyncEnabled();
        mAndroid.isSyncEnabled(mTestAccount);
        mAndroid.isChromeSyncEnabled(mTestAccount);

        // Ensure values were read from cache.
        assertEquals(masterSyncAutomaticallyCalls,
                mSyncContentResolverDelegate.mGetMasterSyncAutomaticallyCalls);
        assertEquals(isSyncableCalls, mSyncContentResolverDelegate.mGetIsSyncableCalls);
        assertEquals(getSyncAutomaticallyAcalls,
                mSyncContentResolverDelegate.mGetSyncAutomaticallyCalls);

        // Do a bunch of reads for alternate account.
        mAndroid.isMasterSyncEnabled();
        mAndroid.isSyncEnabled(mAlternateTestAccount);
        mAndroid.isChromeSyncEnabled(mAlternateTestAccount);

        // Ensure master sync was cached but others are fetched once.
        assertEquals(masterSyncAutomaticallyCalls,
                mSyncContentResolverDelegate.mGetMasterSyncAutomaticallyCalls);
        assertEquals(isSyncableCalls + 1, mSyncContentResolverDelegate.mGetIsSyncableCalls);
        assertEquals(getSyncAutomaticallyAcalls + 1,
                mSyncContentResolverDelegate.mGetSyncAutomaticallyCalls);
    }

    @SmallTest
    @Feature({"Sync"})
    public void testGetContractAuthority() throws Exception {
        assertEquals("The contract authority should be the package name.",
                getInstrumentation().getTargetContext().getPackageName(),
                mAndroid.getContractAuthority());
    }

    @SmallTest
    @Feature({"Sync"})
    public void testCachedAccountSyncSettingsExitEarly() throws InterruptedException {
        mSyncContentResolverDelegate.disableObserverNotifications();

        mCachedAccountSyncSettings.updateSyncSettingsForAccount(null);
        assertTrue("Update sync settings failed to exit early", mCachedAccountSyncSettings
                .mUpdateSyncSettingsForAccountInternalCalls == 0);

        mCachedAccountSyncSettings.updateSyncSettingsForAccount(mTestAccount);
        assertTrue("Update sync settings should not have exited early", mCachedAccountSyncSettings
                .mUpdateSyncSettingsForAccountInternalCalls == 1);

        mCachedAccountSyncSettings.setIsSyncable(mTestAccount);
        assertTrue("setIsSyncable should not have exited early",
                mCachedAccountSyncSettings.mSetIsSyncableInternalCalls == 1);

        mCachedAccountSyncSettings.setIsSyncable(mTestAccount);
        assertTrue("setIsSyncable failed to exit early", mCachedAccountSyncSettings
                .mSetIsSyncableInternalCalls == 1);

        mCachedAccountSyncSettings.setSyncAutomatically(mTestAccount, true);
        assertTrue("setSyncAutomatically should not have to exited early",
                mCachedAccountSyncSettings.mSetSyncAutomaticallyInternalCalls == 1);

        mCachedAccountSyncSettings.setSyncAutomatically(mTestAccount, true);
        assertTrue("setSyncAutomatically failed to exit early",
                mCachedAccountSyncSettings.mSetSyncAutomaticallyInternalCalls == 1);

        mCachedAccountSyncSettings.setSyncAutomatically(mTestAccount, false);
        assertTrue("setSyncAutomatically should not have to exited early",
                mCachedAccountSyncSettings.mSetSyncAutomaticallyInternalCalls == 2);

        mCachedAccountSyncSettings.setSyncAutomatically(mTestAccount, false);
        assertTrue("setSyncAutomatically failed to exit early",
                mCachedAccountSyncSettings.mSetSyncAutomaticallyInternalCalls == 2);
    }

    @SmallTest
    @Feature({"Sync"})
    public void testCachedAccountSyncSettingsDidUpdate() throws InterruptedException {
        // Since we're just testing the cache we disable observer notifications to prevent
        // notifications to AndroidSyncSettings from mutating it.
        mSyncContentResolverDelegate.disableObserverNotifications();

        mCachedAccountSyncSettings.clearUpdateStatus();
        mCachedAccountSyncSettings.getSyncAutomatically(mTestAccount);
        assertTrue("getSyncAutomatically on un-populated cache failed to update DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());

        mCachedAccountSyncSettings.clearUpdateStatus();
        mCachedAccountSyncSettings.getSyncAutomatically(mTestAccount);
        assertFalse("getSyncAutomatically on populated cache updated DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());

        mCachedAccountSyncSettings.updateSyncSettingsForAccount(mAlternateTestAccount);
        assertTrue("updateSyncSettingsForAccount failed to update DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());

        mCachedAccountSyncSettings.clearUpdateStatus();

        mCachedAccountSyncSettings.updateSyncSettingsForAccount(mTestAccount);
        assertTrue("updateSyncSettingsForAccount failed to update DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());

        mCachedAccountSyncSettings.clearUpdateStatus();

        mCachedAccountSyncSettings.updateSyncSettingsForAccount(mTestAccount);
        assertFalse("updateSyncSettingsForAccount updated DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());

        mCachedAccountSyncSettings.clearUpdateStatus();
        mCachedAccountSyncSettings.setIsSyncable(mTestAccount);
        assertTrue("setIsSyncable failed to update DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());

        mCachedAccountSyncSettings.clearUpdateStatus();
        mCachedAccountSyncSettings.setIsSyncable(mTestAccount);
        assertFalse("setIsSyncable updated DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());

        mCachedAccountSyncSettings.clearUpdateStatus();
        mCachedAccountSyncSettings.setSyncAutomatically(mTestAccount, true);
        assertTrue("setSyncAutomatically failed to update DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());

        mCachedAccountSyncSettings.clearUpdateStatus();
        mCachedAccountSyncSettings.setSyncAutomatically(mTestAccount, true);
        assertFalse("setSyncAutomatically updated DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());

        mCachedAccountSyncSettings.clearUpdateStatus();
        mCachedAccountSyncSettings.setSyncAutomatically(mTestAccount, false);
        assertTrue("setSyncAutomatically failed to update DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());

        mCachedAccountSyncSettings.clearUpdateStatus();
        mCachedAccountSyncSettings.setSyncAutomatically(mTestAccount, false);
        assertFalse("setSyncAutomatically updated DidUpdate flag",
                mCachedAccountSyncSettings.getDidUpdateStatus());
    }

    @SmallTest
    @Feature({"Sync"})
    public void testAndroidSyncSettingsPostsNotifications() throws InterruptedException {
        // Turn on syncability.
        mSyncContentResolverDelegate.setMasterSyncAutomatically(true);
        mSyncContentResolverDelegate.waitForLastNotificationCompleted();

        mSyncSettingsObserver.clearNotification();
        mAndroid.isSyncEnabled(mAlternateTestAccount);
        assertTrue("isSyncEnabled on wrongly populated cache did not trigger observers",
                mSyncSettingsObserver.didReceiveNotification());

        mSyncSettingsObserver.clearNotification();
        mAndroid.isSyncEnabled(mTestAccount);
        assertTrue("isSyncEnabled on wrongly populated cache did not trigger observers",
                mSyncSettingsObserver.didReceiveNotification());

        mSyncSettingsObserver.clearNotification();
        mAndroid.enableChromeSync(mTestAccount);
        assertTrue("enableChromeSync did not trigger observers",
                mSyncSettingsObserver.didReceiveNotification());

        mSyncSettingsObserver.clearNotification();
        mAndroid.enableChromeSync(mTestAccount);
        assertFalse("enableChromeSync triggered observers",
                mSyncSettingsObserver.didReceiveNotification());

        mSyncSettingsObserver.clearNotification();
        mAndroid.disableChromeSync(mTestAccount);
        assertTrue("disableChromeSync did not trigger observers",
                mSyncSettingsObserver.didReceiveNotification());

        mSyncSettingsObserver.clearNotification();
        mAndroid.disableChromeSync(mTestAccount);
        assertFalse("disableChromeSync triggered observers",
                mSyncSettingsObserver.didReceiveNotification());
    }
}
