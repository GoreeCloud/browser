package io.goreecloud.browser

import java.time.Instant
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class PrivacyShieldSearchAuthorizationTest {
    private val now = Instant.parse("2026-09-14T12:00:00Z")
    private val request = PrivacyShieldSearchAuthorization.requestIntent("privacy-request-123")

    @Test
    fun canonicalRequestCarriesStableCorrelationIdentity() {
        assertEquals("privacy-request-123", request.requestId)
        assertEquals("goreecloud-browser", request.requesterId)
        assertEquals("application", request.requesterType)
        assertEquals("goreecloud.search.query", request.resourceId)
        assertEquals("query_text", request.resourceClassification)
        assertEquals("search.query", request.operation)
        assertEquals("internet_search", request.purpose)
        assertEquals("private_goreecloud", request.processingZone)
        assertEquals("https://search.goreecloud.com", request.destination)
        assertEquals("none", request.retentionMode)
        assertEquals(false, request.externalDisclosure)
    }

    @Test
    fun exactAllowDecisionProducesCapabilityTokenAuthorization() {
        val evaluation = PrivacyShieldSearchAuthorization.evaluate(
            request = request,
            decision = allowedDecision(),
            now = now,
        )

        assertTrue(evaluation is PrivacyShieldSearchAuthorization.Evaluation.Accepted)
        val accepted = evaluation as PrivacyShieldSearchAuthorization.Evaluation.Accepted
        assertEquals(true, accepted.authorization.accepted)
        assertEquals("privacy-shield:capability:123", accepted.authorization.reference)
    }

    @Test
    fun decisionForDifferentRequestFailsClosed() {
        val evaluation = PrivacyShieldSearchAuthorization.evaluate(
            request = request,
            decision = allowedDecision().copy(requestId = "privacy-request-other"),
            now = now,
        )

        assertEquals(
            PrivacyShieldSearchAuthorization.Evaluation.Rejected(
                PrivacyShieldSearchAuthorization.RejectionReason.REQUEST_ID_MISMATCH,
            ),
            evaluation,
        )
    }

    @Test
    fun nonCanonicalRequestFailsClosed() {
        val evaluation = PrivacyShieldSearchAuthorization.evaluate(
            request = request.copy(destination = "https://example.com"),
            decision = allowedDecision(),
            now = now,
        )

        assertEquals(
            PrivacyShieldSearchAuthorization.Evaluation.Rejected(
                PrivacyShieldSearchAuthorization.RejectionReason.INVALID_REQUEST_INTENT,
            ),
            evaluation,
        )
    }

    @Test
    fun constrainedAllowFailsClosedUntilObligationsCanBeEnforced() {
        val evaluation = PrivacyShieldSearchAuthorization.evaluate(
            request = request,
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
            request = request,
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
            request = request,
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
            request = request,
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
        requestId = request.requestId,
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
