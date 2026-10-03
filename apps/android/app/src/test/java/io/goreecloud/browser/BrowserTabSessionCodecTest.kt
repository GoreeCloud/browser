package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class BrowserTabSessionCodecTest {
    @Test
    fun roundTripPreservesOrderActiveIdentityAndUnicodeTitle() {
        val state = BrowserTabSessionState(
            tabs = listOf(
                BrowserLogicalTab("tab-a", "goreecloud://start", "Start"),
                BrowserLogicalTab("tab-b", "https://example.com/a?b=1", "مثال — Example"),
            ),
            activeTabId = "tab-b",
        )

        val decoded = BrowserTabSessionCodec.decode(BrowserTabSessionCodec.encode(state))

        assertEquals(state, decoded)
    }

    @Test
    fun malformedOrUnknownVersionFailsClosed() {
        assertNull(BrowserTabSessionCodec.decode("other/9\nactive\tdGFi\ntab\tdGFi\taHR0cHM6Ly9leGFtcGxlLmNvbQ\t"))
        assertNull(BrowserTabSessionCodec.decode("goreecloud-browser-tabs/1\nactive\t%%%\ntab\tdGFi\taHR0cHM6Ly9leGFtcGxlLmNvbQ\t"))
        assertNull(BrowserTabSessionCodec.decode("goreecloud-browser-tabs/1\nactive\tdGFi"))
    }

    @Test
    fun duplicateIdentityAndMissingActiveTabFailClosed() {
        val duplicate = listOf(
            "goreecloud-browser-tabs/1",
            "active\tdGFi",
            "tab\tdGFi\taHR0cHM6Ly9hLmV4YW1wbGU\t",
            "tab\tdGFi\taHR0cHM6Ly9iLmV4YW1wbGU\t",
        ).joinToString("\n")
        val missingActive = listOf(
            "goreecloud-browser-tabs/1",
            "active\tbWlzc2luZw",
            "tab\tdGFi\taHR0cHM6Ly9hLmV4YW1wbGU\t",
        ).joinToString("\n")

        assertNull(BrowserTabSessionCodec.decode(duplicate))
        assertNull(BrowserTabSessionCodec.decode(missingActive))
    }

    @Test
    fun serializedPayloadIsBounded() {
        val state = BrowserTabSessionPolicy.initial(
            BrowserLogicalTab("tab", "https://example.com"),
        )
        val encoded = BrowserTabSessionCodec.encode(state)

        assertTrue(encoded.length < 96 * 1024)
        assertNull(BrowserTabSessionCodec.decode("x".repeat(96 * 1024 + 1)))
    }
}
