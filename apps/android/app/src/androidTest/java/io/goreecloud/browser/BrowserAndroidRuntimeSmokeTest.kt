package io.goreecloud.browser

import android.view.View
import android.view.ViewGroup
import android.webkit.CookieManager
import android.webkit.WebSettings
import android.webkit.WebView
import android.widget.EditText
import android.widget.ImageButton
import android.widget.TextView
import androidx.test.core.app.ActivityScenario
import androidx.test.ext.junit.runners.AndroidJUnit4
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Managed Android 15 emulator smoke evidence for the Development Browser shell.
 * This does not establish physical launcher appearance, representative-device
 * usability/performance, live provider acceptance, production signing, or release maturity.
 */
@RunWith(AndroidJUnit4::class)
class BrowserAndroidRuntimeSmokeTest {
    @Test
    fun launcherActivityBuildsBrowserOwnedChromeWithConservativeWebViewDefaults() {
        ActivityScenario.launch(BrowserActivityV2::class.java).use { scenario ->
            scenario.onActivity { activity ->
                val views = collectViews(activity.window.decorView)
                assertTrue(
                    views.filterIsInstance<TextView>()
                        .any { it.text?.toString() == "GoreeCloud Browser" },
                )
                assertTrue(
                    views.filterIsInstance<EditText>()
                        .any { it.contentDescription?.toString() == "Search or address bar" },
                )

                val buttons = views.filterIsInstance<ImageButton>()
                val controls = buttons.mapNotNull { it.contentDescription?.toString() }
                assertTrue(controls.contains("Go"))
                assertTrue(controls.contains("Back"))
                assertTrue(controls.contains("Forward"))
                assertTrue(controls.contains("Start page"))
                assertTrue(controls.contains("Browser menu"))
                assertTrue(controls.contains("Reload") || controls.contains("Stop loading"))
                assertFalse(buttons.first { it.contentDescription?.toString() == "Back" }.isEnabled)
                assertFalse(buttons.first { it.contentDescription?.toString() == "Forward" }.isEnabled)

                val webView = views.filterIsInstance<WebView>().single()
                val settings = webView.settings
                assertFalse(settings.allowFileAccess)
                assertFalse(settings.allowContentAccess)
                assertEquals(WebSettings.MIXED_CONTENT_NEVER_ALLOW, settings.mixedContentMode)
                assertFalse(CookieManager.getInstance().acceptThirdPartyCookies(webView))
                assertNotEquals(0, activity.applicationInfo.icon)
                assertEquals("io.goreecloud.browser.beta", activity.packageName)
            }
        }
    }

    @Test
    fun freeTextSearchRemainsOnLocalAuthorizationRequiredPage() {
        ActivityScenario.launch(BrowserActivityV2::class.java).use { scenario ->
            scenario.onActivity { activity ->
                val views = collectViews(activity.window.decorView)
                views.filterIsInstance<EditText>()
                    .first { it.contentDescription?.toString() == "Search or address bar" }
                    .setText("synthetic acceptance query")
                views.filterIsInstance<ImageButton>()
                    .first { it.contentDescription?.toString() == "Go" }
                    .performClick()
            }

            var observed = ""
            repeat(80) {
                val response = AtomicReference("")
                val received = CountDownLatch(1)
                scenario.onActivity { activity ->
                    val webView = collectViews(activity.window.decorView)
                        .filterIsInstance<WebView>()
                        .single()
                    webView.evaluateJavascript("document.body.innerText") { text ->
                        response.set(text.orEmpty())
                        received.countDown()
                    }
                }
                if (received.await(2, TimeUnit.SECONDS)) {
                    observed = response.get()
                    if (
                        observed.contains("Search authorization required") &&
                        observed.contains("Your query was not sent to GoreeCloud Search")
                    ) {
                        return
                    }
                }
                Thread.sleep(200)
            }

            assertTrue(
                "Expected local fail-closed Search explanation, observed: $observed",
                observed.contains("Search authorization required") &&
                    observed.contains("Your query was not sent to GoreeCloud Search"),
            )
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
}
