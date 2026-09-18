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
        assertEquals(expected, GoreeCloudSearchContract.selectCapability(listOf(unrelated, expected)))
    }

    @Test
    fun unacceptedCapabilityFailsClosedBeforeRequestConstruction() {
        val decision = GoreeCloudSearchContract.authorize(
            query = "privacy browser",
            capability = capability(productionAccepted = false),
            privacyAuthorization = authorization(),
            requesterAuthentication = requesterAuthentication(),
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
            capability = capability(privacyAuthorizationEnforcement = "not_enforced_development"),
            privacyAuthorization = authorization(),
            requesterAuthentication = requesterAuthentication(),
        )
        assertEquals(
            GoreeCloudSearchContract.Decision.Rejected(
                GoreeCloudSearchContract.RejectionReason.INCOMPATIBLE_CAPABILITY,
            ),
            decision,
        )
    }

    @Test
    fun capabilityWithoutAuthenticatedRequesterRequirementFailsClosed() {
        assertFalse(GoreeCloudSearchContract.isCompatible(capability(authenticatedRequesterRequired = false)))
    }

    @Test
    fun mismatchedRequesterAuthenticationCarrierFailsClosed() {
        assertFalse(GoreeCloudSearchContract.isCompatible(capability(authenticatedRequesterAuthority = "untrusted-authority")))
        assertFalse(GoreeCloudSearchContract.isCompatible(capability(authenticatedRequesterScheme = "basic")))
        assertFalse(GoreeCloudSearchContract.isCompatible(capability(authenticatedRequesterHeader = "X-GoreeCloud-Requester")))
    }

    @Test
    fun missingPrivacyAuthorizationFailsClosed() {
        val decision = GoreeCloudSearchContract.authorize(
            query = "privacy browser",
            capability = capability(),
            privacyAuthorization = GoreeCloudSearchContract.PrivacyAuthorization(accepted = false),
            requesterAuthentication = requesterAuthentication(),
        )
        assertEquals(
            GoreeCloudSearchContract.Decision.Rejected(
                GoreeCloudSearchContract.RejectionReason.PRIVACY_AUTHORIZATION_REQUIRED,
            ),
            decision,
        )
    }

    @Test
    fun noncanonicalPrivacyAuthorizationReferenceFailsClosed() {
        val decision = GoreeCloudSearchContract.authorize(
            query = "privacy browser",
            capability = capability(),
            privacyAuthorization = GoreeCloudSearchContract.PrivacyAuthorization(
                accepted = true,
                reference = "privacy-shield:capability:test",
            ),
            requesterAuthentication = requesterAuthentication(),
        )
        assertEquals(
            GoreeCloudSearchContract.Decision.Rejected(
                GoreeCloudSearchContract.RejectionReason.PRIVACY_AUTHORIZATION_REQUIRED,
            ),
            decision,
        )
    }

    @Test
    fun missingRequesterAuthenticationFailsClosed() {
        val decision = GoreeCloudSearchContract.authorize(
            query = "privacy browser",
            capability = capability(),
            privacyAuthorization = authorization(),
        )
        assertEquals(
            GoreeCloudSearchContract.Decision.Rejected(
                GoreeCloudSearchContract.RejectionReason.REQUESTER_AUTHENTICATION_REQUIRED,
            ),
            decision,
        )
    }

    @Test
    fun malformedRequesterAuthenticationFailsClosed() {
        val malformed = listOf("", "opaque credential", "opaque\u0000credential")
        for (credential in malformed) {
            val decision = GoreeCloudSearchContract.authorize(
                query = "privacy browser",
                capability = capability(),
                privacyAuthorization = authorization(),
                requesterAuthentication = GoreeCloudSearchContract.RequesterAuthentication(credential),
            )
            assertEquals(
                GoreeCloudSearchContract.Decision.Rejected(
                    GoreeCloudSearchContract.RejectionReason.REQUESTER_AUTHENTICATION_REQUIRED,
                ),
                decision,
            )
        }
    }

    @Test
    fun whitespaceCapabilityReferenceFailsClosed() {
        assertFalse(GoreeCloudSearchContract.isCanonicalPrivacyCapabilityReference("psc_test capability"))
    }

    @Test
    fun controlCharacterCapabilityReferenceFailsClosed() {
        assertFalse(GoreeCloudSearchContract.isCanonicalPrivacyCapabilityReference("psc_test\u0000capability"))
    }

    @Test
    fun oversizedCapabilityReferenceFailsClosed() {
        val oversized = "psc_" + "a".repeat(GoreeCloudSearchContract.PRIVACY_CAPABILITY_REFERENCE_MAX_LENGTH)
        assertFalse(GoreeCloudSearchContract.isCanonicalPrivacyCapabilityReference(oversized))
    }

    @Test
    fun acceptedEvidenceBuildsPostBodyRequestWithoutQueryInEndpoint() {
        val decision = GoreeCloudSearchContract.authorize(
            query = " privacy browser ",
            capability = capability(maxResults = 50),
            privacyAuthorization = GoreeCloudSearchContract.PrivacyAuthorization(
                accepted = true,
                reference = " psc_test ",
            ),
            requesterAuthentication = requesterAuthentication(),
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
        assertEquals("psc_test", request.authorizationReference)
        assertEquals("Authorization", request.requesterAuthorizationHeader)
        assertEquals("Bearer opaque.identity.credential", request.requesterAuthorizationValue)
        assertFalse(request.endpoint.contains("privacy"))
        assertFalse(request.endpoint.contains("?q="))
        assertFalse(request.toString().contains("opaque.identity.credential"))
        assertFalse(request.toString().contains("psc_test"))
        assertFalse(request.toString().contains("privacy browser"))
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
        reference = "psc_test",
    )

    private fun requesterAuthentication() = GoreeCloudSearchContract.RequesterAuthentication(
        bearerCredential = "opaque.identity.credential",
    )

    private fun capability(
        productionAccepted: Boolean = true,
        methods: Set<String> = setOf("POST", "GET"),
        preferredMethod: String = "POST",
        preferredQueryTransport: String = "json_body",
        privacyAuthorizationEnforcement: String = "required",
        authenticatedRequesterRequired: Boolean = true,
        authenticatedRequesterAuthority: String = GoreeCloudSearchContract.REQUESTER_AUTHENTICATION_AUTHORITY,
        authenticatedRequesterScheme: String = GoreeCloudSearchContract.REQUESTER_AUTHENTICATION_SCHEME,
        authenticatedRequesterHeader: String = GoreeCloudSearchContract.REQUESTER_AUTHENTICATION_HEADER,
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
        authenticatedRequesterRequired = authenticatedRequesterRequired,
        maxRequestBytes = 16 * 1024,
        maxResults = maxResults,
        authenticatedRequesterAuthority = authenticatedRequesterAuthority,
        authenticatedRequesterScheme = authenticatedRequesterScheme,
        authenticatedRequesterHeader = authenticatedRequesterHeader,
    )
}
