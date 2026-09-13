package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class NavigationResolverTest {
    @Test
    fun emptyInputUsesGoreeCloudSearchHome() {
        assertEquals(NavigationResolver.SEARCH_HOME, NavigationResolver.resolve("   "))
    }

    @Test
    fun directHttpsNavigationRemainsIndependentFromSearch() {
        assertEquals(
            "https://example.com/path?q=1",
            NavigationResolver.resolve("https://example.com/path?q=1"),
        )
    }

    @Test
    fun directUnicodeHostUsesCanonicalAsciiALabel() {
        val address = "https://例え.テスト/path"
        assertTrue(NavigationResolver.isAllowedWebUrl(address))
        assertEquals(
            "https://xn--r8jz45g.xn--zckzah/path",
            NavigationResolver.resolve(address),
        )
    }

    @Test
    fun schemeLessUnicodeHostUsesCanonicalAsciiALabel() {
        assertEquals(
            "https://xn--r8jz45g.xn--zckzah/path",
            NavigationResolver.resolve("例え.テスト/path"),
        )
    }

    @Test
    fun malformedExplicitHttpNavigationFailsClosedToSearchHome() {
        val invalid = listOf(
            "https://",
            "https://not a url",
            "https://example.com:bad",
            "https://example.com:65536",
            "https://:443/path",
            "https://user@",
            "https://exa_mple.com/path",
            "https://example.com/\nnext",
        )
        invalid.forEach { address ->
            assertFalse("expected rejection for $address", NavigationResolver.isAllowedWebUrl(address))
            assertEquals(NavigationResolver.SEARCH_HOME, NavigationResolver.resolve(address))
        }
    }

    @Test
    fun explicitPortRangeIsValidated() {
        assertTrue(NavigationResolver.isAllowedWebUrl("https://example.com:0/path"))
        assertTrue(NavigationResolver.isAllowedWebUrl("https://example.com:65535/path"))
        assertFalse(NavigationResolver.isAllowedWebUrl("https://example.com:-1/path"))
        assertFalse(NavigationResolver.isAllowedWebUrl("https://example.com:65536/path"))
    }

    @Test
    fun explicitHttpUserinfoFailsClosedToSearchHome() {
        val address = "https://user:pass@example.com/path"
        assertFalse(NavigationResolver.isAllowedWebUrl(address))
        assertEquals(NavigationResolver.SEARCH_HOME, NavigationResolver.resolve(address))
    }

    @Test
    fun schemeLessUserinfoFailsClosedWithoutSearchForwarding() {
        val inputs = listOf(
            "user@example.com",
            "user:secret@example.com/private",
            "example.com@evil.test/path",
        )

        inputs.forEach { input ->
            assertEquals(NavigationResolver.SEARCH_HOME, NavigationResolver.resolve(input))
        }
    }

    @Test
    fun ordinaryAtTextWithWhitespaceRemainsSearchText() {
        assertEquals(
            "https://search.goreecloud.com/search?q=contact%20user%40example.com",
            NavigationResolver.resolve("contact user@example.com"),
        )
    }

    @Test
    fun bareHostUpgradesToHttps() {
        assertEquals("https://example.com", NavigationResolver.resolve("example.com"))
    }

    @Test
    fun bareLocalhostWithPortUpgradesToHttps() {
        assertEquals(
            "https://localhost:8080/path",
            NavigationResolver.resolve("localhost:8080/path"),
        )
    }

    @Test
    fun bareBracketedIpv6WithPortUpgradesToHttps() {
        assertEquals(
            "https://[::1]:8080/path",
            NavigationResolver.resolve("[::1]:8080/path"),
        )
    }

    @Test
    fun invalidBarePortIsTreatedAsSearchText() {
        assertEquals(
            "https://search.goreecloud.com/search?q=example.com%3Abad",
            NavigationResolver.resolve("example.com:bad"),
        )
    }

    @Test
    fun textQueryUsesOnlyGoreeCloudSearch() {
        assertEquals(
            "https://search.goreecloud.com/search?q=privacy%20browser",
            NavigationResolver.resolve("privacy browser"),
        )
    }

    @Test
    fun alternateSchemesAreNotAcceptedAsWebNavigation() {
        assertTrue(NavigationResolver.isAllowedWebUrl("https://example.com"))
        assertTrue(NavigationResolver.isAllowedWebUrl("http://example.com"))
        assertFalse(NavigationResolver.isAllowedWebUrl("file:///sdcard/example.html"))
        assertFalse(NavigationResolver.isAllowedWebUrl("javascript:alert(1)"))
        assertFalse(NavigationResolver.isAllowedWebUrl("intent://example"))
    }
}
