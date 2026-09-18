package io.goreecloud.browser

/**
 * Browser-owned Android-native mapping for GLAZE UI V1.5 / 1.5.1 Stable.
 *
 * V1.5 inherits the V1.4.1 optical hardening baseline and adds a bounded
 * context/capability presentation-resolution layer. Glaze consumes authority
 * truth supplied by the owning system; it never creates authorization,
 * permission, provider precedence, navigation authority, or execution authority.
 *
 * This source is implementation/mapping evidence only. Browser-local rendered,
 * accessibility, representative-device, performance, rollback, release, and
 * production acceptance remain separate gates.
 */
object GlazeContract {
    const val VERSION = "1.5.1"
    const val STABLE_RELEASE_REVISION = "98da57064ede0f334627b632bc16801f580331af"
    const val REVIEWED_IMPLEMENTATION_ANCHOR = "ee1032a0822ab8e103f8afe48e5c1859fde65cc9"
    const val QUALIFICATION_SOURCE_ANCHOR = "5b59d0e36950d737dba35b58ae58058684e0831b"
    const val QUALIFICATION_INTEGRATION_REVISION = "f7ef915f0aabea6cf92748018f2220a99e3a9c92"
    const val OPTICAL_BASELINE_VERSION = "1.4.1"
    const val OPTICAL_BASELINE_REVISION = "4fab9da0fad2e5c974e0e66ec88632c61745751c"
    const val IMMEDIATE_ROLLBACK_VERSION = "1.5.0"

    // Compatibility names retained for existing Browser source consumers.
    // ACCEPTED_VISUAL_SOURCE identifies the reviewed central Glaze implementation
    // source only; it does not claim Browser-local rendered visual acceptance.
    const val SOURCE_INTEGRATION_ANCHOR = STABLE_RELEASE_REVISION
    const val ACCEPTED_VISUAL_SOURCE = REVIEWED_IMPLEMENTATION_ANCHOR

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

    // Browser-local conservative composition budget. Glaze does not grant
    // Browser authority to turn ordinary app chrome into system UI.
    const val MAX_DOMINANT_GLAZE_PANELS = 1
    const val MAX_SMALL_FLOATING_GLAZE_CONTROLS = 3

    // Inherited V1.4.1 optical memory is decorative and remains bounded.
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
     * Inherited system-shell hierarchy. Browser-owned chrome remains within
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

    enum class CapabilityState {
        Available,
        TemporarilyUnavailable,
        Restricted,
        Unknown,
        Conflict,
    }

    data class OpticalAccessibilitySignals(
        val forcedColors: Boolean = false,
        val reducedTransparency: Boolean = false,
        val increasedContrast: Boolean = false,
    )

    /**
     * Coarse authority provenance only. Browser must not place query text,
     * browsing history, credentials, authorization tokens, private identifiers,
     * or other sensitive content in Glaze capability records.
     */
    data class CapabilityRecord(
        val id: String,
        val state: CapabilityState,
        val authorityDomain: String,
    )

    data class ActionRequest(
        val id: String,
        val requiredCapabilityIds: Set<String>,
        val consequential: Boolean = false,
    )

    data class ActionPresentation(
        val actionId: String,
        val enabled: Boolean,
        val state: CapabilityState,
        val reasonCodes: Set<String>,
        val automaticExecutionAllowed: Boolean = false,
        val authorityInferred: Boolean = false,
        val providerPrecedenceInferred: Boolean = false,
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

    /** Accessibility and task completion outrank inherited optical decoration. */
    fun opticalMode(signals: OpticalAccessibilitySignals): OpticalMode = when {
        signals.forcedColors || signals.reducedTransparency -> OpticalMode.SolidAccessible
        signals.increasedContrast -> OpticalMode.IncreasedContrast
        else -> OpticalMode.Standard
    }

    fun allowsBackdropEffects(signals: OpticalAccessibilitySignals): Boolean =
        opticalMode(signals) != OpticalMode.SolidAccessible

    fun allowsDecorativeEnvironmentalTint(signals: OpticalAccessibilitySignals): Boolean =
        opticalMode(signals) == OpticalMode.Standard

    /**
     * Resolves only presentation eligibility from already authoritative capability
     * state. Missing and ambiguous capability ownership fail closed. The result
     * can never authorize or automatically execute the underlying Browser action.
     */
    fun resolveAction(
        action: ActionRequest,
        capabilities: Collection<CapabilityRecord>,
    ): ActionPresentation {
        val recordsById = capabilities.groupBy { it.id }
        val required = action.requiredCapabilityIds.sorted()

        if (required.isEmpty()) {
            return ActionPresentation(
                actionId = action.id,
                enabled = true,
                state = CapabilityState.Available,
                reasonCodes = emptySet(),
            )
        }

        val reasons = linkedSetOf<String>()
        var resolvedState = CapabilityState.Available

        for (capabilityId in required) {
            val records = recordsById[capabilityId].orEmpty()
            val state = when {
                records.isEmpty() -> CapabilityState.Unknown
                records.size > 1 -> CapabilityState.Conflict
                else -> records.single().state
            }

            when (state) {
                CapabilityState.Available -> Unit
                CapabilityState.TemporarilyUnavailable -> {
                    resolvedState = strongestCapabilityState(resolvedState, state)
                    reasons += "temporarily-unavailable:$capabilityId"
                }
                CapabilityState.Restricted -> {
                    resolvedState = strongestCapabilityState(resolvedState, state)
                    reasons += "restricted-by-authority:$capabilityId"
                }
                CapabilityState.Unknown -> {
                    resolvedState = strongestCapabilityState(resolvedState, state)
                    reasons += "capability-unknown:$capabilityId"
                }
                CapabilityState.Conflict -> {
                    resolvedState = strongestCapabilityState(resolvedState, state)
                    reasons += "capability-conflict:$capabilityId"
                }
            }
        }

        return ActionPresentation(
            actionId = action.id,
            enabled = resolvedState == CapabilityState.Available,
            state = resolvedState,
            reasonCodes = reasons,
            automaticExecutionAllowed = false,
            authorityInferred = false,
            providerPrecedenceInferred = false,
        )
    }

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

    private fun strongestCapabilityState(
        current: CapabilityState,
        candidate: CapabilityState,
    ): CapabilityState {
        val order = mapOf(
            CapabilityState.Available to 0,
            CapabilityState.TemporarilyUnavailable to 1,
            CapabilityState.Restricted to 2,
            CapabilityState.Unknown to 3,
            CapabilityState.Conflict to 4,
        )
        return if (order.getValue(candidate) > order.getValue(current)) candidate else current
    }
}
