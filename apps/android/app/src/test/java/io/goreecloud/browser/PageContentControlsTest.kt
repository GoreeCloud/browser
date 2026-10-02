package io.goreecloud.browser

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class PageContentControlsTest {
    @Test
    fun defaultsPreserveWebCompatibility() {
        val state = PageContentControls.State()

        assertTrue(state.javaScriptEnabled)
        assertTrue(state.imagesEnabled)
        assertEquals("JavaScript · On", PageContentControls.javaScriptLabel(true))
        assertEquals("Images · Off", PageContentControls.imagesLabel(false))
    }
}
