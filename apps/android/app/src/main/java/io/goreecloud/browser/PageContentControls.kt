package io.goreecloud.browser

/**
 * Browser-owned session-local page-content controls.
 *
 * These are presentation/runtime preferences for the current Browser session,
 * not durable profile policy, Privacy Shield authority, or synchronized state.
 */
object PageContentControls {
    data class State(
        val javaScriptEnabled: Boolean = true,
        val imagesEnabled: Boolean = true,
    )

    fun javaScriptLabel(enabled: Boolean): String =
        if (enabled) "JavaScript · On" else "JavaScript · Off"

    fun imagesLabel(enabled: Boolean): String =
        if (enabled) "Images · On" else "Images · Off"
}
