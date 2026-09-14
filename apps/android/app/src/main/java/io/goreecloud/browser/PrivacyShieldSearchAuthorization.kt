package io.goreecloud.browser

import java.time.Instant

/**
 * Narrow Browser adapter for the canonical Privacy Shield authorization
 * decision contract used by remote GoreeCloud Search delegation.
 *
 * Browser does not treat a generic ALLOW bit as sufficient. The decision must
 * authorize the exact Search operation, private GoreeCloud processing zone and
 * destination, contain no obligations Browser cannot yet enforce, remain
 * unexpired, and provide a capability-token reference that can travel with the
 * Search operation.
 */
object PrivacyShieldSearchAuthorization {
    const val REQUIRED_OPERATION = GoreeCloudSearchContract.CAPABILITY_ID
    const val REQUIRED_PROCESSING_ZONE = "private_goreecloud"
    const val REQUIRED_DESTINATION = GoreeCloudSearchContract.SEARCH_ORIGIN
    const val REQUIRED_RETENTION_MODE = "none"

    data class DecisionEvidence(
        val decisionId: String,
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
        INVALID_DECISION_ID,
        OUTCOME_NOT_ALLOW,
        OPERATION_NOT_PERMITTED,
        PROCESSING_ZONE_NOT_PERMITTED,
        DESTINATION_NOT_PERMITTED,
        RETENTION_NOT_PERMITTED,
        UNSUPPORTED_OBLIGATIONS,
        EXPIRED_OR_INVALID_EXPIRY,
        CAPABILITY_TOKEN_REQUIRED,
    }

    fun evaluate(
        decision: DecisionEvidence,
        now: Instant = Instant.now(),
    ): Evaluation {
        if (decision.decisionId.isBlank()) {
            return Evaluation.Rejected(RejectionReason.INVALID_DECISION_ID)
        }
        if (decision.outcome != "ALLOW") {
            // ALLOW_WITH_CONSTRAINTS remains fail-closed until Browser has a
            // complete obligation-enforcement path for this operation.
            return Evaluation.Rejected(RejectionReason.OUTCOME_NOT_ALLOW)
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
        if (decision.obligations.isNotEmpty()) {
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
            ?.takeIf(String::isNotEmpty)
            ?: return Evaluation.Rejected(RejectionReason.CAPABILITY_TOKEN_REQUIRED)

        return Evaluation.Accepted(
            GoreeCloudSearchContract.PrivacyAuthorization(
                accepted = true,
                reference = capabilityReference,
            ),
        )
    }
}
