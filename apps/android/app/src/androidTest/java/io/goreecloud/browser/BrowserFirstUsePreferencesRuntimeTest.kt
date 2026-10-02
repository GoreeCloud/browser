package io.goreecloud.browser

import android.content.Context
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class BrowserFirstUsePreferencesRuntimeTest {
    private val context by lazy {
        ApplicationProvider.getApplicationContext<Context>()
    }

    @Before
    fun resetBefore() {
        clearState()
    }

    @After
    fun resetAfter() {
        clearState()
    }

    @Test
    fun completionReplayAndHintStatePersistLocally() {
        val preferences = BrowserFirstUsePreferences(context)

        assertFalse(preferences.state().completed)
        assertEquals(0, preferences.state().step)
        assertTrue(preferences.state().hintsEnabled)

        assertTrue(preferences.setStep(2))
        assertEquals(2, BrowserFirstUsePreferences(context).state().step)

        assertTrue(preferences.setHintsEnabled(false))
        assertFalse(BrowserFirstUsePreferences(context).state().hintsEnabled)

        assertTrue(preferences.complete())
        val completed = BrowserFirstUsePreferences(context).state()
        assertTrue(completed.completed)
        assertEquals(0, completed.step)

        assertTrue(preferences.dismissChromeHint())
        assertTrue(BrowserFirstUsePreferences(context).state().chromeHintDismissed)

        assertTrue(preferences.resetDismissedHints())
        assertFalse(BrowserFirstUsePreferences(context).state().chromeHintDismissed)

        assertTrue(preferences.replay())
        val replay = BrowserFirstUsePreferences(context).state()
        assertTrue(replay.completed)
        assertEquals(0, replay.step)
    }

    private fun clearState() {
        context.getSharedPreferences(
            BrowserFirstUsePreferences.FILE_NAME,
            Context.MODE_PRIVATE,
        ).edit().clear().commit()
    }
}
