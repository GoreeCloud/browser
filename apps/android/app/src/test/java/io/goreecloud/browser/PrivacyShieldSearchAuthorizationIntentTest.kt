package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Test

class PrivacyShieldSearchAuthorizationIntentTest {
    @Test
    fun requestIntentContainsCanonicalPrivacyShieldDecisionFields() {
        val request = PrivacyShieldSearchAuthorization.requestIntent()

        assertEquals("goreecloud-browser", request.requesterId)
        assertEquals("application", request.requesterType)
        assertEquals("goreecloud.search.query", request.resourceId)
        assertEquals("query_text", request.resourceClassification)
        assertEquals("search.query", request.operation)
        assertEquals("internet_search", request.purpose)
        assertEquals("private_goreecloud", request.processingZone)
        assertEquals("https://search.goreecloud.com", request.destination)
        assertEquals("none", request.retentionMode)
        assertFalse(request.externalDisclosure)
    }
}
