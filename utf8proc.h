/*
 * Copyright (c) 2009 Public Software Group e. V., Berlin, Germany
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


/** 
 * @mainpage
 *
 * uf8proc is a mapping tool for UTF-8 strings with the following features:
 *
 * - decomposing and composing of strings
 * - replacing compatibility characters with their equivalents
 * - stripping of "default ignorable characters"
 *   like SOFT-HYPHEN or ZERO-WIDTH-SPACE
 * - folding of certain characters for string comparison
 *   (e.g. HYPHEN U+2010 and MINUS U+2212 to ASCII "-")
 *   (see "LUMP" option)
 * - optional rejection of strings containing non-assigned code points
 * - stripping of control characters
 * - stripping of character marks (accents, etc.)
 * - transformation of LF, CRLF, CR and NEL to line-feed (LF)
 *   or to the unicode chararacters for paragraph separation (PS)
 *   or line separation (LS).
 * - unicode case folding (for case insensitive string comparisons)
 * - rejection of illegal UTF-8 data
 *   (i.e. UTF-8 encoded UTF-16 surrogates)
 * - support for korean hangul characters
 *
 * Unicode Version 7.0.0 is supported.
 *
 * See @ref utf8proc.h for the API.
 */

/** @file */

#ifndef UTF8PROC_H
#define UTF8PROC_H


#include <stdlib.h>
#include <sys/types.h>
#ifdef _MSC_VER
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
#  ifdef _WIN64
#    define ssize_t __int64
#  else
#    define ssize_t int
#  endif
#  ifndef __cplusplus
typedef unsigned char bool;
enum {false, true};
#  endif
#else
#  include <stdbool.h>
#  include <inttypes.h>
#endif
#include <limits.h>

#ifdef _WIN32
#  ifdef UTF8PROC_EXPORTS
#    define DLLEXPORT __declspec(dllexport)
#  else
#    define DLLEXPORT __declspec(dllimport)
#  endif
#elif __GNUC__ >= 4
#  define DLLEXPORT __attribute__ ((visibility("default")))
#else
#  define DLLEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SSIZE_MAX
#define SSIZE_MAX ((size_t)SIZE_MAX/2)
#endif

/**
 * Option flags used by several functions in the library.
 */
typedef enum {
  /** The given UTF-8 input is NULL terminated. */
  UTF8PROC_NULLTERM  = (1<<0),
  /** Unicode Versioning Stability has to be respected. */
  UTF8PROC_STABLE    = (1<<1),
  /** Compatibility decomposition (i.e. formatting information is lost). */
  UTF8PROC_COMPAT    = (1<<2),
  /** Return a result with decomposed characters. */
  UTF8PROC_COMPOSE   = (1<<3),
  /** Return a result with decomposed characters. */
  UTF8PROC_DECOMPOSE = (1<<4),
  /** Strip "default ignorable characters". */
  UTF8PROC_IGNORE    = (1<<5),
  /** Return an error, if the input contains unassigned code points. */
  UTF8PROC_REJECTNA  = (1<<6),
  /**
   * Indicating that NLF-sequences (LF, CRLF, CR, NEL) are representing a
   * line break, and should be converted to the codepoint for line
   * separation (LS).
   */
  UTF8PROC_NLF2LS    = (1<<7),
  /**
   * Indicating that NLF-sequences are representing a paragraph break, and
   * should be converted to the codepoint for paragraph separation
   * (PS).
   */
  UTF8PROC_NLF2PS    = (1<<8),
  /** Indicating that the meaning of NLF-sequences is unknown. */
  UTF8PROC_NLF2LF    = (UTF8PROC_NLF2LS | UTF8PROC_NLF2PS),
  /** Strips and/or convers control characters.
   *
   * NLF-sequences are transformed into space, except if one of the
   * NLF2LS/PS/LF options is given. HorizontalTab (HT) and FormFeed (FF)
   * are treated as a NLF-sequence in this case.  All other control
   * characters are simply removed.
   */
  UTF8PROC_STRIPCC   = (1<<9),
  /**
   * Performs unicode case folding, to be able to do a case-insensitive
   * string comparison.
   */
  UTF8PROC_CASEFOLD  = (1<<10),
  /**
   * Inserts 0xFF bytes at the beginning of each sequence which is
   * representing a single grapheme cluster (see UAX#29).
   */
  UTF8PROC_CHARBOUND = (1<<11),
  /** Lumps certain characters together.
   *
   * E.g. HYPHEN U+2010 and MINUS U+2212 to ASCII "-". See lump.md for details.
   *
   * If NLF2LF is set, this includes a transformation of paragraph and
   * line separators to ASCII line-feed (LF).
   */
  UTF8PROC_LUMP      = (1<<12),
  /** Strips all character markings.
   *
   * This includes non-spacing, spacing and enclosing (i.e. accents).
   * @note This option works only with @ref UTF8PROC_COMPOSE or
   *       @ref UTF8PROC_DECOMPOSE
   */
  UTF8PROC_STRIPMARK = (1<<13),
} utf8proc_option_t;

/** @name Error codes
 * Error codes being returned by almost all functions.
 */
/** @{ */
/** Memory could not be allocated. */
#define UTF8PROC_ERROR_NOMEM -1
/** The given string is too long to be processed. */
#define UTF8PROC_ERROR_OVERFLOW -2
/** The given string is not a legal UTF-8 string. */
#define UTF8PROC_ERROR_INVALIDUTF8 -3
/** The @ref UTF8PROC_REJECTNA flag was set and an unassigned code point was found. */
#define UTF8PROC_ERROR_NOTASSIGNED -4
/** Invalid options have been used. */
#define UTF8PROC_ERROR_INVALIDOPTS -5
/** @} */

/* @name Types */

/** Holds the value of a property. */
typedef int16_t utf8proc_propval_t;

/** Struct containing information about a codepoint. */
typedef struct utf8proc_property_struct {
  /**
   * Unicode category.
   * @see utf8proc_category_t.
   */
  utf8proc_propval_t category;
  utf8proc_propval_t combining_class;
  /**
   * Bidirectional class.
   * @see utf8proc_bidi_class_t.
   */
  utf8proc_propval_t bidi_class;
  /**
   * @anchor Decomposition type.
   * @see utf8proc_decomp_type_t.
   */
  utf8proc_propval_t decomp_type;
  const int32_t *decomp_mapping;
  const int32_t *casefold_mapping;
  int32_t uppercase_mapping;
  int32_t lowercase_mapping;
  int32_t titlecase_mapping;
  int32_t comb1st_index;
  int32_t comb2nd_index;
  unsigned bidi_mirrored:1;
  unsigned comp_exclusion:1;
  /**
   * Can this codepoint be ignored?
   *
   * Used by @ref utf8proc_decompose_char when @ref UTF8PROC_IGNORE is
   * passed as an option.
   */
  unsigned ignorable:1;
  unsigned control_boundary:1;
  /**
   * Boundclass.
   * @see utf8proc_boundclass_t.
   */
  unsigned boundclass:4;
  /** The width of the codepoint. */
  unsigned charwidth:2;
} utf8proc_property_t;

/** Unicode categories. */
typedef enum {
  UTF8PROC_CATEGORY_CN  = 0, /**< Other, not assigned */
  UTF8PROC_CATEGORY_LU  = 1, /**< Letter, uppercase */
  UTF8PROC_CATEGORY_LL  = 2, /**< Letter, lowercase */
  UTF8PROC_CATEGORY_LT  = 3, /**< Letter, titlecase */
  UTF8PROC_CATEGORY_LM  = 4, /**< Letter, modifier */
  UTF8PROC_CATEGORY_LO  = 5, /**< Letter, other */
  UTF8PROC_CATEGORY_MN  = 6, /**< Mark, nonspacing */
  UTF8PROC_CATEGORY_MC  = 7, /**< Mark, spacing combining */
  UTF8PROC_CATEGORY_ME  = 8, /**< Mark, enclosing */
  UTF8PROC_CATEGORY_ND  = 9, /**< Number, decimal digit */
  UTF8PROC_CATEGORY_NL = 10, /**< Number, letter */
  UTF8PROC_CATEGORY_NO = 11, /**< Number, other */
  UTF8PROC_CATEGORY_PC = 12, /**< Punctuation, connector */
  UTF8PROC_CATEGORY_PD = 13, /**< Punctuation, dash */
  UTF8PROC_CATEGORY_PS = 14, /**< Punctuation, open */
  UTF8PROC_CATEGORY_PE = 15, /**< Punctuation, close */
  UTF8PROC_CATEGORY_PI = 16, /**< Punctuation, initial quote */
  UTF8PROC_CATEGORY_PF = 17, /**< Punctuation, final quote */
  UTF8PROC_CATEGORY_PO = 18, /**< Punctuation, other */
  UTF8PROC_CATEGORY_SM = 19, /**< Symbol, math */
  UTF8PROC_CATEGORY_SC = 20, /**< Symbol, currency */
  UTF8PROC_CATEGORY_SK = 21, /**< Symbol, modifier */
  UTF8PROC_CATEGORY_SO = 22, /**< Symbol, other */
  UTF8PROC_CATEGORY_ZS = 23, /**< Separator, space */
  UTF8PROC_CATEGORY_ZL = 24, /**< Separator, line */
  UTF8PROC_CATEGORY_ZP = 25, /**< Separator, paragraph */
  UTF8PROC_CATEGORY_CC = 26, /**< Other, control */
  UTF8PROC_CATEGORY_CF = 27, /**< Other, format */
  UTF8PROC_CATEGORY_CS = 28, /**< Other, surrogate */
  UTF8PROC_CATEGORY_CO = 29, /**< Other, private use */
} utf8proc_category_t;

/** Bidirectional character classes. */
typedef enum {
  UTF8PROC_BIDI_CLASS_L     = 1, /**< Left-to-Right */
  UTF8PROC_BIDI_CLASS_LRE   = 2, /**< Left-to-Right Embedding */
  UTF8PROC_BIDI_CLASS_LRO   = 3, /**< Left-to-Right Override */
  UTF8PROC_BIDI_CLASS_R     = 4, /**< Right-to-Left */
  UTF8PROC_BIDI_CLASS_AL    = 5, /**< Right-to-Left Arabic */
  UTF8PROC_BIDI_CLASS_RLE   = 6, /**< Right-to-Left Embedding */
  UTF8PROC_BIDI_CLASS_RLO   = 7, /**< Right-to-Left Override */
  UTF8PROC_BIDI_CLASS_PDF   = 8, /**< Pop Directional Format */
  UTF8PROC_BIDI_CLASS_EN    = 9, /**< European Number */
  UTF8PROC_BIDI_CLASS_ES   = 10, /**< European Separator */
  UTF8PROC_BIDI_CLASS_ET   = 11, /**< European Number Terminator */
  UTF8PROC_BIDI_CLASS_AN   = 12, /**< Arabic Number */
  UTF8PROC_BIDI_CLASS_CS   = 13, /**< Common Number Separator */
  UTF8PROC_BIDI_CLASS_NSM  = 14, /**< Nonspacing Mark */
  UTF8PROC_BIDI_CLASS_BN   = 15, /**< Boundary Neutral */
  UTF8PROC_BIDI_CLASS_B    = 16, /**< Paragraph Separator */
  UTF8PROC_BIDI_CLASS_S    = 17, /**< Segment Separator */
  UTF8PROC_BIDI_CLASS_WS   = 18, /**< Whitespace */
  UTF8PROC_BIDI_CLASS_ON   = 19, /**< Other Neutrals */
  UTF8PROC_BIDI_CLASS_LRI  = 20, /**< Left-to-Right Isolate */
  UTF8PROC_BIDI_CLASS_RLI  = 21, /**< Right-to-Left Isolate */
  UTF8PROC_BIDI_CLASS_FSI  = 22, /**< First Strong Isolate */
  UTF8PROC_BIDI_CLASS_PDI  = 23, /**< Pop Directional Isolate */
} utf8proc_bidi_class_t;

/** Decomposition type. */
typedef enum {
  UTF8PROC_DECOMP_TYPE_FONT      = 1, /**< Font */
  UTF8PROC_DECOMP_TYPE_NOBREAK   = 2, /**< Nobreak */
  UTF8PROC_DECOMP_TYPE_INITIAL   = 3, /**< Initial */
  UTF8PROC_DECOMP_TYPE_MEDIAL    = 4, /**< Medial */
  UTF8PROC_DECOMP_TYPE_FINAL     = 5, /**< Final */
  UTF8PROC_DECOMP_TYPE_ISOLATED  = 6, /**< Isolated */
  UTF8PROC_DECOMP_TYPE_CIRCLE    = 7, /**< Circle */
  UTF8PROC_DECOMP_TYPE_SUPER     = 8, /**< Super */
  UTF8PROC_DECOMP_TYPE_SUB       = 9, /**< Sub */
  UTF8PROC_DECOMP_TYPE_VERTICAL = 10, /**< Vertical */
  UTF8PROC_DECOMP_TYPE_WIDE     = 11, /**< Wide */
  UTF8PROC_DECOMP_TYPE_NARROW   = 12, /**< Narrow */
  UTF8PROC_DECOMP_TYPE_SMALL    = 13, /**< Small */
  UTF8PROC_DECOMP_TYPE_SQUARE   = 14, /**< Square */
  UTF8PROC_DECOMP_TYPE_FRACTION = 15, /**< Fraction */
  UTF8PROC_DECOMP_TYPE_COMPAT   = 16, /**< Compat */
} utf8proc_decomp_type_t;

/** Boundclass property. */
typedef enum {
  UTF8PROC_BOUNDCLASS_START              =  0, /**< Start */
  UTF8PROC_BOUNDCLASS_OTHER              =  1, /**< Other */
  UTF8PROC_BOUNDCLASS_CR                 =  2, /**< Cr */
  UTF8PROC_BOUNDCLASS_LF                 =  3, /**< Lf */
  UTF8PROC_BOUNDCLASS_CONTROL            =  4, /**< Control */
  UTF8PROC_BOUNDCLASS_EXTEND             =  5, /**< Extend */
  UTF8PROC_BOUNDCLASS_L                  =  6, /**< L */
  UTF8PROC_BOUNDCLASS_V                  =  7, /**< V */
  UTF8PROC_BOUNDCLASS_T                  =  8, /**< T */
  UTF8PROC_BOUNDCLASS_LV                 =  9, /**< Lv */
  UTF8PROC_BOUNDCLASS_LVT                = 10, /**< Lvt */
  UTF8PROC_BOUNDCLASS_REGIONAL_INDICATOR = 11, /**< Regional indicator */
  UTF8PROC_BOUNDCLASS_SPACINGMARK        = 12, /**< Spacingmark */
} utf8proc_boundclass_t;

/**
 * Array containing the byte lengths of a UTF-8 encoded codepoint based
 * on the first byte.
 */
DLLEXPORT extern const int8_t utf8proc_utf8class[256];

/**
 * Returns the version as a string.
 */
DLLEXPORT const char *utf8proc_version(void);

/**
 * Con
 * Returns a static error string for the given error code.
 */
DLLEXPORT const char *utf8proc_errmsg(ssize_t errcode);

/**
 * Reads a single codepoint from the UTF-8 sequence being pointed to by 'str'.
 * The maximum number of bytes read is 'strlen', unless 'strlen' is
 * negative (in which case up to 4 bytes are read).
 *
 * If a valid codepoint could be read, it is stored in the variable
 * being pointed to by 'codepoint_ref', otherwise that variable will be set to -1.
 * In case of success the number of bytes read is returned, otherwise a
 * negative error code is returned.
 */
DLLEXPORT ssize_t utf8proc_iterate(const uint8_t *str, ssize_t strlen, int32_t *codepoint_ref);

/**
 * Check if a codepoint is valid.
 *
 * @return 1, if the given codepoint is valid, otherwise 0.
 */
DLLEXPORT bool utf8proc_codepoint_valid(int32_t codepoint);

/**
 * Encodes the codepoint as an UTF-8 string in
 * the byte array being pointed to by 'dst'. This array has to be at least
 * 4 bytes long.
 *
 * In case of success the number of bytes written is returned,
 * otherwise 0.
 *
 * This function does not check if the codepoint is a valid unicode code point.
 */
DLLEXPORT ssize_t utf8proc_encode_char(int32_t codepoint, uint8_t *dst);

/**
 * Lookup the properties for a given codepoint.
 *
 * @param codepoint The codepoint.
 *
 * @returns
 * A pointer to a (constant) struct containing information about
 * the codepoint.
 * @par
 * If the codepoint is not existent a pointer to a special struct is
 * returned, where `category` is 0 (@ref UTF8PROC_CATEGORY_CN).
 */
DLLEXPORT const utf8proc_property_t *utf8proc_get_property(int32_t codepoint);

/** Decompose a codepoint into an array of codepoints.
 *
 * @param codepoint the codepoint.
 * @param dst the destination buffer.
 * @param bufsize the size of the destination buffer.
 * @param options one or more of the following flags:
 * - @ref UTF8PROC_REJECTNA  - return an error `codepoint` is unassigned
 * - @ref UTF8PROC_IGNORE    - strip "default ignorable" codepoints
 * - @ref UTF8PROC_CASEFOLD  - apply Unicode casefolding
 * - @ref UTF8PROC_COMPAT    - replace certain codepoints with their
 *                             compatibility decomposition
 * - @ref UTF8PROC_CHARBOUND - insert 0xFF bytes before each grapheme cluster
 * - @ref UTF8PROC_LUMP      - lump certain different codepoints together
 * - @ref UTF8PROC_STRIPMARK - remove all character marks
 * @param last_boundclass
 * This pointer has to point to an integer variable which is storing
 * the last codepoint's boundary class, if the @ref UTF8PROC_CHARBOUND
 * option is used.
 *
 * @return
 * In case of success the number of codepoints written is returned, in case
 * of an error, a negative error code is returned.
 * @par
 * If the number of written codepoints would be bigger than 'bufsize', the
 * buffer (up to 'bufsize') has inpredictable data, and the needed
 * buffer size is returned.
 */
DLLEXPORT ssize_t utf8proc_decompose_char(
  int32_t codepoint, int32_t *dst, ssize_t bufsize,
  utf8proc_option_t options, int *last_boundclass
);

/**
 * Does the same as 'utf8proc_decompose_char', but acts on a whole UTF-8
 * string, and orders the decomposed sequences correctly.
 *
 * If the @ref UTF8PROC_NULLTERM flag in 'options' is set, processing will be stopped,
 * when a NULL byte is encounted, otherwise 'strlen' bytes are processed.
 * The result in form of unicode code points is written into the buffer
 * being pointed to by 'buffer', having the length of 'bufsize' entries.
 * In case of success the number of codepoints written is returned,
 * in case of an error, a negative error code is returned.
 *
 * If the number of written codepoints would be bigger than 'bufsize',
 * the buffer (up to 'bufsize') has inpredictable data, and the needed
 * buffer size is returned.
 */
DLLEXPORT ssize_t utf8proc_decompose(
  const uint8_t *str, ssize_t strlen,
  int32_t *buffer, ssize_t bufsize, utf8proc_option_t options
);

/**
 * Reencodes the sequence of codepoints given by the pointer
 * 'buffer' and 'length' as UTF-8.
 *
 * The result is stored in the same memory area where the data is read.
 *
 * @param buffer the (native-endian UTF-32) unicode codepoints to re-encode.
 * @param length the length (in codepoints) of the buffer.
 * @param options one or more of the following flags:
 * - @ref UTF8PROC_NLF2LS  - convert LF, CRLF, CR and NEL into LS
 * - @ref UTF8PROC_NLF2PS  - convert LF, CRLF, CR and NEL into PS
 * - @ref UTF8PROC_NLF2LF  - convert LF, CRLF, CR and NEL into LF
 * - @ref UTF8PROC_STRIPCC - strip or convert all non-affected control characters
 * - @ref UTF8PROC_COMPOSE - try to combine decomposed codepoints into composite
 *                           codepoints
 * - @ref UTF8PROC_STABLE  - prohibit combining characters which would violate
 *                           the unicode versioning stability
 *
 * @return
 * In case of success the length of the resulting UTF-8 string is
 * returned, otherwise a negative error code is returned.
 *
 * @warning The amount of free space being pointed to by 'buffer', has to
 *          exceed the amount of the input data by one byte, and the
 *          entries of the array pointed to by 'str' have to be in the
 *          range of 0x0000 to 0x10FFFF, otherwise the program might
 *          crash!
 */
DLLEXPORT ssize_t utf8proc_reencode(int32_t *buffer, ssize_t length, utf8proc_option_t options);

/**
 * Given a pair of consecutive codepoints, return whether a grapheme break is
 * permitted between them (as defined by the extended grapheme clusters in UAX#29).
 */
DLLEXPORT bool utf8proc_grapheme_break(int32_t codepoint1, int32_t codepoint2);

/**
 * Given a codepoint, return a character width analogous to wcwidth(codepoint),
 * except that a width of 0 is returned for non-printable codepoints
 * instead of -1 as in wcwidth.
 * 
 * @note
 * If you want to check for particular types of non-printable characters,
 * (analogous to isprint or iscntrl), use @ref utf8proc_category. */
DLLEXPORT int utf8proc_charwidth(int32_t codepoint);

/**
 * Return the Unicode category for the codepoint (one of the
 * @ref utf8proc_category_t constants.)
 */
DLLEXPORT utf8proc_category_t utf8proc_category(int32_t codepoint);

/**
 * Return the two-letter (nul-terminated) Unicode category string for
 * the codepoint (e.g. "Lu" or "Co").
 */
DLLEXPORT const char *utf8proc_category_string(int32_t codepoint);

/**
 * Maps the given UTF-8 string being pointed to by 'str' to a new UTF-8
 * string, which is allocated dynamically, and afterwards pointed to by
 * the pointer being pointed to by 'dstptr'.
 *
 * If the @ref UTF8PROC_NULLTERM flag in the 'options' field is set, the length is
 * determined by a NULL terminator, otherwise the parameter 'strlen' is
 * evaluated to determine the string length, but in any case the result
 * will be NULL terminated (though it might contain NULL characters
 * before). Other flags in the 'options' field are passed to the
 * functions defined above, and regarded as described.
 *
 * In case of success the length of the new string is returned,
 * otherwise a negative error code is returned.
 *
 * *NOTICE*: The memory of the new UTF-8 string will have been allocated
 * with 'malloc', and has theirfore to be freed with 'free'.
 */
DLLEXPORT ssize_t utf8proc_map(
  const uint8_t *str, ssize_t strlen, uint8_t **dstptr, utf8proc_option_t options
);

/** @name Normalized versions.
 *
 * Returns a pointer to newly allocated memory of a NFD, NFC, NFKD or NFKC
 * normalized version of the null-terminated string 'str'.
 */
/** @{ */
DLLEXPORT uint8_t *utf8proc_NFD(const uint8_t *str);
DLLEXPORT uint8_t *utf8proc_NFC(const uint8_t *str);
DLLEXPORT uint8_t *utf8proc_NFKD(const uint8_t *str);
DLLEXPORT uint8_t *utf8proc_NFKC(const uint8_t *str);
/** @} */

#ifdef __cplusplus
}
#endif

#endif

