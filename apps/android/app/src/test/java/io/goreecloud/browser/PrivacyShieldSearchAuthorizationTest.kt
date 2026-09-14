package io.goreecloud.browser

import java.time.Instant
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class PrivacyShieldSearchAuthorizationTest {
    private val now = Instant.parse("2026-09-14T12:00:00Z")

    @Test
    fun exactAllowDecisionProducesCapabilityTokenAuthorization() {
        val evaluation = PrivacyShieldSearchAuthorization.evaluate(
            decision = allowedDecision(),
            now = now,
        )

        assertTrue(evaluation is PrivacyShieldSearchAuthorization.Evaluation.Accepted)
        val accepted = evaluation as PrivacyShieldSearchAuthorization.Evaluation.Accepted
        assertEquals(true, accepted.authorization.accepted)
        assertEquals("privacy-shield:capability:123", accepted.authorization.reference)
    }

    @Test
    fun constrainedAllowFailsClosedUntilObligationsCanBeEnforced() {
        val evaluation = PrivacyShieldSearchAuthorization.evaluate(
            decision = allowedDecision().copy(
                outcome = "ALLOW_WITH_CONSTRAINTS",
                obligations = setOf("redact-sensitive-terms"),
            ),
            now = now,
        )

        assertEquals(
            PrivacyShieldSearchAuthorization.Evaluation.Rejected(
                PrivacyShieldSearchAuthorization.RejectionReason.OUTCOME_NOT_ALLOW,
            ),
            evaluation,
        )
    }

    @Test
    fun wrongDestinationFailsClosed() {
        val evaluation = PrivacyShieldSearchAuthorization.evaluate(
            decision = allowedDecision().copy(
                permittedDestinations = setOf("https://example.com"),
            ),
            now = now,
        )

        assertEquals(
            PrivacyShieldSearchAuthorization.Evaluation.Rejected(
                PrivacyShieldSearchAuthorization.RejectionReason.DESTINATION_NOT_PERMITTED,
            ),
            evaluation,
        )
    }

    @Test
    fun expiredDecisionFailsClosed() {
        val evaluation = PrivacyShieldSearchAuthorization.evaluate(
            decision = allowedDecision().copy(expiresAt = "2026-09-14T11:59:59Z"),
            now = now,
        )

        assertEquals(
            PrivacyShieldSearchAuthorization.Evaluation.Rejected(
                PrivacyShieldSearchAuthorization.RejectionReason.EXPIRED_OR_INVALID_EXPIRY,
            ),
            evaluation,
        )
    }

    @Test
    fun missingCapabilityTokenReferenceFailsClosed() {
        val evaluation = PrivacyShieldSearchAuthorization.evaluate(
            decision = allowedDecision().copy(capabilityTokenReference = null),
            now = now,
        )

        assertEquals(
            PrivacyShieldSearchAuthorization.Evaluation.Rejected(
                PrivacyShieldSearchAuthorization.RejectionReason.CAPABILITY_TOKEN_REQUIRED,
            ),
            evaluation,
        )
    }

    private fun allowedDecision() = PrivacyShieldSearchAuthorization.DecisionEvidence(
        decisionId = "privacy-shield:decision:123",
        outcome = "ALLOW",
        permittedOperations = setOf("search.query"),
        processingZone = "private_goreecloud",
        permittedDestinations = setOf("https://search.goreecloud.com"),
        retentionMode = "none",
        obligations = emptySet(),
        expiresAt = "2026-09-14T12:05:00Z",
        capabilityTokenReference = "privacy-shield:capability:123",
    )
}
