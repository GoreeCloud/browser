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
    const val ENDPOINT = "/api/v1/search"
    const val METHOD = "POST"
    const val QUERY_TRANSPORT = "json_body"
    const val MEDIA_TYPE = "application/json"
    const val MAX_REQUEST_BYTES = 16 * 1024
    private const val GENERAL_CATEGORY = "general"

    data class CapabilityEvidence(
        val id: String,
        val contractVersion: String,
        val authoritative: Boolean,
        val current: Boolean,
        val productionAccepted: Boolean,
        val endpoint: String,
        val methods: Set<String>,
        val preferredMethod: String,
        val preferredQueryTransport: String,
        val requestMediaType: String,
        val responseMediaType: String,
        val privacyAuthorizationRequired: Boolean,
        val maxRequestBytes: Int,
        val maxResults: Int,
    )

    data class PrivacyAuthorization(
        val accepted: Boolean,
        val reference: String? = null,
    )

    data class PostRequest(
        val endpoint: String,
        val method: String,
        val mediaType: String,
        val query: String,
        val category: String,
        val limit: Int,
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

    fun authorize(
        query: String,
        capability: CapabilityEvidence,
        privacyAuthorization: PrivacyAuthorization,
        requestedLimit: Int = 20,
    ): Decision {
        val normalizedQuery = query.trim()
        if (normalizedQuery.isEmpty()) return Decision.Rejected(RejectionReason.EMPTY_QUERY)
        if (!privacyAuthorization.accepted || privacyAuthorization.reference.isNullOrBlank()) {
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
                authorizationReference = privacyAuthorization.reference,
            ),
        )
    }

    fun isCompatible(capability: CapabilityEvidence): Boolean =
        capability.id == CAPABILITY_ID &&
            capability.contractVersion == CONTRACT_VERSION &&
            capability.authoritative &&
            capability.current &&
            capability.productionAccepted &&
            capability.endpoint == ENDPOINT &&
            METHOD in capability.methods &&
            capability.preferredMethod == METHOD &&
            capability.preferredQueryTransport == QUERY_TRANSPORT &&
            capability.requestMediaType == MEDIA_TYPE &&
            capability.responseMediaType == MEDIA_TYPE &&
            capability.privacyAuthorizationRequired &&
            capability.maxRequestBytes == MAX_REQUEST_BYTES &&
            capability.maxResults >= 1
}
