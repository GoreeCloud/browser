package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class BrowserProcessDeathRecoveryPolicyTest {
    @Test
    fun restoresBoundedCanonicalNormalTabProjection() {
        val restored = BrowserProcessDeathRecoveryPolicy.restore(
            flatTabs = arrayOf(
                "tab-start",
                "",
                "Navigation blocked",
                "tab-web",
                "https://例え.テスト/path?x=1",
                "  Example\u202E title  ",
            ),
            activeTabId = "tab-web",
        )

        assertEquals(listOf("tab-start", "tab-web"), restored?.tabs?.map { it.id })
        assertEquals("goreecloud://start", restored?.tabs?.first()?.url)
        assertNull(restored?.tabs?.first()?.title)
        assertEquals(
            "https://xn--r8jz45g.xn--zckzah/path?x=1",
            restored?.activeTab?.url,
        )
        assertTrue(restored?.activeTab?.title?.contains("\u202E") == false)
    }

    @Test
    fun rejectsCredentialBearingDuplicateAndImpossibleState() {
        assertNull(
            BrowserProcessDeathRecoveryPolicy.restore(
                flatTabs = arrayOf(
                    "tab-a",
                    "https://user:secret@example.com/",
                    "Secret",
                ),
                activeTabId = "tab-a",
            ),
        )
        assertNull(
            BrowserProcessDeathRecoveryPolicy.restore(
                flatTabs = arrayOf(
                    "tab-a",
                    "https://example.com/a",
                    "A",
                    "tab-a",
                    "https://example.com/b",
                    "B",
                ),
                activeTabId = "tab-a",
            ),
        )
        assertNull(
            BrowserProcessDeathRecoveryPolicy.restore(
                flatTabs = arrayOf(
                    "tab-a",
                    "https://example.com/",
                    "Example",
                ),
                activeTabId = "missing",
            ),
        )
    }

    @Test
    fun rejectsMalformedFlatProjection() {
        assertNull(BrowserProcessDeathRecoveryPolicy.restore(null, "tab-a"))
        assertNull(BrowserProcessDeathRecoveryPolicy.restore(emptyArray(), "tab-a"))
        assertNull(
            BrowserProcessDeathRecoveryPolicy.restore(
                arrayOf("tab-a", "https://example.com/"),
                "tab-a",
            ),
        )
    }
}
