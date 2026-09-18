package io.goreecloud.browser

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class GoreeCloudSearchSensitiveRenderingTest {
    @Test
    fun postRequestDebugRenderingRedactsQueryAndCapabilityReference() {
        val query = "private health research query"
        val reference = "psc_sensitive_authority_reference"
        val requesterCredential = "sensitive_identity_requester_credential"
        val request = GoreeCloudSearchContract.PostRequest(
            endpoint = GoreeCloudSearchContract.ENDPOINT,
            method = GoreeCloudSearchContract.METHOD,
            mediaType = GoreeCloudSearchContract.MEDIA_TYPE,
            query = query,
            category = "general",
            limit = 10,
            authorizationHeader = GoreeCloudSearchContract.PRIVACY_AUTHORIZATION_HEADER,
            authorizationReference = reference,
            requesterAuthorizationHeader = GoreeCloudSearchContract.REQUESTER_AUTHENTICATION_HEADER,
            requesterAuthorizationValue = "Bearer $requesterCredential",
        )
        val rendered = request.toString()

        assertFalse(rendered.contains(query))
        assertFalse(rendered.contains(reference))
        assertFalse(rendered.contains(requesterCredential))
        assertTrue(rendered.contains("query=<redacted>"))
        assertTrue(rendered.contains("authorizationReference=<redacted>"))
        assertTrue(rendered.contains("requesterAuthorizationValue=<redacted>"))
    }

    @Test
    fun privacyAuthorizationDebugRenderingRedactsCapabilityReference() {
        val reference = "psc_sensitive_authority_reference"
        val rendered = GoreeCloudSearchContract.PrivacyAuthorization(
            accepted = true,
            reference = reference,
        ).toString()

        assertFalse(rendered.contains(reference))
        assertTrue(rendered.contains("reference=<redacted>"))
    }

    @Test
    fun allowedDecisionRenderingDoesNotReExposeNestedSensitiveValues() {
        val query = "private health research query"
        val reference = "psc_sensitive_authority_reference"
        val requesterCredential = "sensitive_identity_requester_credential"
        val rendered = GoreeCloudSearchContract.Decision.Allowed(
            GoreeCloudSearchContract.PostRequest(
                endpoint = GoreeCloudSearchContract.ENDPOINT,
                method = GoreeCloudSearchContract.METHOD,
                mediaType = GoreeCloudSearchContract.MEDIA_TYPE,
                query = query,
                category = "general",
                limit = 10,
                authorizationHeader = GoreeCloudSearchContract.PRIVACY_AUTHORIZATION_HEADER,
                authorizationReference = reference,
                requesterAuthorizationHeader = GoreeCloudSearchContract.REQUESTER_AUTHENTICATION_HEADER,
                requesterAuthorizationValue = "Bearer $requesterCredential",
            ),
        ).toString()

        assertFalse(rendered.contains(query))
        assertFalse(rendered.contains(reference))
        assertFalse(rendered.contains(requesterCredential))
    }
}
