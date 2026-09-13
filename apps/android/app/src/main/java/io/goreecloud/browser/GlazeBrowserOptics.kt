package io.goreecloud.browser

/**
 * Browser-specific GLAZE UI V1.4 Optical Intelligence policy.
 *
 * Web content is an untrusted rendering plane. Page pixels, origin identity,
 * favicon color, CSS/theme color, media frames, document text, browsing history,
 * query text, security indicators, and page-provided metadata must never become
 * implicit optical inputs for trusted Browser-owned chrome.
 */
object GlazeBrowserOptics {
    const val TARGET_VERSION = "1.4.0"
    const val STABLE_SOURCE_REVISION = "84cb3db4884042f0fa25ed6d475a127fb110f596"

    const val PAGE_CONTENT_MAY_DRIVE_TRUSTED_CHROME = false
    const val ORIGIN_IDENTITY_MAY_DRIVE_TRUSTED_CHROME = false
    const val FAVICON_COLOR_MAY_DRIVE_TRUSTED_CHROME = false
    const val PAGE_THEME_COLOR_MAY_DRIVE_TRUSTED_CHROME = false
    const val SECURITY_STATE_MAY_BE_INFERRED_FROM_OPTICS = false
    const val PRIVACY_STATE_MAY_BE_INFERRED_FROM_OPTICS = false
    const val TELEMETRY_REQUIRED = false
    const val REMOTE_CONTEXT_REQUIRED = false
    const val MAX_TRUSTED_CHROME_MEMORY_TINT_INFLUENCE = 0f

    data class Accessibility(
        val reducedTransparency: Boolean = false,
        val increasedContrast: Boolean = false,
        val forcedColors: Boolean = false,
    )

    data class State(
        val mode: Mode,
        val frostStrength: Float,
        val blurScale: Float,
        val semanticProtection: Float,
        val decorativeTintAllowed: Boolean,
        val memoryTintInfluence: Float,
    ) {
        enum class Mode { TRUSTED_NEUTRAL_OPTICAL, SOLID_ACCESSIBLE }
    }

    /**
     * Resolve Browser-owned trusted chrome from non-content accessibility state.
     * Existing Android styling remains effects-free, so this policy can govern
     * future optical enhancement without making transparency a usability
     * dependency.
     */
    fun resolve(accessibility: Accessibility = Accessibility()): State {
        if (accessibility.forcedColors || accessibility.reducedTransparency) {
            return State(
                mode = State.Mode.SOLID_ACCESSIBLE,
                frostStrength = 1f,
                blurScale = 0f,
                semanticProtection = 1f,
                decorativeTintAllowed = false,
                memoryTintInfluence = 0f,
            )
        }

        return State(
            mode = State.Mode.TRUSTED_NEUTRAL_OPTICAL,
            frostStrength = if (accessibility.increasedContrast) 0.78f else 0.62f,
            blurScale = if (accessibility.increasedContrast) 0.54f else 0.62f,
            semanticProtection = 1f,
            decorativeTintAllowed = false,
            memoryTintInfluence = 0f,
        )
    }
}
