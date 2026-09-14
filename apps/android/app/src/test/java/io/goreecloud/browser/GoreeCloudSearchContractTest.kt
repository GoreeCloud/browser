package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class GoreeCloudSearchContractTest {
    @Test
    fun duplicateQueryCapabilitiesFailDiscoveryClosed() {
        val evidence = capability()
        assertNull(GoreeCloudSearchContract.selectCapability(listOf(evidence, evidence)))
    }

    @Test
    fun oneQueryCapabilityIsSelectedFromMixedDiscoveryCollection() {
        val expected = capability()
        val unrelated = expected.copy(id = "search.images")
        assertEquals(
            expected,
            GoreeCloudSearchContract.selectCapability(listOf(unrelated, expected)),
        )
    }

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
    fun developmentServerAuthorizationEvidenceCannotBeMistakenForEnforcedProductionTransport() {
        val decision = GoreeCloudSearchContract.authorize(
            query = "privacy browser",
            capability = capability(
                privacyAuthorizationEnforcement = "not_enforced_development",
            ),
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
        assertEquals("X-GoreeCloud-Privacy-Capability", request.authorizationHeader)
        assertEquals("privacy-shield:capability:test", request.authorizationReference)
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
        reference = "privacy-shield:capability:test",
    )

    private fun capability(
        productionAccepted: Boolean = true,
        methods: Set<String> = setOf("POST", "GET"),
        preferredMethod: String = "POST",
        preferredQueryTransport: String = "json_body",
        privacyAuthorizationEnforcement: String = "required",
        maxResults: Int = 100,
    ) = GoreeCloudSearchContract.CapabilityEvidence(
        id = "search.query",
        contractVersion = "1",
        authoritative = true,
        current = true,
        productionAccepted = productionAccepted,
        endpoint = "/api/v1/search",
        discoveryEndpoint = "/api/v1/status",
        discoveryCollection = "capability_evidence",
        methods = methods,
        preferredMethod = preferredMethod,
        preferredQueryTransport = preferredQueryTransport,
        requestMediaType = "application/json",
        responseMediaType = "application/json",
        privacyAuthorizationRequired = true,
        privacyAuthorizationScheme = "privacy_shield_capability_token_reference",
        privacyAuthorizationHeader = "X-GoreeCloud-Privacy-Capability",
        privacyAuthorizationEnforcement = privacyAuthorizationEnforcement,
        maxRequestBytes = 16 * 1024,
        maxResults = maxResults,
    )
}
