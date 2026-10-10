package io.goreecloud.browser

import android.content.Context
import android.view.View
import android.view.ViewGroup
import android.webkit.GeolocationPermissions
import android.webkit.WebView
import androidx.test.core.app.ActivityScenario
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import java.io.File
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/** Verifies site geolocation is denied by the launched V2 Activity. */
@RunWith(AndroidJUnit4::class)
class BrowserGeolocationDenialRuntimeTest {
    private val context by lazy {
        ApplicationProvider.getApplicationContext<Context>()
    }

    @Before
    fun prepareFreshNormalSession() {
        BrowserNormalSessionProcess.resetForProcessBoundaryTest()
        File(context.noBackupFilesDir, "normal-session").deleteRecursively()
        val firstUse = BrowserFirstUsePreferences(context)
        assertTrue(firstUse.complete())
        assertTrue(firstUse.setHintsEnabled(false))
    }

    @Test
    fun geolocationCallbackDeniesAndDoesNotRetainPermission() {
        ActivityScenario.launch(BrowserActivityV2::class.java).use { scenario ->
            scenario.onActivity { activity ->
                val webView = collectViews(activity.window.decorView)
                    .filterIsInstance<WebView>()
                    .single()
                val chrome = webView.webChromeClient
                assertTrue(chrome != null)
                val origin = "https://geolocation-smoke.example"
                var callbacks = 0
                chrome!!.onGeolocationPermissionsShowPrompt(
                    origin,
                    GeolocationPermissions.Callback { observed, allowed, retained ->
                        assertEquals(origin, observed)
                        assertFalse(allowed)
                        assertFalse(retained)
                        callbacks++
                    },
                )
                assertEquals(1, callbacks)
            }
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
