package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class BrowserProtectionStatusTest {
    @Test
    fun statusSurfaceExposesEnforcedAndFailClosedBoundaries() {
        val entries = BrowserProtectionStatus.current(safeBrowsingEnabled = true)
        val byLabel = entries.associateBy { it.label }

        assertEquals("Blocked", byLabel.getValue("Third-party cookies").state)
        assertEquals("Blocked", byLabel.getValue("Mixed content").state)
        assertEquals("Denied by default", byLabel.getValue("Website permissions").state)
        assertEquals("Fail-closed", byLabel.getValue("Downloads").state)
        assertEquals("Fail-closed", byLabel.getValue("Web search").state)
        assertEquals("Enabled", byLabel.getValue("Android Safe Browsing").state)
        assertEquals(entries.size, byLabel.size)
    }

    @Test
    fun safeBrowsingStatusDoesNotManufactureUnavailablePlatformState() {
        val entry = BrowserProtectionStatus.current(safeBrowsingEnabled = false)
            .single { it.label == "Android Safe Browsing" }

        assertEquals("Unavailable on this platform", entry.state)
        assertTrue(entry.details.contains("cannot claim"))
    }
}
