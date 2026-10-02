package io.goreecloud.browser

/**
 * Browser-owned page text zoom policy for Android.
 *
 * The value is an application-local presentation preference. It does not change
 * system font scale and is not synchronized until an accepted Browser Sync
 * preference contract explicitly includes it.
 */
object PageTextZoom {
    const val DEFAULT_PERCENT = 100
    const val MIN_PERCENT = 75
    const val MAX_PERCENT = 200
    const val STEP_PERCENT = 25

    fun normalize(percent: Int): Int = percent.coerceIn(MIN_PERCENT, MAX_PERCENT)

    fun decrease(percent: Int): Int = normalize(percent - STEP_PERCENT)

    fun increase(percent: Int): Int = normalize(percent + STEP_PERCENT)

    fun label(percent: Int): String = "${normalize(percent)}%"
}
