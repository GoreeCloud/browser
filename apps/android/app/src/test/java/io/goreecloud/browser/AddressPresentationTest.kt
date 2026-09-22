package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class AddressPresentationTest {
    @Test
    fun condensedAddressKeepsHostAtLeadingEdge() {
        assertEquals(
            "search.goreecloud.com/preferences",
            AddressPresentation.condensed("https://search.goreecloud.com/preferences"),
        )
    }

    @Test
    fun condensedAddressPreservesPathAndQueryWithoutScheme() {
        assertEquals(
            "example.com/path/to/page?q=browser",
            AddressPresentation.condensed("https://example.com/path/to/page?q=browser"),
        )
    }

    @Test
    fun rootPathIsNotShown() {
        assertEquals(
            "search.goreecloud.com",
            AddressPresentation.condensed("https://search.goreecloud.com/"),
        )
    }

    @Test
    fun nonDefaultPortRemainsVisible() {
        assertEquals(
            "example.com:8443/settings",
            AddressPresentation.condensed("https://example.com:8443/settings"),
        )
    }

    @Test
    fun opaqueInternalPayloadsAreNotExposed() {
        assertEquals("New tab", AddressPresentation.condensed("about:blank"))
        assertEquals("Local page", AddressPresentation.condensed("data:text/html,<h1>private payload</h1>"))
        assertEquals("Site content", AddressPresentation.condensed("blob:https://example.com/opaque-id"))
        assertEquals("Local content", AddressPresentation.condensed("file:///private/runtime/path"))
    }

    @Test
    fun malformedFallbackRemovesControlCharactersAndBoundsLength() {
        val result = AddressPresentation.condensed("  not a\nurl\t" + "x".repeat(180))

        assertFalse(result.contains('\n'))
        assertFalse(result.contains('\t'))
        assertTrue(result.length <= 120)
        assertTrue(result.endsWith("…"))
    }
}
