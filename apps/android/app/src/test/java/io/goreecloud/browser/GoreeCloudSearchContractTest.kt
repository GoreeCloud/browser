package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class GoreeCloudSearchContractTest {
    @Test
    fun unacceptedCapabilityFailsClosedBeforeRequestConstruction() {
        val decision = GoreeCloudSearchContract.authorize(
            query = "privacy browser",
            capability = capability(productionAccepted = false),
            privacyAuthorization = authorization(),
        )

        assertEquals(
            GoreeCloudSearchContract.Decision.Rejected(
                GoreeCloudSearchContract.RejectionReason.INCOMPATIBLE_CAPABILITY,
            ),
            decision,
        )
    }

    @Test
    fun missingPrivacyAuthorizationFailsClosed() {
        val decision = GoreeCloudSearchContract.authorize(
            query = "privacy browser",
            capability = capability(),
            privacyAuthorization = GoreeCloudSearchContract.PrivacyAuthorization(accepted = false),
        )

        assertEquals(
            GoreeCloudSearchContract.Decision.Rejected(
                GoreeCloudSearchContract.RejectionReason.PRIVACY_AUTHORIZATION_REQUIRED,
            ),
            decision,
        )
    }

    @Test
    fun acceptedEvidenceBuildsPostBodyRequestWithoutQueryInEndpoint() {
        val decision = GoreeCloudSearchContract.authorize(
            query = " privacy browser ",
            capability = capability(maxResults = 50),
            privacyAuthorization = authorization(),
            requestedLimit = 80,
        )

        assertTrue(decision is GoreeCloudSearchContract.Decision.Allowed)
        val request = (decision as GoreeCloudSearchContract.Decision.Allowed).request
        assertEquals("/api/v1/search", request.endpoint)
        assertEquals("POST", request.method)
        assertEquals("application/json", request.mediaType)
        assertEquals("privacy browser", request.query)
        assertEquals("general", request.category)
        assertEquals(50, request.limit)
        assertFalse(request.endpoint.contains("privacy"))
        assertFalse(request.endpoint.contains("?q="))
    }

    @Test
    fun getOrUrlQueryCapabilityIsNeverCompatible() {
        assertFalse(
            GoreeCloudSearchContract.isCompatible(
                capability(
                    methods = setOf("GET"),
                    preferredMethod = "GET",
                    preferredQueryTransport = "url_query",
                ),
            ),
        )
    }

    private fun authorization() = GoreeCloudSearchContract.PrivacyAuthorization(
        accepted = true,
        reference = "privacy-shield:test-evidence",
    )

    private fun capability(
        productionAccepted: Boolean = true,
        methods: Set<String> = setOf("POST", "GET"),
        preferredMethod: String = "POST",
        preferredQueryTransport: String = "json_body",
        maxResults: Int = 100,
    ) = GoreeCloudSearchContract.CapabilityEvidence(
        id = "search.query",
        contractVersion = "1",
        authoritative = true,
        current = true,
        productionAccepted = productionAccepted,
        endpoint = "/api/v1/search",
        methods = methods,
        preferredMethod = preferredMethod,
        preferredQueryTransport = preferredQueryTransport,
        requestMediaType = "application/json",
        responseMediaType = "application/json",
        privacyAuthorizationRequired = true,
        maxRequestBytes = 16 * 1024,
        maxResults = maxResults,
    )
}
