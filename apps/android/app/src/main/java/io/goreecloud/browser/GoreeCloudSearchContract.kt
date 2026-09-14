package io.goreecloud.browser

/**
 * Browser-side executable boundary for GoreeCloud Search delegation.
 *
 * This class does not perform network I/O. It validates independently accepted
 * capability evidence and Privacy Shield authorization, then produces the only
 * remote request shape Browser is allowed to hand to a transport adapter.
 */
object GoreeCloudSearchContract {
    const val CAPABILITY_ID = "search.query"
    const val CONTRACT_VERSION = "1"
    const val DISCOVERY_ENDPOINT = "/api/v1/status"
    const val DISCOVERY_COLLECTION = "capability_evidence"
    const val ENDPOINT = "/api/v1/search"
    const val SEARCH_ORIGIN = "https://search.goreecloud.com"
    const val METHOD = "POST"
    const val QUERY_TRANSPORT = "json_body"
    const val MEDIA_TYPE = "application/json"
    const val PRIVACY_AUTHORIZATION_SCHEME = "privacy_shield_capability_token_reference"
    const val PRIVACY_AUTHORIZATION_HEADER = "X-GoreeCloud-Privacy-Capability"
    const val PRIVACY_AUTHORIZATION_ENFORCEMENT = "required"
    const val MAX_REQUEST_BYTES = 16 * 1024
    const val PRIVACY_CAPABILITY_REFERENCE_MAX_LENGTH = 512
    private const val GENERAL_CATEGORY = "general"
    private const val PRIVACY_CAPABILITY_REFERENCE_PREFIX = "psc_"

    data class CapabilityEvidence(
        val id: String,
        val contractVersion: String,
        val authoritative: Boolean,
        val current: Boolean,
        val productionAccepted: Boolean,
        val endpoint: String,
        val discoveryEndpoint: String,
        val discoveryCollection: String,
        val methods: Set<String>,
        val preferredMethod: String,
        val preferredQueryTransport: String,
        val requestMediaType: String,
        val responseMediaType: String,
        val privacyAuthorizationRequired: Boolean,
        val privacyAuthorizationScheme: String,
        val privacyAuthorizationHeader: String,
        val privacyAuthorizationEnforcement: String,
        val authenticatedRequesterRequired: Boolean,
        val maxRequestBytes: Int,
        val maxResults: Int,
    )

    data class PrivacyAuthorization(
        val accepted: Boolean,
        /** Reference to the Privacy Shield capability token, not raw policy state. */
        val reference: String? = null,
    )

    data class PostRequest(
        val endpoint: String,
        val method: String,
        val mediaType: String,
        val query: String,
        val category: String,
        val limit: Int,
        val authorizationHeader: String,
        val authorizationReference: String,
    )

    sealed interface Decision {
        data class Allowed(val request: PostRequest) : Decision
        data class Rejected(val reason: RejectionReason) : Decision
    }

    enum class RejectionReason {
        EMPTY_QUERY,
        PRIVACY_AUTHORIZATION_REQUIRED,
        INCOMPATIBLE_CAPABILITY,
    }

    /**
     * Selects the one unambiguous Search query capability from a discovery
     * collection. Missing or duplicate records fail closed.
     */
    fun selectCapability(capabilities: List<CapabilityEvidence>): CapabilityEvidence? {
        val matches = capabilities.filter { it.id == CAPABILITY_ID }
        return matches.singleOrNull()
    }

    fun authorize(
        query: String,
        capability: CapabilityEvidence,
        privacyAuthorization: PrivacyAuthorization,
        requestedLimit: Int = 20,
    ): Decision {
        val normalizedQuery = query.trim()
        if (normalizedQuery.isEmpty()) return Decision.Rejected(RejectionReason.EMPTY_QUERY)

        val authorizationReference = privacyAuthorization.reference?.trim()
        if (
            !privacyAuthorization.accepted ||
            authorizationReference.isNullOrEmpty() ||
            !isCanonicalPrivacyCapabilityReference(authorizationReference)
        ) {
            return Decision.Rejected(RejectionReason.PRIVACY_AUTHORIZATION_REQUIRED)
        }
        if (!isCompatible(capability)) {
            return Decision.Rejected(RejectionReason.INCOMPATIBLE_CAPABILITY)
        }

        return Decision.Allowed(
            PostRequest(
                endpoint = ENDPOINT,
                method = METHOD,
                mediaType = MEDIA_TYPE,
                query = normalizedQuery,
                category = GENERAL_CATEGORY,
                limit = requestedLimit.coerceIn(1, capability.maxResults),
                authorizationHeader = PRIVACY_AUTHORIZATION_HEADER,
                authorizationReference = authorizationReference,
            ),
        )
    }

    fun isCompatible(capability: CapabilityEvidence): Boolean =
        capability.id == CAPABILITY_ID &&
            capability.contractVersion == CONTRACT_VERSION &&
            capability.authoritative &&
            capability.current &&
            capability.productionAccepted &&
            capability.discoveryEndpoint == DISCOVERY_ENDPOINT &&
            capability.discoveryCollection == DISCOVERY_COLLECTION &&
            capability.endpoint == ENDPOINT &&
            METHOD in capability.methods &&
            capability.preferredMethod == METHOD &&
            capability.preferredQueryTransport == QUERY_TRANSPORT &&
            capability.requestMediaType == MEDIA_TYPE &&
            capability.responseMediaType == MEDIA_TYPE &&
            capability.privacyAuthorizationRequired &&
            capability.privacyAuthorizationScheme == PRIVACY_AUTHORIZATION_SCHEME &&
            capability.privacyAuthorizationHeader == PRIVACY_AUTHORIZATION_HEADER &&
            capability.privacyAuthorizationEnforcement == PRIVACY_AUTHORIZATION_ENFORCEMENT &&
            capability.authenticatedRequesterRequired &&
            capability.maxRequestBytes == MAX_REQUEST_BYTES &&
            capability.maxResults >= 1

    fun isCanonicalPrivacyCapabilityReference(reference: String): Boolean =
        reference.startsWith(PRIVACY_CAPABILITY_REFERENCE_PREFIX) &&
            reference.length > PRIVACY_CAPABILITY_REFERENCE_PREFIX.length &&
            reference.length <= PRIVACY_CAPABILITY_REFERENCE_MAX_LENGTH &&
            reference.none { character ->
                character.isWhitespace() || Character.isISOControl(character.code)
            }
}
