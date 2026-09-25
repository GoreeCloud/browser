package io.goreecloud.browser

/**
 * Narrow text-safety boundary for Search delegation and omnibox classification.
 *
 * Ordinary language and shaping characters remain valid. C0/C1 controls and
 * Unicode bidirectional formatting controls are rejected because they can alter
 * transport framing or visually reorder untrusted query/credential text.
 */
internal object SearchBoundaryTextSafety {
    fun containsUnsupportedControl(value: String): Boolean =
        value.any(::isUnsupportedControl)

    fun isUnsupportedControl(character: Char): Boolean =
        Character.isISOControl(character.code) || isBidirectionalFormattingControl(character)

    private fun isBidirectionalFormattingControl(character: Char): Boolean =
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
