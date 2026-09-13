package io.goreecloud.browser

import java.net.IDN
import java.net.URI
import java.util.Locale

/**
 * Browser-owned boundary for internationalized HTTP(S) host identity.
 *
 * Unicode domain names are converted to their ASCII A-label form before Browser
 * grants direct-navigation authority or presents a condensed origin. This keeps
 * internationalized domains usable while avoiding a Browser-owned Unicode host
 * rendering that could visually resemble an unrelated ASCII origin.
 *
 * This is intentionally not a complete confusable/trust engine. DNS, certificate
 * validation, registrable-domain policy, UTS #39 analysis, reputation, and
 * Wardveil decisions remain separate authorities.
 */
object InternationalizedHostPolicy {
    data class CanonicalAuthority(
        val host: String,
        val authority: String,
    )

    fun canonicalizeHttpUrl(url: String): String? {
        val uri = runCatching { URI(url) }.getOrNull() ?: return null
        val scheme = uri.scheme?.lowercase(Locale.ROOT) ?: return null
        if (scheme != "https" && scheme != "http") return null
        if (uri.rawUserInfo != null) return null

        val rawAuthority = uri.rawAuthority ?: return null
        val canonicalAuthority = canonicalAuthority(rawAuthority) ?: return null

        return buildString {
            append(scheme)
            append("://")
            append(canonicalAuthority.authority)
            append(uri.rawPath.orEmpty())
            uri.rawQuery?.let {
                append('?')
                append(it)
            }
            uri.rawFragment?.let {
                append('#')
                append(it)
            }
        }
    }

    fun canonicalAuthority(rawAuthority: String): CanonicalAuthority? {
        if (rawAuthority.isEmpty() || '@' in rawAuthority) return null

        if (rawAuthority.startsWith('[')) {
            val closingBracket = rawAuthority.indexOf(']')
            if (closingBracket <= 1) return null

            val host = rawAuthority.substring(0, closingBracket + 1)
            val suffix = rawAuthority.substring(closingBracket + 1)
            if (!validPortSuffix(suffix)) return null
            return CanonicalAuthority(host = host, authority = host + suffix)
        }

        val colon = rawAuthority.lastIndexOf(':')
        if (colon >= 0 && rawAuthority.indexOf(':') != colon) return null

        val rawHost = if (colon >= 0) rawAuthority.substring(0, colon) else rawAuthority
        if (rawHost.isEmpty()) return null

        val portSuffix = if (colon >= 0) rawAuthority.substring(colon) else ""
        if (!validPortSuffix(portSuffix)) return null

        val asciiHost = runCatching {
            IDN.toASCII(rawHost, IDN.USE_STD3_ASCII_RULES)
        }.getOrNull()?.takeIf { it.isNotEmpty() }?.lowercase(Locale.ROOT) ?: return null

        return CanonicalAuthority(
            host = asciiHost,
            authority = asciiHost + portSuffix,
        )
    }

    private fun validPortSuffix(suffix: String): Boolean {
        if (suffix.isEmpty()) return true
        if (!suffix.startsWith(':')) return false

        val portText = suffix.substring(1)
        if (portText.isEmpty() || portText.any { !it.isDigit() }) return false
        val port = portText.toIntOrNull() ?: return false
        return port in 0..65535
    }
}
