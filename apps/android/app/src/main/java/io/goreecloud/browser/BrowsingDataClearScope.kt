package io.goreecloud.browser

/**
 * User-visible scope for the Android Browser local browsing-data reset.
 *
 * Browser settings are intentionally excluded. They are preferences, not
 * browsing data, and should survive an ordinary clear-browsing-data action.
 */
object BrowsingDataClearScope {
    val clearedItems: List<String> = listOf(
        "Cookies and website sign-in state",
        "Website storage",
        "Cached web content",
        "Form data",
        "WebView navigation history",
        "WebView SSL preferences",
    )

    val preservedItems: List<String> = listOf(
        "Browser settings such as page text size",
        "Android app permissions",
    )
}
