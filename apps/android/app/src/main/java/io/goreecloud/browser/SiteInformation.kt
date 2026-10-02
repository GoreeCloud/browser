package io.goreecloud.browser

import java.net.URI
import java.util.Locale

/**
 * Browser-owned, presentation-only site/origin summary.
 *
 * This reports the accepted URL origin and transport scheme without turning
 * HTTPS into a Wardveil verdict, trust claim, or substitute for engine-owned
 * certificate details.
 */
object SiteInformation {
    data class Summary(
        val displayOrigin: String,
        val transportLabel: String,
        val details: String,
        val isWebsite: Boolean,
    ) {
        val menuSubtitle: String
            get() = if (isWebsite) "$displayOrigin • $transportLabel" else "Development build"
    }

    fun forUrl(currentUrl: String): Summary {
        val canonical = NavigationResolver.canonicalizeWebUrl(currentUrl)
            ?: return Summary(
                displayOrigin = "Browser-owned local page",
                transportLabel = "No website connection",
                details = "This surface is generated locally by GoreeCloud Browser. It is not a website origin and is not exposed through page-address Copy or Share actions.",
                isWebsite = false,
            )

        val uri = URI(canonical)
        val scheme = uri.scheme.lowercase(Locale.ROOT)
        val displayOrigin = buildString {
            append(uri.host.orEmpty())
            val port = uri.port
            val defaultPort = (scheme == "https" && port == 443) || (scheme == "http" && port == 80)
            if (port >= 0 && !defaultPort) append(":$port")
        }

        return if (scheme == "https") {
            Summary(
                displayOrigin = displayOrigin,
                transportLabel = "HTTPS",
                details = "The page uses HTTPS transport. Browser still treats web content as untrusted, and this Development surface does not replace certificate details from the active engine.",
                isWebsite = true,
            )
        } else {
            Summary(
                displayOrigin = displayOrigin,
                transportLabel = "HTTP — not encrypted",
                details = "The page uses unencrypted HTTP transport. Avoid entering sensitive information unless you understand the risk.",
                isWebsite = true,
            )
        }
    }
}
