package io.goreecloud.browser

import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.After
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class BrowserFirstUsePreferencesRuntimeTest {
    private val context by lazy {
        ApplicationProvider.getApplicationContext<android.content.Context>()
    }

    @Before
    fun resetBefore() {
        context.getSharedPreferences("goreecloud-browser-first-use", android.content.Context.MODE_PRIVATE)
            .edit()
            .clear()
            .commit()
    }

    @After
    fun resetAfter() {
        resetBefore()
    }

    @Test
    fun completionReplayAndHintStatePersistLocally() {
        val preferences = BrowserFirstUsePreferences(context)

        assertFalse(preferences.state().completed)
        assertEquals(0, preferences.state().step)
        assertTrue(preferences.state().hintsEnabled)

        preferences.setStep(2)
        assertEquals(2, BrowserFirstUsePreferences(context).state().step)

        preferences.setHintsEnabled(false)
        assertFalse(BrowserFirstUsePreferences(context).state().hintsEnabled)

        preferences.complete()
        val completed = BrowserFirstUsePreferences(context).state()
        assertTrue(completed.completed)
        assertEquals(0, completed.step)

        preferences.dismissChromeHint()
        assertTrue(BrowserFirstUsePreferences(context).state().chromeHintDismissed)
        preferences.resetDismissedHints()
        assertFalse(BrowserFirstUsePreferences(context).state().chromeHintDismissed)

        preferences.replay()
        val replay = BrowserFirstUsePreferences(context).state()
        assertTrue(replay.completed)
        assertEquals(0, replay.step)
    }
}
