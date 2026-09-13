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
            "84cb3db4884042f0fa25ed6d475a127fb110f596",
            GlazeContract.STABLE_RELEASE_REVISION,
        )
        assertEquals("1.4.1", GlazeContract.HUMAN_VISUAL_VALIDATION_FOLLOW_UP)
        assertEquals(GlazeContract.VERSION, GlazeBrowserOptics.TARGET_VERSION)
        assertEquals(GlazeContract.STABLE_RELEASE_REVISION, GlazeBrowserOptics.STABLE_SOURCE_REVISION)
    }

    @Test
    fun touchTargetFloorsRemainAccessibleUnderV14() {
        assertEquals(48, GlazeContract.targetFloorDp(touchAssistance = false))
        assertEquals(56, GlazeContract.targetFloorDp(touchAssistance = true))
        assertTrue(GlazeContract.satisfiesGeneralTargetFloor(48))
        assertFalse(GlazeContract.satisfiesGeneralTargetFloor(47))
        assertTrue(GlazeContract.satisfiesTouchAssistanceTargetFloor(56))
        assertFalse(GlazeContract.satisfiesTouchAssistanceTargetFloor(55))
    }

    @Test
    fun mobileChromePreservesApplicationAuthorityAndGlazeBudget() {
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
    fun mobileChromePreservesEffectsFreeFallback() {
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

    @Test
    fun pageAndOriginDataCannotDriveTrustedChromeOptics() {
        assertFalse(GlazeBrowserOptics.PAGE_CONTENT_MAY_DRIVE_TRUSTED_CHROME)
        assertFalse(GlazeBrowserOptics.ORIGIN_IDENTITY_MAY_DRIVE_TRUSTED_CHROME)
        assertFalse(GlazeBrowserOptics.FAVICON_COLOR_MAY_DRIVE_TRUSTED_CHROME)
        assertFalse(GlazeBrowserOptics.PAGE_THEME_COLOR_MAY_DRIVE_TRUSTED_CHROME)
        assertFalse(GlazeBrowserOptics.SECURITY_STATE_MAY_BE_INFERRED_FROM_OPTICS)
        assertFalse(GlazeBrowserOptics.PRIVACY_STATE_MAY_BE_INFERRED_FROM_OPTICS)
        assertFalse(GlazeBrowserOptics.TELEMETRY_REQUIRED)
        assertFalse(GlazeBrowserOptics.REMOTE_CONTEXT_REQUIRED)
        assertEquals(0f, GlazeBrowserOptics.MAX_TRUSTED_CHROME_MEMORY_TINT_INFLUENCE, 0f)
    }

    @Test
    fun accessibilityModesFailClosedWithoutDecorativeTint() {
        val reducedTransparency = GlazeBrowserOptics.resolve(
            GlazeBrowserOptics.Accessibility(reducedTransparency = true),
        )
        assertEquals(GlazeBrowserOptics.State.Mode.SOLID_ACCESSIBLE, reducedTransparency.mode)
        assertEquals(0f, reducedTransparency.blurScale, 0f)
        assertEquals(1f, reducedTransparency.semanticProtection, 0f)
        assertFalse(reducedTransparency.decorativeTintAllowed)

        val forcedColors = GlazeBrowserOptics.resolve(
            GlazeBrowserOptics.Accessibility(forcedColors = true),
        )
        assertEquals(GlazeBrowserOptics.State.Mode.SOLID_ACCESSIBLE, forcedColors.mode)
        assertEquals(0f, forcedColors.memoryTintInfluence, 0f)
        assertFalse(forcedColors.decorativeTintAllowed)

        val increasedContrast = GlazeBrowserOptics.resolve(
            GlazeBrowserOptics.Accessibility(increasedContrast = true),
        )
        assertTrue(increasedContrast.frostStrength > GlazeBrowserOptics.resolve().frostStrength)
        assertEquals(0f, increasedContrast.memoryTintInfluence, 0f)
        assertFalse(increasedContrast.decorativeTintAllowed)
    }
}
