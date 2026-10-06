package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class BrowserDurableRecoveryPolicyTest {
    @Test
    fun acceptsFreshAndEmptyRecoveryMarkersOnlyWhenShapeIsExact() {
        assertEquals(
            BrowserDurableStartupKind.Fresh,
            BrowserDurableRecoveryPolicy.parse(arrayOf("fresh")).kind,
        )
        assertEquals(
            BrowserDurableStartupKind.RecoveredEmpty,
            BrowserDurableRecoveryPolicy.parse(arrayOf("recovered_empty")).kind,
        )
        assertEquals(
            BrowserDurableStartupKind.Unavailable,
            BrowserDurableRecoveryPolicy.parse(arrayOf("fresh", "unexpected")).kind,
        )
    }

    @Test
    fun reconstructsCanonicalBoundedRecoveredTabs() {
        val recovered = BrowserDurableRecoveryPolicy.parse(
            arrayOf(
                "recovered",
                "tab-b",
                "tab-a",
                "",
                "",
                "tab-b",
                "https://例え.テスト/path?x=1",
                "  Example title  ",
            ),
        )

        assertEquals(BrowserDurableStartupKind.Recovered, recovered.kind)
        assertEquals(listOf("tab-a", "tab-b"), recovered.state?.tabs?.map { it.id })
        assertEquals("tab-b", recovered.state?.activeTabId)
        assertEquals("goreecloud://start", recovered.state?.tabs?.first()?.url)
        assertNull(recovered.state?.tabs?.first()?.title)
        assertEquals(
            "https://xn--r8jz45g.xn--zckzah/path?x=1",
            recovered.state?.activeTab?.url,
        )
        assertEquals("Example title", recovered.state?.activeTab?.title)
    }

    @Test
    fun rejectsMalformedCredentialBearingOrImpossibleProjection() {
        val cases = listOf(
            arrayOf("recovered", "tab-a"),
            arrayOf(
                "recovered", "tab-a",
                "tab-a", "https://user:secret@example.com/private", "Secret",
            ),
            arrayOf(
                "recovered", "missing",
                "tab-a", "https://example.com/", "Example",
            ),
            arrayOf(
                "recovered", "tab-a",
                "tab-a", "https://example.com/", "One",
                "tab-a", "https://example.org/", "Duplicate",
            ),
        )

        cases.forEach { fields ->
            val parsed = BrowserDurableRecoveryPolicy.parse(fields)
            assertEquals(BrowserDurableStartupKind.Unavailable, parsed.kind)
            assertNull(parsed.state)
        }
    }
}
