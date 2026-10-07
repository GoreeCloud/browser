package io.goreecloud.browser

/**
 * Defense-in-depth boundary for Browser-owned durable Normal-session recovery.
 *
 * The C++ recovery core is the durable authority. Android still revalidates the
 * recovered projection before constructing logical tabs or WebViews so stale,
 * malformed, credential-bearing, oversized, or impossible state never becomes
 * Android browser state.
 */
object BrowserProcessDeathRecoveryPolicy {
    private const val INTERNAL_HOME = "goreecloud://start"

    fun restore(
        flatTabs: Array<String>?,
        activeTabId: String?,
    ): BrowserTabSessionState? {
        if (flatTabs == null || activeTabId.isNullOrBlank()) return null
        if (flatTabs.isEmpty() || flatTabs.size % 3 != 0) return null

        val tabCount = flatTabs.size / 3
        if (tabCount !in 1..BrowserTabSessionPolicy.MAX_TABS) return null

        val tabs = ArrayList<BrowserLogicalTab>(tabCount)
        val ids = HashSet<String>()
        repeat(tabCount) { index ->
            val offset = index * 3
            val id = flatTabs[offset]
            if (
                id.isBlank() ||
                id.length > BrowserTabSessionPolicy.MAX_TAB_ID_LENGTH ||
                !ids.add(id)
            ) {
                return null
            }

            val durableUrl = flatTabs[offset + 1]
            val url = if (durableUrl.isBlank()) {
                INTERNAL_HOME
            } else {
                NavigationResolver.canonicalizeWebUrl(durableUrl) ?: return null
            }

            val durableTitle = flatTabs[offset + 2]
            val title = if (url == INTERNAL_HOME || durableTitle.isBlank()) {
                null
            } else {
                PageTitlePresentation.safe(durableTitle, url)
                    .take(BrowserTabSessionPolicy.MAX_TITLE_LENGTH)
            }

            tabs += BrowserLogicalTab(
                id = id,
                url = url,
                title = title,
            )
        }

        return runCatching {
            BrowserTabSessionState(
                tabs = tabs,
                activeTabId = activeTabId,
            )
        }.getOrNull()
    }
}
