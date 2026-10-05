package io.goreecloud.browser

internal object BrowserLocalPages {
    fun secureHead(baseCss: String, title: String? = null): String = buildString {
        append("<head>")
        append("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">")
        append(
            "<meta http-equiv=\"Content-Security-Policy\" " +
                "content=\"default-src 'none'; style-src 'unsafe-inline'; " +
                "base-uri 'none'; form-action 'none'; object-src 'none'\">",
        )
        append("<meta name=\"referrer\" content=\"no-referrer\">")
        if (title != null) {
            append("<title>")
            append(title)
            append("</title>")
        }
        append("<style>")
        append(baseCss)
        append("</style></head>")
    }

    fun pageUnavailableHtml(baseCss: String): String = """
        <!doctype html><html>${secureHead(baseCss, "Page unavailable")}<body><main>
        <div class="mark">G</div><h1>Page unavailable</h1>
        <p>GoreeCloud Browser could not load this page. Check the address or connection, then use Reload to try again.</p>
        <p>No remote engine or network error details are shown on this local recovery surface.</p>
        </main></body></html>
    """.trimIndent()

    fun blockedWebNavigationHtml(baseCss: String, safeLabel: String): String = """
        <!doctype html><html>${secureHead(baseCss, "Navigation blocked")}<body><main>
        <div class="mark">G</div><h1>Navigation blocked</h1>
        <p>This page tried to open a link type that GoreeCloud Browser does not currently allow from web content.</p>
        <p>The blocked target was not opened, sent to GoreeCloud Search, or exposed on this local safety surface.</p>
        <p class="query">$safeLabel</p>
        <p>Use Back to return to the previous page.</p>
        </main></body></html>
    """.trimIndent()
}
