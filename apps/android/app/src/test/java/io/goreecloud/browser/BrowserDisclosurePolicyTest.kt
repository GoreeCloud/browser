package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class BrowserDisclosurePolicyTest {
    @Test
    fun allowedWebsiteAddressCanBeDisclosed() {
        assertEquals(
            "https://example.com/path?q=1",
            BrowserDisclosurePolicy.shareablePageUrl("https://example.com/path?q=1"),
        )
    }

    @Test
    fun browserOwnedAndResourceAddressesStayInsideBrowser() {
        for (url in listOf(
            "goreecloud://start",
            "data:text/html,<h1>local</h1>",
            "file:///private/runtime/path",
            "content://example/item",
            "blob:https://example.com/opaque-id",
        )) {
            assertNull(BrowserDisclosurePolicy.shareablePageUrl(url))
        }
    }

    @Test
    fun unsafeWebsiteShapesCannotBypassDisclosurePolicy() {
        for (url in listOf(
            "https://user:pass@example.com/private",
            "https://example.com:0/",
            "https://127.1/",
        )) {
            assertNull(BrowserDisclosurePolicy.shareablePageUrl(url))
        }
    }
}
