package io.goreecloud.browser

import android.webkit.CookieManager
import android.webkit.WebStorage
import android.webkit.WebView

/**
 * Clears Browser-owned Android WebView browsing data without touching Browser
 * application preferences or Android permission state.
 */
object BrowserBrowsingDataCleaner {
    fun clear(webView: WebView, onComplete: () -> Unit) {
        clear(listOf(webView), onComplete)
    }

    fun clear(webViews: Collection<WebView>, onComplete: () -> Unit) {
        webViews.distinct().forEach { webView ->
            webView.stopLoading()
            webView.clearCache(true)
            webView.clearHistory()
            webView.clearFormData()
            webView.clearSslPreferences()
        }
        WebStorage.getInstance().deleteAllData()

        val cookies = CookieManager.getInstance()
        cookies.removeAllCookies {
            cookies.flush()
            onComplete()
        }
    }
}
