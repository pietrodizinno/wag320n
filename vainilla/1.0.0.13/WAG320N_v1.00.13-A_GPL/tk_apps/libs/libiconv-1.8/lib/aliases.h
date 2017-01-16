/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -t -L ANSI-C -H aliases_hash -N aliases_lookup -G -W aliases -7 -C -k '1,3-11,$' -i 1 aliases.gperf  */
struct alias { const char* name; unsigned int encoding_index; };

#define TOTAL_KEYWORDS 103
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 20
#define MIN_HASH_VALUE 4
#define MAX_HASH_VALUE 425
/* maximum key range = 422, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
aliases_hash (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      426, 426, 426, 426, 426, 426, 426, 426, 426, 426,
      426, 426, 426, 426, 426, 426, 426, 426, 426, 426,
      426, 426, 426, 426, 426, 426, 426, 426, 426, 426,
      426, 426, 426, 426, 426, 426, 426, 426, 426, 426,
      426, 426, 426, 426, 426,   1,   1, 426,  66,   1,
       86,  31,  16,   1,   6,   6,  36,   1, 426, 426,
      426, 426, 426, 426, 426,   6,  51,   1,  46,   1,
       51,  11,  16,   1,   1,   1,  91,  56,  21,   1,
       21, 426,   1,   1,   1,   1,   1,   1,   6, 426,
      426, 426, 426, 426, 426,  31, 426, 426, 426, 426,
      426, 426, 426, 426, 426, 426, 426, 426, 426, 426,
      426, 426, 426, 426, 426, 426, 426, 426, 426, 426,
      426, 426, 426, 426, 426, 426, 426, 426
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 11:
        hval += asso_values[str[10]];
      case 10:
        hval += asso_values[str[9]];
      case 9:
        hval += asso_values[str[8]];
      case 8:
        hval += asso_values[str[7]];
      case 7:
        hval += asso_values[str[6]];
      case 6:
        hval += asso_values[str[5]];
      case 5:
        hval += asso_values[str[4]];
      case 4:
        hval += asso_values[str[3]];
      case 3:
        hval += asso_values[str[2]];
      case 2:
      case 1:
        hval += asso_values[str[0]];
        break;
    }
  return hval + asso_values[str[len - 1]];
}

static const struct alias aliases[] =
  {
    {""}, {""}, {""}, {""},
    {"US", ei_ascii},
    {""},
    {"UHC", ei_cp949},
    {""},
    {"SJIS", ei_sjis},
    {""}, {""}, {""}, {""},
    {"CHAR", ei_local_char},
    {""},
    {"ASCII", ei_ascii},
    {"GBK", ei_ces_gbk},
    {""}, {""},
    {"CSASCII", ei_ascii},
    {"ISO-IR-159", ei_jisx0212},
    {"US-ASCII", ei_ascii},
    {""}, {""}, {""},
    {"CP949", ei_cp949},
    {"ISO-IR-6", ei_ascii},
    {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {"CHINESE", ei_gb2312},
    {"ISO-IR-149", ei_ksc5601},
    {""}, {""}, {""}, {""},
    {"UCS-4", ei_ucs4},
    {""},
    {"CSUCS4", ei_ucs4},
    {"ISO646-US", ei_ascii},
    {""}, {""}, {""}, {""}, {""}, {""},
    {"CP936", ei_ces_gbk},
    {""}, {""}, {""}, {""},
    {"CP367", ei_ascii},
    {""},
    {"KOREAN", ei_ksc5601},
    {""}, {""}, {""}, {""}, {""},
    {"ISO-IR-87", ei_jisx0208},
    {"WCHAR_T", ei_local_wchar_t},
    {"MS-EE", ei_cp1250},
    {""},
    {"UNICODE-1-1", ei_ucs2be},
    {"SHIFT-JIS", ei_sjis},
    {"MS-TURK", ei_cp1254},
    {""}, {""},
    {"UTF-16", ei_utf16},
    {""}, {""}, {""}, {""}, {""}, {""},
    {"UCS-4BE", ei_ucs4be},
    {""},
    {"MS-GREEK", ei_cp1253},
    {"ISO_646.IRV:1991", ei_ascii},
    {"CSUNICODE", ei_ucs2},
    {""},
    {"CSSHIFTJIS", ei_sjis},
    {""},
    {"CSUNICODE11", ei_ucs2be},
    {"ISO-IR-58", ei_gb2312},
    {""}, {""}, {""},
    {"ISO-8859-15", ei_iso8859_15},
    {""},
    {"MS-ANSI", ei_cp1252},
    {""},
    {"CSISO159JISX02121990", ei_jisx0212},
    {"CP1251", ei_cp1251},
    {"SHIFT_JIS", ei_sjis},
    {"CSKSC56011987", ei_ksc5601},
    {""}, {""}, {""}, {""}, {""}, {""},
    {"CSISO87JISX0208", ei_jisx0208},
    {"CP1256", ei_cp1256},
    {""}, {""}, {""}, {""},
    {"IBM367", ei_ascii},
    {""}, {""},
    {"ANSI_X3.4-1986", ei_ascii},
    {"KSC_5601", ei_ksc5601},
    {"WINDOWS-31J", ei_cp932},
    {""},
    {"UCS-4LE", ei_ucs4le},
    {""},
    {"UTF-16BE", ei_utf16be},
    {"ISO_8859-15", ei_iso8859_15},
    {""}, {""}, {""},
    {"MS_KANJI", ei_sjis},
    {"CP1254", ei_cp1254},
    {""},
    {"UCS-4-SWAPPED", ei_ucs4swapped},
    {"UTF-8", ei_utf8},
    {"ISO-10646-UCS-4", ei_ucs4},
    {""}, {""}, {""},
    {"UNICODEBIG", ei_ucs2be},
    {""}, {""}, {""}, {""},
    {"CP950", ei_cp950},
    {""}, {""}, {""}, {""},
    {"ANSI_X3.4-1968", ei_ascii},
    {""}, {""}, {""},
    {"UCS-2BE", ei_ucs2be},
    {"UCS-4-INTERNAL", ei_ucs4internal},
    {""}, {""}, {""}, {""},
    {"KS_C_5601-1989", ei_ksc5601},
    {""},
    {"CP1253", ei_cp1253},
    {""},
    {"UNICODELITTLE", ei_ucs2le},
    {"KS_C_5601-1987", ei_ksc5601},
    {"UTF-16LE", ei_utf16le},
    {"ISO_8859-15:1998", ei_iso8859_15},
    {""}, {""},
    {"X0201", ei_jisx0201},
    {""}, {""}, {""}, {""}, {""},
    {"850", ei_cp850},
    {""},
    {"WINDOWS-1251", ei_cp1251},
    {""},
    {"CP850", ei_cp850},
    {""}, {""},
    {"WINDOWS-1256", ei_cp1256},
    {"MS-ARAB", ei_cp1256},
    {"UCS-2", ei_ucs2},
    {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {"WINDOWS-1254", ei_cp1254},
    {"UCS-2LE", ei_ucs2le},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {"UCS-2-SWAPPED", ei_ucs2swapped},
    {""},
    {"ISO-10646-UCS-2", ei_ucs2},
    {""},
    {"WINDOWS-1253", ei_cp1253},
    {""}, {""}, {""}, {""}, {""}, {""},
    {"CP932", ei_cp932},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {"UCS-2-INTERNAL", ei_ucs2internal},
    {""}, {""}, {""}, {""}, {""}, {""},
    {"CP1250", ei_cp1250},
    {""}, {""},
    {"ISO-IR-203", ei_iso8859_15},
    {"UTF-32BE", ei_utf32be},
    {"IBM850", ei_cp850},
    {""}, {""},
    {"X0208", ei_jisx0208},
    {""}, {""},
    {"WINDOWS-1250", ei_cp1250},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""},
    {"JISX0201-1976", ei_jisx0201},
    {""}, {""}, {""}, {""}, {""},
    {"CSHALFWIDTHKATAKANA", ei_jisx0201},
    {""}, {""},
    {"WINDOWS-1252", ei_cp1252},
    {""}, {""}, {""},
    {"UTF-32", ei_utf32},
    {""}, {""},
    {"JIS_C6226-1983", ei_jisx0208},
    {""},
    {"CP1252", ei_cp1252},
    {"JIS_X0201", ei_jisx0201},
    {""},
    {"X0212", ei_jisx0212},
    {"UTF-32LE", ei_utf32le},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {"JIS0208", ei_jisx0208},
    {""},
    {"CSISO58GB231280", ei_gb2312},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {"JIS_X0208", ei_jisx0208},
    {""},
    {"JIS_X0208-1983", ei_jisx0208},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""},
    {"JIS_X0212-1990", ei_jisx0212},
    {""},
    {"JIS_X0212.1990-0", ei_jisx0212},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""},
    {"JIS_X0212", ei_jisx0212},
    {""},
    {"JIS_X0208-1990", ei_jisx0208},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {"CSPC850MULTILINGUAL", ei_cp850},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {""}, {""}, {""},
    {"GB_2312-80", ei_gb2312}
  };

#ifdef __GNUC__
__inline
#endif
const struct alias *
aliases_lookup (register const char *str, register unsigned int len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = aliases_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = aliases[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &aliases[key];
        }
    }
  return 0;
}
