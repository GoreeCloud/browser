package io.goreecloud.browser

import android.os.SystemClock
import android.view.accessibility.AccessibilityNodeInfo
import androidx.test.core.app.ActivityScenario
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.After
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class BrowserFirstUseWizardRuntimeTest {
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
    fun freshLaunchPresentsAndCompletesRequiredSetup() {
        ActivityScenario.launch(BrowserActivityV2::class.java).use {
            assertTrue(waitForText("Welcome to GoreeCloud Browser"))
            clickText("Continue")

            assertTrue(waitForText("Private by default"))
            clickText("Continue")

            assertTrue(waitForText("Navigate with clear boundaries"))
            clickText("Start browsing")

            assertTrue(waitUntil {
                BrowserFirstUsePreferences(context).state().completed
            })
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
        repeat(50) {
            val root = uiAutomation.rootInActiveWindow
            val match = root?.findAccessibilityNodeInfosByText(text)
                ?.firstOrNull { node -> node.text?.toString() == text }
            if (match != null) return match
            SystemClock.sleep(100)
        }
        return null
    }

    private fun waitUntil(predicate: () -> Boolean): Boolean {
        repeat(50) {
            if (predicate()) return true
            SystemClock.sleep(100)
        }
        return false
    }
}
