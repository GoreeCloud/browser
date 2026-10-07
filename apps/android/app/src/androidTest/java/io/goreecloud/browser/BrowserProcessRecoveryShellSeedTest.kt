package io.goreecloud.browser

import android.content.Context
import android.view.View
import android.view.ViewGroup
import android.widget.EditText
import android.widget.ImageButton
import androidx.test.core.app.ActivityScenario
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Seeds a real durable Normal-session fixture for the shell-driven process
 * boundary check in android-beta.yml.
 *
 * During the ordinary connected test suite the fixture is deleted at the end.
 * The workflow reruns this one test with goreecloud.recovery.seed=1, then kills
 * the target app process and verifies the next launcher Activity restores from
 * the Browser-owned native journal/checkpoint authority.
 */
@RunWith(AndroidJUnit4::class)
class BrowserProcessRecoveryShellSeedTest {
    private val context by lazy {
        ApplicationProvider.getApplicationContext<Context>()
    }

    @Test
    fun seedAbnormalDurableNormalSessionForShellBoundary() {
        BrowserNormalSessionProcess.resetForProcessBoundaryTest()
        val recoveryDirectory = File(context.noBackupFilesDir, "normal-session")
        recoveryDirectory.deleteRecursively()

        val preferences = BrowserFirstUsePreferences(context)
        assertTrue(preferences.complete())
        assertTrue(preferences.setHintsEnabled(false))

        val scenario = ActivityScenario.launch(BrowserActivityV2::class.java)
        scenario.onActivity { activity ->
            var views = collectViews(activity.window.decorView)
            views.filterIsInstance<ImageButton>()
                .first { it.contentDescription?.toString() == "New tab" }
                .performClick()

            views = collectViews(activity.window.decorView)
            views.filterIsInstance<EditText>()
                .first { it.contentDescription?.toString() == "Search or address bar" }
                .setText(RECOVERY_URL)
            views.filterIsInstance<ImageButton>()
                .first { it.contentDescription?.toString() == "Go" }
                .performClick()

            val tabs = collectViews(activity.window.decorView)
                .filter { it.contentDescription?.toString()?.startsWith("Tab ") == true }
            assertEquals(2, tabs.size)
            assertTrue(tabs[1].contentDescription.toString().endsWith(", selected"))
        }

        BrowserNormalSessionProcess.resetForProcessBoundaryTest()
        scenario.close()

        val retainForShell =
            InstrumentationRegistry.getArguments().getString(SEED_ARGUMENT) == "1"
        if (!retainForShell) {
            recoveryDirectory.deleteRecursively()
        }
    }

    private fun collectViews(view: View): List<View> = when (view) {
        is ViewGroup -> buildList {
            add(view)
            repeat(view.childCount) { index ->
                addAll(collectViews(view.getChildAt(index)))
            }
        }
        else -> listOf(view)
    }

    companion object {
        const val SEED_ARGUMENT = "goreecloud.recovery.seed"
        const val RECOVERY_URL = "https://example.com/goreecloud-shell-recovery"
    }
}
