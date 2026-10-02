package io.goreecloud.browser

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class BrowsingDataClearScopeTest {
    @Test
    fun browsingDataResetMakesDestructiveAndPreservedBoundariesExplicit() {
        assertTrue(BrowsingDataClearScope.clearedItems.any { it.contains("Cookies") })
        assertTrue(BrowsingDataClearScope.clearedItems.any { it.contains("Website storage") })
        assertTrue(BrowsingDataClearScope.clearedItems.any { it.contains("navigation history") })
        assertTrue(BrowsingDataClearScope.preservedItems.any { it.contains("Browser settings") })
        assertFalse(BrowsingDataClearScope.clearedItems.any { it.contains("Browser settings") })
    }
}
