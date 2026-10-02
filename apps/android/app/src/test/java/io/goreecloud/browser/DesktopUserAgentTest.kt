package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class DesktopUserAgentTest {
    @Test
    fun desktopSiteUserAgentKeepsEngineVersionAndRemovesMobileTokens() {
        val mobile = "Mozilla/5.0 (Linux; Android 15; Pixel Build/AP3A; wv) " +
            "AppleWebKit/537.36 Version/4.0 Chrome/152.0.7977.83 Mobile Safari/537.36 " +
            "GoreeCloudBrowser/0.1.0-beta.1+android.10 Android"

        val desktop = DesktopUserAgent.fromMobile(mobile)

        assertTrue(desktop.startsWith("Mozilla/5.0 (X11; Linux x86_64)"))
        assertTrue(desktop.contains("Chrome/152.0.7977.83"))
        assertTrue(desktop.contains("GoreeCloudBrowser/0.1.0-beta.1+android.10 DesktopSite"))
        assertFalse(desktop.contains("Android 15"))
        assertFalse(desktop.contains(" Mobile "))
        assertFalse(desktop.contains("; wv"))
        assertFalse(desktop.contains("Version/4.0"))
    }

    @Test
    fun whitespaceIsNormalizedWithoutInventingAnEngineVersion() {
        assertEquals("custom agent", DesktopUserAgent.fromMobile("  custom   agent  "))
    }
}
