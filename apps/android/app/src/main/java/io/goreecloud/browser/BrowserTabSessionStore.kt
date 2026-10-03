package io.goreecloud.browser

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject

/**
 * Device-local recovery store for the regular Browser tab session.
 *
 * This intentionally persists only bounded logical tab identity, URL, title,
 * and active-tab selection. It does not serialize cookies, form contents,
 * page DOM, WebView history, credentials, or private/profile state.
 */
class BrowserTabSessionStore(context: Context) {
    private val preferences =
        context.getSharedPreferences(PREFERENCES_NAME, Context.MODE_PRIVATE)

    fun read(): BrowserTabSessionState? {
        val raw = preferences.getString(KEY_SESSION, null)?.takeIf { it.isNotBlank() }
            ?: return null
        return BrowserTabSessionCodec.decode(raw)
    }

    fun write(state: BrowserTabSessionState): Boolean =
        preferences.edit()
            .putString(KEY_SESSION, BrowserTabSessionCodec.encode(state))
            .commit()

    fun clear(): Boolean = preferences.edit().remove(KEY_SESSION).commit()

    companion object {
        private const val PREFERENCES_NAME = "goreecloud_browser_tab_session"
        private const val KEY_SESSION = "regular_session_v1"
    }
}

object BrowserTabSessionCodec {
    private const val VERSION = 1
    private const val INTERNAL_HOME = "goreecloud://start"

    fun encode(state: BrowserTabSessionState): String {
        val tabs = JSONArray()
        state.tabs.forEach { tab ->
            tabs.put(
                JSONObject()
                    .put("id", tab.id)
                    .put("url", tab.url)
                    .put("title", tab.title ?: JSONObject.NULL),
            )
        }
        return JSONObject()
            .put("version", VERSION)
            .put("activeTabId", state.activeTabId)
            .put("tabs", tabs)
            .toString()
    }

    fun decode(raw: String): BrowserTabSessionState? {
        return try {
            val root = JSONObject(raw)
            if (root.optInt("version", -1) != VERSION) return null

            val array = root.optJSONArray("tabs") ?: return null
            if (array.length() !in 1..BrowserTabSessionPolicy.MAX_TABS) return null

            val tabs = mutableListOf<BrowserLogicalTab>()
            repeat(array.length()) { index ->
                val item = array.optJSONObject(index) ?: return null
                val id = item.optString("id", "")
                val url = item.optString("url", "")
                if (url != INTERNAL_HOME && !NavigationResolver.isAllowedWebUrl(url)) {
                    return null
                }
                val title = if (item.isNull("title")) {
                    null
                } else {
                    item.optString("title", "")
                        .trim()
                        .takeIf { it.isNotEmpty() }
                }
                tabs += BrowserLogicalTab(id = id, url = url, title = title)
            }

            val activeTabId = root.optString("activeTabId", "")
            BrowserTabSessionState(tabs = tabs, activeTabId = activeTabId)
        } catch (_: Exception) {
            null
        }
    }
}
