package io.goreecloud.browser

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class NavigationIntentSensitiveRenderingTest {
    @Test
    fun searchIntentDoesNotRenderTypedQuery() {
        val query = "private typed search"
        val rendered = NavigationResolver.Intent.Search(query).toString()

        assertFalse(rendered.contains(query))
        assertTrue(rendered.contains("query=<redacted>"))
    }

    @Test
    fun navigationIntentDoesNotRenderBrowsingUrl() {
        val url = "https://example.com/private/path"
        val rendered = NavigationResolver.Intent.Navigate(url).toString()

        assertFalse(rendered.contains(url))
        assertTrue(rendered.contains("url=<redacted>"))
    }

    @Test
    fun blockedIntentDoesNotRenderRejectedTypedInput() {
        val input = "javascript:private-input"
        val rendered = NavigationResolver.Intent.Blocked(input).toString()

        assertFalse(rendered.contains(input))
        assertTrue(rendered.contains("input=<redacted>"))
    }
}
