package io.goreecloud.browser

/**
 * External-disclosure boundary for Browser page addresses.
 *
 * Browser-owned local/resource URLs are implementation details and must not be
 * placed on the Android clipboard or sent through ACTION_SEND. Only an HTTP(S)
 * address that passes the same navigation policy used by the omnibox can leave
 * Browser through page-address sharing actions.
 */
object BrowserDisclosurePolicy {
    fun shareablePageUrl(currentUrl: String): String? =
        NavigationResolver.canonicalizeWebUrl(currentUrl)
}
