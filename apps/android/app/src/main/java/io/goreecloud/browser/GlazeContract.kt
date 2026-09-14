package io.goreecloud.browser

/**
 * Browser-owned Android-native mapping metadata for Glaze UI V1.4 / 1.4.0 Stable.
 *
 * V1.4 inherits the V1.3 ergonomic, adaptive-navigation, system-shell, material,
 * accessibility, and degradation foundations while adding the local deterministic
 * Optical Engine contract. This is source-mapping evidence only; rendered/native-
 * device visual and accessibility acceptance remain separate promotion gates.
 */
object GlazeContract {
    const val VERSION = "1.4.0"
    const val SOURCE_INTEGRATION_ANCHOR = "a20374734dae6a119b28448f5e6b3232253b6da7"

    // Compatibility names retained for existing Browser source consumers. Both
    // resolve to the current V1.4 source-integration authority rather than an
    // invented downstream release identity.
    const val STABLE_RELEASE_REVISION = SOURCE_INTEGRATION_ANCHOR
    const val ACCEPTED_VISUAL_SOURCE = SOURCE_INTEGRATION_ANCHOR

    const val GENERAL_TARGET_DP = 48
    const val TOUCH_ASSISTANCE_TARGET_DP = 56
    const val CHROME_GUTTER_DP = 8
    const val CONTROL_GAP_DP = 6
    const val OMNIBOX_HEIGHT_DP = 56
    const val BOTTOM_TOOLBAR_HEIGHT_DP = 56
    const val OMNIBOX_CORNER_DP = 28
    const val CHROME_CONTROL_CORNER_DP = 22
    const val MENU_CORNER_DP = 28
    const val MENU_ACTION_HEIGHT_DP = 56
    const val PROGRESS_HEIGHT_DP = 2
    const val AUTO_HIDE_SCROLL_THRESHOLD_DP = 72
    const val SCROLL_DIRECTION_SLOP_DP = 6

    // Browser-local conservative composition budget. V1.4 does not grant
    // Browser authority to turn ordinary app chrome into system UI.
    const val MAX_DOMINANT_GLAZE_PANELS = 1
    const val MAX_SMALL_FLOATING_GLAZE_CONTROLS = 3

    // V1.4 environmental color memory is decorative and capped by Glaze at 8%.
    const val MAX_ENVIRONMENTAL_MEMORY_INFLUENCE = 0.08

    enum class MaterialLevel {
        Canvas,
        Surface,
        SoftGlaze,
        Glaze,
        DeepGlaze,
        LiveGlaze,
    }

    enum class Clarity {
        Clear,
        Balanced,
        Solid,
    }

    enum class Expression {
        Calm,
        Balanced,
        Expressive,
    }

    enum class ButtonRole {
        Quiet,
        Soft,
        Glaze,
        Emphasis,
    }

    /**
     * Inherited V1.3 system-shell hierarchy. Browser-owned chrome remains within
     * Application scope; local Browser search/menu surfaces do not become Control
     * Center, system panels, or other platform-authoritative UI.
     */
    enum class ShellSurface {
        Workspace,
        Application,
        SystemOverlay,
        SystemPanel,
        CriticalSystem,
    }

    enum class InteractionState {
        Rest,
        Hover,
        Focus,
        Pressed,
        Selected,
        Disabled,
        Loading,
        Error,
    }

    enum class OpticalMode {
        Standard,
        IncreasedContrast,
        SolidAccessible,
    }

    data class OpticalAccessibilitySignals(
        val forcedColors: Boolean = false,
        val reducedTransparency: Boolean = false,
        val increasedContrast: Boolean = false,
    )

    data class AndroidBrowserMapping(
        val canvas: MaterialLevel,
        val topChrome: MaterialLevel,
        val omnibox: MaterialLevel,
        val addressField: MaterialLevel,
        val bottomChrome: MaterialLevel,
        val browserMenu: MaterialLevel,
        val shellSurface: ShellSurface,
        val clarity: Clarity,
        val expression: Expression,
        val dominantGlazePanels: Int,
        val smallFloatingGlazeControls: Int,
        val declaresUniversalSearch: Boolean,
        val declaresControlCenter: Boolean,
        val noActionBar: Boolean,
        val developerStatusInNormalChrome: Boolean,
        val effectsFreeFallback: Boolean,
        val usesNativeControls: Boolean,
        val usesVectorChromeIcons: Boolean,
        val usesPlatformPopupMenu: Boolean,
        val scrollAwareTopChrome: Boolean,
    )

    val ANDROID_BROWSER_MAPPING = AndroidBrowserMapping(
        canvas = MaterialLevel.Canvas,
        topChrome = MaterialLevel.Canvas,
        omnibox = MaterialLevel.SoftGlaze,
        addressField = MaterialLevel.Surface,
        bottomChrome = MaterialLevel.Surface,
        browserMenu = MaterialLevel.SoftGlaze,
        shellSurface = ShellSurface.Application,
        clarity = Clarity.Balanced,
        expression = Expression.Calm,
        dominantGlazePanels = 1,
        smallFloatingGlazeControls = 1,
        declaresUniversalSearch = false,
        declaresControlCenter = false,
        noActionBar = true,
        developerStatusInNormalChrome = false,
        effectsFreeFallback = true,
        usesNativeControls = true,
        usesVectorChromeIcons = true,
        usesPlatformPopupMenu = false,
        scrollAwareTopChrome = true,
    )

    fun targetFloorDp(touchAssistance: Boolean): Int =
        if (touchAssistance) TOUCH_ASSISTANCE_TARGET_DP else GENERAL_TARGET_DP

    fun satisfiesGeneralTargetFloor(sizeDp: Int): Boolean = sizeDp >= GENERAL_TARGET_DP

    fun satisfiesTouchAssistanceTargetFloor(sizeDp: Int): Boolean =
        sizeDp >= TOUCH_ASSISTANCE_TARGET_DP

    fun satisfiesSystemGlazeBudget(mapping: AndroidBrowserMapping): Boolean =
        mapping.dominantGlazePanels in 0..MAX_DOMINANT_GLAZE_PANELS &&
            mapping.smallFloatingGlazeControls in 0..MAX_SMALL_FLOATING_GLAZE_CONTROLS

    /** Accessibility and task completion outrank V1.4 optical decoration. */
    fun opticalMode(signals: OpticalAccessibilitySignals): OpticalMode = when {
        signals.forcedColors || signals.reducedTransparency -> OpticalMode.SolidAccessible
        signals.increasedContrast -> OpticalMode.IncreasedContrast
        else -> OpticalMode.Standard
    }

    fun allowsBackdropEffects(signals: OpticalAccessibilitySignals): Boolean =
        opticalMode(signals) != OpticalMode.SolidAccessible

    fun allowsDecorativeEnvironmentalTint(signals: OpticalAccessibilitySignals): Boolean =
        opticalMode(signals) == OpticalMode.Standard

    /** Higher value means higher Browser presentation priority. */
    fun statePriority(state: InteractionState): Int = when (state) {
        InteractionState.Rest -> 0
        InteractionState.Hover -> 1
        InteractionState.Selected -> 2
        InteractionState.Focus -> 3
        InteractionState.Pressed -> 4
        InteractionState.Loading -> 5
        InteractionState.Error -> 6
        InteractionState.Disabled -> 7
    }

    fun fixedChromeHeightDp(): Int =
        OMNIBOX_HEIGHT_DP + (CHROME_GUTTER_DP * 2) + BOTTOM_TOOLBAR_HEIGHT_DP

    fun collapsedChromeHeightDp(): Int = BOTTOM_TOOLBAR_HEIGHT_DP
}
