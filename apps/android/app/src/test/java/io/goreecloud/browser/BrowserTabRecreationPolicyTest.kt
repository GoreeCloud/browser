package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class BrowserTabRecreationPolicyTest {
    @Test
    fun restoresCanonicalSafeOrderedTabGraph() {
        val restored = BrowserTabRecreationPolicy.restore(
            ids = listOf("tab-a", "tab-b"),
            urls = listOf(
                "goreecloud://start",
                "https://例え.テスト/path?x=1",
            ),
            titles = listOf("", "  Example\u202E title  "),
            activeTabId = "tab-b",
        )

        assertEquals(listOf("tab-a", "tab-b"), restored?.tabs?.map { it.id })
        assertEquals("tab-b", restored?.activeTabId)
        assertEquals(
            "https://xn--r8jz45g.xn--zckzah/path?x=1",
            restored?.activeTab?.url,
        )
        assertTrue(restored?.activeTab?.title?.contains("\u202E") == false)
        assertNull(restored?.tabs?.first()?.title)
    }

    @Test
    fun rejectsUnsafeLocationsAndImpossibleSelection() {
        assertNull(
            BrowserTabRecreationPolicy.restore(
                ids = listOf("tab-a"),
                urls = listOf("https://user:secret@example.com/"),
                titles = listOf("Secret"),
                activeTabId = "tab-a",
            ),
        )
        assertNull(
            BrowserTabRecreationPolicy.restore(
                ids = listOf("tab-a"),
                urls = listOf("goreecloud://start"),
                titles = listOf(""),
                activeTabId = "missing",
            ),
        )
    }

    @Test
    fun rejectsMismatchedOrDuplicateMetadata() {
        assertNull(
            BrowserTabRecreationPolicy.restore(
                ids = listOf("tab-a", "tab-a"),
                urls = listOf("goreecloud://start", "goreecloud://start"),
                titles = listOf("", ""),
                activeTabId = "tab-a",
            ),
        )
        assertNull(
            BrowserTabRecreationPolicy.restore(
                ids = listOf("tab-a", "tab-b"),
                urls = listOf("goreecloud://start"),
                titles = listOf("", ""),
                activeTabId = "tab-a",
            ),
        )
    }

    @Test
    fun normalizationDoesNotCreateDurableEngineState() {
        val state = BrowserTabSessionState(
            tabs = listOf(
                BrowserLogicalTab("tab-a", "goreecloud://start"),
                BrowserLogicalTab("tab-b", "https://example.com/path", " Example "),
            ),
            activeTabId = "tab-b",
        )

        val normalized = BrowserTabRecreationPolicy.normalized(state)

        assertEquals(listOf("tab-a", "tab-b"), normalized?.tabs?.map { it.id })
        assertEquals("https://example.com/path", normalized?.activeTab?.url)
        assertEquals("Example", normalized?.activeTab?.title)
    }
}
