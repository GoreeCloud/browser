package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Test

class BlockedNavigationPresentationTest {
    @Test
    fun supportedSchemeSyntaxIsReducedToSchemeOnly() {
        val raw = "mailto:private.user@example.com?subject=secret"

        val label = BlockedNavigationPresentation.label(raw)

        assertEquals("mailto: link", label)
        assertFalse(label.contains("private.user"))
        assertFalse(label.contains("secret"))
    }

    @Test
    fun intentPayloadIsNotExposed() {
        val raw = "intent://example/#Intent;scheme=bank;S.token=private-token;end"

        val label = BlockedNavigationPresentation.label(raw)

        assertEquals("intent: link", label)
        assertFalse(label.contains("private-token"))
        assertFalse(label.contains("bank"))
    }

    @Test
    fun malformedTargetUsesGenericLabel() {
        assertEquals(
            "Unsupported link",
            BlockedNavigationPresentation.label(" not a uri "),
        )
    }
}
