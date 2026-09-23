package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class InternationalizedHostPolicyTest {
    @Test
    fun preservesPortPathQueryAndFragmentWhileCanonicalizingHost() {
        assertEquals(
            "https://xn--r8jz45g.xn--zckzah:8443/path?q=1#section",
            InternationalizedHostPolicy.canonicalizeHttpUrl(
                "HTTPS://例え.テスト:8443/path?q=1#section",
            ),
        )
    }

    @Test
    fun validBracketedIpv6IsPreservedAndInvalidBracketedHostIsRejected() {
        assertEquals(
            "https://[2001:db8::1]:8443/path",
            InternationalizedHostPolicy.canonicalizeHttpUrl(
                "https://[2001:db8::1]:8443/path",
            ),
        )
        assertNull(
            InternationalizedHostPolicy.canonicalizeHttpUrl(
                "https://[example.com]/path",
            ),
        )
    }

    @Test
    fun invalidPortAndStd3HostFailClosed() {
        assertNull(InternationalizedHostPolicy.canonicalizeHttpUrl("https://example.com:65536"))
        assertNull(InternationalizedHostPolicy.canonicalizeHttpUrl("https://exa_mple.com/path"))
    }
}
