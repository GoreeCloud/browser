package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class BrowserFailureStatePolicyTest {
    @Test
    fun persistsOnlyCanonicalNavigationApprovedRetryUrls() {
        assertEquals(
            "https://xn--r8jz45g.xn--zckzah/path?retry=1",
            BrowserFailureStatePolicy.persistableRetryUrl(
                "https://例え.テスト/path?retry=1",
            ),
        )
        assertEquals(
            "http://localhost:8080/status",
            BrowserFailureStatePolicy.persistableRetryUrl(
                "http://localhost:8080/status",
            ),
        )
    }

    @Test
    fun rejectsMissingLocalCredentialAndMalformedRetryState() {
        for (url in listOf(
            null,
            "",
            "https://",
            "https://example.com:70000/",
            "https://user:secret@example.com/private",
            "goreecloud://start",
            "data:text/html,<p>local</p>",
            "file:///tmp/page.html",
        )) {
            assertNull(BrowserFailureStatePolicy.persistableRetryUrl(url))
        }
    }

    @Test
    fun restoresOnlyOnBrowserLocalDocumentWithoutBlockedLinkAuthority() {
        val retry = "https://example.com/retry"

        assertEquals(
            retry,
            BrowserFailureStatePolicy.restorable(
                retryUrl = retry,
                restoredDocumentIsBrowserLocal = true,
                blockedWebNavigationVisible = false,
            )?.retryUrl,
        )
        assertNull(
            BrowserFailureStatePolicy.restorable(
                retryUrl = retry,
                restoredDocumentIsBrowserLocal = false,
                blockedWebNavigationVisible = false,
            ),
        )
        assertNull(
            BrowserFailureStatePolicy.restorable(
                retryUrl = retry,
                restoredDocumentIsBrowserLocal = true,
                blockedWebNavigationVisible = true,
            ),
        )
    }

    @Test
    fun failureTitleIsBrowserOwnedAndFixed() {
        assertEquals("Page unavailable", BrowserFailureStatePolicy.PAGE_UNAVAILABLE_TITLE)
    }
}
