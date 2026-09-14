package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class GlazeContractTest {
    @Test
    fun androidBrowserTargetsCurrentStableGlazeContract() {
        assertEquals("1.4.0", GlazeContract.VERSION)
        assertEquals(
            "a20374734dae6a119b28448f5e6b3232253b6da7",
            GlazeContract.SOURCE_INTEGRATION_ANCHOR,
        )
        assertEquals(
            GlazeContract.SOURCE_INTEGRATION_ANCHOR,
            GlazeContract.STABLE_RELEASE_REVISION,
        )
        assertEquals(
            GlazeContract.SOURCE_INTEGRATION_ANCHOR,
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
    fun v14OpticalAccessibilityPrecedenceFailsSafe() {
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
