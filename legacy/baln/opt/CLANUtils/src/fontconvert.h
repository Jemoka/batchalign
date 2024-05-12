/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#ifndef FONTCONVERTDEF
#define FONTCONVERTDEF

#include "wstring.h"

extern "C"
{
#ifndef EOS			/* End Of String marker			 */
	#define EOS 0 // '\0'
#endif

#define NOCHANGE	0
#define UNIXDATA	1
#define DOSDATA		2
#define MACDATA		3
#define WIN95DATA	4
#define UNICODEDATA	5

#define MONACO		10
#define COURIER		11
#define DOS850		12
#define DOS860		13
#define WIN95cour	14
#define WIN95ipa	15
#define MACipa		16
#define WIN95cain	17
#define MACcain		18
#define MACjpn		19
#define WINjpn		20
#define MACchn_B5	21
#define MACchn_GB	22
#define MACchn		23
#define WINchn_B5	24
#define WINchn_GB	25
#define WINchn		26
#define MACGenevaCE	27
#define WIN95courCE	28
#define UNICYRIL	29
#define UNIIPAPH	30
#define MACKor_HUNG	31
#define WINKor_HUNG	32
#define MACArialUC	33
#define WINArialUC	34
#define MACThai		35
#define WINThai		36
#define WIN95Latin5	37
#define MacLatin5	38
#define WINFxdsys	39
#define MacFxdsys	40
#define MacCAFont	41
#define WINCAFont	42

typedef struct {
	char FName[256];
	long FSize;
	short FHeight;
	int   CharSet;
	short Encod;
} FONTINFO;

#define FONTARGS char *c, long *index, char *st, long *cnt, NewFontInfo *finfo
struct NewFontInfo {
	char  fontName[256];
	const char  *fontPref;
	long  fontSize;
	int   CharSet;
	short fontId;
	short charHeight;
	short platform;
	short fontType;
	short orgFType;
	char  isUTF;
	short Encod;
	short orgEncod;
	long  (*fontTable)(FONTARGS);
} ;
typedef struct NewFontInfo NewFontInfo;

#define UTF8_IS_SINGLE(uchar) (((uchar)&0x80)==0)
#define UTF8_IS_LEAD(uchar) ((unsigned char)((uchar)-0xc0)<0x3e)
#define UTF8_IS_TRAIL(uchar) (((uchar)&0xc0)==0x80)

extern const char *GetDatasFont(const char *line, char ec, NewFontInfo *cfinfo, char MBC, NewFontInfo *finfo);
extern char GetFontNumber(char *fname, short *font);
extern char FindTTable(NewFontInfo *finfo, short thisPlatform);
extern short my_CharacterByteType(const char *org, short pos, NewFontInfo *finfo);
extern short getFontType(const char *fontName, char isPC);
extern short GetEncode(const char *, const char *, short , int , char );

}
#endif /* FONTCONVERTDEF */
