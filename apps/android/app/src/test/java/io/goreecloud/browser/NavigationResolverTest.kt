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
    fun textQueryIsClassifiedBeforeRemoteSearchDestinationIsUsed() {
        val intent = NavigationResolver.classify(" privacy browser ")
        assertEquals(
            NavigationResolver.Intent.Search(
                query = "privacy browser",
                url = "https://search.goreecloud.com/search?q=privacy%20browser",
            ),
            intent,
        )
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
