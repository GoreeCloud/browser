package io.goreecloud.browser

import org.junit.Assert.assertEquals
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
    fun condensedAddressPreservesFragmentIdentity() {
        assertEquals(
            "example.com/path?q=browser#results",
            AddressPresentation.condensed("https://example.com/path?q=browser#results"),
        )
    }

    @Test
    fun condensedAddressKeepsExplicitPortVisible() {
        assertEquals(
            "example.com:8443/path",
            AddressPresentation.condensed("https://example.com:8443/path"),
        )
    }

    @Test
    fun condensedUnicodeHostUsesCanonicalAsciiALabel() {
        assertEquals(
            "xn--r8jz45g.xn--zckzah/path?q=1#section",
            AddressPresentation.condensed("https://例え.テスト/path?q=1#section"),
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
    fun nonHostAddressFallsBackToOriginalText() {
        assertEquals("about:blank", AddressPresentation.condensed("about:blank"))
    }
}
