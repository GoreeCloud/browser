package io.goreecloud.browser

/**
 * Activity-recreation-only policy for the Browser-owned Android tab graph.
 *
 * This is deliberately not a durable session store. It validates the bounded
 * logical metadata that may be copied into Android instance state while keeping
 * process-death/session-journal authority outside this tranche.
 */
object BrowserTabRecreationPolicy {
    private const val INTERNAL_HOME = "goreecloud://start"

    fun restore(
        ids: List<String>?,
        urls: List<String>?,
        titles: List<String>?,
        activeTabId: String?,
    ): BrowserTabSessionState? {
        if (ids == null || urls == null || titles == null || activeTabId == null) return null
        if (ids.size !in 1..BrowserTabSessionPolicy.MAX_TABS) return null
        if (urls.size != ids.size || titles.size != ids.size) return null
        if (ids.distinct().size != ids.size) return null

        val tabs = ids.indices.map { index ->
            val id = ids[index]
            if (id.isBlank() || id.length > BrowserTabSessionPolicy.MAX_TAB_ID_LENGTH) return null

            val rawUrl = urls[index]
            val url = when {
                rawUrl == INTERNAL_HOME -> INTERNAL_HOME
                else -> NavigationResolver.canonicalizeWebUrl(rawUrl) ?: return null
            }

            val rawTitle = titles[index].trim().takeIf { it.isNotEmpty() }
            val title = if (url == INTERNAL_HOME) {
                null
            } else {
                rawTitle?.let { PageTitlePresentation.safe(it, url) }
                    ?.take(BrowserTabSessionPolicy.MAX_TITLE_LENGTH)
            }

            BrowserLogicalTab(id = id, url = url, title = title)
        }

        return runCatching {
            BrowserTabSessionState(tabs = tabs, activeTabId = activeTabId)
        }.getOrNull()
    }

    fun normalized(state: BrowserTabSessionState): BrowserTabSessionState? =
        restore(
            ids = state.tabs.map { it.id },
            urls = state.tabs.map { it.url },
            titles = state.tabs.map { it.title.orEmpty() },
            activeTabId = state.activeTabId,
        )
}
