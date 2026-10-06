package io.goreecloud.browser

import android.content.Context
import java.io.File
import java.util.UUID

internal object BrowserNormalSessionNative {
    init {
        System.loadLibrary("goreecloud_browser_android_recovery")
    }

    external fun nativeOpen(storageDirectory: String, freshEpoch: String): Long
    external fun nativeWasRecovered(handle: Long): Boolean
    external fun nativeRecoveredFlatTabs(handle: Long): Array<String>?
    external fun nativeRecoveredActiveTabId(handle: Long): String?
    external fun nativeDiscardRecoveredAndRestart(handle: Long, freshEpoch: String): Boolean
    external fun nativeSeed(
        handle: Long,
        windowId: String,
        flatTabs: Array<String>,
        activeTabId: String,
    ): Boolean
    external fun nativeTabOpened(
        handle: Long,
        windowId: String,
        tabId: String,
        url: String,
        title: String,
        position: Int,
    ): Boolean
    external fun nativeTabSelected(handle: Long, windowId: String, tabId: String): Boolean
    external fun nativeTabClosed(handle: Long, windowId: String, tabId: String): Boolean
    external fun nativeTabNavigated(
        handle: Long,
        windowId: String,
        tabId: String,
        url: String,
    ): Boolean
    external fun nativeTabTitled(
        handle: Long,
        windowId: String,
        tabId: String,
        title: String,
    ): Boolean
    external fun nativeBackground(handle: Long): Boolean
    external fun nativeResume(handle: Long): Boolean
    external fun nativeCleanShutdown(handle: Long): Boolean
    external fun nativeClose(handle: Long)
}

internal class BrowserNormalSessionRecovery private constructor(
    private var handle: Long,
    val recoveredState: BrowserTabSessionState?,
    private var seeded: Boolean,
) {
    private var failedAfterParticipation = false
    private var closed = false

    fun bindInitialState(state: BrowserTabSessionState): Boolean {
        if (closed || handle == 0L || seeded) return true

        val flat = state.tabs.flatMap { tab ->
            listOf(tab.id, tab.url, tab.title.orEmpty())
        }.toTypedArray()
        val success = BrowserNormalSessionNative.nativeSeed(
            handle,
            WINDOW_ID,
            flat,
            state.activeTabId,
        )
        if (!success) {
            // Native seed failure erases the partial durable fixture before
            // disabling the coordinator, so Browser can remain usable without
            // claiming process-death recoverability.
            failedAfterParticipation = false
            handle = closeNative(handle)
            return false
        }
        seeded = true
        return true
    }

    fun tabOpened(tab: BrowserLogicalTab, position: Int): Boolean =
        durableMutation {
            BrowserNormalSessionNative.nativeTabOpened(
                handle,
                WINDOW_ID,
                tab.id,
                tab.url,
                tab.title.orEmpty(),
                position,
            )
        }

    fun tabSelected(tabId: String): Boolean =
        durableMutation {
            BrowserNormalSessionNative.nativeTabSelected(handle, WINDOW_ID, tabId)
        }

    fun tabClosed(tabId: String): Boolean =
        durableMutation {
            BrowserNormalSessionNative.nativeTabClosed(handle, WINDOW_ID, tabId)
        }

    fun tabNavigated(tabId: String, url: String): Boolean =
        durableMutation {
            BrowserNormalSessionNative.nativeTabNavigated(handle, WINDOW_ID, tabId, url)
        }

    fun tabTitled(tabId: String, title: String?): Boolean =
        durableMutation {
            BrowserNormalSessionNative.nativeTabTitled(
                handle,
                WINDOW_ID,
                tabId,
                title.orEmpty(),
            )
        }

    fun background(): Boolean =
        durableMutation {
            BrowserNormalSessionNative.nativeBackground(handle)
        }

    fun resume(): Boolean =
        durableMutation {
            BrowserNormalSessionNative.nativeResume(handle)
        }

    fun cleanShutdown(): Boolean {
        if (closed || handle == 0L || !seeded) return true
        val success = BrowserNormalSessionNative.nativeCleanShutdown(handle)
        if (!success) failedAfterParticipation = true
        return success
    }

    fun closeWithoutClean() {
        if (closed) return
        handle = closeNative(handle)
        closed = true
    }

    private inline fun durableMutation(block: () -> Boolean): Boolean {
        if (closed || handle == 0L || !seeded) return true
        if (failedAfterParticipation) return false
        val success = block()
        if (!success) failedAfterParticipation = true
        return success
    }

    private fun closeNative(value: Long): Long {
        if (value != 0L) BrowserNormalSessionNative.nativeClose(value)
        return 0L
    }

    companion object {
        private const val WINDOW_ID = "android-main"

        fun open(context: Context): BrowserNormalSessionRecovery {
            val epoch = UUID.randomUUID().toString()
            val directory = File(context.noBackupFilesDir, "normal-session")
            val handle = runCatching {
                BrowserNormalSessionNative.nativeOpen(directory.absolutePath, epoch)
            }.getOrDefault(0L)
            if (handle == 0L) {
                return BrowserNormalSessionRecovery(
                    handle = 0L,
                    recoveredState = null,
                    seeded = false,
                )
            }

            val nativeRecovered = BrowserNormalSessionNative.nativeWasRecovered(handle)
            if (!nativeRecovered) {
                return BrowserNormalSessionRecovery(
                    handle = handle,
                    recoveredState = null,
                    seeded = false,
                )
            }

            val recovered = BrowserProcessDeathRecoveryPolicy.restore(
                flatTabs = BrowserNormalSessionNative.nativeRecoveredFlatTabs(handle),
                activeTabId = BrowserNormalSessionNative.nativeRecoveredActiveTabId(handle),
            )
            if (recovered != null) {
                return BrowserNormalSessionRecovery(
                    handle = handle,
                    recoveredState = recovered,
                    seeded = true,
                )
            }

            val restarted = BrowserNormalSessionNative.nativeDiscardRecoveredAndRestart(
                handle,
                UUID.randomUUID().toString(),
            )
            if (!restarted) {
                BrowserNormalSessionNative.nativeClose(handle)
                return BrowserNormalSessionRecovery(
                    handle = 0L,
                    recoveredState = null,
                    seeded = false,
                )
            }
            return BrowserNormalSessionRecovery(
                handle = handle,
                recoveredState = null,
                seeded = false,
            )
        }
    }
}

/**
 * Process-local holder. Same-process Activity recreation reuses one live native
 * coordinator; a real process restart necessarily constructs a new holder and
 * consults the Browser-owned durable store.
 */
internal object BrowserNormalSessionProcess {
    @Volatile
    private var current: BrowserNormalSessionRecovery? = null

    @Synchronized
    fun acquire(context: Context): BrowserNormalSessionRecovery {
        current?.let { return it }
        return BrowserNormalSessionRecovery.open(context.applicationContext).also {
            current = it
        }
    }

    @Synchronized
    fun finishCleanly(session: BrowserNormalSessionRecovery) {
        if (current !== session) return
        session.cleanShutdown()
        session.closeWithoutClean()
        current = null
    }

    @Synchronized
    fun simulateProcessDeathForTest(session: BrowserNormalSessionRecovery) {
        if (current !== session) return
        session.closeWithoutClean()
        current = null
    }
}
