package io.goreecloud.browser

import android.app.Activity
import android.app.Dialog
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.net.Uri
import android.net.http.SslError
import android.os.Build
import android.os.Bundle
import android.text.Editable
import android.text.TextWatcher
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
import android.webkit.WebStorage
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

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        glaze = GlazeNativeStyle(this)
        glaze.applyWindow(this)
        firstUsePreferences = BrowserFirstUsePreferences(this)
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
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) safeBrowsingEnabled = true
            userAgentString = "$userAgentString GoreeCloudBrowser/${BuildConfig.VERSION_NAME} Android"
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
            text = if (currentUrl == INTERNAL_HOME) "Development build" else Uri.parse(currentUrl).host.orEmpty()
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

        addAction("Copy page address") {
            val clipboard = getSystemService(CLIPBOARD_SERVICE) as android.content.ClipboardManager
            clipboard.setPrimaryClip(android.content.ClipData.newPlainText("Page address", currentUrl))
        }
        addAction("Share page") {
            startActivity(Intent.createChooser(Intent(Intent.ACTION_SEND).apply {
                type = "text/plain"
                putExtra(Intent.EXTRA_TEXT, currentUrl)
            }, "Share page"))
        }
        addAction("Find in page") {
            showFindInPageDialog()
        }

        val firstUseState = firstUsePreferences.state()
        addAction("Contextual hints: " + if (firstUseState.hintsEnabled) "On" else "Off") {
            firstUsePreferences.setHintsEnabled(!firstUseState.hintsEnabled)
            renderContextualHint()
        }
        if (firstUseState.hintsEnabled && firstUseState.chromeHintDismissed) {
            addAction("Reset dismissed hints") {
                firstUsePreferences.resetDismissedHints()
                renderContextualHint()
            }
        }
        addAction("Replay setup") {
            firstUsePreferences.replay()
            showFirstUseDialog(replay = true)
        }
        addAction("Clear browsing data") {
            showClearBrowsingDataDialog()
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

    private fun showFindInPageDialog() {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)

        val sheet = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            contentDescription = "Find in page"
        }
        glaze.styleMenuSheet(sheet)

        val title = TextView(this).apply { text = "Find in page" }
        glaze.styleMenuTitle(title)
        sheet.addView(title)

        val query = EditText(this).apply {
            hint = "Text on this page"
            contentDescription = "Find text on page"
            isSingleLine = true
            imeOptions = EditorInfo.IME_ACTION_SEARCH
            inputType = android.text.InputType.TYPE_CLASS_TEXT
            setPadding(dp(16), 0, dp(16), 0)
        }
        glaze.styleAddressField(query)
        sheet.addView(
            query,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.OMNIBOX_HEIGHT_DP),
            ),
        )

        val status = TextView(this).apply {
            text = "Type text to find on this page"
            contentDescription = "Find in page result count"
        }
        glaze.styleMenuSubtitle(status)
        sheet.addView(status)

        val actions = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.END
        }
        val previous = TextView(this).apply {
            text = "Previous"
            contentDescription = "Previous match"
            glaze.styleMenuAction(this)
            isEnabled = false
            alpha = 0.45f
            setOnClickListener {
                if (query.text.isNotBlank()) webView.findNext(false)
            }
        }
        val next = TextView(this).apply {
            text = "Next"
            contentDescription = "Next match"
            glaze.styleMenuAction(this)
            isEnabled = false
            alpha = 0.45f
            setOnClickListener {
                if (query.text.isNotBlank()) webView.findNext(true)
            }
        }
        val close = TextView(this).apply {
            text = "Close"
            contentDescription = "Close Find in page"
            glaze.styleMenuAction(this)
            setOnClickListener { dialog.dismiss() }
        }
        listOf(previous, next, close).forEach { control ->
            actions.addView(
                control,
                LinearLayout.LayoutParams(
                    0,
                    dp(GlazeContract.MENU_ACTION_HEIGHT_DP),
                    1f,
                ),
            )
        }
        sheet.addView(actions)

        webView.setFindListener { activeMatchOrdinal, numberOfMatches, doneCounting ->
            runOnUiThread {
                status.text = FindInPagePresentation.status(
                    activeMatchOrdinal = activeMatchOrdinal,
                    numberOfMatches = numberOfMatches,
                    doneCounting = doneCounting,
                )
                val canNavigate = doneCounting && numberOfMatches > 0
                previous.isEnabled = canNavigate
                next.isEnabled = canNavigate
                previous.alpha = if (canNavigate) 1f else 0.45f
                next.alpha = if (canNavigate) 1f else 0.45f
            }
        }

        query.addTextChangedListener(object : TextWatcher {
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
            ) {
                val text = value?.toString().orEmpty()
                if (text.isBlank()) {
                    webView.clearMatches()
                    status.text = "Type text to find on this page"
                    previous.isEnabled = false
                    next.isEnabled = false
                    previous.alpha = 0.45f
                    next.alpha = 0.45f
                } else {
                    status.text = "Searching…"
                    previous.isEnabled = false
                    next.isEnabled = false
                    previous.alpha = 0.45f
                    next.alpha = 0.45f
                    webView.findAllAsync(text)
                }
            }

            override fun afterTextChanged(value: Editable?) = Unit
        })
        query.setOnEditorActionListener { _, actionId, event ->
            val search = actionId == EditorInfo.IME_ACTION_SEARCH ||
                (event?.keyCode == KeyEvent.KEYCODE_ENTER && event.action == KeyEvent.ACTION_DOWN)
            if (search && query.text.isNotBlank()) {
                webView.findNext(true)
                true
            } else {
                false
            }
        }

        dialog.setOnDismissListener {
            webView.setFindListener(null)
            webView.clearMatches()
            hideKeyboard()
        }
        dialog.setContentView(sheet)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.setOnShowListener {
            query.requestFocus()
            query.post {
                val imm = getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager
                imm.showSoftInput(query, InputMethodManager.SHOW_IMPLICIT)
            }
        }
        dialog.show()
        dialog.window?.apply {
            setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
            setGravity(Gravity.BOTTOM)
        }
    }

    private fun showClearBrowsingDataDialog() {
        val dialog = Dialog(this)
        dialog.requestWindowFeature(Window.FEATURE_NO_TITLE)

        val sheet = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            contentDescription = "Clear browsing data confirmation"
        }
        glaze.styleMenuSheet(sheet)

        val title = TextView(this).apply { text = "Clear browsing data?" }
        glaze.styleMenuTitle(title)
        sheet.addView(title)

        val body = TextView(this).apply {
            text = "This clears Browser cookies, local site storage, cached web content, form data, and this WebView history on this device. It does not delete downloaded files or GoreeCloud account data."
        }
        glaze.styleMenuSubtitle(body)
        sheet.addView(body)

        val cancel = TextView(this).apply {
            text = "Cancel"
            contentDescription = "Cancel clearing browsing data"
            glaze.styleMenuAction(this)
            setOnClickListener { dialog.dismiss() }
        }
        sheet.addView(
            cancel,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.MENU_ACTION_HEIGHT_DP),
            ),
        )

        val clear = TextView(this).apply {
            text = "Clear browsing data"
            contentDescription = "Confirm clearing browsing data"
            glaze.styleMenuAction(this)
            setOnClickListener {
                dialog.dismiss()
                WebStorage.getInstance().deleteAllData()
                webView.clearCache(true)
                webView.clearFormData()
                webView.clearHistory()
                CookieManager.getInstance().removeAllCookies {
                    CookieManager.getInstance().flush()
                    runOnUiThread {
                        showStartPage()
                        Toast.makeText(
                            this@BrowserActivityV2,
                            "Browsing data cleared",
                            Toast.LENGTH_SHORT,
                        ).show()
                    }
                }
            }
        }
        sheet.addView(
            clear,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.MENU_ACTION_HEIGHT_DP),
            ),
        )

        dialog.setContentView(sheet)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.show()
        dialog.window?.apply {
            setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
            setGravity(Gravity.CENTER)
        }
    }

    private fun renderContextualHint() {
        contextualHintRow?.let(topChrome::removeView)
        contextualHintRow = null

        val state = firstUsePreferences.state()
        if (!state.completed || !state.hintsEnabled || state.chromeHintDismissed) return

        val row = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            contentDescription = "Browser tip"
            setPadding(dp(8), dp(2), 0, 0)
        }
        val message = TextView(this).apply {
            text = "Tip: tap the address bar to enter a website. Free-text Search stays local until a governed Search provider is available."
        }
        glaze.styleMenuSubtitle(message)
        row.addView(
            message,
            LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f),
        )
        val dismiss = TextView(this).apply {
            text = "Dismiss"
            contentDescription = "Dismiss Browser tip"
            glaze.styleMenuAction(this)
            setOnClickListener {
                firstUsePreferences.dismissChromeHint()
                renderContextualHint()
            }
        }
        row.addView(
            dismiss,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                dp(GlazeContract.MENU_ACTION_HEIGHT_DP),
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
            contentDescription = if (replay) "Browser setup review" else "Browser first-use setup"
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

        val hintToggle = TextView(this)
        glaze.styleMenuAction(hintToggle)
        sheet.addView(
            hintToggle,
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(GlazeContract.MENU_ACTION_HEIGHT_DP),
            ),
        )

        val actions = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.END
        }
        val returnButton = TextView(this).apply {
            text = "Return to Browser"
            contentDescription = "Return to Browser"
            glaze.styleMenuAction(this)
            visibility = if (replay) View.VISIBLE else View.GONE
            setOnClickListener { dialog.dismiss() }
        }
        val back = TextView(this).apply {
            text = "Back"
            glaze.styleMenuAction(this)
        }
        val next = TextView(this).apply {
            glaze.styleMenuAction(this)
        }
        actions.addView(returnButton, LinearLayout.LayoutParams(0, dp(GlazeContract.MENU_ACTION_HEIGHT_DP), 1f))
        actions.addView(back, LinearLayout.LayoutParams(0, dp(GlazeContract.MENU_ACTION_HEIGHT_DP), 1f))
        actions.addView(next, LinearLayout.LayoutParams(0, dp(GlazeContract.MENU_ACTION_HEIGHT_DP), 1f))
        sheet.addView(actions)

        val steps = listOf(
            "Welcome to GoreeCloud Browser" to
                "Browser chrome is GoreeCloud-owned. Android System WebView supplies the engine while GoreeCloud controls navigation, privacy defaults, and the visible experience.",
            "Private by default" to
                "File and content access are disabled, mixed content is blocked, third-party cookies are disabled, site permission requests fail closed, and certificate errors are never bypassed.",
            "Navigate with clear boundaries" to
                "Enter website addresses directly. Free-text Search stays local until accepted Privacy Shield authorization and a compatible Search provider are available. Optional hints never hide security or failure messages.",
        )
        var step = BrowserFirstUsePolicy.normalizeStep(firstUsePreferences.state().step)

        fun renderStep() {
            val state = firstUsePreferences.state()
            progress.text = "Step " + (step + 1) + " of " + BrowserFirstUsePolicy.STEP_COUNT
            title.text = steps[step].first
            body.text = steps[step].second
            back.visibility = if (step == 0) View.INVISIBLE else View.VISIBLE
            next.text = if (step == BrowserFirstUsePolicy.STEP_COUNT - 1) "Start browsing" else "Continue"
            next.contentDescription = next.text
            hintToggle.visibility =
                if (step == BrowserFirstUsePolicy.STEP_COUNT - 1) View.VISIBLE else View.GONE
            hintToggle.text = "Contextual hints: " + if (state.hintsEnabled) "On" else "Off"
            hintToggle.contentDescription = hintToggle.text
        }

        back.setOnClickListener {
            step = BrowserFirstUsePolicy.previousStep(step)
            firstUsePreferences.setStep(step)
            renderStep()
        }
        next.setOnClickListener {
            if (step == BrowserFirstUsePolicy.STEP_COUNT - 1) {
                firstUsePreferences.complete()
                dialog.dismiss()
                renderContextualHint()
            } else {
                step = BrowserFirstUsePolicy.nextStep(step)
                firstUsePreferences.setStep(step)
                renderStep()
            }
        }
        hintToggle.setOnClickListener {
            val state = firstUsePreferences.state()
            firstUsePreferences.setHintsEnabled(!state.hintsEnabled)
            renderStep()
        }

        renderStep()
        dialog.setContentView(sheet)
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
        dialog.show()
        dialog.window?.apply {
            setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            setLayout(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
            setGravity(Gravity.CENTER)
        }
    }

    private fun chromeButton(icon: Int, description: String, action: (View) -> Unit): ImageButton =
        ImageButton(this).apply {
            setImageResource(icon)
            contentDescription = description
            setOnClickListener(action)
            glaze.styleChromeButton(this, GlazeContract.ButtonRole.Quiet)
        }

    private fun hideKeyboard() {
        (getSystemService(INPUT_METHOD_SERVICE) as? InputMethodManager)
            ?.hideSoftInputFromWindow(addressField.windowToken, 0)
    }

    private fun isInternalStartUrl(url: String): Boolean =
        url.startsWith(START_BASE_URL) || url.startsWith("data:text/html")

    private fun dp(value: Int): Int = glaze.dp(value)

    companion object {
        private const val INTERNAL_HOME = "goreecloud://start"
        private const val START_BASE_URL = "https://start.goreecloud.local/"
    }
}
