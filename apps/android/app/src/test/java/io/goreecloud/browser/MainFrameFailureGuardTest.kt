package io.goreecloud.browser

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class MainFrameFailureGuardTest {
    @Test
    fun activeNavigationCallbackIsAccepted() {
        assertTrue(
            MainFrameFailureGuard.shouldAccept(
                "https://example.com/path?q=1",
                "https://example.com/path?q=1",
            ),
        )
    }

    @Test
    fun hostCaseDefaultPortAndFragmentDoNotCreateFalseStaleResult() {
        assertTrue(
            MainFrameFailureGuard.shouldAccept(
                "HTTPS://Example.COM/path?q=1#section",
                "https://example.com:443/path?q=1",
            ),
        )
    }

    @Test
    fun staleCallbackFromPreviousNavigationIsIgnored() {
        assertFalse(
            MainFrameFailureGuard.shouldAccept(
                "https://new.example/page",
                "https://old.example/page",
            ),
        )
    }

    @Test
    fun differentPathOrQueryIsNotTheActiveNavigation() {
        assertFalse(
            MainFrameFailureGuard.shouldAccept(
                "https://example.com/new?q=2",
                "https://example.com/old?q=1",
            ),
        )
    }

    @Test
    fun missingOrNonWebCallbackCannotMutateCurrentSurface() {
        assertFalse(MainFrameFailureGuard.shouldAccept("https://example.com/", null))
        assertFalse(MainFrameFailureGuard.shouldAccept("goreecloud://start", "https://example.com/"))
        assertFalse(MainFrameFailureGuard.shouldAccept("https://example.com/", "data:text/html,error"))
    }
}
