/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -E -C -n -o -t -k '*' -NfindValue -Hhash_val -Wwordlist_value -D cssvalues.gperf  */
/* This file is automatically generated from cssvalues.in by makevalues, do not edit */
/* Copyright 1999 W. Bastian */
#include "cssvalues.h"
struct css_value {
    const char *name;
    int id;
};
/* maximum key range = 1641, duplicates = 1 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash_val (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641,   30, 1641, 1641,    0,   10,
        15,   20,   25,   30,   35,   40,    5,    0, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641,    0,   87,   30,
       196,  195,  102,    9,   85,   65,   10,  203,    0,   19,
        25,  180,  104,    5,    4,    5,    0,  185,  184,   85,
        35,    5,  120, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641, 1641,
      1641, 1641, 1641, 1641, 1641, 1641
    };
  register int hval = 0;

  switch (len)
    {
      default:
      case 22:
        hval += asso_values[(unsigned char)str[21]];
      case 21:
        hval += asso_values[(unsigned char)str[20]];
      case 20:
        hval += asso_values[(unsigned char)str[19]];
      case 19:
        hval += asso_values[(unsigned char)str[18]];
      case 18:
        hval += asso_values[(unsigned char)str[17]];
      case 17:
        hval += asso_values[(unsigned char)str[16]];
      case 16:
        hval += asso_values[(unsigned char)str[15]];
      case 15:
        hval += asso_values[(unsigned char)str[14]];
      case 14:
        hval += asso_values[(unsigned char)str[13]];
      case 13:
        hval += asso_values[(unsigned char)str[12]];
      case 12:
        hval += asso_values[(unsigned char)str[11]];
      case 11:
        hval += asso_values[(unsigned char)str[10]];
      case 10:
        hval += asso_values[(unsigned char)str[9]];
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
const struct css_value *
findValue (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 252,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 22,
      MIN_HASH_VALUE = 0,
      MAX_HASH_VALUE = 1640
    };

  static const struct css_value wordlist_value[] =
    {
      {"900", CSS_VAL_900},
      {"ltr", CSS_VAL_LTR},
      {"rtl", CSS_VAL_RTL},
      {"800", CSS_VAL_800},
      {"start", CSS_VAL_START},
      {"100", CSS_VAL_100},
      {"200", CSS_VAL_200},
      {"gray", CSS_VAL_GRAY},
      {"300", CSS_VAL_300},
      {"small", CSS_VAL_SMALL},
      {"400", CSS_VAL_400},
      {"500", CSS_VAL_500},
      {"600", CSS_VAL_600},
      {"700", CSS_VAL_700},
      {"x-small", CSS_VAL_X_SMALL},
      {"always", CSS_VAL_ALWAYS},
      {"static", CSS_VAL_STATIC},
      {"fast", CSS_VAL_FAST},
      {"mix", CSS_VAL_MIX},
      {"xx-small", CSS_VAL_XX_SMALL},
      {"fantasy", CSS_VAL_FANTASY},
      {"wait", CSS_VAL_WAIT},
      {"italic", CSS_VAL_ITALIC},
      {"right", CSS_VAL_RIGHT},
      {"thin", CSS_VAL_THIN},
      {"hiragana", CSS_VAL_HIRAGANA},
      {"aqua", CSS_VAL_AQUA},
      {"small-caps", CSS_VAL_SMALL_CAPS},
      {"teal", CSS_VAL_TEAL},
      {"large", CSS_VAL_LARGE},
      {"larger", CSS_VAL_LARGER},
      {"grey", CSS_VAL_GREY},
      {"navy", CSS_VAL_NAVY},
      {"scroll", CSS_VAL_SCROLL},
      {"initial", CSS_VAL_INITIAL},
      {"smaller", CSS_VAL_SMALLER},
      {"cross", CSS_VAL_CROSS},
      {"normal", CSS_VAL_NORMAL},
      {"text", CSS_VAL_TEXT},
      {"graytext", CSS_VAL_GRAYTEXT},
      {"slow", CSS_VAL_SLOW},
      {"x-large", CSS_VAL_X_LARGE},
      {"sub", CSS_VAL_SUB},
      {"lime", CSS_VAL_LIME},
      {"table", CSS_VAL_TABLE},
      {"top", CSS_VAL_TOP},
      {"up", CSS_VAL_UP},
      {"inset", CSS_VAL_INSET},
      {"disc", CSS_VAL_DISC},
      {"left", CSS_VAL_LEFT},
      {"single", CSS_VAL_SINGLE},
      {"icon", CSS_VAL_ICON},
      {"pre", CSS_VAL_PRE},
      {"hand", CSS_VAL_HAND},
      {"xx-large", CSS_VAL_XX_LARGE},
      {"scrollbar", CSS_VAL_SCROLLBAR},
      {"status-bar", CSS_VAL_STATUS_BAR},
      {"crop", CSS_VAL_CROP},
      {"stretch", CSS_VAL_STRETCH},
      {"black", CSS_VAL_BLACK},
      {"circle", CSS_VAL_CIRCLE},
      {"armenian", CSS_VAL_ARMENIAN},
      {"run-in", CSS_VAL_RUN_IN},
      {"both", CSS_VAL_BOTH},
      {"show", CSS_VAL_SHOW},
      {"portrait", CSS_VAL_PORTRAIT},
      {"lighter", CSS_VAL_LIGHTER},
      {"transparent", CSS_VAL_TRANSPARENT},
      {"compact", CSS_VAL_COMPACT},
      {"auto", CSS_VAL_AUTO},
      {"serif", CSS_VAL_SERIF},
      {"justify", CSS_VAL_JUSTIFY},
      {"inline", CSS_VAL_INLINE},
      {"crosshair", CSS_VAL_CROSSHAIR},
      {"list-item", CSS_VAL_LIST_ITEM},
      {"blink", CSS_VAL_BLINK},
      {"thick", CSS_VAL_THICK},
      {"help", CSS_VAL_HELP},
      {"square", CSS_VAL_SQUARE},
      {"red", CSS_VAL_RED},
      {"nowrap", CSS_VAL_NOWRAP},
      {"highlight", CSS_VAL_HIGHLIGHT},
      {"caption", CSS_VAL_CAPTION},
      {"maroon", CSS_VAL_MAROON},
      {"orange", CSS_VAL_ORANGE},
      {"end", CSS_VAL_END},
      {"alternate", CSS_VAL_ALTERNATE},
      {"menu", CSS_VAL_MENU},
      {"none", CSS_VAL_NONE},
      {"green", CSS_VAL_GREEN},
      {"white", CSS_VAL_WHITE},
      {"katakana", CSS_VAL_KATAKANA},
      {"sans-serif", CSS_VAL_SANS_SERIF},
      {"inherit", CSS_VAL_INHERIT},
      {"higher", CSS_VAL_HIGHER},
      {"solid", CSS_VAL_SOLID},
      {"center", CSS_VAL_CENTER},
      {"silver", CSS_VAL_SILVER},
      {"small-caption", CSS_VAL_SMALL_CAPTION},
      {"slide", CSS_VAL_SLIDE},
      {"bold", CSS_VAL_BOLD},
      {"lower", CSS_VAL_LOWER},
      {"yellow", CSS_VAL_YELLOW},
      {"bottom", CSS_VAL_BOTTOM},
      {"blue", CSS_VAL_BLUE},
      {"ridge", CSS_VAL_RIDGE},
      {"fuchsia", CSS_VAL_FUCHSIA},
      {"invert", CSS_VAL_INVERT},
      {"ahead", CSS_VAL_AHEAD},
      {"vertical", CSS_VAL_VERTICAL},
      {"down", CSS_VAL_DOWN},
      {"georgian", CSS_VAL_GEORGIAN},
      {"super", CSS_VAL_SUPER},
      {"narrower", CSS_VAL_NARROWER},
      {"repeat", CSS_VAL_REPEAT},
      {"block", CSS_VAL_BLOCK},
      {"unfurl", CSS_VAL_UNFURL},
      {"separate", CSS_VAL_SEPARATE},
      {"decimal", CSS_VAL_DECIMAL},
      {"inline-axis", CSS_VAL_INLINE_AXIS},
      {"collapse", CSS_VAL_COLLAPSE},
      {"-khtml-right", CSS_VAL__KHTML_RIGHT},
      {"repeat-y", CSS_VAL_REPEAT_Y},
      {"table-cell", CSS_VAL_TABLE_CELL},
      {"hide", CSS_VAL_HIDE},
      {"infinite", CSS_VAL_INFINITE},
      {"text-top", CSS_VAL_TEXT_TOP},
      {"wider", CSS_VAL_WIDER},
      {"below", CSS_VAL_BELOW},
      {"inside", CSS_VAL_INSIDE},
      {"hiragana-iroha", CSS_VAL_HIRAGANA_IROHA},
      {"landscape", CSS_VAL_LANDSCAPE},
      {"loud", CSS_VAL_LOUD},
      {"repeat-x", CSS_VAL_REPEAT_X},
      {"outset", CSS_VAL_OUTSET},
      {"multiple", CSS_VAL_MULTIPLE},
      {"baseline", CSS_VAL_BASELINE},
      {"pointer", CSS_VAL_POINTER},
      {"level", CSS_VAL_LEVEL},
      {"forwards", CSS_VAL_FORWARDS},
      {"move", CSS_VAL_MOVE},
      {"capitalize", CSS_VAL_CAPITALIZE},
      {"table-row", CSS_VAL_TABLE_ROW},
      {"lower-latin", CSS_VAL_LOWER_LATIN},
      {"purple", CSS_VAL_PURPLE},
      {"fixed", CSS_VAL_FIXED},
      {"-khtml-text", CSS_VAL__KHTML_TEXT},
      {"visible", CSS_VAL_VISIBLE},
      {"infotext", CSS_VAL_INFOTEXT},
      {"marquee", CSS_VAL_MARQUEE},
      {"backwards", CSS_VAL_BACKWARDS},
      {"s-resize", CSS_VAL_S_RESIZE},
      {"olive", CSS_VAL_OLIVE},
      {"avoid", CSS_VAL_AVOID},
      {"highlighttext", CSS_VAL_HIGHLIGHTTEXT},
      {"captiontext", CSS_VAL_CAPTIONTEXT},
      {"block-axis", CSS_VAL_BLOCK_AXIS},
      {"window", CSS_VAL_WINDOW},
      {"n-resize", CSS_VAL_N_RESIZE},
      {"relative", CSS_VAL_RELATIVE},
      {"above", CSS_VAL_ABOVE},
      {"hebrew", CSS_VAL_HEBREW},
      {"absolute", CSS_VAL_ABSOLUTE},
      {"menutext", CSS_VAL_MENUTEXT},
      {"horizontal", CSS_VAL_HORIZONTAL},
      {"bolder", CSS_VAL_BOLDER},
      {"-khtml-left", CSS_VAL__KHTML_LEFT},
      {"cursive", CSS_VAL_CURSIVE},
      {"-khtml-box", CSS_VAL__KHTML_BOX},
      {"middle", CSS_VAL_MIDDLE},
      {"dashed", CSS_VAL_DASHED},
      {"default", CSS_VAL_DEFAULT},
      {"medium", CSS_VAL_MEDIUM},
      {"lower-alpha", CSS_VAL_LOWER_ALPHA},
      {"inline-table", CSS_VAL_INLINE_TABLE},
      {"embed", CSS_VAL_EMBED},
      {"lowercase", CSS_VAL_LOWERCASE},
      {"w-resize", CSS_VAL_W_RESIZE},
      {"sw-resize", CSS_VAL_SW_RESIZE},
      {"buttontext", CSS_VAL_BUTTONTEXT},
      {"-khtml-xxx-large", CSS_VAL__KHTML_XXX_LARGE},
      {"upper-latin", CSS_VAL_UPPER_LATIN},
      {"table-caption", CSS_VAL_TABLE_CAPTION},
      {"oblique", CSS_VAL_OBLIQUE},
      {"lower-roman", CSS_VAL_LOWER_ROMAN},
      {"nw-resize", CSS_VAL_NW_RESIZE},
      {"text-bottom", CSS_VAL_TEXT_BOTTOM},
      {"-khtml-auto", CSS_VAL__KHTML_AUTO},
      {"no-repeat", CSS_VAL_NO_REPEAT},
      {"monospace", CSS_VAL_MONOSPACE},
      {"table-column", CSS_VAL_TABLE_COLUMN},
      {"groove", CSS_VAL_GROOVE},
      {"message-box", CSS_VAL_MESSAGE_BOX},
      {"hidden", CSS_VAL_HIDDEN},
      {"-khtml-nowrap", CSS_VAL__KHTML_NOWRAP},
      {"dotted", CSS_VAL_DOTTED},
      {"reverse", CSS_VAL_REVERSE},
      {"katakana-iroha", CSS_VAL_KATAKANA_IROHA},
      {"buttonface", CSS_VAL_BUTTONFACE},
      {"e-resize", CSS_VAL_E_RESIZE},
      {"upper-alpha", CSS_VAL_UPPER_ALPHA},
      {"se-resize", CSS_VAL_SE_RESIZE},
      {"-khtml-center", CSS_VAL__KHTML_CENTER},
      {"uppercase", CSS_VAL_UPPERCASE},
      {"outside", CSS_VAL_OUTSIDE},
      {"ne-resize", CSS_VAL_NE_RESIZE},
      {"-khtml-body", CSS_VAL__KHTML_BODY},
      {"double", CSS_VAL_DOUBLE},
      {"overline", CSS_VAL_OVERLINE},
      {"upper-roman", CSS_VAL_UPPER_ROMAN},
      {"line-through", CSS_VAL_LINE_THROUGH},
      {"windowtext", CSS_VAL_WINDOWTEXT},
      {"activecaption", CSS_VAL_ACTIVECAPTION},
      {"buttonhighlight", CSS_VAL_BUTTONHIGHLIGHT},
      {"underline", CSS_VAL_UNDERLINE},
      {"inline-block", CSS_VAL_INLINE_BLOCK},
      {"background", CSS_VAL_BACKGROUND},
      {"expanded", CSS_VAL_EXPANDED},
      {"windowframe", CSS_VAL_WINDOWFRAME},
      {"inactivecaption", CSS_VAL_INACTIVECAPTION},
      {"threedface", CSS_VAL_THREEDFACE},
      {"close-quote", CSS_VAL_CLOSE_QUOTE},
      {"appworkspace", CSS_VAL_APPWORKSPACE},
      {"buttonshadow", CSS_VAL_BUTTONSHADOW},
      {"condensed", CSS_VAL_CONDENSED},
      {"-khtml-inline-box", CSS_VAL__KHTML_INLINE_BOX},
      {"threedhighlight", CSS_VAL_THREEDHIGHLIGHT},
      {"table-row-group", CSS_VAL_TABLE_ROW_GROUP},
      {"open-quote", CSS_VAL_OPEN_QUOTE},
      {"lower-greek", CSS_VAL_LOWER_GREEK},
      {"activeborder", CSS_VAL_ACTIVEBORDER},
      {"ultra-expanded", CSS_VAL_ULTRA_EXPANDED},
      {"inactivecaptiontext", CSS_VAL_INACTIVECAPTIONTEXT},
      {"cjk-ideographic", CSS_VAL_CJK_IDEOGRAPHIC},
      {"extra-expanded", CSS_VAL_EXTRA_EXPANDED},
      {"threedshadow", CSS_VAL_THREEDSHADOW},
      {"inactiveborder", CSS_VAL_INACTIVEBORDER},
      {"no-close-quote", CSS_VAL_NO_CLOSE_QUOTE},
      {"semi-expanded", CSS_VAL_SEMI_EXPANDED},
      {"table-column-group", CSS_VAL_TABLE_COLUMN_GROUP},
      {"ultra-condensed", CSS_VAL_ULTRA_CONDENSED},
      {"infobackground", CSS_VAL_INFOBACKGROUND},
      {"extra-condensed", CSS_VAL_EXTRA_CONDENSED},
      {"no-open-quote", CSS_VAL_NO_OPEN_QUOTE},
      {"semi-condensed", CSS_VAL_SEMI_CONDENSED},
      {"threedlightshadow", CSS_VAL_THREEDLIGHTSHADOW},
      {"bidi-override", CSS_VAL_BIDI_OVERRIDE},
      {"table-footer-group", CSS_VAL_TABLE_FOOTER_GROUP},
      {"table-header-group", CSS_VAL_TABLE_HEADER_GROUP},
      {"decimal-leading-zero", CSS_VAL_DECIMAL_LEADING_ZERO},
      {"threeddarkshadow", CSS_VAL_THREEDDARKSHADOW},
      {"-khtml-baseline-middle", CSS_VAL__KHTML_BASELINE_MIDDLE}
    };

  static const short lookup[] =
    {
         0,   -1,   -1,   -1, -259,    3, -251,   -2,
        -1,    4,    5,   -1,   -1,   -1,   -1,    6,
        -1,   -1,    7,   -1,    8,   -1,   -1,   -1,
         9,   10,   -1,   -1,   -1,   -1,   11,   -1,
        -1,   -1,   -1,   12,   -1,   -1,   -1,   -1,
        13,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   14,   -1,   -1,   -1,   -1,   -1,   15,
        -1,   -1,   -1,   -1,   16,   -1,   -1,   -1,
        -1,   -1,   -1,   17,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   18,
        -1,   -1,   -1,   -1,   19,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   20,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   21,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        22,   -1,   -1,   23,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   24,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   25,   -1,   26,   -1,
        -1,   27,   -1,   28,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        29,   -1,   -1,   -1,   30,   31,   32,   -1,
        -1,   -1,   -1,   33,   34,   -1,   -1,   35,
        36,   -1,   -1,   -1,   37,   -1,   38,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        39,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   40,   -1,
        -1,   41,   -1,   -1,   -1,   42,   -1,   43,
        -1,   -1,   44,   -1,   45,   -1,   -1,   -1,
        -1,   46,   47,   -1,   -1,   -1,   -1,   -1,
        48,   49,   -1,   50,   51,   -1,   -1,   52,
        -1,   -1,   53,   -1,   54,   -1,   55,   -1,
        -1,   -1,   -1,   -1,   56,   -1,   57,   58,
        59,   -1,   -1,   -1,   60,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   61,   62,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        63,   -1,   -1,   64,   -1,   65,   66,   -1,
        -1,   -1,   67,   68,   -1,   69,   -1,   -1,
        -1,   -1,   -1,   70,   71,   -1,   -1,   72,
        -1,   -1,   73,   74,   75,   -1,   -1,   76,
        77,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   78,   79,   -1,   -1,   80,   -1,
        -1,   -1,   -1,   81,   82,   -1,   -1,   -1,
        83,   -1,   -1,   -1,   -1,   84,   -1,   -1,
        85,   -1,   -1,   86,   -1,   -1,   -1,   -1,
        87,   88,   -1,   -1,   89,   -1,   90,   91,
        -1,   -1,   -1,   -1,   92,   -1,   -1,   93,
        -1,   -1,   -1,   94,   -1,   -1,   95,   -1,
        -1,   96,   -1,   -1,   -1,   97,   -1,   -1,
        -1,   -1,   98,   -1,   -1,   99,   -1,  100,
       101,  102,  103,  104,   -1,  105,   -1,   -1,
       106,  107,   -1,   -1,  108,   -1,  109,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  110,  111,
        -1,   -1,   -1,   -1,   -1,  112,   -1,   -1,
        -1,  113,  114,   -1,  115,  116,   -1,  117,
        -1,  118,   -1,   -1,   -1,   -1,  119,   -1,
        -1,   -1,  120,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  121,   -1,   -1,  122,   -1,   -1,
        -1,  123,   -1,   -1,   -1,  124,  125,   -1,
       126,  127,   -1,  128,   -1,   -1,   -1,  129,
       130,   -1,   -1,  131,   -1,   -1,   -1,   -1,
        -1,  132,   -1,  133,   -1,  134,   -1,   -1,
       135,   -1,   -1,   -1,  136,  137,  138,   -1,
       139,   -1,  140,  141,   -1,  142,   -1,   -1,
       143,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       144,  145,   -1,   -1,   -1,  146,   -1,   -1,
        -1,  147,  148,  149,   -1,   -1,   -1,   -1,
        -1,   -1,  150,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  151,   -1,   -1,   -1,   -1,
       152,  153,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  154,  155,  156,  157,   -1,   -1,  158,
        -1,   -1,   -1,  159,   -1,   -1,  160,   -1,
        -1,   -1,   -1,  161,  162,   -1,  163,   -1,
        -1,   -1,   -1,  164,   -1,   -1,  165,   -1,
       166,   -1,   -1,   -1,  167,  168,   -1,  169,
        -1,   -1,   -1,   -1,   -1,  170,  171,  172,
        -1,   -1,   -1,  173,   -1,   -1,   -1,  174,
        -1,   -1,   -1,   -1,  175,   -1,  176,   -1,
        -1,   -1,   -1,  177,   -1,   -1,   -1,   -1,
       178,   -1,   -1,  179,   -1,   -1,  180,   -1,
       181,   -1,   -1,   -1,  182,  183,   -1,   -1,
        -1,   -1,  184,   -1,  185,   -1,  186,   -1,
        -1,   -1,   -1,   -1,  187,  188,   -1,   -1,
        -1,   -1,  189,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  190,
       191,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       192,   -1,  193,   -1,   -1,  194,   -1,  195,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  196,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  197,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  198,   -1,   -1,   -1,
        -1,  199,   -1,  200,   -1,   -1,  201,   -1,
       202,   -1,   -1,   -1,   -1,   -1,  203,   -1,
        -1,   -1,  204,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  205,  206,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  207,   -1,   -1,   -1,   -1,
       208,   -1,  209,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  210,
        -1,   -1,  211,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  212,   -1,
       213,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  214,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  215,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  216,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  217,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  218,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       219,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  220,   -1,   -1,  221,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  222,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  223,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  224,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  225,   -1,   -1,   -1,  226,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  227,   -1,   -1,
        -1,   -1,   -1,  228,  229,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  230,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  231,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  232,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  233,   -1,
        -1,   -1,  234,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  235,   -1,   -1,   -1,  236,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       237,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  238,   -1,   -1,  239,
        -1,   -1,  240,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  241,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  242,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  243,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  244,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  245,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  246,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  247,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  248,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  249,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  250,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       251
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash_val (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist_value[index].name;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return &wordlist_value[index];
            }
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register const struct css_value *wordptr = &wordlist_value[TOTAL_KEYWORDS + lookup[offset]];
              register const struct css_value *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  register const char *s = wordptr->name;

                  if (*str == *s && !strcmp (str + 1, s + 1))
                    return wordptr;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}
static const char * const valueList[] = {
"",
"inherit", 
"initial", 
"none", 
"hidden", 
"inset", 
"groove", 
"ridge", 
"outset", 
"dotted", 
"dashed", 
"solid", 
"double", 
"caption", 
"icon", 
"menu", 
"message-box", 
"small-caption", 
"status-bar", 
"italic", 
"oblique", 
"small-caps", 
"normal", 
"bold", 
"bolder", 
"lighter", 
"100", 
"200", 
"300", 
"400", 
"500", 
"600", 
"700", 
"800", 
"900", 
"xx-small", 
"x-small", 
"small", 
"medium", 
"large", 
"x-large", 
"xx-large", 
"-khtml-xxx-large", 
"smaller", 
"larger", 
"wider", 
"narrower", 
"ultra-condensed", 
"extra-condensed", 
"condensed", 
"semi-condensed", 
"semi-expanded", 
"expanded", 
"extra-expanded", 
"ultra-expanded", 
"serif", 
"sans-serif", 
"cursive", 
"fantasy", 
"monospace", 
"-khtml-body", 
"aqua", 
"black", 
"blue", 
"fuchsia", 
"gray", 
"green", 
"lime", 
"maroon", 
"navy", 
"olive", 
"orange", 
"purple", 
"red", 
"silver", 
"teal", 
"white", 
"yellow", 
"transparent", 
"activeborder", 
"activecaption", 
"appworkspace", 
"background", 
"buttonface", 
"buttonhighlight", 
"buttonshadow", 
"buttontext", 
"captiontext", 
"graytext", 
"highlight", 
"highlighttext", 
"inactiveborder", 
"inactivecaption", 
"inactivecaptiontext", 
"infobackground", 
"infotext", 
"menutext", 
"scrollbar", 
"threeddarkshadow", 
"threedface", 
"threedhighlight", 
"threedlightshadow", 
"threedshadow", 
"window", 
"windowframe", 
"windowtext", 
"grey", 
"-khtml-text", 
"repeat", 
"repeat-x", 
"repeat-y", 
"no-repeat", 
"baseline", 
"middle", 
"sub", 
"super", 
"text-top", 
"text-bottom", 
"top", 
"bottom", 
"-khtml-baseline-middle", 
"-khtml-auto", 
"left", 
"right", 
"center", 
"justify", 
"-khtml-left", 
"-khtml-right", 
"-khtml-center", 
"outside", 
"inside", 
"disc", 
"circle", 
"square", 
"decimal", 
"decimal-leading-zero", 
"lower-roman", 
"upper-roman", 
"lower-greek", 
"lower-alpha", 
"lower-latin", 
"upper-alpha", 
"upper-latin", 
"hebrew", 
"armenian", 
"georgian", 
"cjk-ideographic", 
"hiragana", 
"katakana", 
"hiragana-iroha", 
"katakana-iroha", 
"inline", 
"block", 
"list-item", 
"run-in", 
"compact", 
"inline-block", 
"table", 
"inline-table", 
"table-row-group", 
"table-header-group", 
"table-footer-group", 
"table-row", 
"table-column-group", 
"table-column", 
"table-cell", 
"table-caption", 
"-khtml-box", 
"-khtml-inline-box", 
"auto", 
"crosshair", 
"default", 
"pointer", 
"move", 
"e-resize", 
"ne-resize", 
"nw-resize", 
"n-resize", 
"se-resize", 
"sw-resize", 
"s-resize", 
"w-resize", 
"text", 
"wait", 
"help", 
"ltr", 
"rtl", 
"capitalize", 
"uppercase", 
"lowercase", 
"visible", 
"collapse", 
"above", 
"absolute", 
"always", 
"avoid", 
"below", 
"bidi-override", 
"blink", 
"both", 
"close-quote", 
"crop", 
"cross", 
"embed", 
"fixed", 
"hand", 
"hide", 
"higher", 
"invert", 
"landscape", 
"level", 
"line-through", 
"loud", 
"lower", 
"marquee", 
"mix", 
"no-close-quote", 
"no-open-quote", 
"nowrap", 
"open-quote", 
"overline", 
"portrait", 
"pre", 
"relative", 
"scroll", 
"separate", 
"show", 
"static", 
"thick", 
"thin", 
"underline", 
"-khtml-nowrap", 
"stretch", 
"start", 
"end", 
"reverse", 
"horizontal", 
"vertical", 
"inline-axis", 
"block-axis", 
"single", 
"multiple", 
"forwards", 
"backwards", 
"ahead", 
"up", 
"down", 
"slow", 
"fast", 
"infinite", 
"slide", 
"alternate", 
"unfurl", 
    0
};
DOMString getValueName(unsigned short id)
{
    if(id >= CSS_VAL_TOTAL || id == 0)
      return DOMString();
    else
      return DOMString(valueList[id]);
};

