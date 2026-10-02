package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class InternationalizedHostPolicyTest {
    @Test
    fun unicodeHostCanonicalizesWhilePreservingUrlComponents() {
        assertEquals(
            "https://xn--r8jz45g.xn--zckzah:8443/path?q=1#section",
            InternationalizedHostPolicy.canonicalizeHttpUrl(
                "HTTPS://例え.テスト:8443/path?q=1#section",
            ),
        )
    }

    @Test
    fun trailingDnsRootDotRemainsValidAndCanonical() {
        assertEquals(
            "https://example.com./path",
            InternationalizedHostPolicy.canonicalizeHttpUrl("https://EXAMPLE.com./path"),
        )
    }

    @Test
    fun validBracketedIpv6IsPreservedAndInvalidBracketedHostsFailClosed() {
        assertEquals(
            "https://[2001:db8::1]:8443/path",
            InternationalizedHostPolicy.canonicalizeHttpUrl(
                "https://[2001:DB8::1]:8443/path",
            ),
        )
        assertNull(InternationalizedHostPolicy.canonicalizeHttpUrl("https://[example.com]/path"))
        assertNull(InternationalizedHostPolicy.canonicalizeHttpUrl("https://[127.0.0.1]/path"))
        assertNull(InternationalizedHostPolicy.canonicalizeHttpUrl("https://[localhost]/path"))
    }

    @Test
    fun numericHostsRequireCanonicalDottedDecimalIpv4() {
        assertEquals(
            "https://127.0.0.1/path",
            InternationalizedHostPolicy.canonicalizeHttpUrl("https://127.0.0.1/path"),
        )

        for (raw in listOf(
            "https://256.0.0.1/path",
            "https://127.1/path",
            "https://127.00.0.1/path",
            "https://1.2.3.4.5/path",
        )) {
            assertNull(InternationalizedHostPolicy.canonicalizeHttpUrl(raw))
        }
    }

    @Test
    fun invalidPortCredentialsAndStd3HostFailClosed() {
        assertNull(InternationalizedHostPolicy.canonicalizeHttpUrl("https://example.com:0"))
        assertNull(InternationalizedHostPolicy.canonicalizeHttpUrl("https://example.com:65536"))
        assertNull(InternationalizedHostPolicy.canonicalizeHttpUrl("https://exa_mple.com/path"))
        assertNull(
            InternationalizedHostPolicy.canonicalizeHttpUrl(
                "https://user:pass@example.com/private",
            ),
        )
    }
}
