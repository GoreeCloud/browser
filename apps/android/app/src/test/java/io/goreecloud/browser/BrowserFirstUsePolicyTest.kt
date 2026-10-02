package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Test

class BrowserFirstUsePolicyTest {
    @Test
    fun setupStepIsClampedToKnownWizardRange() {
        assertEquals(0, BrowserFirstUsePolicy.normalizeStep(-20))
        assertEquals(0, BrowserFirstUsePolicy.normalizeStep(0))
        assertEquals(2, BrowserFirstUsePolicy.normalizeStep(2))
        assertEquals(2, BrowserFirstUsePolicy.normalizeStep(99))
    }

    @Test
    fun nextAndPreviousStayInsideWizardRange() {
        assertEquals(1, BrowserFirstUsePolicy.nextStep(0))
        assertEquals(2, BrowserFirstUsePolicy.nextStep(2))
        assertEquals(0, BrowserFirstUsePolicy.previousStep(0))
        assertEquals(1, BrowserFirstUsePolicy.previousStep(2))
    }
}
