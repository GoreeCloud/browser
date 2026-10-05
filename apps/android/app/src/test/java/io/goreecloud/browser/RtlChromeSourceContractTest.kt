package io.goreecloud.browser

import java.io.File
import org.junit.Assert.assertTrue
import org.junit.Test

class RtlChromeSourceContractTest {
    @Test
    fun manifestAndHistoryControlsPreserveRtlDirectionality() {
        val manifest = sourceText("src/main/AndroidManifest.xml")
        val back = sourceText("src/main/res/drawable/ic_back.xml")
        val forward = sourceText("src/main/res/drawable/ic_forward.xml")
        val activity = sourceText("src/main/java/io/goreecloud/browser/BrowserActivityV2.kt")

        assertTrue(manifest.contains("android:supportsRtl=\"true\""))
        assertTrue(back.contains("android:autoMirrored=\"true\""))
        assertTrue(forward.contains("android:autoMirrored=\"true\""))
        assertTrue(activity.contains("R.drawable.ic_back"))
        assertTrue(activity.contains("R.drawable.ic_forward"))
    }

    private fun sourceText(relativePath: String): String {
        val candidates = listOf(
            File(relativePath),
            File("app/$relativePath"),
        )
        val source = candidates.firstOrNull { it.isFile }
            ?: error("Unable to resolve Android source fixture: $relativePath from ${File(".").absolutePath}")
        return source.readText()
    }
}
