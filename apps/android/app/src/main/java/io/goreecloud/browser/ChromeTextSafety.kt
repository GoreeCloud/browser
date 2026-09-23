package io.goreecloud.browser

/**
 * Removes Unicode bidirectional formatting controls from untrusted text before
 * it is rendered in Browser-owned chrome.
 *
 * These code points can change visual ordering without changing the underlying
 * string, so page content must not be able to use them to spoof title/address
 * identity. This is presentation-only; navigation authority is unchanged.
 */
internal object ChromeTextSafety {
    fun stripBidirectionalControls(value: String): String =
        value.filterNot { character -> isBidirectionalControl(character) }

    private fun isBidirectionalControl(character: Char): Boolean =
        when (character) {
            '\u061C',
            '\u200E',
            '\u200F',
            '\u202A',
            '\u202B',
            '\u202C',
            '\u202D',
            '\u202E',
            '\u2066',
            '\u2067',
            '\u2068',
            '\u2069',
            -> true
            else -> false
        }
}
