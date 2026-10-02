package io.goreecloud.browser

import android.content.Context

data class BrowserFirstUseState(
    val completed: Boolean,
    val step: Int,
    val hintsEnabled: Boolean,
    val chromeHintDismissed: Boolean,
)

object BrowserFirstUsePolicy {
    const val STEP_COUNT = 3

    fun normalizeStep(step: Int): Int = step.coerceIn(0, STEP_COUNT - 1)

    fun nextStep(step: Int): Int = normalizeStep(step + 1)

    fun previousStep(step: Int): Int = normalizeStep(step - 1)
}

/**
 * Device-local first-use and guidance state.
 *
 * Writes use synchronous SharedPreferences commits because setup progression is
 * an interruption/recreation boundary: Browser must not advance visible setup
 * state unless the corresponding local state was durably accepted.
 */
class BrowserFirstUsePreferences(context: Context) {
    private val preferences = context.applicationContext.getSharedPreferences(
        FILE_NAME,
        Context.MODE_PRIVATE,
    )

    fun state(): BrowserFirstUseState = BrowserFirstUseState(
        completed = preferences.getBoolean(KEY_COMPLETED, false),
        step = BrowserFirstUsePolicy.normalizeStep(preferences.getInt(KEY_STEP, 0)),
        hintsEnabled = preferences.getBoolean(KEY_HINTS_ENABLED, true),
        chromeHintDismissed = preferences.getBoolean(KEY_CHROME_HINT_DISMISSED, false),
    )

    fun setStep(step: Int): Boolean =
        preferences.edit()
            .putInt(KEY_STEP, BrowserFirstUsePolicy.normalizeStep(step))
            .commit()

    fun complete(): Boolean =
        preferences.edit()
            .putBoolean(KEY_COMPLETED, true)
            .putInt(KEY_STEP, 0)
            .commit()

    fun replay(): Boolean =
        preferences.edit()
            .putInt(KEY_STEP, 0)
            .commit()

    fun setHintsEnabled(enabled: Boolean): Boolean =
        preferences.edit()
            .putBoolean(KEY_HINTS_ENABLED, enabled)
            .commit()

    fun dismissChromeHint(): Boolean =
        preferences.edit()
            .putBoolean(KEY_CHROME_HINT_DISMISSED, true)
            .commit()

    fun resetDismissedHints(): Boolean =
        preferences.edit()
            .putBoolean(KEY_CHROME_HINT_DISMISSED, false)
            .commit()

    companion object {
        const val FILE_NAME = "goreecloud-browser-first-use"

        private const val KEY_COMPLETED = "completed"
        private const val KEY_STEP = "step"
        private const val KEY_HINTS_ENABLED = "hints_enabled"
        private const val KEY_CHROME_HINT_DISMISSED = "chrome_hint_dismissed"
    }
}
