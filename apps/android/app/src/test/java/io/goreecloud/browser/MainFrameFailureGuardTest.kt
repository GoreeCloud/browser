package io.goreecloud.browser

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class MainFrameFailureGuardTest {
    @Test
    fun activeNavigationFailureIsPresented() {
        assertTrue(
            MainFrameFailureGuard.shouldPresent(
                "https://example.com/path?q=1",
                "https://example.com/path?q=1",
            ),
        )
    }

    @Test
    fun hostCaseDefaultPortAndFragmentDoNotCreateFalseStaleResult() {
        assertTrue(
            MainFrameFailureGuard.shouldPresent(
                "HTTPS://Example.COM/path?q=1#section",
                "https://example.com:443/path?q=1",
            ),
        )
    }

    @Test
    fun staleFailureFromPreviousNavigationIsIgnored() {
        assertFalse(
            MainFrameFailureGuard.shouldPresent(
                "https://new.example/page",
                "https://old.example/page",
            ),
        )
    }

    @Test
    fun differentPathOrQueryIsNotTheActiveNavigation() {
        assertFalse(
            MainFrameFailureGuard.shouldPresent(
                "https://example.com/new?q=2",
                "https://example.com/old?q=1",
            ),
        )
    }

    @Test
    fun missingOrNonWebFailureCannotTriggerRecoverySurface() {
        assertFalse(MainFrameFailureGuard.shouldPresent("https://example.com/", null))
        assertFalse(MainFrameFailureGuard.shouldPresent("goreecloud://start", "https://example.com/"))
        assertFalse(MainFrameFailureGuard.shouldPresent("https://example.com/", "data:text/html,error"))
    }
}
