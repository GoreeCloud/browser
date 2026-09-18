package io.goreecloud.browser

internal object BrowserLocalPages {
    fun pageUnavailableHtml(baseCss: String): String = """
        <!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
        <title>Page unavailable</title><style>$baseCss</style></head><body><main>
        <div class="mark">G</div><h1>Page unavailable</h1>
        <p>GoreeCloud Browser could not load this page. Check the address or connection, then use Reload to try again.</p>
        <p>No remote engine or network error details are shown on this local recovery surface.</p>
        </main></body></html>
    """.trimIndent()
}
