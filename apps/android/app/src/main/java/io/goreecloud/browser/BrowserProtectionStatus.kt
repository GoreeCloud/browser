package io.goreecloud.browser

/**
 * Truthful Browser-owned Development protection state for the Android status UI.
 *
 * Entries describe only controls enforced by Browser/WebView or explicit
 * fail-closed boundaries. They must not manufacture Privacy Shield, Wardveil,
 * Policy, Search, or other provider authority.
 */
object BrowserProtectionStatus {
    data class Entry(
        val label: String,
        val state: String,
        val details: String,
    )

    fun current(safeBrowsingEnabled: Boolean): List<Entry> = listOf(
        Entry(
            label = "Third-party cookies",
            state = "Blocked",
            details = "Android WebView third-party cookies are disabled for Browser pages.",
        ),
        Entry(
            label = "Mixed content",
            state = "Blocked",
            details = "HTTPS pages cannot load insecure HTTP subresources through WebView mixed-content mode.",
        ),
        Entry(
            label = "Local file and content access",
            state = "Blocked",
            details = "Web content cannot use WebView file or content access in this Development shell.",
        ),
        Entry(
            label = "Website permissions",
            state = "Denied by default",
            details = "Camera, microphone, geolocation, and other site permissions remain denied until Browser PermissionBroker runtime UX and required authority adapters are accepted.",
        ),
        Entry(
            label = "Downloads",
            state = "Fail-closed",
            details = "Downloads remain blocked from ordinary release until the Android path can satisfy the accepted Wardveil verification and release contract.",
        ),
        Entry(
            label = "Web search",
            state = "Fail-closed",
            details = "Free-text queries stay local until accepted Privacy Shield authorization and compatible GoreeCloud Search capability evidence are available.",
        ),
        Entry(
            label = "Android Safe Browsing",
            state = if (safeBrowsingEnabled) "Enabled" else "Unavailable on this platform",
            details = if (safeBrowsingEnabled) {
                "Browser enables the Android WebView Safe Browsing control. This is separate from Wardveil Security acceptance."
            } else {
                "Browser cannot claim the Android WebView Safe Browsing control on this platform level."
            },
        ),
    )
}
