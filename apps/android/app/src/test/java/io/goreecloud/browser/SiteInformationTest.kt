package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SiteInformationTest {
    @Test
    fun httpsSiteReportsOriginWithoutClaimingTrustVerdict() {
        val info = SiteInformation.forUrl("https://example.com:443/path")

        assertTrue(info.isWebsite)
        assertEquals("example.com", info.displayOrigin)
        assertEquals("HTTPS", info.transportLabel)
        assertTrue(info.details.contains("does not replace certificate details"))
    }

    @Test
    fun httpSiteMakesUnencryptedTransportExplicit() {
        val info = SiteInformation.forUrl("http://example.com:8080/path")

        assertTrue(info.isWebsite)
        assertEquals("example.com:8080", info.displayOrigin)
        assertEquals("HTTP — not encrypted", info.transportLabel)
    }

    @Test
    fun internalPageDoesNotPresentWebsiteOrigin() {
        val info = SiteInformation.forUrl("goreecloud://start")

        assertFalse(info.isWebsite)
        assertEquals("Browser-owned local page", info.displayOrigin)
        assertEquals("No website connection", info.transportLabel)
        assertEquals("Development build", info.menuSubtitle)
    }
}
