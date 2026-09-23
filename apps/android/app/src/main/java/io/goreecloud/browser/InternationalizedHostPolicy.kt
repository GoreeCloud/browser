package io.goreecloud.browser

import java.net.IDN
import java.net.Inet6Address
import java.net.InetAddress
import java.net.URI
import java.util.Locale

/**
 * Browser-owned boundary for internationalized HTTP(S) host identity.
 *
 * Unicode domain names are converted to ASCII A-label form before direct
 * navigation or Browser-owned address presentation. This keeps host identity
 * deterministic without claiming DNS, certificate, reputation, registrable-
 * domain, or complete Unicode-confusable trust.
 */
internal object InternationalizedHostPolicy {
    data class CanonicalAuthority(
        val host: String,
        val authority: String,
    )

    fun canonicalizeHttpUrl(rawUrl: String): String? {
        val uri = runCatching { URI(rawUrl.trim()) }.getOrNull() ?: return null
        val scheme = uri.scheme?.lowercase(Locale.ROOT) ?: return null
        if (scheme != "https" && scheme != "http") return null
        if (uri.rawUserInfo != null) return null

        val rawAuthority = uri.rawAuthority ?: return null
        val canonical = canonicalAuthority(rawAuthority) ?: return null

        return buildString {
            append(scheme)
            append("://")
            append(canonical.authority)
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
        if (rawAuthority.isBlank() || '@' in rawAuthority) return null
        if (rawAuthority.any(Char::isISOControl)) return null

        if (rawAuthority.startsWith('[')) {
            val closingBracket = rawAuthority.indexOf(']')
            if (closingBracket <= 1) return null
            val host = rawAuthority.substring(0, closingBracket + 1)
            val suffix = rawAuthority.substring(closingBracket + 1)
            if (!validPortSuffix(suffix)) return null

            val literal = host.substring(1, host.length - 1)
            if ('%' in literal) return null
            val parsed = runCatching { InetAddress.getByName(literal) }.getOrNull()
            if (parsed !is Inet6Address) return null
            val canonicalHost = host.lowercase(Locale.ROOT)
            return CanonicalAuthority(host = canonicalHost, authority = canonicalHost + suffix)
        }

        val colon = rawAuthority.lastIndexOf(':')
        if (colon >= 0 && rawAuthority.indexOf(':') != colon) return null

        val rawHost = if (colon >= 0) rawAuthority.substring(0, colon) else rawAuthority
        val portSuffix = if (colon >= 0) rawAuthority.substring(colon) else ""
        if (rawHost.isBlank() || !validPortSuffix(portSuffix)) return null

        val asciiHost = runCatching {
            IDN.toASCII(rawHost, IDN.USE_STD3_ASCII_RULES)
        }.getOrNull()
            ?.takeIf { it.isNotBlank() && it.length <= 253 }
            ?.lowercase(Locale.ROOT)
            ?: return null

        if (asciiHost.split('.').any { it.isEmpty() || it.length > 63 }) return null

        return CanonicalAuthority(
            host = asciiHost,
            authority = asciiHost + portSuffix,
        )
    }

    private fun validPortSuffix(suffix: String): Boolean {
        if (suffix.isEmpty()) return true
        if (!suffix.startsWith(':')) return false
        val text = suffix.substring(1)
        if (text.isEmpty() || text.any { !it.isDigit() }) return false
        return text.toIntOrNull()?.let { it in 0..65535 } == true
    }
}
