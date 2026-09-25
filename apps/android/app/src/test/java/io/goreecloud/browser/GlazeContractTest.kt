package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class GlazeContractTest {
    private fun capability(
        id: String,
        state: GlazeContract.CapabilityState,
        authority: String = "browser",
    ) = GlazeContract.CapabilityRecord(id, state, authority)

    @Test
    fun androidBrowserTargetsCurrentStableGlazeContract() {
        assertEquals("1.6.0", GlazeContract.VERSION)
        assertEquals(
            "a7180679ea851389e0f3004515f9a25f420e716d",
            GlazeContract.STABLE_RELEASE_REVISION,
        )
        assertEquals(
            "c7509c79256b04b0aa67cb9dd0737d7588e0ae4a",
            GlazeContract.REVIEWED_IMPLEMENTATION_ANCHOR,
        )
        assertEquals(
            "c7509c79256b04b0aa67cb9dd0737d7588e0ae4a",
            GlazeContract.QUALIFICATION_SOURCE_ANCHOR,
        )
        assertEquals(
            "354f5759385c28596fcfec26a3ad525e89fb1c35",
            GlazeContract.QUALIFICATION_INTEGRATION_REVISION,
        )
        assertEquals("1.4.1", GlazeContract.OPTICAL_BASELINE_VERSION)
        assertEquals(
            "4fab9da0fad2e5c974e0e66ec88632c61745751c",
            GlazeContract.OPTICAL_BASELINE_REVISION,
        )
        assertEquals("1.5.1", GlazeContract.IMMEDIATE_ROLLBACK_VERSION)
        assertEquals(
            GlazeContract.STABLE_RELEASE_REVISION,
            GlazeContract.SOURCE_INTEGRATION_ANCHOR,
        )
        assertEquals(
            GlazeContract.REVIEWED_IMPLEMENTATION_ANCHOR,
            GlazeContract.ACCEPTED_VISUAL_SOURCE,
        )
    }

    @Test
    fun inheritedTouchTargetFloorsRemainBounded() {
        assertEquals(48, GlazeContract.targetFloorDp(touchAssistance = false))
        assertEquals(56, GlazeContract.targetFloorDp(touchAssistance = true))
        assertTrue(GlazeContract.satisfiesGeneralTargetFloor(48))
        assertFalse(GlazeContract.satisfiesGeneralTargetFloor(47))
        assertTrue(GlazeContract.satisfiesTouchAssistanceTargetFloor(56))
        assertFalse(GlazeContract.satisfiesTouchAssistanceTargetFloor(55))
    }

    @Test
    fun inheritedOpticalAccessibilityPrecedenceFailsSafe() {
        val forcedColors = GlazeContract.OpticalAccessibilitySignals(forcedColors = true)
        val reducedTransparency = GlazeContract.OpticalAccessibilitySignals(reducedTransparency = true)
        val increasedContrast = GlazeContract.OpticalAccessibilitySignals(increasedContrast = true)
        val normal = GlazeContract.OpticalAccessibilitySignals()

        assertEquals(GlazeContract.OpticalMode.SolidAccessible, GlazeContract.opticalMode(forcedColors))
        assertEquals(GlazeContract.OpticalMode.SolidAccessible, GlazeContract.opticalMode(reducedTransparency))
        assertEquals(GlazeContract.OpticalMode.IncreasedContrast, GlazeContract.opticalMode(increasedContrast))
        assertEquals(GlazeContract.OpticalMode.Standard, GlazeContract.opticalMode(normal))

        assertFalse(GlazeContract.allowsBackdropEffects(forcedColors))
        assertFalse(GlazeContract.allowsBackdropEffects(reducedTransparency))
        assertFalse(GlazeContract.allowsDecorativeEnvironmentalTint(increasedContrast))
        assertTrue(GlazeContract.allowsBackdropEffects(normal))
        assertTrue(GlazeContract.allowsDecorativeEnvironmentalTint(normal))
        assertEquals(0.08, GlazeContract.MAX_ENVIRONMENTAL_MEMORY_INFLUENCE, 0.0)
    }

    @Test
    fun solidAccessibleModeOutranksIncreasedContrast() {
        val combined = GlazeContract.OpticalAccessibilitySignals(
            reducedTransparency = true,
            increasedContrast = true,
        )

        assertEquals(GlazeContract.OpticalMode.SolidAccessible, GlazeContract.opticalMode(combined))
        assertFalse(GlazeContract.allowsBackdropEffects(combined))
        assertFalse(GlazeContract.allowsDecorativeEnvironmentalTint(combined))
    }

    @Test
    fun availableCapabilityEnablesPresentationWithoutCreatingExecutionAuthority() {
        val result = GlazeContract.resolveAction(
            GlazeContract.ActionRequest(
                id = "navigate",
                requiredCapabilityIds = setOf("browser.navigate"),
            ),
            listOf(capability("browser.navigate", GlazeContract.CapabilityState.Available)),
        )

        assertTrue(result.enabled)
        assertEquals(GlazeContract.CapabilityState.Available, result.state)
        assertFalse(result.automaticExecutionAllowed)
        assertFalse(result.authorityInferred)
        assertFalse(result.providerPrecedenceInferred)
    }

    @Test
    fun missingCapabilityFailsClosedWithoutDisablingUnrelatedBrowserCapability() {
        val navigation = GlazeContract.resolveAction(
            GlazeContract.ActionRequest(
                id = "navigate",
                requiredCapabilityIds = setOf("browser.navigate"),
            ),
            listOf(capability("browser.navigate", GlazeContract.CapabilityState.Available)),
        )
        val search = GlazeContract.resolveAction(
            GlazeContract.ActionRequest(
                id = "search",
                requiredCapabilityIds = setOf("search.query"),
            ),
            listOf(capability("browser.navigate", GlazeContract.CapabilityState.Available)),
        )

        assertTrue(navigation.enabled)
        assertFalse(search.enabled)
        assertEquals(GlazeContract.CapabilityState.Unknown, search.state)
        assertTrue(search.reasonCodes.contains("capability-unknown:search.query"))
    }

    @Test
    fun duplicateCapabilityOwnershipFailsClosedWithoutInferringProviderPrecedence() {
        val result = GlazeContract.resolveAction(
            GlazeContract.ActionRequest(
                id = "search",
                requiredCapabilityIds = setOf("search.query"),
            ),
            listOf(
                capability("search.query", GlazeContract.CapabilityState.Available, "search-service-a"),
                capability("search.query", GlazeContract.CapabilityState.Available, "search-service-b"),
            ),
        )

        assertFalse(result.enabled)
        assertEquals(GlazeContract.CapabilityState.Conflict, result.state)
        assertTrue(result.reasonCodes.contains("capability-conflict:search.query"))
        assertFalse(result.providerPrecedenceInferred)
    }

    @Test
    fun restrictedConsequentialActionRemainsPresentationDisabledAndCannotAutoExecute() {
        val result = GlazeContract.resolveAction(
            GlazeContract.ActionRequest(
                id = "remote-search",
                requiredCapabilityIds = setOf("privacy.search-transmission"),
                consequential = true,
            ),
            listOf(
                capability(
                    "privacy.search-transmission",
                    GlazeContract.CapabilityState.Restricted,
                    "privacy-shield",
                ),
            ),
        )

        assertFalse(result.enabled)
        assertEquals(GlazeContract.CapabilityState.Restricted, result.state)
        assertTrue(result.reasonCodes.contains("restricted-by-authority:privacy.search-transmission"))
        assertFalse(result.automaticExecutionAllowed)
        assertFalse(result.authorityInferred)
    }

    @Test
    fun mobileChromePreservesApplicationAuthorityAndBrowserBudget() {
        val mapping = GlazeContract.ANDROID_BROWSER_MAPPING

        assertEquals(GlazeContract.MaterialLevel.Canvas, mapping.canvas)
        assertEquals(GlazeContract.MaterialLevel.Canvas, mapping.topChrome)
        assertEquals(GlazeContract.MaterialLevel.SoftGlaze, mapping.omnibox)
        assertEquals(GlazeContract.MaterialLevel.Surface, mapping.addressField)
        assertEquals(GlazeContract.MaterialLevel.Surface, mapping.bottomChrome)
        assertEquals(GlazeContract.MaterialLevel.SoftGlaze, mapping.browserMenu)
        assertEquals(GlazeContract.ShellSurface.Application, mapping.shellSurface)
        assertEquals(GlazeContract.Clarity.Balanced, mapping.clarity)
        assertEquals(GlazeContract.Expression.Calm, mapping.expression)
        assertTrue(GlazeContract.satisfiesSystemGlazeBudget(mapping))
        assertFalse(mapping.declaresUniversalSearch)
        assertFalse(mapping.declaresControlCenter)
    }

    @Test
    fun mobileChromeRemovesDevelopmentScaffoldingFromNormalBrowsing() {
        val mapping = GlazeContract.ANDROID_BROWSER_MAPPING

        assertTrue(mapping.noActionBar)
        assertFalse(mapping.developerStatusInNormalChrome)
        assertTrue(mapping.effectsFreeFallback)
        assertTrue(mapping.usesNativeControls)
        assertTrue(mapping.usesVectorChromeIcons)
        assertFalse(mapping.usesPlatformPopupMenu)
        assertTrue(mapping.scrollAwareTopChrome)
        assertTrue(mapping.supportsLargeTextReflow)
        assertTrue(mapping.preservesNonColorStatusMeaning)
        assertTrue(mapping.distinguishesCapabilityStates)
        assertTrue(mapping.boundedVisualComplexity)
    }

    @Test
    fun interactionPriorityKeepsDisabledAboveErrorAndFocus() {
        assertTrue(
            GlazeContract.statePriority(GlazeContract.InteractionState.Disabled) >
                GlazeContract.statePriority(GlazeContract.InteractionState.Error),
        )
        assertTrue(
            GlazeContract.statePriority(GlazeContract.InteractionState.Error) >
                GlazeContract.statePriority(GlazeContract.InteractionState.Pressed),
        )
        assertTrue(
            GlazeContract.statePriority(GlazeContract.InteractionState.Pressed) >
                GlazeContract.statePriority(GlazeContract.InteractionState.Focus),
        )
        assertTrue(
            GlazeContract.statePriority(GlazeContract.InteractionState.Focus) >
                GlazeContract.statePriority(GlazeContract.InteractionState.Selected),
        )
        assertTrue(
            GlazeContract.statePriority(GlazeContract.InteractionState.Selected) >
                GlazeContract.statePriority(GlazeContract.InteractionState.Hover),
        )
        assertTrue(
            GlazeContract.statePriority(GlazeContract.InteractionState.Hover) >
                GlazeContract.statePriority(GlazeContract.InteractionState.Rest),
        )
    }

    @Test
    fun mobileChromeBudgetsExpandedAndCollapsedViewportStates() {
        assertEquals(128, GlazeContract.fixedChromeHeightDp())
        assertEquals(56, GlazeContract.collapsedChromeHeightDp())
        assertTrue(
            GlazeContract.collapsedChromeHeightDp() < GlazeContract.fixedChromeHeightDp(),
        )
    }
}
