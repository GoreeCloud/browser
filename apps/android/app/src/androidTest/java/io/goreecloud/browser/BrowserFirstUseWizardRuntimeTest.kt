package io.goreecloud.browser

import android.content.Context
import android.os.SystemClock
import android.view.accessibility.AccessibilityNodeInfo
import androidx.test.core.app.ActivityScenario
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class BrowserFirstUseWizardRuntimeTest {
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
    fun freshLaunchPresentsAndCompletesRequiredSetup() {
        ActivityScenario.launch(BrowserActivityV2::class.java).use {
            assertTrue(waitForText("Welcome to GoreeCloud Browser"))
            clickText("Continue")

            assertTrue(waitForText("Privacy & security by default"))
            clickText("Continue")

            assertTrue(waitForText("Browse with clear boundaries"))
            clickText("Start browsing")

            assertTrue(waitUntil {
                BrowserFirstUsePreferences(context).state().completed
            })
        }
    }

    @Test
    fun recreationResumesDurablyAcceptedSetupStep() {
        ActivityScenario.launch(BrowserActivityV2::class.java).use { scenario ->
            assertTrue(waitForText("Welcome to GoreeCloud Browser"))
            clickText("Continue")
            assertTrue(waitForText("Privacy & security by default"))
            assertEquals(1, BrowserFirstUsePreferences(context).state().step)

            scenario.recreate()

            assertTrue(waitForText("Privacy & security by default"))
            assertEquals(1, BrowserFirstUsePreferences(context).state().step)
        }
    }

    private fun clickText(text: String) {
        val node = waitForNode(text)
            ?: throw AssertionError("Could not find accessibility node with text: $text")
        if (!node.performAction(AccessibilityNodeInfo.ACTION_CLICK)) {
            throw AssertionError("Accessibility click failed for: $text")
        }
    }

    private fun waitForText(text: String): Boolean = waitForNode(text) != null

    private fun waitForNode(text: String): AccessibilityNodeInfo? {
        val uiAutomation = InstrumentationRegistry.getInstrumentation().uiAutomation
        repeat(60) {
            val root = uiAutomation.rootInActiveWindow
            val match = root?.findAccessibilityNodeInfosByText(text)
                ?.firstOrNull { node -> node.text?.toString() == text }
            if (match != null) return match
            SystemClock.sleep(100)
        }
        return null
    }

    private fun waitUntil(predicate: () -> Boolean): Boolean {
        repeat(60) {
            if (predicate()) return true
            SystemClock.sleep(100)
        }
        return false
    }

    private fun clearState() {
        context.getSharedPreferences(
            BrowserFirstUsePreferences.FILE_NAME,
            Context.MODE_PRIVATE,
        ).edit().clear().commit()
    }
}
