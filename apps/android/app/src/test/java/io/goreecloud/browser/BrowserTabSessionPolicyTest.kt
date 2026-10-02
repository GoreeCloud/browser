package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class BrowserTabSessionPolicyTest {
    @Test
    fun newTabGetsStableLogicalIdentityAndCanBecomeActive() {
        val initial = BrowserTabSessionPolicy.initial(
            BrowserLogicalTab("tab-1", "goreecloud://start"),
        )
        val result = BrowserTabSessionPolicy.open(
            initial,
            BrowserLogicalTab("tab-2", "https://example.com"),
        )

        assertTrue(result is BrowserTabMutation.Updated)
        val state = (result as BrowserTabMutation.Updated).state
        assertEquals(listOf("tab-1", "tab-2"), state.tabs.map { it.id })
        assertEquals("tab-2", state.activeTabId)
    }

    @Test
    fun closingActiveTabSelectsNearestRemainingTab() {
        var state = BrowserTabSessionPolicy.initial(
            BrowserLogicalTab("a", "goreecloud://start"),
        )
        state = (BrowserTabSessionPolicy.open(
            state,
            BrowserLogicalTab("b", "https://b.example"),
        ) as BrowserTabMutation.Updated).state
        state = (BrowserTabSessionPolicy.open(
            state,
            BrowserLogicalTab("c", "https://c.example"),
        ) as BrowserTabMutation.Updated).state

        val result = BrowserTabSessionPolicy.close(state, "b")
        val updated = (result as BrowserTabMutation.Updated).state
        assertEquals(listOf("a", "c"), updated.tabs.map { it.id })
        assertEquals("c", updated.activeTabId)
    }

    @Test
    fun closingInactiveTabPreservesActiveIdentity() {
        var state = BrowserTabSessionPolicy.initial(
            BrowserLogicalTab("a", "goreecloud://start"),
        )
        state = (BrowserTabSessionPolicy.open(
            state,
            BrowserLogicalTab("b", "https://b.example"),
        ) as BrowserTabMutation.Updated).state
        state = (BrowserTabSessionPolicy.open(
            state,
            BrowserLogicalTab("c", "https://c.example"),
        ) as BrowserTabMutation.Updated).state

        val updated = (BrowserTabSessionPolicy.close(
            state,
            "a",
        ) as BrowserTabMutation.Updated).state

        assertEquals(listOf("b", "c"), updated.tabs.map { it.id })
        assertEquals("c", updated.activeTabId)
    }

    @Test
    fun closingLastTabFailsClosed() {
        val state = BrowserTabSessionPolicy.initial(
            BrowserLogicalTab("only", "goreecloud://start"),
        )
        assertEquals(
            BrowserTabMutation.LastTabProtected,
            BrowserTabSessionPolicy.close(state, "only"),
        )
    }

    @Test
    fun duplicateAndUnknownIdentitiesFailClosed() {
        val state = BrowserTabSessionPolicy.initial(
            BrowserLogicalTab("tab", "goreecloud://start"),
        )
        assertEquals(
            BrowserTabMutation.DuplicateTabId,
            BrowserTabSessionPolicy.open(
                state,
                BrowserLogicalTab("tab", "https://example.com"),
            ),
        )
        assertEquals(
            BrowserTabMutation.TabNotFound,
            BrowserTabSessionPolicy.select(state, "missing"),
        )
    }

    @Test
    fun tabCountIsBounded() {
        var state = BrowserTabSessionPolicy.initial(
            BrowserLogicalTab("tab-0", "goreecloud://start"),
        )
        repeat(BrowserTabSessionPolicy.MAX_TABS - 1) { index ->
            state = (BrowserTabSessionPolicy.open(
                state,
                BrowserLogicalTab(
                    "tab-${index + 1}",
                    "https://example.com/${index + 1}",
                ),
                activate = false,
            ) as BrowserTabMutation.Updated).state
        }

        assertEquals(
            BrowserTabMutation.MaxTabsReached,
            BrowserTabSessionPolicy.open(
                state,
                BrowserLogicalTab("overflow", "https://example.com/overflow"),
            ),
        )
    }

    @Test
    fun locationAndTitleUpdatesAreBoundToExactLogicalTab() {
        var state = BrowserTabSessionPolicy.initial(
            BrowserLogicalTab("tab-1", "goreecloud://start"),
        )
        state = (BrowserTabSessionPolicy.open(
            state,
            BrowserLogicalTab("tab-2", "https://example.com"),
            activate = false,
        ) as BrowserTabMutation.Updated).state

        state = (BrowserTabSessionPolicy.updateLocation(
            state,
            "tab-2",
            "https://example.com/next",
        ) as BrowserTabMutation.Updated).state
        state = (BrowserTabSessionPolicy.updateTitle(
            state,
            "tab-2",
            "  Example title  ",
        ) as BrowserTabMutation.Updated).state

        assertEquals("goreecloud://start", state.tabs[0].url)
        assertEquals("https://example.com/next", state.tabs[1].url)
        assertEquals("Example title", state.tabs[1].title)
    }

    @Test
    fun invalidLocationFailsClosedWithoutMutatingSession() {
        val state = BrowserTabSessionPolicy.initial(
            BrowserLogicalTab("tab-1", "goreecloud://start"),
        )

        assertEquals(
            BrowserTabMutation.InvalidLocation,
            BrowserTabSessionPolicy.updateLocation(state, "tab-1", "   "),
        )
        assertEquals(
            BrowserTabMutation.InvalidLocation,
            BrowserTabSessionPolicy.updateLocation(
                state,
                "tab-1",
                "x".repeat(BrowserTabSessionPolicy.MAX_URL_LENGTH + 1),
            ),
        )
        assertEquals("goreecloud://start", state.activeTab.url)
    }

    @Test
    fun titleNormalizationIsBoundedAndBlankTitleBecomesNull() {
        var state = BrowserTabSessionPolicy.initial(
            BrowserLogicalTab("tab-1", "goreecloud://start"),
        )

        state = (BrowserTabSessionPolicy.updateTitle(
            state,
            "tab-1",
            "x".repeat(BrowserTabSessionPolicy.MAX_TITLE_LENGTH + 20),
        ) as BrowserTabMutation.Updated).state
        assertEquals(BrowserTabSessionPolicy.MAX_TITLE_LENGTH, state.activeTab.title?.length)

        state = (BrowserTabSessionPolicy.updateTitle(
            state,
            "tab-1",
            "   ",
        ) as BrowserTabMutation.Updated).state
        assertNull(state.activeTab.title)
    }
}
