package io.goreecloud.browser

data class BrowserLogicalTab(
    val id: String,
    val url: String,
    val title: String? = null,
) {
    init {
        require(id.isNotBlank() && id.length <= BrowserTabSessionPolicy.MAX_TAB_ID_LENGTH)
        require(url.isNotBlank() && url.length <= BrowserTabSessionPolicy.MAX_URL_LENGTH)
        require(title == null || title.length <= BrowserTabSessionPolicy.MAX_TITLE_LENGTH)
    }
}

data class BrowserTabSessionState(
    val tabs: List<BrowserLogicalTab>,
    val activeTabId: String,
) {
    init {
        require(tabs.isNotEmpty())
        require(tabs.size <= BrowserTabSessionPolicy.MAX_TABS)
        require(tabs.map { it.id }.distinct().size == tabs.size)
        require(tabs.any { it.id == activeTabId })
    }

    val activeTab: BrowserLogicalTab
        get() = tabs.first { it.id == activeTabId }
}

sealed interface BrowserTabMutation {
    data class Updated(val state: BrowserTabSessionState) : BrowserTabMutation
    data object MaxTabsReached : BrowserTabMutation
    data object DuplicateTabId : BrowserTabMutation
    data object TabNotFound : BrowserTabMutation
    data object LastTabProtected : BrowserTabMutation
}

object BrowserTabSessionPolicy {
    const val MAX_TABS = 32
    const val MAX_TAB_ID_LENGTH = 128
    const val MAX_URL_LENGTH = 8_192
    const val MAX_TITLE_LENGTH = 256

    fun initial(tab: BrowserLogicalTab): BrowserTabSessionState =
        BrowserTabSessionState(tabs = listOf(tab), activeTabId = tab.id)

    fun open(
        state: BrowserTabSessionState,
        tab: BrowserLogicalTab,
        activate: Boolean = true,
    ): BrowserTabMutation {
        if (state.tabs.any { it.id == tab.id }) return BrowserTabMutation.DuplicateTabId
        if (state.tabs.size >= MAX_TABS) return BrowserTabMutation.MaxTabsReached

        return BrowserTabMutation.Updated(
            state.copy(
                tabs = state.tabs + tab,
                activeTabId = if (activate) tab.id else state.activeTabId,
            ),
        )
    }

    fun select(state: BrowserTabSessionState, tabId: String): BrowserTabMutation {
        if (state.tabs.none { it.id == tabId }) return BrowserTabMutation.TabNotFound
        return BrowserTabMutation.Updated(state.copy(activeTabId = tabId))
    }

    fun close(state: BrowserTabSessionState, tabId: String): BrowserTabMutation {
        val index = state.tabs.indexOfFirst { it.id == tabId }
        if (index < 0) return BrowserTabMutation.TabNotFound
        if (state.tabs.size == 1) return BrowserTabMutation.LastTabProtected

        val remaining = state.tabs.filterNot { it.id == tabId }
        val nextActive = if (state.activeTabId != tabId) {
            state.activeTabId
        } else {
            remaining.getOrNull(index)?.id ?: remaining.last().id
        }

        return BrowserTabMutation.Updated(
            BrowserTabSessionState(tabs = remaining, activeTabId = nextActive),
        )
    }

    fun updateLocation(
        state: BrowserTabSessionState,
        tabId: String,
        url: String,
    ): BrowserTabMutation {
        if (state.tabs.none { it.id == tabId }) return BrowserTabMutation.TabNotFound
        val updated = state.tabs.map { tab ->
            if (tab.id == tabId) BrowserLogicalTab(tab.id, url, tab.title) else tab
        }
        return BrowserTabMutation.Updated(state.copy(tabs = updated))
    }

    fun updateTitle(
        state: BrowserTabSessionState,
        tabId: String,
        title: String?,
    ): BrowserTabMutation {
        if (state.tabs.none { it.id == tabId }) return BrowserTabMutation.TabNotFound
        val bounded = title
            ?.trim()
            ?.takeIf { it.isNotEmpty() }
            ?.take(MAX_TITLE_LENGTH)
        val updated = state.tabs.map { tab ->
            if (tab.id == tabId) tab.copy(title = bounded) else tab
        }
        return BrowserTabMutation.Updated(state.copy(tabs = updated))
    }
}
