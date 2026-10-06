package io.goreecloud.browser

import android.content.Context
import android.view.View
import android.view.ViewGroup
import android.webkit.CookieManager
import android.webkit.WebSettings
import android.webkit.WebView
import android.widget.EditText
import android.widget.ImageButton
import android.widget.TextView
import androidx.test.core.app.ActivityScenario
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertNotSame
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Managed Android 15 emulator smoke evidence for the Development Browser shell.
 * This does not establish physical launcher appearance, representative-device
 * usability/performance, live provider acceptance, production signing, or release maturity.
 */
@RunWith(AndroidJUnit4::class)
class BrowserAndroidRuntimeSmokeTest {
    private val context by lazy {
        ApplicationProvider.getApplicationContext<Context>()
    }

    @Before
    fun completeFirstUseForNonOnboardingSmoke() {
        val preferences = BrowserFirstUsePreferences(context)
        assertTrue(preferences.complete())
        assertTrue(preferences.setHintsEnabled(false))
    }

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
                assertEquals(PageTextZoom.DEFAULT_PERCENT, settings.textZoom)
                assertTrue(settings.javaScriptEnabled)
                assertTrue(settings.loadsImagesAutomatically)
                assertFalse(settings.blockNetworkImage)
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

    @Test
    fun liveTabsUseIndependentWebViewsAndIsolateBackgroundFailureChrome() {
        val backgroundFailureUrl = "http://127.0.0.1:1/goreecloud-tab-background-smoke"
        val firstWebView = AtomicReference<WebView>()
        val secondWebView = AtomicReference<WebView>()

        ActivityScenario.launch(BrowserActivityV2::class.java).use { scenario ->
            scenario.onActivity { activity ->
                val views = collectViews(activity.window.decorView)
                firstWebView.set(views.filterIsInstance<WebView>().single())
                views.filterIsInstance<ImageButton>()
                    .first { it.contentDescription?.toString() == "New tab" }
                    .performClick()

                val afterOpen = collectViews(activity.window.decorView)
                secondWebView.set(afterOpen.filterIsInstance<WebView>().single())
                assertNotSame(firstWebView.get(), secondWebView.get())
                assertEquals(
                    2,
                    afterOpen.filterIsInstance<TextView>()
                        .count { it.contentDescription?.toString()?.startsWith("Tab ") == true },
                )

                afterOpen.filterIsInstance<TextView>()
                    .first { it.contentDescription?.toString()?.startsWith("Tab 1:") == true }
                    .performClick()

                val afterSwitch = collectViews(activity.window.decorView)
                assertSame(firstWebView.get(), afterSwitch.filterIsInstance<WebView>().single())

                afterSwitch.filterIsInstance<EditText>()
                    .first { it.contentDescription?.toString() == "Search or address bar" }
                    .setText(backgroundFailureUrl)
                afterSwitch.filterIsInstance<ImageButton>()
                    .first { it.contentDescription?.toString() == "Go" }
                    .performClick()
                collectViews(activity.window.decorView)
                    .filterIsInstance<TextView>()
                    .first { it.contentDescription?.toString()?.startsWith("Tab 2:") == true }
                    .performClick()
            }

            Thread.sleep(1_000)

            scenario.onActivity { activity ->
                val views = collectViews(activity.window.decorView)
                assertSame(secondWebView.get(), views.filterIsInstance<WebView>().single())
                assertTrue(
                    views.filterIsInstance<TextView>()
                        .none { it.text?.toString() == BrowserFailureStatePolicy.PAGE_UNAVAILABLE_TITLE },
                )
                views.filterIsInstance<TextView>()
                    .first { it.contentDescription?.toString()?.startsWith("Tab 1:") == true }
                    .performClick()
            }

            assertTrue(
                "Expected background tab failure state to become visible when reselected",
                waitForChromeText(scenario, BrowserFailureStatePolicy.PAGE_UNAVAILABLE_TITLE),
            )

            scenario.onActivity { activity ->
                val views = collectViews(activity.window.decorView)
                assertSame(firstWebView.get(), views.filterIsInstance<WebView>().single())
                assertTrue(
                    views.filterIsInstance<TextView>().any {
                        it.contentDescription?.toString()?.startsWith("Tab 1:") == true &&
                            it.text?.toString() == BrowserFailureStatePolicy.PAGE_UNAVAILABLE_TITLE
                    },
                )
                views.filterIsInstance<ImageButton>()
                    .first { it.contentDescription?.toString()?.startsWith("Close tab 1:") == true }
                    .performClick()

                val afterClose = collectViews(activity.window.decorView)
                assertSame(secondWebView.get(), afterClose.filterIsInstance<WebView>().single())
                assertEquals(
                    1,
                    afterClose.filterIsInstance<TextView>()
                        .count { it.contentDescription?.toString()?.startsWith("Tab ") == true },
                )
            }
        }
    }

    @Test
    fun liveTabOrderSelectionAndSafeNavigationSurviveActivityRecreation() {
        val secondTabUrl = "https://example.com/goreecloud-tab-recreation"

        ActivityScenario.launch(BrowserActivityV2::class.java).use { scenario ->
            scenario.onActivity { activity ->
                var views = collectViews(activity.window.decorView)
                views.filterIsInstance<ImageButton>()
                    .first { it.contentDescription?.toString() == "New tab" }
                    .performClick()

                views = collectViews(activity.window.decorView)
                views.filterIsInstance<EditText>()
                    .first { it.contentDescription?.toString() == "Search or address bar" }
                    .setText(secondTabUrl)
                views.filterIsInstance<ImageButton>()
                    .first { it.contentDescription?.toString() == "Go" }
                    .performClick()
                collectViews(activity.window.decorView)
                    .filterIsInstance<TextView>()
                    .first { it.contentDescription?.toString()?.startsWith("Tab 1:") == true }
                    .performClick()
            }

            scenario.recreate()

            scenario.onActivity { activity ->
                val views = collectViews(activity.window.decorView)
                val tabs = views.filterIsInstance<TextView>()
                    .filter { it.contentDescription?.toString()?.startsWith("Tab ") == true }
                assertEquals(2, tabs.size)
                assertTrue(tabs[0].contentDescription.toString().startsWith("Tab 1:"))
                assertTrue(tabs[0].contentDescription.toString().endsWith(", selected"))
                assertTrue(tabs[1].contentDescription.toString().startsWith("Tab 2:"))
                assertEquals(1, views.filterIsInstance<WebView>().size)

                tabs[1].performClick()
                val afterSwitch = collectViews(activity.window.decorView)
                assertEquals(
                    "example.com/goreecloud-tab-recreation",
                    afterSwitch.filterIsInstance<EditText>()
                        .first { it.contentDescription?.toString() == "Search or address bar" }
                        .text
                        .toString(),
                )
                assertEquals(1, afterSwitch.filterIsInstance<WebView>().size)
            }
        }
    }

    @Test
    fun pageUnavailableRetryStateSurvivesActivityRecreation() {
        val retryUrl = "http://127.0.0.1:1/goreecloud-recovery-smoke"

        ActivityScenario.launch(BrowserActivityV2::class.java).use { scenario ->
            scenario.onActivity { activity ->
                val views = collectViews(activity.window.decorView)
                views.filterIsInstance<EditText>()
                    .first { it.contentDescription?.toString() == "Search or address bar" }
                    .setText(retryUrl)
                views.filterIsInstance<ImageButton>()
                    .first { it.contentDescription?.toString() == "Go" }
                    .performClick()
            }

            assertTrue(
                "Expected Browser-owned failure surface before recreation",
                waitForChromeText(scenario, BrowserFailureStatePolicy.PAGE_UNAVAILABLE_TITLE),
            )

            scenario.recreate()

            assertTrue(
                "Expected Browser-owned failure surface after recreation",
                waitForChromeText(scenario, BrowserFailureStatePolicy.PAGE_UNAVAILABLE_TITLE),
            )
            scenario.onActivity { activity ->
                val views = collectViews(activity.window.decorView)
                val address = views.filterIsInstance<EditText>()
                    .first { it.contentDescription?.toString() == "Search or address bar" }
                    .text
                    .toString()
                assertEquals("127.0.0.1:1/goreecloud-recovery-smoke", address)

                val reload = views.filterIsInstance<ImageButton>()
                    .first { it.contentDescription?.toString() == "Reload" }
                assertTrue(reload.isEnabled)
            }
        }
    }

    @Test
    fun browsingDataCleanerRemovesWebViewCookies() {
        val cookieUrl = "https://example.com/"
        val cookieName = "goreecloud_clear_test"

        ActivityScenario.launch(BrowserActivityV2::class.java).use { scenario ->
            val cookieSet = CountDownLatch(1)
            scenario.onActivity {
                CookieManager.getInstance().setCookie(
                    cookieUrl,
                    "$cookieName=1; Path=/",
                ) {
                    cookieSet.countDown()
                }
            }
            assertTrue(cookieSet.await(5, TimeUnit.SECONDS))
            assertTrue(
                CookieManager.getInstance().getCookie(cookieUrl)
                    ?.contains("$cookieName=1") == true,
            )

            val cleared = CountDownLatch(1)
            scenario.onActivity { activity ->
                val webView = collectViews(activity.window.decorView)
                    .filterIsInstance<WebView>()
                    .single()
                BrowserBrowsingDataCleaner.clear(webView) {
                    cleared.countDown()
                }
            }

            assertTrue(cleared.await(5, TimeUnit.SECONDS))
            assertFalse(
                CookieManager.getInstance().getCookie(cookieUrl)
                    ?.contains("$cookieName=1") == true,
            )
        }
    }

    private fun waitForChromeText(
        scenario: ActivityScenario<BrowserActivityV2>,
        expected: String,
    ): Boolean {
        repeat(80) {
            val observed = AtomicReference(false)
            scenario.onActivity { activity ->
                observed.set(
                    collectViews(activity.window.decorView)
                        .filterIsInstance<TextView>()
                        .any { it.text?.toString() == expected },
                )
            }
            if (observed.get()) return true
            Thread.sleep(200)
        }
        return false
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
