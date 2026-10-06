package io.goreecloud.browser

import android.content.Context
import java.io.File
import java.util.UUID

/**
 * Process-scoped Android adapter over the Browser-owned C++ Normal-session
 * runtime. Kotlin never owns the durable journal/checkpoint schema.
 */
internal object BrowserNormalSessionRecovery {
    const val WINDOW_ID = "android-main"
    private const val PROFILE_ID = "android-default"
    private const val STORE_DIRECTORY = "normal-session-v1"

    private var attached = false
    private var recoveryActive = false

    fun attach(context: Context): BrowserDurableStartup {
        if (attached) {
            return BrowserDurableStartup(BrowserDurableStartupKind.Reused)
        }

        val processId = UUID.randomUUID().toString()
        val raw = runCatching {
            BrowserNormalSessionNative.start(
                storageDirectory = File(
                    context.applicationContext.noBackupFilesDir,
                    STORE_DIRECTORY,
                ).absolutePath,
                journalId = "android-journal-$processId",
                profileId = PROFILE_ID,
                sessionEpoch = "android-epoch-$processId",
                checkpointPrefix = "android-checkpoint-$processId",
            )
        }.getOrNull()

        val startup = BrowserDurableRecoveryPolicy.parse(raw)
        attached = true
        recoveryActive = startup.kind != BrowserDurableStartupKind.Unavailable

        if (!recoveryActive) {
            runCatching { BrowserNormalSessionNative.release() }
        }
        return startup
    }

    fun seedFreshState(state: BrowserTabSessionState): Boolean {
        if (!recoveryActive) return true
        if (!BrowserNormalSessionNative.windowOpened(WINDOW_ID)) return false
        state.tabs.forEachIndexed { index, tab ->
            if (!BrowserNormalSessionNative.tabOpened(
                    WINDOW_ID,
                    tab.id,
                    tab.url,
                    tab.title.orEmpty(),
                    index,
                )
            ) {
                return false
            }
        }
        return BrowserNormalSessionNative.tabSelected(WINDOW_ID, state.activeTabId)
    }

    fun tabOpened(tab: BrowserLogicalTab, position: Int): Boolean =
        !recoveryActive || BrowserNormalSessionNative.tabOpened(
            WINDOW_ID,
            tab.id,
            tab.url,
            tab.title.orEmpty(),
            position,
        )

    fun tabSelected(tabId: String): Boolean =
        !recoveryActive || BrowserNormalSessionNative.tabSelected(WINDOW_ID, tabId)

    fun tabClosedBeforeMutation(tabId: String): Boolean =
        !recoveryActive || BrowserNormalSessionNative.tabClosed(WINDOW_ID, tabId)

    fun tabNavigated(tabId: String, url: String): Boolean =
        !recoveryActive || BrowserNormalSessionNative.tabNavigated(WINDOW_ID, tabId, url)

    fun tabTitled(tabId: String, title: String?): Boolean =
        !recoveryActive || BrowserNormalSessionNative.tabTitled(
            WINDOW_ID,
            tabId,
            title.orEmpty(),
        )

    fun background(): Boolean =
        !recoveryActive || BrowserNormalSessionNative.background()

    fun resume(): Boolean =
        !recoveryActive || BrowserNormalSessionNative.resume()

    fun finishCleanly(): Boolean {
        if (!attached) return true

        val result = if (recoveryActive) {
            BrowserNormalSessionNative.windowClosed(WINDOW_ID) &&
                BrowserNormalSessionNative.cleanShutdown()
        } else {
            true
        }

        runCatching { BrowserNormalSessionNative.release() }
        attached = false
        recoveryActive = false
        return result
    }

    fun recoveryEnabled(): Boolean = recoveryActive
}

private object BrowserNormalSessionNative {
    init {
        System.loadLibrary("goreecloud_browser_android_session")
    }

    external fun start(
        storageDirectory: String,
        journalId: String,
        profileId: String,
        sessionEpoch: String,
        checkpointPrefix: String,
    ): Array<String>

    external fun windowOpened(windowId: String): Boolean
    external fun windowClosed(windowId: String): Boolean

    external fun tabOpened(
        windowId: String,
        tabId: String,
        url: String,
        title: String,
        position: Int,
    ): Boolean

    external fun tabSelected(windowId: String, tabId: String): Boolean
    external fun tabClosed(windowId: String, tabId: String): Boolean
    external fun tabNavigated(windowId: String, tabId: String, url: String): Boolean
    external fun tabTitled(windowId: String, tabId: String, title: String): Boolean

    external fun background(): Boolean
    external fun resume(): Boolean
    external fun cleanShutdown(): Boolean
    external fun release()
}
