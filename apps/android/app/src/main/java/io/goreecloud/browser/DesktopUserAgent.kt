package io.goreecloud.browser

/**
 * Produces a bounded desktop-site user agent from the active Android WebView
 * user agent without pinning Browser to a stale Chromium version.
 */
object DesktopUserAgent {
    fun fromMobile(userAgent: String): String {
        val trimmed = userAgent.trim()
        if (trimmed.isEmpty()) return trimmed

        var result = trimmed
            .replace(Regex("""Mozilla/5\.0 \([^)]*\)"""), "Mozilla/5.0 (X11; Linux x86_64)")
            .replace("; wv", "")
            .replace(" Version/4.0", "")
            .replace(" Mobile", "")
            .replace(
                Regex("""GoreeCloudBrowser/([^\s]+)\s+Android"""),
                "GoreeCloudBrowser/$1 DesktopSite",
            )

        result = result.replace(Regex("""\s+"""), " ").trim()
        return result
    }
}
