package io.goreecloud.browser

import android.annotation.TargetApi
import android.app.Activity
import android.app.Dialog
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.net.Uri
import android.net.http.SslError
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.KeyEvent
import android.view.View
import android.view.ViewGroup
import android.view.Window
import android.view.WindowManager
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.webkit.CookieManager
import android.webkit.GeolocationPermissions
import android.webkit.PermissionRequest
import android.webkit.SafeBrowsingResponse
import android.webkit.SslErrorHandler
import android.webkit.WebChromeClient
import android.webkit.WebResourceRequest
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.EditText
import android.widget.FrameLayout
import android.widget.ImageButton
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast

/**
 * Installable Android beta shell for GoreeCloud Browser.
 *
 * Android System WebView/Chromium is used only as the web-engine dependency.
 * GoreeCloud owns the mobile chrome, navigation/search policy, permission
 * defaults, download gate, Glaze UI mapping, and product behavior here.
 */
class BrowserActivity : Activity() {
    private lateinit var webView: WebView
    private lateinit var topChrome: LinearLayout
    private lateinit var addressField: EditText
    private lateinit var schemeBadge: TextView
    private lateinit var backButton: ImageButton
    private lateinit var forwardButton: ImageButton
    private lateinit var reloadButton: ImageButton
    private lateinit var progressBar: ProgressBar
    private lateinit var glaze: GlazeNativeStyle

    private var currentUrl: String = NavigationResolver.SEARCH_HOME
    private var pageLoading = false
    private var topChromeVisible = true

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        glaze = GlazeNativeStyle(this)
        glaze.applyWindow(this)

        buildBrowserSurface()
        configureEngine()

        if (savedInstanceState == null) {
            navigate(initialUrlFromIntent(intent))
        } else if (webView.restoreState(savedInstanceState) == null) {
            navigate(initialUrlFromIntent(intent))
        } else {
            currentUrl = webView.url ?: NavigationResolver.SEARCH_HOME
            updateOmniboxPresentation()
            updateNavigationControls()
        }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        navigate(initialUrlFromIntent(intent))
    }

    override fun onSaveInstanceState(outState: Bundle) {
        webView.saveState(outState)
        super.onSaveInstanceState(outState)
    }

    @Deprecated("Android framework back dispatch remains supported for the API 26 beta floor")
    override fun onBackPressed() {
        if (::addressField.isInitialized && addressField.hasFocus()) {
            addressField.clearFocus()
            hideKeyboard()
            updateOmniboxPresentation()
            return
        }

        if (::webView.isInitialized && webView.canGoBack()) {
            webView.goBack()
        } else {
            super.onBackPressed()
        }
    }

    override fun onDestroy() {
        if (::webView.isInitialized) {
            webView.stopLoading()
            webView.webChromeClient = null
            webView.webViewClient = WebViewClient()
            webView.loadUrl("about:blank")
            webView.removeAllViews()
            webView.destroy()
        }
        super.onDestroy()
    }

    private fun buildBrowserSurface() {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
        }
        glaze.styleCanvas(root)

        topChrome = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
        }
        glaze.styleTopChrome(topChrome)

        val omnibox = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
        }
        glaze.styleOmniboxCapsule(omnibox)

        schemeBadge = TextView(this).apply {
            text = "HTTPS"
            contentDescription = "Address scheme HTTPS"
        }
        glaze.styleSchemeBadge(schemeBadge)
        omnibox.addView(
            schemeBadge,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                dp(GlazeContract.GENERAL_TARGET_DP),
            ),
        )

        addressField = EditText(this).apply {
            hint = "Search GoreeCloud or enter address"
            contentDescription = "Unified search and address bar"
            isSingleLine = true
            imeOptions = EditorInfo.IME_ACTION_GO
            inputType = android.text.InputType.TYPE_CLASS_TEXT or
                android.text.InputType.TYPE_TEXT_VARIATION_URI
            setOnFocusChangeListener { _, hasFocus ->
                if (hasFocus) {
                    setTopChromeVisible(true)
                    setText(currentUrl)
                    selectAll()
                } else {
                    updateOmniboxPresentation()
                }
            }
            setOnEditorActionListener { _, actionId, event ->
                val go = actionId == EditorInfo.IME_ACTION_GO ||
                    (event?.keyCode == KeyEvent.KEYCODE_ENTER &&
                        event.action == KeyEvent.ACTION_DOWN)
                if (go) {
                    navigate(text.toString())
                    true
                } else {
                    false
                }
            }
        }
        glaze.styleAddressField(addressField)
        omnibox.addView(
            addressField,
            LinearLayout.LayoutParams(
                0,
                LinearLayout.LayoutParams.MATCH_PARENT,
                1f,
            ).apply {
                marginStart = dp(2)
            },
        )

        val goButton = ImageButton(this).apply {
            setImageResource(R.drawable.ic_go)
            contentDescription = "Navigate"
            setOnClickListener { navigate(addressField.text.toString()) }
        }
        glaze.styleChromeButton(goButton, GlazeContract.ButtonRole.Emphasis)
        omnibox.addView(
            goButton,
            LinearLayout.LayoutParams(
                dp(GlazeContract.GENERAL_TARGET_DP),
                dp(GlazeContract.GENERAL_TARGET_DP),
            ).apply {
                marginStart = dp(2)
            },
        )

        topChrome.addView(
            omnibox,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.OMNIBOX_HEIGHT_DP),
            ),
        )
        root.addView(
            topChrome,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val contentFrame = FrameLayout(this)
        webView = WebView(this)
        glaze.styleWebContent(webView)
        contentFrame.addView(
            webView,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT,
            ),
        )

        progressBar = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply {
            max = 100
            progress = 0
            visibility = View.GONE
            contentDescription = "Page loading progress"
            importantForAccessibility = View.IMPORTANT_FOR_ACCESSIBILITY_NO
        }
        glaze.styleProgress(progressBar)
        contentFrame.addView(
            progressBar,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.PROGRESS_HEIGHT_DP),
                Gravity.TOP,
            ),
        )

        root.addView(
            contentFrame,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                0,
                1f,
            ),
        )

        val bottomToolbar = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
        }
        glaze.styleBottomToolbar(bottomToolbar)

        backButton = chromeButton(R.drawable.ic_back, "Back") {
            if (webView.canGoBack()) webView.goBack()
        }
        bottomToolbar.addView(backButton, toolbarButtonParams())

        forwardButton = chromeButton(R.drawable.ic_forward, "Forward") {
            if (webView.canGoForward()) webView.goForward()
        }
        bottomToolbar.addView(forwardButton, toolbarButtonParams())

        val homeButton = chromeButton(R.drawable.ic_home, "GoreeCloud Search home") {
            navigate(NavigationResolver.SEARCH_HOME)
        }
        bottomToolbar.addView(homeButton, toolbarButtonParams())

        reloadButton = chromeButton(R.drawable.ic_reload, "Reload") {
            if (pageLoading) {
                webView.stopLoading()
                pageLoading = false
                progressBar.visibility = View.GONE
                updateNavigationControls()
            } else {
                webView.reload()
            }
        }
        bottomToolbar.addView(reloadButton, toolbarButtonParams())

        val menuButton = chromeButton(R.drawable.ic_more, "Browser menu") {
            showBrowserMenu()
        }
        bottomToolbar.addView(menuButton, toolbarButtonParams())

        root.addView(
            bottomToolbar,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.BOTTOM_TOOLBAR_HEIGHT_DP),
            ),
        )

        setContentView(root)
        updateNavigationControls()
    }

    private fun configureEngine() {
        WebView.setWebContentsDebuggingEnabled(BuildConfig.DEBUG)

        webView.settings.apply {
            javaScriptEnabled = true
            domStorageEnabled = true
            loadsImagesAutomatically = true
            mediaPlaybackRequiresUserGesture = true
            allowFileAccess = false
            allowContentAccess = false
            builtInZoomControls = true
            displayZoomControls = false
            setSupportZoom(true)
            mixedContentMode = android.webkit.WebSettings.MIXED_CONTENT_NEVER_ALLOW
            safeBrowsingEnabled = true
            userAgentString = "$userAgentString GoreeCloudBrowser/${BuildConfig.VERSION_NAME} Android"
        }

        CookieManager.getInstance().apply {
            setAcceptCookie(true)
            setAcceptThirdPartyCookies(webView, false)
        }

        webView.setOnScrollChangeListener { _, _, scrollY, _, oldScrollY ->
            if (!addressField.hasFocus()) {
                val delta = scrollY - oldScrollY
                when {
                    scrollY <= dp(GlazeContract.CHROME_GUTTER_DP) -> setTopChromeVisible(true)
                    delta > dp(GlazeContract.SCROLL_DIRECTION_SLOP_DP) &&
                        scrollY > dp(GlazeContract.AUTO_HIDE_SCROLL_THRESHOLD_DP) ->
                        setTopChromeVisible(false)
                    delta < -dp(GlazeContract.SCROLL_DIRECTION_SLOP_DP) ->
                        setTopChromeVisible(true)
                }
            }
        }

        webView.webViewClient = object : WebViewClient() {
            override fun shouldOverrideUrlLoading(view: WebView, request: WebResourceRequest): Boolean {
                val target = request.url.toString()
                if (NavigationResolver.isAllowedWebUrl(target)) return false

                if (request.hasGesture()) openExternalUri(request.url)
                return true
            }

            override fun onPageStarted(view: WebView, url: String, favicon: Bitmap?) {
                super.onPageStarted(view, url, favicon)
                currentUrl = url
                pageLoading = true
                setTopChromeVisible(true)
                progressBar.visibility = View.VISIBLE
                if (!addressField.hasFocus()) updateOmniboxPresentation()
                updateNavigationControls()
            }

            override fun onPageFinished(view: WebView, url: String) {
                super.onPageFinished(view, url)
                currentUrl = url
                pageLoading = false
                progressBar.visibility = View.GONE
                if (!addressField.hasFocus()) updateOmniboxPresentation()
                updateNavigationControls()
            }

            override fun onReceivedSslError(view: WebView, handler: SslErrorHandler, error: SslError) {
                handler.cancel()
                Toast.makeText(
                    this@BrowserActivity,
                    "Blocked: the site certificate could not be verified.",
                    Toast.LENGTH_LONG,
                ).show()
            }

            @TargetApi(Build.VERSION_CODES.O_MR1)
            override fun onSafeBrowsingHit(
                view: WebView,
                request: WebResourceRequest,
                threatType: Int,
                callback: SafeBrowsingResponse,
            ) {
                callback.backToSafety(true)
                Toast.makeText(
                    this@BrowserActivity,
                    "Blocked by Android Safe Browsing.",
                    Toast.LENGTH_LONG,
                ).show()
            }
        }

        webView.webChromeClient = object : WebChromeClient() {
            override fun onProgressChanged(view: WebView, newProgress: Int) {
                progressBar.progress = newProgress
                pageLoading = newProgress < 100
                progressBar.visibility = if (pageLoading) View.VISIBLE else View.GONE
                updateNavigationControls()
            }

            override fun onPermissionRequest(request: PermissionRequest) {
                request.deny()
            }

            override fun onGeolocationPermissionsShowPrompt(
                origin: String,
                callback: GeolocationPermissions.Callback,
            ) {
                callback.invoke(origin, false, false)
            }
        }

        webView.setDownloadListener { _, _, _, _, _ ->
            Toast.makeText(
                this,
                "Downloads are unavailable until the Android Wardveil release gate is integrated.",
                Toast.LENGTH_LONG,
            ).show()
        }
    }

    private fun navigate(input: String) {
        val target = NavigationResolver.resolve(input)
        currentUrl = target
        setTopChromeVisible(true)
        addressField.clearFocus()
        hideKeyboard()
        updateOmniboxPresentation()
        webView.loadUrl(target)
        webView.requestFocus()
    }

    private fun initialUrlFromIntent(intent: Intent?): String {
        val candidate = intent?.data?.toString().orEmpty()
        return if (NavigationResolver.isAllowedWebUrl(candidate)) {
            candidate
        } else {
            NavigationResolver.SEARCH_HOME
        }
    }

    private fun openExternalUri(uri: Uri) {
        val external = Intent(Intent.ACTION_VIEW, uri)
        if (external.resolveActivity(packageManager) != null) {
            startActivity(external)
        } else {
            Toast.makeText(this, "No app can open this link.", Toast.LENGTH_SHORT).show()
        }
    }

    private fun updateOmniboxPresentation() {
        val uri = runCatching { Uri.parse(currentUrl) }.getOrNull()
        val scheme = uri?.scheme?.lowercase()
        schemeBadge.text = when (scheme) {
            "https" -> "HTTPS"
            "http" -> "HTTP"
            else -> "WEB"
        }
        schemeBadge.contentDescription = "Address scheme ${schemeBadge.text}"

        if (!addressField.hasFocus()) {
            addressField.setText(AddressPresentation.condensed(currentUrl))
            addressField.setSelection(0)
            addressField.post {
                if (!addressField.hasFocus()) {
                    addressField.scrollTo(0, 0)
                }
            }
        }
    }

    private fun setTopChromeVisible(visible: Boolean) {
        if (!::topChrome.isInitialized || topChromeVisible == visible) return
        topChromeVisible = visible
        topChrome.clearAnimation()
        topChrome.visibility = if (visible) View.VISIBLE else View.GONE
    }

    private fun updateNavigationControls() {
        setButtonEnabled(backButton, ::webView.isInitialized && webView.canGoBack())
        setButtonEnabled(forwardButton, ::webView.isInitialized && webView.canGoForward())

        if (::reloadButton.isInitialized) {
            reloadButton.setImageResource(if (pageLoading) R.drawable.ic_stop else R.drawable.ic_reload)
            reloadButton.contentDescription = if (pageLoading) "Stop loading" else "Reload"
        }
    }

    private fun setButtonEnabled(button: ImageButton, enabled: Boolean) {
        button.isEnabled = enabled
        button.alpha = if (enabled) 1f else 0.38f
    }

    private fun chromeButton(icon: Int, description: String, action: (View) -> Unit): ImageButton =
        ImageButton(this).apply {
            setImageResource(icon)
            contentDescription = description
            setOnClickListener(action)
            glaze.styleChromeButton(this, GlazeContract.ButtonRole.Quiet)
        }

    private fun toolbarButtonParams() = LinearLayout.LayoutParams(
        0,
        LinearLayout.LayoutParams.MATCH_PARENT,
        1f,
    ).apply {
        marginStart = dp(1)
        marginEnd = dp(1)
    }

    private fun showBrowserMenu() {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)

        val container = FrameLayout(this).apply {
            setPadding(dp(12), 0, dp(12), dp(12))
            contentDescription = "Browser menu"
        }
        val sheet = LinearLayout(this)
        glaze.styleMenuSheet(sheet)

        val title = TextView(this).apply {
            text = "GoreeCloud Browser"
        }
        glaze.styleMenuTitle(title)
        sheet.addView(
            title,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val subtitle = TextView(this).apply {
            text = currentPageHost()
        }
        glaze.styleMenuSubtitle(subtitle)
        sheet.addView(
            subtitle,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        sheet.addView(
            menuAction("Copy page address", dialog) { copyCurrentAddress() },
            menuActionParams(),
        )
        sheet.addView(
            menuAction("Share page", dialog) { shareCurrentAddress() },
            menuActionParams(),
        )
        sheet.addView(
            menuAction("About this beta", dialog) { showBetaInfo() },
            menuActionParams(),
        )

        container.addView(
            sheet,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        dialog.setContentView(container)
        dialog.setCanceledOnTouchOutside(true)
        dialog.show()
        dialog.window?.apply {
            setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            addFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND)
            attributes = attributes.apply {
                gravity = Gravity.BOTTOM
                dimAmount = 0.18f
            }
            decorView.setPadding(0, 0, 0, 0)
            setLayout(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
            )
        }
    }

    private fun menuAction(label: String, dialog: Dialog, action: () -> Unit): TextView =
        TextView(this).apply {
            text = label
            contentDescription = label
            glaze.styleMenuAction(this)
            setOnClickListener {
                dialog.dismiss()
                action()
            }
        }

    private fun menuActionParams() = LinearLayout.LayoutParams(
        LinearLayout.LayoutParams.MATCH_PARENT,
        dp(GlazeContract.MENU_ACTION_HEIGHT_DP),
    ).apply {
        topMargin = dp(4)
    }

    private fun currentPageHost(): String {
        val host = runCatching { Uri.parse(currentUrl).host }.getOrNull()
        return host?.takeIf { it.isNotBlank() } ?: "Current page"
    }

    private fun copyCurrentAddress() {
        val clipboard = getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
        clipboard.setPrimaryClip(ClipData.newPlainText("Page address", currentUrl))
        Toast.makeText(this, "Page address copied.", Toast.LENGTH_SHORT).show()
    }

    private fun shareCurrentAddress() {
        val share = Intent(Intent.ACTION_SEND).apply {
            type = "text/plain"
            putExtra(Intent.EXTRA_TEXT, currentUrl)
        }
        startActivity(Intent.createChooser(share, "Share page"))
    }

    private fun showBetaInfo() {
        Toast.makeText(
            this,
            "GoreeCloud Browser ${BuildConfig.VERSION_NAME}. Installable beta; production signing, downloads, site permissions, and full platform acceptance remain pending.",
            Toast.LENGTH_LONG,
        ).show()
    }

    private fun hideKeyboard() {
        val input = getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
        input.hideSoftInputFromWindow(addressField.windowToken, 0)
    }

    private fun dp(value: Int): Int = glaze.dp(value)
}
