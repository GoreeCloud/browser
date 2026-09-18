package io.goreecloud.browser

import java.time.Instant
import java.util.UUID

/**
 * Narrow Browser adapter for the canonical Privacy Shield authorization
 * decision contract used by remote GoreeCloud Search delegation.
 *
 * Private GoreeCloud processing is intentionally represented by Privacy Shield
 * as ALLOW_WITH_CONSTRAINTS. Browser accepts only the exact Search constraint
 * set that is satisfied by Privacy Shield authority-side evidence/receipt work
 * plus Search-side processing-zone enforcement. Any additional obligation fails
 * closed until Browser/Search have explicit enforcement for it.
 */
object PrivacyShieldSearchAuthorization {
    const val REQUESTER_ID = "goreecloud-browser"
    const val REQUESTER_TYPE = "application"
    const val RESOURCE_ID = "goreecloud.search.query"
    const val RESOURCE_CLASSIFICATION = "query_text"
    const val REQUIRED_OPERATION = GoreeCloudSearchContract.CAPABILITY_ID
    const val REQUIRED_PURPOSE = "internet_search"
    const val REQUIRED_PROCESSING_ZONE = "private_goreecloud"
    const val REQUIRED_DESTINATION = GoreeCloudSearchContract.SEARCH_ORIGIN
    const val REQUIRED_RETENTION_MODE = "none"
    const val REQUIRED_OUTCOME = "ALLOW_WITH_CONSTRAINTS"
    val REQUIRED_OBLIGATIONS: Set<String> = setOf(
        "record_privacy_evidence",
        "generate_privacy_receipt",
        "enforce_processing_zone",
    )

    /**
     * Canonical Search-specific authorization intent. The request identifier is
     * created before Privacy Shield is called so the returned decision can be
     * proven to belong to this exact operation rather than merely looking
     * compatible with it.
     */
    data class RequestIntent(
        val requestId: String,
        val requesterId: String = REQUESTER_ID,
        val requesterType: String = REQUESTER_TYPE,
        val resourceId: String = RESOURCE_ID,
        val resourceClassification: String = RESOURCE_CLASSIFICATION,
        val operation: String = REQUIRED_OPERATION,
        val purpose: String = REQUIRED_PURPOSE,
        val processingZone: String = REQUIRED_PROCESSING_ZONE,
        val destination: String = REQUIRED_DESTINATION,
        val retentionMode: String = REQUIRED_RETENTION_MODE,
        val externalDisclosure: Boolean = false,
    )

    data class DecisionEvidence(
        val decisionId: String,
        val requestId: String,
        val outcome: String,
        val permittedOperations: Set<String>,
        val processingZone: String,
        val permittedDestinations: Set<String>,
        val retentionMode: String,
        val obligations: Set<String> = emptySet(),
        val expiresAt: String? = null,
        val capabilityTokenReference: String? = null,
    )

    sealed interface Evaluation {
        data class Accepted(
            val authorization: GoreeCloudSearchContract.PrivacyAuthorization,
        ) : Evaluation

        data class Rejected(val reason: RejectionReason) : Evaluation
    }

    enum class RejectionReason {
        INVALID_REQUEST_INTENT,
        INVALID_DECISION_ID,
        REQUEST_ID_MISMATCH,
        OUTCOME_NOT_SUPPORTED,
        OPERATION_NOT_PERMITTED,
        PROCESSING_ZONE_NOT_PERMITTED,
        DESTINATION_NOT_PERMITTED,
        RETENTION_NOT_PERMITTED,
        UNSUPPORTED_OBLIGATIONS,
        EXPIRED_OR_INVALID_EXPIRY,
        CAPABILITY_TOKEN_REQUIRED,
    }

    fun requestIntent(requestId: String = UUID.randomUUID().toString()): RequestIntent {
        val normalizedRequestId = requestId.trim()
        require(normalizedRequestId.isNotEmpty()) { "Privacy Shield request ID must not be blank" }
        return RequestIntent(requestId = normalizedRequestId)
    }

    fun evaluate(
        request: RequestIntent,
        decision: DecisionEvidence,
        now: Instant = Instant.now(),
    ): Evaluation {
        if (!isCanonicalRequest(request)) {
            return Evaluation.Rejected(RejectionReason.INVALID_REQUEST_INTENT)
        }
        if (decision.decisionId.isBlank()) {
            return Evaluation.Rejected(RejectionReason.INVALID_DECISION_ID)
        }
        if (decision.requestId != request.requestId) {
            return Evaluation.Rejected(RejectionReason.REQUEST_ID_MISMATCH)
        }
        if (decision.outcome != REQUIRED_OUTCOME) {
            return Evaluation.Rejected(RejectionReason.OUTCOME_NOT_SUPPORTED)
        }
        if (REQUIRED_OPERATION !in decision.permittedOperations) {
            return Evaluation.Rejected(RejectionReason.OPERATION_NOT_PERMITTED)
        }
        if (decision.processingZone != REQUIRED_PROCESSING_ZONE) {
            return Evaluation.Rejected(RejectionReason.PROCESSING_ZONE_NOT_PERMITTED)
        }
        if (REQUIRED_DESTINATION !in decision.permittedDestinations) {
            return Evaluation.Rejected(RejectionReason.DESTINATION_NOT_PERMITTED)
        }
        if (decision.retentionMode != REQUIRED_RETENTION_MODE) {
            return Evaluation.Rejected(RejectionReason.RETENTION_NOT_PERMITTED)
        }
        if (decision.obligations != REQUIRED_OBLIGATIONS) {
            return Evaluation.Rejected(RejectionReason.UNSUPPORTED_OBLIGATIONS)
        }
        if (decision.expiresAt != null) {
            val expiry = runCatching { Instant.parse(decision.expiresAt) }.getOrNull()
                ?: return Evaluation.Rejected(RejectionReason.EXPIRED_OR_INVALID_EXPIRY)
            if (!expiry.isAfter(now)) {
                return Evaluation.Rejected(RejectionReason.EXPIRED_OR_INVALID_EXPIRY)
            }
        }
        val capabilityReference = decision.capabilityTokenReference
            ?.trim()
            ?.takeIf(GoreeCloudSearchContract::isCanonicalPrivacyCapabilityReference)
            ?: return Evaluation.Rejected(RejectionReason.CAPABILITY_TOKEN_REQUIRED)

        return Evaluation.Accepted(
            GoreeCloudSearchContract.PrivacyAuthorization(
                accepted = true,
                reference = capabilityReference,
            ),
        )
    }

    private fun isCanonicalRequest(request: RequestIntent): Boolean =
        request.requestId.isNotBlank() &&
            request.requesterId == REQUESTER_ID &&
            request.requesterType == REQUESTER_TYPE &&
            request.resourceId == RESOURCE_ID &&
            request.resourceClassification == RESOURCE_CLASSIFICATION &&
            request.operation == REQUIRED_OPERATION &&
            request.purpose == REQUIRED_PURPOSE &&
            request.processingZone == REQUIRED_PROCESSING_ZONE &&
            request.destination == REQUIRED_DESTINATION &&
            request.retentionMode == REQUIRED_RETENTION_MODE &&
            !request.externalDisclosure
}
