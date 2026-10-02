package io.goreecloud.browser

import java.net.IDN
import java.net.Inet6Address
import java.net.InetAddress
import java.net.URI
import java.util.Locale

/**
 * Browser-owned syntactic boundary for internationalized HTTP(S) host identity.
 *
 * Unicode DNS names are converted to ASCII A-label form before navigation or
 * Browser-owned address presentation. This is deterministic host
 * canonicalization only; it does not claim DNS, certificate, reputation,
 * registrable-domain, or Unicode-confusable trust.
 */
internal object InternationalizedHostPolicy {
    data class CanonicalAuthority(
        val host: String,
        val authority: String,
    )

    fun canonicalizeHttpUrl(rawUrl: String): String? {
        if (SearchBoundaryTextSafety.containsUnsupportedControl(rawUrl)) return null

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
        if (SearchBoundaryTextSafety.containsUnsupportedControl(rawAuthority)) return null

        if (rawAuthority.startsWith('[')) {
            val closingBracket = rawAuthority.indexOf(']')
            if (closingBracket <= 1) return null

            val host = rawAuthority.substring(0, closingBracket + 1)
            val suffix = rawAuthority.substring(closingBracket + 1)
            if (!validPortSuffix(suffix)) return null

            val literal = host.substring(1, host.length - 1)
            if ('%' in literal || ':' !in literal) return null

            // The colon requirement prevents bracketed DNS names from reaching
            // InetAddress parsing. The remaining input is IPv6-shaped only, so
            // this validation cannot become an ordinary hostname lookup.
            val parsed = runCatching { InetAddress.getByName(literal) }.getOrNull()
            if (parsed !is Inet6Address) return null

            val canonicalHost = host.lowercase(Locale.ROOT)
            return CanonicalAuthority(
                host = canonicalHost,
                authority = canonicalHost + suffix,
            )
        }

        val colon = rawAuthority.lastIndexOf(':')
        if (colon >= 0 && rawAuthority.indexOf(':') != colon) return null

        val rawHost = if (colon >= 0) rawAuthority.substring(0, colon) else rawAuthority
        val portSuffix = if (colon >= 0) rawAuthority.substring(colon) else ""
        if (rawHost.isBlank() || !validPortSuffix(portSuffix)) return null

        val trailingRootDot = rawHost.endsWith('.')
        val hostBody = if (trailingRootDot) rawHost.dropLast(1) else rawHost
        if (hostBody.isBlank()) return null

        val asciiBody = runCatching {
            IDN.toASCII(hostBody, IDN.USE_STD3_ASCII_RULES)
        }.getOrNull()
            ?.takeIf { it.isNotBlank() }
            ?.lowercase(Locale.ROOT)
            ?: return null

        if (asciiBody.split('.').any { it.isEmpty() || it.length > 63 }) return null

        val asciiHost = asciiBody + if (trailingRootDot) "." else ""
        if (asciiHost.length > 253) return null

        val numericCandidate = asciiHost.removeSuffix(".")
        if (
            numericCandidate.all { it.isDigit() || it == '.' } &&
            !isCanonicalIpv4(numericCandidate)
        ) {
            return null
        }

        return CanonicalAuthority(
            host = asciiHost,
            authority = asciiHost + portSuffix,
        )
    }

    private fun isCanonicalIpv4(host: String): Boolean {
        val octets = host.split('.')
        if (octets.size != 4) return false
        return octets.all { octet ->
            octet.isNotEmpty() &&
                octet.length <= 3 &&
                (octet.length == 1 || !octet.startsWith('0')) &&
                octet.toIntOrNull()?.let { it in 0..255 } == true
        }
    }

    private fun validPortSuffix(suffix: String): Boolean {
        if (suffix.isEmpty()) return true
        if (!suffix.startsWith(':')) return false
        val text = suffix.substring(1)
        if (text.isEmpty() || text.any { !it.isDigit() }) return false
        return text.toIntOrNull()?.let { it in 1..65535 } == true
    }
}
