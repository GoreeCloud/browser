package io.goreecloud.browser

internal enum class BrowserDurableStartupKind {
    Fresh,
    Recovered,
    RecoveredEmpty,
    Reused,
    Unavailable,
}

internal data class BrowserDurableStartup(
    val kind: BrowserDurableStartupKind,
    val state: BrowserTabSessionState? = null,
)

/**
 * Validates the bounded projection returned by the Browser-owned native
 * Normal-session recovery core before any durable state becomes Android UI.
 */
internal object BrowserDurableRecoveryPolicy {
    private const val INTERNAL_HOME = "goreecloud://start"

    fun parse(fields: Array<String>?): BrowserDurableStartup {
        if (fields.isNullOrEmpty()) return unavailable()

        return when (fields[0]) {
            "fresh", "clean_fresh" -> {
                if (fields.size != 1) unavailable()
                else BrowserDurableStartup(BrowserDurableStartupKind.Fresh)
            }
            "reused" -> {
                if (fields.size != 1) unavailable()
                else BrowserDurableStartup(BrowserDurableStartupKind.Reused)
            }
            "recovered_empty" -> {
                if (fields.size != 1) unavailable()
                else BrowserDurableStartup(BrowserDurableStartupKind.RecoveredEmpty)
            }
            "recovered" -> parseRecovered(fields)
            "unavailable" -> unavailable()
            else -> unavailable()
        }
    }

    private fun parseRecovered(fields: Array<String>): BrowserDurableStartup {
        if (fields.size < 5 || (fields.size - 2) % 3 != 0) return unavailable()
        val activeTabId = fields[1]
        val tabCount = (fields.size - 2) / 3
        if (tabCount !in 1..BrowserTabSessionPolicy.MAX_TABS) return unavailable()

        val ids = ArrayList<String>(tabCount)
        val urls = ArrayList<String>(tabCount)
        val titles = ArrayList<String>(tabCount)
        repeat(tabCount) { index ->
            val offset = 2 + index * 3
            ids += fields[offset]
            urls += fields[offset + 1].ifEmpty { INTERNAL_HOME }
            titles += fields[offset + 2]
        }

        val state = BrowserTabRecreationPolicy.restore(
            ids = ids,
            urls = urls,
            titles = titles,
            activeTabId = activeTabId,
        ) ?: return unavailable()

        return BrowserDurableStartup(
            kind = BrowserDurableStartupKind.Recovered,
            state = state,
        )
    }

    private fun unavailable(): BrowserDurableStartup =
        BrowserDurableStartup(BrowserDurableStartupKind.Unavailable)
}
