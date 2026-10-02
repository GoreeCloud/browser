package io.goreecloud.browser

import android.app.Activity
import android.app.Dialog
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.net.http.SslError
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.KeyEvent
import android.view.View
import android.view.ViewGroup
import android.view.Window
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.webkit.CookieManager
import android.webkit.PermissionRequest
import android.webkit.SslErrorHandler
import android.webkit.WebChromeClient
import android.webkit.WebResourceError
import android.webkit.WebResourceRequest
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.EditText
import android.widget.FrameLayout
import android.widget.ImageButton
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast

/**
 * GoreeCloud Browser Android shell.
 *
 * Chromium/WebView remains the engine. GoreeCloud owns the visible browser
 * chrome, local start surface, navigation behavior, privacy defaults, and
 * Glaze UI presentation.
 *
 * Free-text Search is intentionally fail-closed in this Development shell. The
 * omnibox classifies Search before a remote destination is loaded, and Browser
 * does not delegate the query until accepted Privacy Shield authorization and
 * compatible GoreeCloud Search capability evidence are available at runtime.
 */
class BrowserActivityV2 : Activity() {
    private lateinit var glaze: GlazeNativeStyle
    private lateinit var webView: WebView
    private lateinit var topChrome: LinearLayout
    private lateinit var addressField: EditText
    private lateinit var pageTitle: TextView
    private lateinit var backButton: ImageButton
    private lateinit var forwardButton: ImageButton
    private lateinit var reloadButton: ImageButton
    private lateinit var progressBar: ProgressBar
    private lateinit var firstUsePreferences: BrowserFirstUsePreferences
    private var contextualHintRow: LinearLayout? = null

    private var currentUrl: String = INTERNAL_HOME
    private var loading = false
    private var failedMainFrameUrl: String? = null
    private var chromeOverrideTitle: String? = null
    private var pageTextZoomPercent = PageTextZoom.DEFAULT_PERCENT
    private var desktopSiteEnabled = false
    private var pageScriptsEnabled = true
    private var pageImagesEnabled = true
    private var clearHistoryAfterNextPageFinished = false
    private lateinit var mobileUserAgent: String

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        glaze = GlazeNativeStyle(this)
        glaze.applyWindow(this)
        firstUsePreferences = BrowserFirstUsePreferences(this)
        pageTextZoomPercent = PageTextZoom.normalize(
            getSharedPreferences(PREFERENCES_NAME, MODE_PRIVATE)
                .getInt(PREF_PAGE_TEXT_ZOOM, PageTextZoom.DEFAULT_PERCENT),
        )
        desktopSiteEnabled = savedInstanceState?.getBoolean(STATE_DESKTOP_SITE, false) == true
        pageScriptsEnabled = savedInstanceState?.getBoolean(STATE_PAGE_SCRIPTS, true) ?: true
        pageImagesEnabled = savedInstanceState?.getBoolean(STATE_PAGE_IMAGES, true) ?: true
        buildBrowserSurface()
        configureWebView()

        if (savedInstanceState != null && webView.restoreState(savedInstanceState) != null) {
            currentUrl = webView.url ?: INTERNAL_HOME
            refreshChrome()
        } else {
            val external = intent?.data?.toString().orEmpty()
            if (NavigationResolver.isAllowedWebUrl(external)) navigate(external) else showStartPage()
        }

        renderContextualHint()
        if (!firstUsePreferences.state().completed) {
            showFirstUseDialog(replay = false)
        }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        val external = intent.data?.toString().orEmpty()
        if (NavigationResolver.isAllowedWebUrl(external)) navigate(external) else showStartPage()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        webView.saveState(outState)
        outState.putBoolean(STATE_DESKTOP_SITE, desktopSiteEnabled)
        outState.putBoolean(STATE_PAGE_SCRIPTS, pageScriptsEnabled)
        outState.putBoolean(STATE_PAGE_IMAGES, pageImagesEnabled)
        super.onSaveInstanceState(outState)
    }

    @Deprecated("Android framework back dispatch is retained for the current API floor")
    override fun onBackPressed() {
        when {
            addressField.hasFocus() -> {
                addressField.clearFocus()
                hideKeyboard()
                refreshChrome()
            }
            webView.canGoBack() -> webView.goBack()
            else -> super.onBackPressed()
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
        val root = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        glaze.styleCanvas(root)

        topChrome = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(10), dp(8), dp(10), dp(8))
        }
        glaze.styleTopChrome(topChrome)

        val identityRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }

        pageTitle = TextView(this).apply {
            text = "GoreeCloud Browser"
            textSize = 13f
            setTextColor(glaze.palette.textSecondary)
            maxLines = 1
        }
        identityRow.addView(
            pageTitle,
            LinearLayout.LayoutParams(0, dp(GlazeContract.GENERAL_TARGET_DP), 1f).apply {
                gravity = Gravity.CENTER_VERTICAL
            },
        )

        val menuButton = chromeButton(R.drawable.ic_more, "Browser menu") {
            showBrowserMenu()
        }
        identityRow.addView(
            menuButton,
            LinearLayout.LayoutParams(
                dp(GlazeContract.GENERAL_TARGET_DP),
                dp(GlazeContract.GENERAL_TARGET_DP),
            ),
        )
        topChrome.addView(identityRow)

        val omnibox = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        glaze.styleOmniboxCapsule(omnibox)

        addressField = EditText(this).apply {
            hint = "Search or enter address"
            contentDescription = "Search or address bar"
            isSingleLine = true
            imeOptions = EditorInfo.IME_ACTION_GO
            inputType = android.text.InputType.TYPE_CLASS_TEXT or android.text.InputType.TYPE_TEXT_VARIATION_URI
            setPadding(dp(16), 0, dp(8), 0)
            setOnFocusChangeListener { _, focused ->
                if (focused) {
                    setText(if (currentUrl == INTERNAL_HOME) "" else currentUrl)
                    selectAll()
                } else {
                    refreshChrome()
                }
            }
            setOnEditorActionListener { _, actionId, event ->
                val go = actionId == EditorInfo.IME_ACTION_GO ||
                    (event?.keyCode == KeyEvent.KEYCODE_ENTER && event.action == KeyEvent.ACTION_DOWN)
                if (go) {
                    navigate(text.toString())
                    true
                } else false
            }
        }
        glaze.styleAddressField(addressField)
        omnibox.addView(
            addressField,
            LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1f),
        )

        val goButton = chromeButton(R.drawable.ic_go, "Go") { navigate(addressField.text.toString()) }
        omnibox.addView(
            goButton,
            LinearLayout.LayoutParams(
                dp(GlazeContract.GENERAL_TARGET_DP),
                dp(GlazeContract.GENERAL_TARGET_DP),
            ),
        )
        topChrome.addView(
            omnibox,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.OMNIBOX_HEIGHT_DP),
            ),
        )
        root.addView(topChrome)

        val content = FrameLayout(this)
        webView = WebView(this)
        glaze.styleWebContent(webView)
        content.addView(
            webView,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT,
            ),
        )

        progressBar = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply {
            max = 100
            visibility = View.GONE
            importantForAccessibility = View.IMPORTANT_FOR_ACCESSIBILITY_NO
        }
        glaze.styleProgress(progressBar)
        content.addView(
            progressBar,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.PROGRESS_HEIGHT_DP),
                Gravity.TOP,
            ),
        )
        root.addView(
            content,
            LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f),
        )

        val bottom = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        glaze.styleBottomToolbar(bottom)

        backButton = chromeButton(R.drawable.ic_back, "Back") {
            if (webView.canGoBack()) webView.goBack()
        }
        forwardButton = chromeButton(R.drawable.ic_forward, "Forward") {
            if (webView.canGoForward()) webView.goForward()
        }
        val homeButton = chromeButton(R.drawable.ic_home, "Start page") { showStartPage() }
        reloadButton = chromeButton(R.drawable.ic_reload, "Reload") {
            when {
                loading -> webView.stopLoading()
                failedMainFrameUrl != null -> navigateToUrl(failedMainFrameUrl.orEmpty())
                currentUrl == INTERNAL_HOME -> showStartPage()
                else -> webView.reload()
            }
        }
        val bottomMenu = chromeButton(R.drawable.ic_more, "Browser menu") { showBrowserMenu() }

        listOf(backButton, forwardButton, homeButton, reloadButton, bottomMenu).forEach {
            bottom.addView(it, LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT, 1f))
        }
        root.addView(
            bottom,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.BOTTOM_TOOLBAR_HEIGHT_DP),
            ),
        )

        setContentView(root)
        updateNavigationButtons()
    }

    private fun configureWebView() {
        WebView.setWebContentsDebuggingEnabled(BuildConfig.DEBUG)
        webView.settings.apply {
            javaScriptEnabled = pageScriptsEnabled
            domStorageEnabled = true
            loadsImagesAutomatically = pageImagesEnabled
            blockNetworkImage = !pageImagesEnabled
            mediaPlaybackRequiresUserGesture = true
            allowFileAccess = false
            allowContentAccess = false
            builtInZoomControls = true
            displayZoomControls = false
            setSupportZoom(true)
            textZoom = pageTextZoomPercent
            mixedContentMode = android.webkit.WebSettings.MIXED_CONTENT_NEVER_ALLOW
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) safeBrowsingEnabled = true
            mobileUserAgent =
                "$userAgentString GoreeCloudBrowser/${BuildConfig.VERSION_NAME} Android"
            userAgentString = if (desktopSiteEnabled) {
                DesktopUserAgent.fromMobile(mobileUserAgent)
            } else {
                mobileUserAgent
            }
            useWideViewPort = desktopSiteEnabled
            loadWithOverviewMode = desktopSiteEnabled
        }

        CookieManager.getInstance().apply {
            setAcceptCookie(true)
            setAcceptThirdPartyCookies(webView, false)
        }

        webView.webViewClient = object : WebViewClient() {
            override fun shouldOverrideUrlLoading(view: WebView, request: WebResourceRequest): Boolean {
                return !NavigationResolver.isAllowedWebUrl(request.url.toString())
            }

            override fun onPageStarted(view: WebView, url: String, favicon: Bitmap?) {
                if (!isInternalStartUrl(url)) {
                    currentUrl = url
                    failedMainFrameUrl = null
                    chromeOverrideTitle = null
                }
                loading = true
                progressBar.visibility = View.VISIBLE
                refreshChrome()
            }

            override fun onPageFinished(view: WebView, url: String) {
                if (isInternalStartUrl(url)) {
                    if (currentUrl != INTERNAL_HOME && chromeOverrideTitle == null) return
                } else {
                    if (!MainFrameFailureGuard.shouldAccept(currentUrl, url)) return
                    currentUrl = url
                }
                loading = false
                progressBar.visibility = View.GONE
                if (clearHistoryAfterNextPageFinished) {
                    view.clearHistory()
                    clearHistoryAfterNextPageFinished = false
                }
                refreshChrome()
            }

            override fun onReceivedError(
                view: WebView,
                request: WebResourceRequest,
                error: WebResourceError,
            ) {
                val failedUrl = request.url.toString()
                if (
                    !request.isForMainFrame ||
                    isInternalStartUrl(failedUrl) ||
                    !MainFrameFailureGuard.shouldAccept(currentUrl, failedUrl)
                ) {
                    return
                }
                showPageUnavailable(failedUrl)
            }

            override fun onReceivedSslError(view: WebView, handler: SslErrorHandler, error: SslError) {
                handler.cancel()
                if (MainFrameFailureGuard.shouldAccept(currentUrl, error.url)) {
                    showPageUnavailable(error.url)
                }
            }
        }

        webView.webChromeClient = object : WebChromeClient() {
            override fun onProgressChanged(view: WebView, newProgress: Int) {
                progressBar.progress = newProgress
                loading = newProgress < 100
                progressBar.visibility = if (loading) View.VISIBLE else View.GONE
                updateNavigationButtons()
            }

            override fun onPermissionRequest(request: PermissionRequest) {
                request.deny()
            }
        }

        webView.setDownloadListener { _, _, _, _, _ ->
            Toast.makeText(
                this,
                "Downloads remain gated until Wardveil download integration is ready.",
                Toast.LENGTH_LONG,
            ).show()
        }
    }

    private fun navigate(raw: String) {
        when (val intent = NavigationResolver.classify(raw)) {
            NavigationResolver.Intent.Home -> showStartPage()
            is NavigationResolver.Intent.Navigate -> navigateToUrl(intent.url)
            is NavigationResolver.Intent.Search -> showSearchAuthorizationRequired(intent.query)
            is NavigationResolver.Intent.Blocked -> showBlockedNavigation(intent.input)
        }
    }

    private fun navigateToUrl(target: String) {
        failedMainFrameUrl = null
        chromeOverrideTitle = null
        currentUrl = target
        addressField.clearFocus()
        hideKeyboard()
        webView.loadUrl(target)
        webView.requestFocus()
        refreshChrome()
    }

    private fun showStartPage() {
        failedMainFrameUrl = null
        chromeOverrideTitle = null
        currentUrl = INTERNAL_HOME
        addressField.clearFocus()
        hideKeyboard()
        webView.loadDataWithBaseURL(START_BASE_URL, startHtml(), "text/html", "UTF-8", null)
        refreshChrome()
    }

    private fun showSearchAuthorizationRequired(query: String) {
        failedMainFrameUrl = null
        chromeOverrideTitle = null
        currentUrl = INTERNAL_HOME
        addressField.clearFocus()
        hideKeyboard()
        val escaped = android.text.TextUtils.htmlEncode(query)
        val html = """
            <!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
            <style>${baseCss()}</style></head><body><main>
            <div class="mark">G</div><h1>Search authorization required</h1>
            <p>This Development build has not accepted the runtime Privacy Shield authorization and compatible GoreeCloud Search capability evidence required for remote Search delegation.</p>
            <p>Your query was not sent to GoreeCloud Search.</p>
            <p class="query">$escaped</p><p>You can still enter a complete website address in the address bar.</p>
            </main></body></html>
        """.trimIndent()
        webView.loadDataWithBaseURL(START_BASE_URL, html, "text/html", "UTF-8", null)
        refreshChrome()
    }

    private fun showBlockedNavigation(input: String) {
        failedMainFrameUrl = null
        chromeOverrideTitle = null
        currentUrl = INTERNAL_HOME
        addressField.clearFocus()
        hideKeyboard()
        val escaped = android.text.TextUtils.htmlEncode(input)
        val html = """
            <!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
            <style>${baseCss()}</style></head><body><main>
            <div class="mark">G</div><h1>Navigation blocked</h1>
            <p>Browser rejected this input because it is not a valid safe HTTP(S) destination and must not be silently reinterpreted as a Search query.</p>
            <p>The input was not opened and was not sent to GoreeCloud Search.</p>
            <p class="query">$escaped</p>
            </main></body></html>
        """.trimIndent()
        webView.loadDataWithBaseURL(START_BASE_URL, html, "text/html", "UTF-8", null)
        refreshChrome()
    }

    private fun showPageUnavailable(failedUrl: String?) {
        val retryUrl = failedUrl
            ?.takeIf(NavigationResolver::isAllowedWebUrl)
            ?: currentUrl.takeIf(NavigationResolver::isAllowedWebUrl)

        failedMainFrameUrl = retryUrl
        chromeOverrideTitle = "Page unavailable"
        if (retryUrl != null) currentUrl = retryUrl
        loading = false
        progressBar.visibility = View.GONE
        addressField.clearFocus()
        hideKeyboard()
        webView.loadDataWithBaseURL(
            START_BASE_URL,
            BrowserLocalPages.pageUnavailableHtml(baseCss()),
            "text/html",
            "UTF-8",
            null,
        )
        refreshChrome()
    }

    private fun startHtml(): String = """
        <!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
        <style>${baseCss()}</style></head><body><main>
        <div class="mark">G</div>
        <h1>Browse the web</h1>
        <p>Enter a website address above. This Development build blocks third-party cookies and denies site permissions by default.</p>
        <section><strong>GoreeCloud Search</strong><span>Free-text Search remains local and fail-closed until Browser has accepted Privacy Shield authorization and compatible Search capability evidence for remote delegation.</span></section>
        </main></body></html>
    """.trimIndent()

    private fun baseCss(): String = """
        :root{color-scheme:light dark}*{box-sizing:border-box}body{margin:0;font-family:system-ui,-apple-system,sans-serif;background:#f5f7fb;color:#172033}
        main{min-height:100vh;padding:54px 24px 40px;max-width:720px;margin:auto}.mark{width:58px;height:58px;border-radius:18px;display:grid;place-items:center;background:linear-gradient(145deg,#3b82f6,#174ea6);color:white;font-weight:800;font-size:25px;box-shadow:0 12px 34px #174ea633}
        h1{font-size:30px;margin:22px 0 10px}p{font-size:16px;line-height:1.55;color:#5b6577}section{margin-top:18px;padding:18px;border:1px solid #dce3ef;border-radius:20px;background:#ffffffcc}section strong,section span{display:block}section span{margin-top:6px;color:#687386;line-height:1.45}.query{font-weight:700;color:#174ea6;overflow-wrap:anywhere}
        @media(prefers-color-scheme:dark){body{background:#0b0f16;color:#f4f7fb}p,section span{color:#aeb8c7}section{background:#111821;border-color:#253043}}
    """.trimIndent()

    private fun refreshChrome() {
        if (!::addressField.isInitialized) return
        pageTitle.text = chromeOverrideTitle ?: if (currentUrl == INTERNAL_HOME) {
            PageTitlePresentation.PRODUCT_TITLE
        } else {
            PageTitlePresentation.safe(webView.title, currentUrl)
        }
        if (!addressField.hasFocus()) {
            addressField.setText(
                if (currentUrl == INTERNAL_HOME) "" else AddressPresentation.condensed(currentUrl),
            )
            addressField.setSelection(0)
        }
        updateNavigationButtons()
    }

    private fun updateNavigationButtons() {
        if (::backButton.isInitialized) setEnabled(backButton, webView.canGoBack())
        if (::forwardButton.isInitialized) setEnabled(forwardButton, webView.canGoForward())
        if (::reloadButton.isInitialized) {
            reloadButton.setImageResource(if (loading) R.drawable.ic_stop else R.drawable.ic_reload)
            reloadButton.contentDescription = if (loading) "Stop loading" else "Reload"
        }
    }

    private fun setEnabled(button: ImageButton, enabled: Boolean) {
        button.isEnabled = enabled
        button.alpha = if (enabled) 1f else 0.34f
    }

    private fun showBrowserMenu() {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)

        val sheet = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            contentDescription = "Browser menu"
        }
        glaze.styleMenuSheet(sheet)

        val title = TextView(this).apply { text = "GoreeCloud Browser" }
        glaze.styleMenuTitle(title)
        sheet.addView(
            title,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val subtitle = TextView(this).apply {
            text = SiteInformation.forUrl(currentUrl).menuSubtitle
        }
        glaze.styleMenuSubtitle(subtitle)
        sheet.addView(
            subtitle,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        fun addAction(label: String, action: () -> Unit) {
            val item = TextView(this).apply {
                text = label
                setOnClickListener {
                    action()
                    dialog.dismiss()
                }
            }
            glaze.styleMenuAction(item)
            sheet.addView(
                item,
                LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT,
                ),
            )
        }

        addAction("Site information") { showSiteInformation() }
        addAction("Privacy & security") { showPrivacyAndSecurityStatus() }
        addAction("Find in page") { showFindInPage() }
        addAction("Page controls") { showPageControls() }
        addAction("Guidance & tips") { showGuidanceSettings() }
        addAction("Clear browsing data") { showClearBrowsingDataConfirmation() }

        BrowserDisclosurePolicy.shareablePageUrl(currentUrl)?.let { shareablePageUrl ->
            addAction("Copy page address") {
                val clipboard =
                    getSystemService(CLIPBOARD_SERVICE) as android.content.ClipboardManager
                clipboard.setPrimaryClip(
                    android.content.ClipData.newPlainText("Page address", shareablePageUrl),
                )
            }
            addAction("Share page") {
                startActivity(Intent.createChooser(Intent(Intent.ACTION_SEND).apply {
                    type = "text/plain"
                    putExtra(Intent.EXTRA_TEXT, shareablePageUrl)
                }, "Share page"))
            }
        }

        addAction("About this development build") {
            Toast.makeText(
                this,
                "GoreeCloud Browser ${BuildConfig.VERSION_NAME}",
                Toast.LENGTH_SHORT,
            ).show()
        }

        dialog.setContentView(sheet)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.show()
        dialog.window?.setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
        dialog.window?.setGravity(Gravity.BOTTOM)
    }

    private fun showGuidanceSettings() {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)

        val sheet = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            contentDescription = "Guidance and tips"
        }
        glaze.styleMenuSheet(sheet)

        val title = TextView(this).apply { text = "Guidance & tips" }
        glaze.styleMenuTitle(title)
        sheet.addView(
            title,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val subtitle = TextView(this).apply {
            text = "Review Browser setup or control optional local tips."
        }
        glaze.styleMenuSubtitle(subtitle)
        sheet.addView(
            subtitle,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        fun action(label: String): TextView = TextView(this).apply {
            text = label
            glaze.styleMenuAction(this)
        }

        val replay = action("Replay setup")
        val hints = action("")
        val resetHint = action("Show dismissed Browser tip again")
        val close = action("Close")

        fun refreshGuidanceActions() {
            val state = firstUsePreferences.state()
            hints.text = "Contextual tips · " + if (state.hintsEnabled) "On" else "Off"
            resetHint.visibility =
                if (state.chromeHintDismissed) View.VISIBLE else View.GONE
        }

        replay.setOnClickListener {
            if (firstUsePreferences.replay()) {
                dialog.dismiss()
                showFirstUseDialog(replay = true)
            } else {
                showGuidanceWriteFailure()
            }
        }
        hints.setOnClickListener {
            val state = firstUsePreferences.state()
            if (firstUsePreferences.setHintsEnabled(!state.hintsEnabled)) {
                renderContextualHint()
                refreshGuidanceActions()
            } else {
                showGuidanceWriteFailure()
            }
        }
        resetHint.setOnClickListener {
            if (firstUsePreferences.resetDismissedHints()) {
                renderContextualHint()
                refreshGuidanceActions()
            } else {
                showGuidanceWriteFailure()
            }
        }
        close.setOnClickListener { dialog.dismiss() }

        listOf(replay, hints, resetHint, close).forEach { item ->
            sheet.addView(
                item,
                LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT,
                ),
            )
        }

        refreshGuidanceActions()

        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(
                sheet,
                ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
        }
        dialog.setContentView(scroll)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.show()
        dialog.window?.setLayout(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
        )
        dialog.window?.setGravity(Gravity.BOTTOM)
    }

    private fun renderContextualHint() {
        contextualHintRow?.let { topChrome.removeView(it) }
        contextualHintRow = null

        val state = firstUsePreferences.state()
        if (!state.completed || !state.hintsEnabled || state.chromeHintDismissed) return

        val row = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            contentDescription = "Browser tip"
            setPadding(dp(8), dp(2), dp(2), dp(2))
        }

        val message = TextView(this).apply {
            text = "Tip: enter a full website address to navigate directly. Free-text Search stays local until governed Search authorization is available."
        }
        glaze.styleMenuSubtitle(message)
        row.addView(
            message,
            LinearLayout.LayoutParams(
                0,
                LinearLayout.LayoutParams.WRAP_CONTENT,
                1f,
            ),
        )

        val dismiss = TextView(this).apply {
            text = "Got it"
            contentDescription = "Dismiss Browser tip"
            gravity = Gravity.CENTER
            glaze.styleMenuAction(this)
            setOnClickListener {
                if (firstUsePreferences.dismissChromeHint()) {
                    renderContextualHint()
                } else {
                    showGuidanceWriteFailure()
                }
            }
        }
        row.addView(
            dismiss,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                dp(GlazeContract.GENERAL_TARGET_DP),
            ),
        )

        contextualHintRow = row
        topChrome.addView(
            row,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )
    }

    private fun showFirstUseDialog(replay: Boolean) {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)
        dialog.setCancelable(replay)
        dialog.setCanceledOnTouchOutside(replay)

        val sheet = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            contentDescription =
                if (replay) "Browser setup review" else "Browser first-use setup"
        }
        glaze.styleMenuSheet(sheet)

        val progress = TextView(this)
        glaze.styleMenuSubtitle(progress)
        sheet.addView(progress)

        val title = TextView(this)
        glaze.styleMenuTitle(title)
        sheet.addView(title)

        val body = TextView(this)
        glaze.styleMenuSubtitle(body)
        sheet.addView(body)

        val hintToggle = TextView(this).apply {
            gravity = Gravity.CENTER_VERTICAL
            glaze.styleMenuAction(this)
        }
        sheet.addView(
            hintToggle,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val actions = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }

        fun wizardAction(label: String): TextView = TextView(this).apply {
            text = label
            gravity = Gravity.CENTER
            glaze.styleMenuAction(this)
        }

        val returnButton = wizardAction("Return to Browser").apply {
            visibility = if (replay) View.VISIBLE else View.GONE
        }
        val back = wizardAction("Back")
        val next = wizardAction("Continue")

        actions.addView(
            returnButton,
            LinearLayout.LayoutParams(0, dp(56), 1f),
        )
        actions.addView(back, LinearLayout.LayoutParams(0, dp(56), 1f))
        actions.addView(next, LinearLayout.LayoutParams(0, dp(56), 1f))
        sheet.addView(
            actions,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val steps = listOf(
            "Welcome to GoreeCloud Browser" to
                "Browser chrome, navigation policy, privacy controls, and local state are GoreeCloud-owned. Android System WebView supplies the web engine behind those Browser-owned boundaries.",
            "Privacy & security by default" to
                "Mixed content is blocked, third-party cookies are off by default, file/content access is disabled, certificate errors fail closed, and website permission requests stay denied until governed authority is available. Clear browsing data is always available from the Browser menu.",
            "Browse with clear boundaries" to
                "Enter complete website addresses directly. Free-text Search remains local until accepted GoreeCloud Search authorization is available. Find in page stays inside the current WebView, and Page controls groups text size, Desktop site, JavaScript, and automatic image loading. Optional tips never hide security or failure messages.",
        )

        var step = BrowserFirstUsePolicy.normalizeStep(firstUsePreferences.state().step)

        fun renderStep() {
            val state = firstUsePreferences.state()
            progress.text = "Step " + (step + 1) + " of " + BrowserFirstUsePolicy.STEP_COUNT
            title.text = steps[step].first
            body.text = steps[step].second
            back.visibility = if (step == 0) View.INVISIBLE else View.VISIBLE
            next.text =
                if (step == BrowserFirstUsePolicy.STEP_COUNT - 1) "Start browsing" else "Continue"
            next.contentDescription = next.text
            hintToggle.visibility =
                if (step == BrowserFirstUsePolicy.STEP_COUNT - 1) View.VISIBLE else View.GONE
            hintToggle.text =
                "Contextual tips · " + if (state.hintsEnabled) "On" else "Off"
            hintToggle.contentDescription = hintToggle.text
        }

        returnButton.setOnClickListener { dialog.dismiss() }

        back.setOnClickListener {
            val candidate = BrowserFirstUsePolicy.previousStep(step)
            if (firstUsePreferences.setStep(candidate)) {
                step = candidate
                renderStep()
            } else {
                showGuidanceWriteFailure()
            }
        }

        next.setOnClickListener {
            if (step == BrowserFirstUsePolicy.STEP_COUNT - 1) {
                if (firstUsePreferences.complete()) {
                    dialog.dismiss()
                    renderContextualHint()
                } else {
                    showGuidanceWriteFailure()
                }
            } else {
                val candidate = BrowserFirstUsePolicy.nextStep(step)
                if (firstUsePreferences.setStep(candidate)) {
                    step = candidate
                    renderStep()
                } else {
                    showGuidanceWriteFailure()
                }
            }
        }

        hintToggle.setOnClickListener {
            val state = firstUsePreferences.state()
            if (firstUsePreferences.setHintsEnabled(!state.hintsEnabled)) {
                renderStep()
            } else {
                showGuidanceWriteFailure()
            }
        }

        renderStep()

        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(
                sheet,
                ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
        }
        dialog.setContentView(scroll)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.show()
        dialog.window?.setLayout(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
        )
        dialog.window?.setGravity(Gravity.BOTTOM)
    }

    private fun showGuidanceWriteFailure() {
        Toast.makeText(
            this,
            "Browser could not save this guidance setting. Try again.",
            Toast.LENGTH_LONG,
        ).show()
    }

    private fun showSiteInformation() {
        val info = SiteInformation.forUrl(currentUrl)
        showInformationSheet(
            titleText = "Site information",
            subtitleText = info.displayOrigin,
            rows = listOf(
                "Connection" to info.transportLabel,
                "Details" to info.details,
            ),
        )
    }

    private fun showPrivacyAndSecurityStatus() {
        val entries = BrowserProtectionStatus.current(
            safeBrowsingEnabled = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O,
        )
        showInformationSheet(
            titleText = "Privacy & security",
            subtitleText = "Current Development defaults",
            rows = entries.map { entry ->
                entry.label to "${entry.state}\n${entry.details}"
            },
        )
    }

    private fun showInformationSheet(
        titleText: String,
        subtitleText: String,
        rows: List<Pair<String, String>>,
    ) {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)

        val sheet = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            contentDescription = "$titleText sheet"
        }
        glaze.styleMenuSheet(sheet)

        val title = TextView(this).apply { text = titleText }
        glaze.styleMenuTitle(title)
        sheet.addView(
            title,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val subtitle = TextView(this).apply { text = subtitleText }
        glaze.styleMenuSubtitle(subtitle)
        sheet.addView(
            subtitle,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        rows.forEach { (label, value) ->
            val heading = TextView(this).apply {
                text = label
                textSize = 14f
                setTextColor(glaze.palette.textPrimary)
                setPadding(dp(12), dp(10), dp(12), 0)
            }
            sheet.addView(
                heading,
                LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT,
                ),
            )

            val body = TextView(this).apply { text = value }
            glaze.styleMenuSubtitle(body)
            sheet.addView(
                body,
                LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT,
                ),
            )
        }

        val close = TextView(this).apply {
            text = "Close"
            setOnClickListener { dialog.dismiss() }
        }
        glaze.styleMenuAction(close)
        sheet.addView(
            close,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(
                sheet,
                ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
        }
        dialog.setContentView(scroll)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.show()
        dialog.window?.setLayout(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
        )
        dialog.window?.setGravity(Gravity.BOTTOM)
    }

    private fun showFindInPage() {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)

        val sheet = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            contentDescription = "Find in page"
        }
        glaze.styleMenuSheet(sheet)

        val title = TextView(this).apply { text = "Find in page" }
        glaze.styleMenuTitle(title)
        sheet.addView(
            title,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val privacyNote = TextView(this).apply {
            text = "Searches only this page. Find text stays inside the active WebView."
        }
        glaze.styleMenuSubtitle(privacyNote)
        sheet.addView(
            privacyNote,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val queryShell = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        glaze.styleOmniboxCapsule(queryShell)

        val query = EditText(this).apply {
            hint = "Find text"
            contentDescription = "Find in page query"
            isSingleLine = true
            imeOptions = EditorInfo.IME_ACTION_SEARCH
            filters = arrayOf(
                android.text.InputFilter.LengthFilter(FindInPageStatus.MAX_QUERY_LENGTH),
            )
        }
        glaze.styleAddressField(query)
        queryShell.addView(
            query,
            LinearLayout.LayoutParams(
                0,
                dp(GlazeContract.OMNIBOX_HEIGHT_DP),
                1f,
            ),
        )
        sheet.addView(
            queryShell,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.OMNIBOX_HEIGHT_DP),
            ),
        )

        val status = TextView(this).apply {
            text = FindInPageStatus.label("", 0, 0, true)
        }
        glaze.styleMenuSubtitle(status)
        sheet.addView(
            status,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val controls = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }

        fun findAction(label: String): TextView = TextView(this).apply {
            text = label
            gravity = Gravity.CENTER
            glaze.styleMenuAction(this)
        }

        val previous = findAction("Previous")
        val next = findAction("Next")
        val close = findAction("Close")
        controls.addView(previous, LinearLayout.LayoutParams(0, dp(56), 1f))
        controls.addView(next, LinearLayout.LayoutParams(0, dp(56), 1f))
        controls.addView(close, LinearLayout.LayoutParams(0, dp(56), 1f))
        sheet.addView(
            controls,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        var activeQuery = ""
        setTextActionEnabled(previous, false)
        setTextActionEnabled(next, false)

        webView.setFindListener { activeMatchOrdinal, numberOfMatches, isDoneCounting ->
            status.text = FindInPageStatus.label(
                activeQuery,
                activeMatchOrdinal,
                numberOfMatches,
                isDoneCounting,
            )
            val hasMatches = activeQuery.isNotEmpty() && isDoneCounting && numberOfMatches > 0
            setTextActionEnabled(previous, hasMatches)
            setTextActionEnabled(next, hasMatches)
        }

        query.addTextChangedListener(object : android.text.TextWatcher {
            override fun beforeTextChanged(
                value: CharSequence?,
                start: Int,
                count: Int,
                after: Int,
            ) = Unit

            override fun onTextChanged(
                value: CharSequence?,
                start: Int,
                before: Int,
                count: Int,
            ) = Unit

            override fun afterTextChanged(value: android.text.Editable?) {
                activeQuery = value?.toString().orEmpty()
                if (activeQuery.isEmpty()) {
                    webView.clearMatches()
                    status.text = FindInPageStatus.label("", 0, 0, true)
                    setTextActionEnabled(previous, false)
                    setTextActionEnabled(next, false)
                } else {
                    status.text = FindInPageStatus.label(activeQuery, 0, 0, false)
                    setTextActionEnabled(previous, false)
                    setTextActionEnabled(next, false)
                    webView.findAllAsync(activeQuery)
                }
            }
        })

        previous.setOnClickListener { webView.findNext(false) }
        next.setOnClickListener { webView.findNext(true) }
        close.setOnClickListener { dialog.dismiss() }

        dialog.setOnDismissListener {
            webView.clearMatches()
            webView.setFindListener(null)
            hideKeyboard(query)
        }
        dialog.setContentView(sheet)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.show()
        dialog.window?.setLayout(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
        )
        dialog.window?.setGravity(Gravity.BOTTOM)

        query.requestFocus()
        query.post {
            (getSystemService(INPUT_METHOD_SERVICE) as? InputMethodManager)
                ?.showSoftInput(query, InputMethodManager.SHOW_IMPLICIT)
        }
    }

    private fun showPageControls() {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)

        val sheet = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            contentDescription = "Page controls"
        }
        glaze.styleMenuSheet(sheet)

        val title = TextView(this).apply { text = "Page controls" }
        glaze.styleMenuTitle(title)
        sheet.addView(
            title,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val note = TextView(this).apply {
            text = "Session-local content and presentation controls for website pages."
        }
        glaze.styleMenuSubtitle(note)
        sheet.addView(
            note,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        fun action(label: String): TextView = TextView(this).apply {
            text = label
            glaze.styleMenuAction(this)
        }

        val textSize = action("Text size · ${PageTextZoom.label(pageTextZoomPercent)}")
        val desktop = action("Desktop site · ${if (desktopSiteEnabled) "On" else "Off"}")
        val scripts = action(PageContentControls.javaScriptLabel(pageScriptsEnabled))
        val images = action(PageContentControls.imagesLabel(pageImagesEnabled))
        val close = action("Close")

        listOf(textSize, desktop, scripts, images, close).forEach { item ->
            sheet.addView(
                item,
                LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT,
                ),
            )
        }

        textSize.setOnClickListener {
            dialog.dismiss()
            showTextZoomControls()
        }
        desktop.setOnClickListener {
            setDesktopSiteEnabled(!desktopSiteEnabled)
            desktop.text = "Desktop site · ${if (desktopSiteEnabled) "On" else "Off"}"
        }
        scripts.setOnClickListener {
            setPageScriptsEnabled(!pageScriptsEnabled)
            scripts.text = PageContentControls.javaScriptLabel(pageScriptsEnabled)
        }
        images.setOnClickListener {
            setPageImagesEnabled(!pageImagesEnabled)
            images.text = PageContentControls.imagesLabel(pageImagesEnabled)
        }
        close.setOnClickListener { dialog.dismiss() }

        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(
                sheet,
                ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
        }
        dialog.setContentView(scroll)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.show()
        dialog.window?.setLayout(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
        )
        dialog.window?.setGravity(Gravity.BOTTOM)
    }

    private fun showTextZoomControls() {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)

        val sheet = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            contentDescription = "Page text size"
        }
        glaze.styleMenuSheet(sheet)

        val title = TextView(this).apply { text = "Page text size" }
        glaze.styleMenuTitle(title)
        sheet.addView(
            title,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val note = TextView(this).apply {
            text = "Changes website text size in Browser only. System font size is unchanged."
        }
        glaze.styleMenuSubtitle(note)
        sheet.addView(
            note,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val value = TextView(this).apply {
            gravity = Gravity.CENTER
            textSize = 24f
            setTextColor(glaze.palette.textPrimary)
            setPadding(dp(12), dp(8), dp(12), dp(12))
        }
        sheet.addView(
            value,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val controls = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }

        fun zoomAction(label: String): TextView = TextView(this).apply {
            text = label
            gravity = Gravity.CENTER
            glaze.styleMenuAction(this)
        }

        val smaller = zoomAction("Smaller")
        val reset = zoomAction("Reset")
        val larger = zoomAction("Larger")
        controls.addView(smaller, LinearLayout.LayoutParams(0, dp(56), 1f))
        controls.addView(reset, LinearLayout.LayoutParams(0, dp(56), 1f))
        controls.addView(larger, LinearLayout.LayoutParams(0, dp(56), 1f))
        sheet.addView(
            controls,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val close = zoomAction("Close")
        sheet.addView(
            close,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(56),
            ),
        )

        fun refreshZoomControls() {
            value.text = PageTextZoom.label(pageTextZoomPercent)
            setTextActionEnabled(
                smaller,
                pageTextZoomPercent > PageTextZoom.MIN_PERCENT,
            )
            setTextActionEnabled(
                reset,
                pageTextZoomPercent != PageTextZoom.DEFAULT_PERCENT,
            )
            setTextActionEnabled(
                larger,
                pageTextZoomPercent < PageTextZoom.MAX_PERCENT,
            )
        }

        smaller.setOnClickListener {
            applyPageTextZoom(PageTextZoom.decrease(pageTextZoomPercent))
            refreshZoomControls()
        }
        reset.setOnClickListener {
            applyPageTextZoom(PageTextZoom.DEFAULT_PERCENT)
            refreshZoomControls()
        }
        larger.setOnClickListener {
            applyPageTextZoom(PageTextZoom.increase(pageTextZoomPercent))
            refreshZoomControls()
        }
        close.setOnClickListener { dialog.dismiss() }

        refreshZoomControls()
        dialog.setContentView(sheet)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.show()
        dialog.window?.setLayout(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
        )
        dialog.window?.setGravity(Gravity.BOTTOM)
    }

    private fun applyPageTextZoom(percent: Int) {
        val normalized = PageTextZoom.normalize(percent)
        pageTextZoomPercent = normalized
        webView.settings.textZoom = normalized
        getSharedPreferences(PREFERENCES_NAME, MODE_PRIVATE)
            .edit()
            .putInt(PREF_PAGE_TEXT_ZOOM, normalized)
            .apply()
    }

    private fun setPageScriptsEnabled(enabled: Boolean) {
        if (pageScriptsEnabled == enabled) return
        pageScriptsEnabled = enabled
        webView.settings.javaScriptEnabled = enabled
        reloadWebsiteAfterPageControlChange()
    }

    private fun setPageImagesEnabled(enabled: Boolean) {
        if (pageImagesEnabled == enabled) return
        pageImagesEnabled = enabled
        webView.settings.loadsImagesAutomatically = enabled
        webView.settings.blockNetworkImage = !enabled
        reloadWebsiteAfterPageControlChange()
    }

    private fun reloadWebsiteAfterPageControlChange() {
        if (NavigationResolver.isAllowedWebUrl(currentUrl)) {
            failedMainFrameUrl = null
            chromeOverrideTitle = null
            webView.reload()
        } else {
            refreshChrome()
        }
    }

    private fun setDesktopSiteEnabled(enabled: Boolean) {
        if (desktopSiteEnabled == enabled) return
        desktopSiteEnabled = enabled
        webView.settings.apply {
            userAgentString = if (enabled) {
                DesktopUserAgent.fromMobile(mobileUserAgent)
            } else {
                mobileUserAgent
            }
            useWideViewPort = enabled
            loadWithOverviewMode = enabled
        }

        if (NavigationResolver.isAllowedWebUrl(currentUrl)) {
            failedMainFrameUrl = null
            chromeOverrideTitle = null
            webView.reload()
        } else {
            refreshChrome()
        }

        Toast.makeText(
            this,
            if (enabled) "Desktop site enabled for this Browser session." else
                "Mobile site mode restored for this Browser session.",
            Toast.LENGTH_SHORT,
        ).show()
    }

    private fun showClearBrowsingDataConfirmation() {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)

        val sheet = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            contentDescription = "Clear browsing data confirmation"
        }
        glaze.styleMenuSheet(sheet)

        val title = TextView(this).apply { text = "Clear browsing data?" }
        glaze.styleMenuTitle(title)
        sheet.addView(
            title,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val warning = TextView(this).apply {
            text = "This signs you out of websites and removes local website data from this Browser app. The action cannot be undone."
        }
        glaze.styleMenuSubtitle(warning)
        sheet.addView(
            warning,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val clearedHeading = TextView(this).apply {
            text = "Will be cleared"
            textSize = 14f
            setTextColor(glaze.palette.textPrimary)
            setPadding(dp(12), dp(8), dp(12), dp(2))
        }
        sheet.addView(
            clearedHeading,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val cleared = TextView(this).apply {
            text = BrowsingDataClearScope.clearedItems.joinToString(separator = "\n") { "• $it" }
        }
        glaze.styleMenuSubtitle(cleared)
        sheet.addView(
            cleared,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val preservedHeading = TextView(this).apply {
            text = "Will stay"
            textSize = 14f
            setTextColor(glaze.palette.textPrimary)
            setPadding(dp(12), dp(8), dp(12), dp(2))
        }
        sheet.addView(
            preservedHeading,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val preserved = TextView(this).apply {
            text = BrowsingDataClearScope.preservedItems.joinToString(separator = "\n") { "• $it" }
        }
        glaze.styleMenuSubtitle(preserved)
        sheet.addView(
            preserved,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        val actions = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }

        fun action(label: String): TextView = TextView(this).apply {
            text = label
            gravity = Gravity.CENTER
            glaze.styleMenuAction(this)
        }

        val cancel = action("Cancel")
        val clear = action("Clear data")
        actions.addView(cancel, LinearLayout.LayoutParams(0, dp(56), 1f))
        actions.addView(clear, LinearLayout.LayoutParams(0, dp(56), 1f))
        sheet.addView(
            actions,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT,
            ),
        )

        cancel.setOnClickListener { dialog.dismiss() }
        clear.setOnClickListener {
            setTextActionEnabled(clear, false)
            setTextActionEnabled(cancel, false)
            BrowserBrowsingDataCleaner.clear(webView) {
                runOnUiThread {
                    clearHistoryAfterNextPageFinished = true
                    showStartPage()
                    dialog.dismiss()
                    Toast.makeText(
                        this,
                        "Browsing data cleared. Browser settings were kept.",
                        Toast.LENGTH_LONG,
                    ).show()
                }
            }
        }

        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(
                sheet,
                ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
        }

        dialog.setContentView(scroll)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.show()
        dialog.window?.setLayout(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
        )
        dialog.window?.setGravity(Gravity.BOTTOM)
    }

    private fun setTextActionEnabled(view: TextView, enabled: Boolean) {
        view.isEnabled = enabled
        view.alpha = if (enabled) 1f else 0.34f
    }

    private fun chromeButton(icon: Int, description: String, action: (View) -> Unit): ImageButton =
        ImageButton(this).apply {
            setImageResource(icon)
            contentDescription = description
            setOnClickListener(action)
            glaze.styleChromeButton(this, GlazeContract.ButtonRole.Quiet)
        }

    private fun hideKeyboard(view: View = addressField) {
        (getSystemService(INPUT_METHOD_SERVICE) as? InputMethodManager)
            ?.hideSoftInputFromWindow(view.windowToken, 0)
    }

    private fun isInternalStartUrl(url: String): Boolean =
        url.startsWith(START_BASE_URL) || url.startsWith("data:text/html")

    private fun dp(value: Int): Int = glaze.dp(value)

    companion object {
        private const val INTERNAL_HOME = "goreecloud://start"
        private const val START_BASE_URL = "https://start.goreecloud.local/"
        private const val PREFERENCES_NAME = "goreecloud_browser_preferences"
        private const val PREF_PAGE_TEXT_ZOOM = "page_text_zoom_percent"
        private const val STATE_DESKTOP_SITE = "desktop_site_enabled"
        private const val STATE_PAGE_SCRIPTS = "page_scripts_enabled"
        private const val STATE_PAGE_IMAGES = "page_images_enabled"
    }
}
