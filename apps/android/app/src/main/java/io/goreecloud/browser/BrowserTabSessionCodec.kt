package io.goreecloud.browser

import java.nio.charset.StandardCharsets
import java.util.Base64

/**
 * Bounded, versioned persistence codec for Browser-owned logical tab/session state.
 *
 * Only logical tab identity, last accepted location, bounded title, and active
 * tab identity are durable here. WebView history remains a separate runtime
 * concern and is not fabricated by this codec.
 */
object BrowserTabSessionCodec {
    private const val VERSION = "goreecloud-browser-tabs/1"
    private const val MAX_SERIALIZED_BYTES = 96 * 1024

    fun encode(state: BrowserTabSessionState): String {
        val lines = ArrayList<String>(state.tabs.size + 2)
        lines += VERSION
        lines += "active\t${encodeField(state.activeTabId)}"
        state.tabs.forEach { tab ->
            lines += buildString {
                append("tab\t")
                append(encodeField(tab.id))
                append('\t')
                append(encodeField(tab.url))
                append('\t')
                append(encodeField(tab.title.orEmpty()))
            }
        }
        val encoded = lines.joinToString("\n")
        require(encoded.toByteArray(StandardCharsets.UTF_8).size <= MAX_SERIALIZED_BYTES)
        return encoded
    }

    fun decode(serialized: String?): BrowserTabSessionState? {
        if (serialized.isNullOrBlank()) return null
        if (serialized.toByteArray(StandardCharsets.UTF_8).size > MAX_SERIALIZED_BYTES) return null

        val lines = serialized.lineSequence().toList()
        if (lines.size < 3 || lines.first() != VERSION) return null

        val activeParts = lines[1].split('\t')
        if (activeParts.size != 2 || activeParts[0] != "active") return null
        val activeId = decodeField(activeParts[1])?.takeIf { it.isNotBlank() } ?: return null

        val tabs = ArrayList<BrowserLogicalTab>(lines.size - 2)
        for (line in lines.drop(2)) {
            val parts = line.split('\t')
            if (parts.size != 4 || parts[0] != "tab") return null
            val id = decodeField(parts[1]) ?: return null
            val url = decodeField(parts[2]) ?: return null
            val title = decodeField(parts[3])?.takeIf { it.isNotBlank() }
            if (tabs.size >= BrowserTabSessionPolicy.MAX_TABS) return null
            try {
                tabs += BrowserLogicalTab(id = id, url = url, title = title)
            } catch (_: IllegalArgumentException) {
                return null
            }
        }

        return try {
            BrowserTabSessionState(tabs = tabs, activeTabId = activeId)
        } catch (_: IllegalArgumentException) {
            null
        }
    }

    private fun encodeField(value: String): String =
        Base64.getUrlEncoder()
            .withoutPadding()
            .encodeToString(value.toByteArray(StandardCharsets.UTF_8))

    private fun decodeField(value: String): String? =
        try {
            String(Base64.getUrlDecoder().decode(value), StandardCharsets.UTF_8)
        } catch (_: IllegalArgumentException) {
            null
        }
}
