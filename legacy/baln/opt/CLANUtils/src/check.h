/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#ifndef CHECKDEF
#define CHECKDEF

extern "C"
{

#define CHECK_MORPHS "-#~"

#define NUMSPCHARS 54 // up-arrow

#define isPostCodeMark(x,y) ((x == '+' || x == '-') && y == ' ')

#define DEPFILE   "depfile.cut"
//#define DEPADD    "00depadd.cut" // 16-10-02
#define UTTDELSYM "[UTD]"
#define NUTTDELSYM "[-UTD]"
#define SUFFIXSYM "[AFX]" // is affix matches don't check the rest, if no affix -> do normal check
#define UTTIGNORE "[IGN]"
#define WORDCKECKSYM "[NOWORDCHECK]"
#define UPREFSSYM "[UPREFS *]"
#define DEPENDENT "@Dependent"
#define AGEOF     "@Age of *"
#define SEXOF	  "@Sex of *"
#define SESOF	  "@Ses of *"
#define BIRTHOF   "@Birth of *"
#define EDUCOF    "@Education of *"
#define GROUPOF   "@Group of *"
#define IDOF      "@ID"
#define DATEOF	  "@Date"
#define PARTICIPANTS "@Participants"

#define ALNUM 0x1
#define ALPHA 0x2
#define DIGIT 0x3
#define OR_CH 0x4
#define PARAO 0x5
#define PARAC 0x6
#define ONE_T 0x7
#define ZEROT 0x8

enum { // CA CHARS
	CA_NOT_ALL = 0,
	CA_APPLY_SHIFT_TO_HIGH_PITCH,	// up-arrow ↑ 0x2191
	CA_APPLY_SHIFT_TO_LOW_PITCH,	// down-arrow ↓ 0x2193
	CA_APPLY_RISETOHIGH,			// rise to high ⇗ 0x21D7
	CA_APPLY_RISETOMID,				// rise to mid ↗ 0x2197
	CA_APPLY_LEVEL,					// level → 0x2192
	CA_APPLY_FALLTOMID,				// fall to mid ↘ 0x2198
	CA_APPLY_FALLTOLOW,				// fall to low ⇘ 0x21D8
	CA_APPLY_UNMARKEDENDING,		// unmarked ending ∞ 0x221E
	CA_APPLY_CONTINUATION,			// continuation ≋ 0x224B
	CA_APPLY_INHALATION,			// inhalation ∙ 0x2219
	CA_APPLY_LATCHING,				// latching ≈ 0x2248
	CA_APPLY_UPTAKE,				// uptake ≡ 0x2261
	CA_APPLY_OPTOPSQ,				// raised ⌈ 0x2308
	CA_APPLY_OPBOTSQ,				// lowered ⌊ 0x230A
	CA_APPLY_CLTOPSQ,				// raised ⌉ 0x2309
	CA_APPLY_CLBOTSQ,				// lowered ⌋ 0x230B
	CA_APPLY_FASTER,				// faster ∆ 0x2206
	CA_APPLY_SLOWER,				// slower ∇ 0x2207
	CA_APPLY_CREAKY,				// creaky ⁎ 0x204E
	CA_APPLY_UNSURE,				// unsure ⁇ 0x2047
	CA_APPLY_SOFTER,				// softer ° 0x00B0
	CA_APPLY_LOUDER,				// louder ◉ 0x25C9
	CA_APPLY_LOW_PITCH,				// low pitch  ▁ 0x2581
	CA_APPLY_HIGH_PITCH,			// high pitch ▔ 0x2594
	CA_APPLY_SMILE_VOICE,			// smile voice ☺ 0x263A
	CA_BREATHY_VOICE,				// breathy-voice ♋ 0x264b
	CA_APPLY_WHISPER,				// whisper ∬ 0x222C
	CA_APPLY_YAWN,					// yawn Ϋ 0x03AB
	CA_APPLY_SINGING,				// singing ∮ 0x222E
	CA_APPLY_PRECISE,				// precise § 0x00A7
	CA_APPLY_CONSTRICTION,			// constriction ∾ 0x223E
	CA_APPLY_PITCH_RESET,			// pitch reset ↻ 0x21BB
	CA_APPLY_LAUGHINWORD,			// laugh in a word Ἡ 0x1F29
	NOTCA_VOCATIVE,					// Vocative or summons - ‡ 0x2021
	NOTCA_ARABIC_DOT_DIACRITIC,		// Arabic dot diacritic - ạ 0x0323
	NOTCA_ARABIC_RAISED,			// Arabic raised - ʰ 0x02B0
	NOTCA_STRESS,					// Stress - ̄ 0x0304
	NOTCA_GLOTTAL_STOP,				// Glottal stop - ʔ 0x0294
	NOTCA_HEBREW_GLOTTAL,			// Hebrew glottal - ʕ 0x0295
	NOTCA_CARON,					// caron -  ̌ 0x030C
	NOTCA_VOWELS_COLON,				// colon for long vowels - ː 0x02D0
	NOTCA_GROUP_START,				// Group start marker - ‹ 0x2039
	NOTCA_GROUP_END,				// Group end marker - › 0x203A
	NOTCA_DOUBLE_COMMA,				// Tag or sentence final particle - „ 0x201E
	NOTCA_RAISED_STROKE,			// raised stroke - ˈ 0x02C8
	NOTCA_LOWERED_STROKE,			// lowered stroke - ˌ 0x02CC
	NOTCA_SIGN_GROUP_START,			// sign group start marker -〔 0x3014
	NOTCA_SIGN_GROUP_END,			// sign group end marker - 〕0x3015
	NOTCA_UNDERLINE,				// underline - s̲̲s̲ 0x0332
	NOTCA_MISSINGWORD,				// %pho missing word - … 0x2026
	NOTCA_OPEN_QUOTE,				// open quote “ 0x201C
	NOTCA_CLOSE_QUOTE,				// close quote ” 0x201D
	NOTCA_CROSSED_EQUAL,			// crossed equal - ≠ 0x2260
	NOTCA_LEFT_ARROW_CIRCLE		// left arrow with circle - ↫ 0x21AB
} ;

extern const char *lHeaders[];

}

#endif /* CHECKDEF */
