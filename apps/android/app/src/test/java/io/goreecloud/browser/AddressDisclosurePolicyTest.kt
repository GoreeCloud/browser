package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class AddressDisclosurePolicyTest {
    @Test
    fun explicitHttpUserinfoIsRemovedBeforeExternalDisclosure() {
        assertEquals(
            "https://example.com:8443/path?q=browser#section",
            AddressDisclosurePolicy.forExternalUse(
                "https://user:password@example.com:8443/path?q=browser#section",
            ),
        )
    }

    @Test
    fun encodedUserinfoIsRemovedWithoutRewritingRemainingUrl() {
        assertEquals(
            "https://example.com/path?next=%2Fhome#done",
            AddressDisclosurePolicy.forExternalUse(
                "https://user%40name:p%40ss@example.com/path?next=%2Fhome#done",
            ),
        )
    }

    @Test
    fun ordinaryAllowedWebAddressRemainsUnchanged() {
        val address = "HTTPS://example.com:443/path?q=A%2FB#Frag"
        assertEquals(address, AddressDisclosurePolicy.forExternalUse(address))
    }

    @Test
    fun internalAndNonWebAddressesCannotLeaveBrowserAsPageAddresses() {
        assertNull(AddressDisclosurePolicy.forExternalUse("goreecloud://start"))
        assertNull(AddressDisclosurePolicy.forExternalUse("data:text/html,<p>local</p>"))
        assertNull(AddressDisclosurePolicy.forExternalUse("mailto:user@example.com"))
    }

    @Test
    fun malformedOrControlBearingAddressesFailClosed() {
        assertNull(AddressDisclosurePolicy.forExternalUse("not a url"))
        assertNull(AddressDisclosurePolicy.forExternalUse(" https://example.com/"))
        assertNull(AddressDisclosurePolicy.forExternalUse("https://example.com/\nprivate"))
    }
}
