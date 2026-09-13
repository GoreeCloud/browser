package io.goreecloud.browser

import android.app.Activity
import android.content.Context
import android.content.res.ColorStateList
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.graphics.drawable.StateListDrawable
import android.os.Build
import android.view.Gravity
import android.view.View
import android.webkit.WebView
import android.widget.EditText
import android.widget.ImageButton
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView

/**
 * Effects-free Android-native GLAZE UI V1.4 mapping for Browser-owned chrome.
 *
 * Native controls retain Android semantics while Browser maps the current Glaze
 * material hierarchy, target floor, focus/pressed state, spacing and appearance
 * behavior. Blur and transparency are deliberately not required for usability.
 *
 * V1.4 Optical Intelligence is governed separately by [GlazeBrowserOptics].
 * This style implementation does not sample page pixels, origin identity,
 * favicons, CSS/theme colors, browsing/query content, or remote context to style
 * trusted Browser chrome.
 */
class GlazeNativeStyle(private val context: Context) {
    data class Palette(
        val canvas: Int,
        val surface: Int,
        val softGlaze: Int,
        val glaze: Int,
        val deepGlaze: Int,
        val liveGlaze: Int,
        val textPrimary: Int,
        val textSecondary: Int,
        val outline: Int,
        val accent: Int,
    )

    private val night = context.resources.configuration.uiMode and
        Configuration.UI_MODE_NIGHT_MASK == Configuration.UI_MODE_NIGHT_YES

    val palette: Palette = buildPalette()

    fun applyWindow(activity: Activity) {
        activity.window.statusBarColor = palette.canvas
        activity.window.navigationBarColor = if (!night && Build.VERSION.SDK_INT < Build.VERSION_CODES.O_MR1) {
            Color.BLACK
        } else {
            palette.canvas
        }

        var flags = activity.window.decorView.systemUiVisibility
        if (!night) {
            flags = flags or View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O_MR1) {
                flags = flags or View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR
            }
        }
        activity.window.decorView.systemUiVisibility = flags
    }

    fun styleCanvas(view: View) {
        view.setBackgroundColor(palette.canvas)
    }

    fun styleTopChrome(view: LinearLayout) {
        view.gravity = Gravity.CENTER_VERTICAL
        view.setPadding(
            dp(GlazeContract.CHROME_GUTTER_DP),
            dp(GlazeContract.CHROME_GUTTER_DP),
            dp(GlazeContract.CHROME_GUTTER_DP),
            dp(GlazeContract.CHROME_GUTTER_DP),
        )
        view.background = material(
            GlazeContract.MaterialLevel.Canvas,
            0,
            outlined = false,
        )
    }

    fun styleOmniboxCapsule(view: LinearLayout) {
        view.gravity = Gravity.CENTER_VERTICAL
        view.minimumHeight = dp(GlazeContract.OMNIBOX_HEIGHT_DP)
        view.setPadding(dp(4), dp(4), dp(4), dp(4))
        view.background = material(
            GlazeContract.MaterialLevel.SoftGlaze,
            dp(GlazeContract.OMNIBOX_CORNER_DP),
            outlined = true,
        )
    }

    fun styleSchemeBadge(view: TextView) {
        view.gravity = Gravity.CENTER
        view.setTextColor(palette.textSecondary)
        view.textSize = 10f
        view.setAllCaps(true)
        view.minWidth = dp(44)
        view.minimumWidth = dp(44)
        view.setPadding(dp(8), 0, dp(8), 0)
        view.background = material(
            GlazeContract.MaterialLevel.Surface,
            dp(18),
            outlined = false,
        )
    }

    fun styleChromeButton(button: ImageButton, role: GlazeContract.ButtonRole) {
        val levels = when (role) {
            GlazeContract.ButtonRole.Quiet ->
                GlazeContract.MaterialLevel.Canvas to GlazeContract.MaterialLevel.SoftGlaze
            GlazeContract.ButtonRole.Soft ->
                GlazeContract.MaterialLevel.Surface to GlazeContract.MaterialLevel.Glaze
            GlazeContract.ButtonRole.Glaze ->
                GlazeContract.MaterialLevel.Glaze to GlazeContract.MaterialLevel.DeepGlaze
            GlazeContract.ButtonRole.Emphasis ->
                GlazeContract.MaterialLevel.Glaze to GlazeContract.MaterialLevel.DeepGlaze
        }

        button.minimumWidth = dp(GlazeContract.GENERAL_TARGET_DP)
        button.minimumHeight = dp(GlazeContract.GENERAL_TARGET_DP)
        button.setPadding(dp(12), dp(12), dp(12), dp(12))
        button.scaleType = ImageView.ScaleType.CENTER
        button.imageTintList = ColorStateList.valueOf(palette.textPrimary)
        button.background = interactiveBackground(
            levels.first,
            levels.second,
            GlazeContract.CHROME_CONTROL_CORNER_DP,
        )
    }

    fun styleAddressField(field: EditText) {
        field.setTextColor(palette.textPrimary)
        field.setHintTextColor(palette.textSecondary)
        field.textSize = 16f
        field.minHeight = dp(GlazeContract.GENERAL_TARGET_DP)
        field.minimumHeight = dp(GlazeContract.GENERAL_TARGET_DP)
        field.setPadding(dp(10), 0, dp(8), 0)
        field.background = null
        field.setSelectAllOnFocus(false)
    }

    fun styleBottomToolbar(view: LinearLayout) {
        view.gravity = Gravity.CENTER_VERTICAL
        view.setPadding(dp(6), 0, dp(6), 0)
        view.background = material(
            GlazeContract.MaterialLevel.Surface,
            0,
            outlined = false,
        )
    }

    fun styleMenuSheet(view: LinearLayout) {
        view.orientation = LinearLayout.VERTICAL
        view.setPadding(dp(12), dp(12), dp(12), dp(16))
        view.background = material(
            GlazeContract.MaterialLevel.SoftGlaze,
            dp(GlazeContract.MENU_CORNER_DP),
            outlined = true,
        )
    }

    fun styleMenuTitle(view: TextView) {
        view.setTextColor(palette.textPrimary)
        view.textSize = 20f
        view.setPadding(dp(12), dp(8), dp(12), dp(2))
    }

    fun styleMenuSubtitle(view: TextView) {
        view.setTextColor(palette.textSecondary)
        view.textSize = 13f
        view.setPadding(dp(12), 0, dp(12), dp(10))
    }

    fun styleMenuAction(view: TextView) {
        view.gravity = Gravity.CENTER_VERTICAL
        view.setTextColor(palette.textPrimary)
        view.textSize = 17f
        view.minimumHeight = dp(GlazeContract.MENU_ACTION_HEIGHT_DP)
        view.setPadding(dp(18), 0, dp(18), 0)
        view.isClickable = true
        view.isFocusable = true
        view.background = interactiveBackground(
            GlazeContract.MaterialLevel.Surface,
            GlazeContract.MaterialLevel.Glaze,
            GlazeContract.CHROME_CONTROL_CORNER_DP,
        )
    }

    fun styleProgress(progressBar: ProgressBar) {
        progressBar.progressTintList = ColorStateList.valueOf(palette.accent)
        progressBar.progressBackgroundTintList = ColorStateList.valueOf(Color.TRANSPARENT)
    }

    fun styleWebContent(webView: WebView) {
        webView.setBackgroundColor(palette.canvas)
    }

    private fun interactiveBackground(
        resting: GlazeContract.MaterialLevel,
        active: GlazeContract.MaterialLevel,
        cornerDp: Int,
    ): StateListDrawable = StateListDrawable().apply {
        addState(
            intArrayOf(android.R.attr.state_pressed),
            material(active, dp(cornerDp), focused = true, outlined = false),
        )
        addState(
            intArrayOf(android.R.attr.state_focused),
            material(active, dp(cornerDp), focused = true, outlined = true),
        )
        addState(intArrayOf(), material(resting, dp(cornerDp), outlined = false))
    }

    private fun material(
        level: GlazeContract.MaterialLevel,
        cornerRadiusPx: Int,
        focused: Boolean = false,
        outlined: Boolean = false,
    ): GradientDrawable = GradientDrawable().apply {
        shape = GradientDrawable.RECTANGLE
        cornerRadius = cornerRadiusPx.toFloat()
        setColor(colorFor(level))
        if (focused || outlined) {
            setStroke(
                dp(if (focused) 2 else 1),
                if (focused) palette.accent else palette.outline,
            )
        }
    }

    private fun colorFor(level: GlazeContract.MaterialLevel): Int = when (level) {
        GlazeContract.MaterialLevel.Canvas -> palette.canvas
        GlazeContract.MaterialLevel.Surface -> palette.surface
        GlazeContract.MaterialLevel.SoftGlaze -> palette.softGlaze
        GlazeContract.MaterialLevel.Glaze -> palette.glaze
        GlazeContract.MaterialLevel.DeepGlaze -> palette.deepGlaze
        GlazeContract.MaterialLevel.LiveGlaze -> palette.liveGlaze
    }

    private fun buildPalette(): Palette {
        val fallbackCanvas = if (night) Color.rgb(18, 19, 22) else Color.rgb(248, 249, 252)
        val fallbackText = if (night) Color.WHITE else Color.rgb(27, 29, 33)
        val fallbackSecondary = if (night) Color.rgb(194, 198, 207) else Color.rgb(88, 92, 101)
        val fallbackAccent = if (night) Color.rgb(147, 179, 255) else Color.rgb(68, 101, 238)

        val canvas = themedColor(android.R.attr.colorBackground, fallbackCanvas)
        val text = themedColor(android.R.attr.textColorPrimary, fallbackText)
        val secondary = themedColor(android.R.attr.textColorSecondary, fallbackSecondary)
        val accent = themedColor(android.R.attr.colorAccent, fallbackAccent)

        return Palette(
            canvas = canvas,
            surface = blend(canvas, text, if (night) 0.055f else 0.028f),
            softGlaze = blend(canvas, accent, if (night) 0.10f else 0.055f),
            glaze = blend(canvas, accent, if (night) 0.16f else 0.09f),
            deepGlaze = blend(canvas, accent, if (night) 0.24f else 0.15f),
            liveGlaze = blend(canvas, accent, if (night) 0.34f else 0.22f),
            textPrimary = text,
            textSecondary = secondary,
            outline = blend(canvas, text, if (night) 0.20f else 0.13f),
            accent = accent,
        )
    }

    private fun themedColor(attribute: Int, fallback: Int): Int {
        val values = context.obtainStyledAttributes(intArrayOf(attribute))
        return try {
            values.getColor(0, fallback)
        } finally {
            values.recycle()
        }
    }

    private fun blend(base: Int, overlay: Int, amount: Float): Int {
        val inverse = 1f - amount
        return Color.rgb(
            (Color.red(base) * inverse + Color.red(overlay) * amount).toInt(),
            (Color.green(base) * inverse + Color.green(overlay) * amount).toInt(),
            (Color.blue(base) * inverse + Color.blue(overlay) * amount).toInt(),
        )
    }

    fun dp(value: Int): Int = (value * context.resources.displayMetrics.density).toInt()
}
