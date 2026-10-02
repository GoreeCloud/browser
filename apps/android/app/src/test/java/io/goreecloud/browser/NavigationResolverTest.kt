package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class NavigationResolverTest {
    @Test
    fun emptyInputUsesHomeIntentAndSearchHomeCompatibilityUrl() {
        assertEquals(NavigationResolver.Intent.Home, NavigationResolver.classify("   "))
        assertEquals(NavigationResolver.SEARCH_HOME, NavigationResolver.resolve("   "))
    }

    @Test
    fun directHttpsNavigationRemainsIndependentFromSearch() {
        val expected = "https://example.com/path?q=1"
        assertEquals(
            NavigationResolver.Intent.Navigate(expected),
            NavigationResolver.classify(expected),
        )
        assertEquals(expected, NavigationResolver.resolve(expected))
    }

    @Test
    fun bareHostUpgradesToHttpsWithoutBecomingSearch() {
        assertEquals(
            NavigationResolver.Intent.Navigate("https://example.com"),
            NavigationResolver.classify("example.com"),
        )
        assertEquals("https://example.com", NavigationResolver.resolve("example.com"))
    }

    @Test
    fun bareHostWithPortRemainsDirectNavigation() {
        assertEquals(
            NavigationResolver.Intent.Navigate("https://example.com:8443/path"),
            NavigationResolver.classify("example.com:8443/path"),
        )
    }

    @Test
    fun unicodeHttpHostUsesCanonicalAsciiALabel() {
        assertEquals(
            NavigationResolver.Intent.Navigate(
                "https://xn--r8jz45g.xn--zckzah/path?q=1#section",
            ),
            NavigationResolver.classify("https://例え.テスト/path?q=1#section"),
        )
    }

    @Test
    fun schemeLessUnicodeHostUsesCanonicalAsciiALabel() {
        assertEquals(
            NavigationResolver.Intent.Navigate(
                "https://xn--r8jz45g.xn--zckzah/path",
            ),
            NavigationResolver.classify("例え.テスト/path"),
        )
    }

    @Test
    fun invalidStd3AndBracketedNonIpv6NavigationFailClosed() {
        for (raw in listOf(
            "https://exa_mple.com/path",
            "https://[example.com]/path",
            "https://[127.0.0.1]/path",
        )) {
            assertEquals(NavigationResolver.Intent.Blocked(raw), NavigationResolver.classify(raw))
            assertFalse(NavigationResolver.isAllowedWebUrl(raw))
        }
    }

    @Test
    fun textQueryIsClassifiedWithoutConstructingRemoteSearchUrl() {
        val intent = NavigationResolver.classify(" privacy browser ")
        assertEquals(
            NavigationResolver.Intent.Search(query = "privacy browser"),
            intent,
        )
        assertEquals("", NavigationResolver.resolve("privacy browser"))
    }

    @Test
    fun leadingAndTrailingControlCharactersCannotBypassOmniboxValidation() {
        for (raw in listOf("\nexample.com", "example.com\r", "\tprivacy browser", "privacy browser\u007f")) {
            assertEquals(NavigationResolver.Intent.Blocked(raw), NavigationResolver.classify(raw))
            assertEquals("", NavigationResolver.resolve(raw))
        }
    }

    @Test
    fun credentialBearingWebUrlFailsClosedInsteadOfNavigatingOrSearching() {
        val raw = "https://user:pass@example.com/private"
        assertEquals(NavigationResolver.Intent.Blocked(raw), NavigationResolver.classify(raw))
        assertEquals("", NavigationResolver.resolve(raw))
        assertFalse(NavigationResolver.isAllowedWebUrl(raw))
    }

    @Test
    fun malformedHttpUrlFailsClosedInsteadOfBecomingSearch() {
        val raw = "https://"
        assertEquals(NavigationResolver.Intent.Blocked(raw), NavigationResolver.classify(raw))
        assertEquals("", NavigationResolver.resolve(raw))
    }

    @Test
    fun explicitUnsupportedSchemesFailClosedInsteadOfBecomingSearch() {
        for (raw in listOf(
            "file:///sdcard/example.html",
            "javascript:alert(1)",
            "intent://example",
            "mailto:user@example.com",
        )) {
            assertEquals(NavigationResolver.Intent.Blocked(raw), NavigationResolver.classify(raw))
            assertEquals("", NavigationResolver.resolve(raw))
        }
    }

    @Test
    fun alternateSchemesAreNotAcceptedAsWebNavigation() {
        assertTrue(NavigationResolver.isAllowedWebUrl("https://example.com"))
        assertTrue(NavigationResolver.isAllowedWebUrl("http://example.com"))
        assertFalse(NavigationResolver.isAllowedWebUrl("file:///sdcard/example.html"))
        assertFalse(NavigationResolver.isAllowedWebUrl("javascript:alert(1)"))
        assertFalse(NavigationResolver.isAllowedWebUrl("intent://example"))
        assertFalse(NavigationResolver.isAllowedWebUrl("\nhttps://example.com"))
        assertFalse(NavigationResolver.isAllowedWebUrl("https://example.com\r"))
    }

    @Test
    fun zeroAndOutOfRangePortsFailClosed() {
        for (raw in listOf(
            "https://example.com:0/",
            "https://example.com:65536/",
            "example.com:0/path",
            "example.com:65536/path",
        )) {
            assertEquals(NavigationResolver.Intent.Blocked(raw), NavigationResolver.classify(raw))
            assertFalse(NavigationResolver.isAllowedWebUrl(raw))
        }
    }

    @Test
    fun ambiguousOrInvalidNumericHostsFailClosed() {
        for (raw in listOf(
            "https://127.1/",
            "https://127.01.0.1/",
            "https://999.1.1.1/",
            "999.1.1.1",
        )) {
            assertEquals(NavigationResolver.Intent.Blocked(raw), NavigationResolver.classify(raw))
            assertFalse(NavigationResolver.isAllowedWebUrl(raw))
        }

        assertTrue(NavigationResolver.isAllowedWebUrl("https://127.0.0.1/"))
        assertEquals(
            "https://127.0.0.1/",
            NavigationResolver.canonicalizeWebUrl("https://127.0.0.1/"),
        )
    }
}
