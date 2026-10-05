package io.goreecloud.browser

internal data class BrowserFailureState(
    val retryUrl: String,
)

/**
 * Browser-owned recreation contract for the local main-frame failure surface.
 *
 * Only a canonical HTTP(S) retry URL that is already accepted by Browser
 * navigation policy may enter Android Activity instance state. Restoration is
 * also conditioned on the WebView still showing a Browser-local document and
 * on no more-specific blocked-navigation surface being authoritative.
 *
 * Remote error details, arbitrary page/title text, credentials, and blocked
 * deep-link payloads are intentionally outside this state.
 */
internal object BrowserFailureStatePolicy {
    const val BUNDLE_RETRY_URL_KEY = "goreecloud.browser.failure.retry_url"
    const val PAGE_UNAVAILABLE_TITLE = "Page unavailable"

    fun persistableRetryUrl(retryUrl: String?): String? =
        retryUrl?.let(NavigationResolver::canonicalizeWebUrl)

    fun restorable(
        retryUrl: String?,
        restoredDocumentIsBrowserLocal: Boolean,
        blockedWebNavigationVisible: Boolean,
    ): BrowserFailureState? {
        if (!restoredDocumentIsBrowserLocal || blockedWebNavigationVisible) return null
        val canonical = persistableRetryUrl(retryUrl) ?: return null
        return BrowserFailureState(retryUrl = canonical)
    }
}
