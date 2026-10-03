package io.goreecloud.browser

import android.content.Context
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class BrowserTabSessionStoreRuntimeTest {
    private val context by lazy {
        ApplicationProvider.getApplicationContext<Context>()
    }

    @Before
    fun clearSession() {
        assertTrue(BrowserTabSessionStore(context).clear())
    }

    @Test
    fun regularTabMetadataPersistsWithoutPageState() {
        val state = BrowserTabSessionState(
            tabs = listOf(
                BrowserLogicalTab("tab-a", "goreecloud://start"),
                BrowserLogicalTab("tab-b", "https://example.com/path", " Example "),
            ),
            activeTabId = "tab-b",
        )

        assertTrue(BrowserTabSessionStore(context).write(state))

        val restored = BrowserTabSessionStore(context).read()
        assertEquals(listOf("tab-a", "tab-b"), restored?.tabs?.map { it.id })
        assertEquals("https://example.com/path", restored?.activeTab?.url)
        assertEquals(" Example ", restored?.activeTab?.title)
    }

    @Test
    fun codecRejectsUnsafeOrMalformedRecoveredLocations() {
        assertNull(
            BrowserTabSessionCodec.decode(
                """{"version":1,"activeTabId":"tab-a","tabs":[{"id":"tab-a","url":"javascript:alert(1)","title":null}]}""",
            ),
        )
        assertNull(BrowserTabSessionCodec.decode("""{"version":1,"tabs":[]}"""))
        assertNull(BrowserTabSessionCodec.decode("not-json"))
    }

    @Test
    fun codecNormalizesRecoveredTitlesAndPreservesActiveSelection() {
        val decoded = BrowserTabSessionCodec.decode(
            """{"version":1,"activeTabId":"tab-b","tabs":[{"id":"tab-a","url":"goreecloud://start","title":null},{"id":"tab-b","url":"https://example.com/","title":"  Example  "}]}""",
        )

        assertEquals("tab-b", decoded?.activeTabId)
        assertEquals("Example", decoded?.activeTab?.title)
    }
}
