/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#include "common.h"
#include <ctype.h>
#include "c_clan.h"

#include "cu.h"
#include "fontconvert.h"
#include "stringparser.h"


extern "C" {
	char defUniFontName[256];
	long defUniFontSize;
	long stickyFontSize;
	short ArialUnicodeFOND;
	short SecondDefUniFOND;
}

#define kTextEncodingMacThai 21
#define kTextEncodingWindowsLatin5 0x0504

/************************************************************/
/*	 Find and set the current font of the tier			*/
short GetEncode(const char  *fontPref, const char *fontName, short fontType, int CharSet, char isShowErr) {
	return(0);
}

short getFontType(const char *fontName, char isPC) {
	short type;

	if (!uS.mStricmp(fontName, "ARIAL UNICODE MS") || !uS.mStricmp(fontName, "ARIAL")) {
		type = ((isPC) ? WINArialUC : MACArialUC);
	} else if (!uS.mStricmp(fontName, "CAFONT")) {
		type = ((isPC) ? WINCAFont : MacCAFont);
	} else if (!uS.mStrnicmp(fontName, "FIXEDSYS EXCELSIOR", 18)) {
		type = ((isPC) ? WINFxdsys : MacFxdsys);
	} else {
		type = UNICODEDATA;
	}

	return(type);
}

static void	IDFont(char *tFontName, char isUTF, NewFontInfo *finfo) {
	finfo->fontTable = NULL;
	if (finfo->platform == WIN95DATA) {
		finfo->fontPref = "Win95:";
		if (!strcmp(tFontName, "ARIAL UNICODE MS") || !strcmp(tFontName, "ARIAL"))
			finfo->fontType = WINArialUC;
		else if (!strcmp(tFontName, "CAFONT"/*UNICODEFONT*/))
			finfo->fontType = WINCAFont;
		else if ((finfo->CharSet == 238) &&
			  (!strcmp(tFontName, "COURIER NEW") || !strcmp(tFontName, "ARIAL") ||
			   !strcmp(tFontName, "COURIER NEW CE") || !strcmp(tFontName, "ARIAL CE") ||
			   !strcmp(tFontName, "TIMES NEW ROMAN") || !strcmp(tFontName, "ARIAL UNICODE MS")))
			finfo->fontType = WIN95courCE;
		else if (finfo->CharSet == 222)
			finfo->fontType = WINThai;
		else if ((finfo->CharSet == 162) &&
			  (!strcmp(tFontName, "COURIER NEW") || !strcmp(tFontName, "ARIAL") ||
			   !strcmp(tFontName, "TIMES NEW ROMAN") || !strcmp(tFontName, "ARIAL UNICODE MS")))
			finfo->fontType = WIN95Latin5;
		else if (finfo->CharSet == 134)
			finfo->fontType = WINchn_GB;
		else if (finfo->CharSet == 136)
			finfo->fontType = WINchn_B5;
		else if (finfo->CharSet == 128)
			finfo->fontType = WINjpn;
		else if (finfo->CharSet == 129)
			finfo->fontType = WINKor_HUNG;
		else if (finfo->CharSet == -2 || (finfo->CharSet >= 0 && finfo->CharSet <= 2)) {
			if (!strcmp(tFontName, "COURIER") || !strcmp(tFontName, "COURIER NEW"))
				finfo->fontType = WIN95cour;
			else if (!strcmp(tFontName, "COURIER NEW CE") || !strcmp(tFontName, "ARIAL CE"))
				finfo->fontType = WIN95courCE;
			else if (!strcmp(tFontName, "IPAPHON"))
				finfo->fontType = WIN95ipa;
			else if (!strncmp(tFontName, "FIXEDSYS EXCELSIOR", 18))
				finfo->fontType = WINFxdsys;
			else if (!strcmp(tFontName, "CAFONT"))
				finfo->fontType = WINCAFont;
			else if (!strcmp(tFontName, "CA"))
				finfo->fontType = WIN95cain;
			else if (!strncmp(tFontName, "CHN ", 4))
				finfo->fontType = WINchn;
			else if (!strncmp(tFontName, "JPN ", 4))
				finfo->fontType = WINjpn;
			else
				finfo->fontType = NOCHANGE;
		} else
			finfo->fontType = NOCHANGE;
	} else if (!strcmp(tFontName, "REALDOSCODEPAGE")) {
		finfo->fontPref = "";
		finfo->platform = DOSDATA;
		if (finfo->fontSize == 850) finfo->fontType = DOS850;
		else if (finfo->fontSize == 860) finfo->fontType = DOS860;
		else if (finfo->fontSize == 437) finfo->fontType = DOS850;
		else finfo->fontType = NOCHANGE;
	} else if (!strcmp(tFontName, "REALUNIX")) {
		finfo->fontPref = "";
		finfo->platform = UNIXDATA;
		finfo->fontType = NOCHANGE;
	} else {
		finfo->fontPref = "";
		finfo->platform = MACDATA;
		if (/* isUTF &&  */!strcmp(tFontName, "ARIAL UNICODE MS"))
			finfo->fontType = MACArialUC;
		else if (/* isUTF && */!strcmp(tFontName, "CAFONT"/*UNICODEFONT*/))
			finfo->fontType = MacCAFont;
		else if  (!strcmp(tFontName, "IPAPHON"))
			finfo->fontType = MACipa;
		else if  (!strcmp(tFontName, "CA"))
			finfo->fontType = MACcain;
		else if  (!strcmp(tFontName, "COURIER CE") || !strcmp(tFontName, "GENEVA CE") ||
				  !strcmp(tFontName, "CHICAGO CE"))
			finfo->fontType = MACGenevaCE;
		else if (!strcmp(tFontName, "MONACO")  || !strcmp(tFontName, "GENEVA"))
			finfo->fontType = MONACO;
		else if (!strcmp(tFontName, "COURIER")  || !strcmp(tFontName, "CHICAGO") || 
				 !strcmp(tFontName, "TIMES")	|| !strcmp(tFontName, "CHARCOAL"))
			finfo->fontType = COURIER;
		else if  (!strcmp(tFontName, "OSAKA"))
			finfo->fontType = MACjpn;
		else if  (!strcmp(tFontName, "TAIPEI")  || !strcmp(tFontName, "APPLE LIGOTHIC MEDIUM") ||
				  !strcmp(tFontName, "BIAUKAI") || !strcmp(tFontName, "APPLE LISUNG LIGHT"))
			finfo->fontType = MACchn_B5;
		else if  (!strcmp(tFontName, "BEIJING") || !strcmp(tFontName, "FANG SONG") ||
				  !strcmp(tFontName, "SONG")    || !strcmp(tFontName, "HEI") ||
				  !strcmp(tFontName, "KAI"))
			finfo->fontType = MACchn_GB;
		else if  (!strcmp(tFontName, "APPLEMYUNGJO"))
			finfo->fontType = MACKor_HUNG;
		else if (!strncmp(tFontName, "FIXEDSYS EXCELSIOR", 18))
			finfo->fontType = MacFxdsys;
		else if (!strcmp(tFontName, "CAFONT"))
			finfo->fontType = MacCAFont;
		else if (finfo->CharSet == 0)
			finfo->fontType = COURIER;
		else if (finfo->CharSet == 21)
			finfo->fontType = MACThai;
		else if (finfo->CharSet == 1)
			finfo->fontType = MACjpn;
		else if (finfo->CharSet == 2)
			finfo->fontType = MACchn_B5;
		else  if (finfo->CharSet == 25)
			finfo->fontType = MACchn_GB;
		else  if (finfo->CharSet == 3)
			finfo->fontType = MACKor_HUNG;
		else
			finfo->fontType = NOCHANGE;
	}
	if (!strcmp(tFontName, "ARIAL UNICODE MS")/* && finfo->CharSet < 2*/) {
		if (finfo->platform == WIN95DATA)
			finfo->orgFType = WINArialUC;
		else
			finfo->orgFType = MACArialUC;
	} else if (!strcmp(tFontName,"CAFONT"/*UNICODEFONT*/)/* && finfo->CharSet < 2*/) {
		if (finfo->platform == WIN95DATA)
			finfo->orgFType = WINCAFont;
		else
			finfo->orgFType = MacCAFont;
	} else
		finfo->orgFType = finfo->fontType;
}

const char *GetDatasFont(const char *line, char ec, NewFontInfo *cfinfo, char isUTF, NewFontInfo *finfo) {
	int  i, j;
	char tFontName[256];
	char t[BUFSIZ+1];

	if (line == NULL)
		return(NULL);

	finfo->platform = NOCHANGE;
	for (i=0; isSpace(line[i]) || line[i] == '\n'; i++) ;

	for (j=0; j < BUFSIZ && line[i] && line[i] != ':' && line[i] != ec; i++) {
		t[j++] = line[i];
	}
	t[j] = EOS;
	if (j >= 255)
		return(NULL);
	strcpy(finfo->fontName, t);
	strcpy(tFontName, finfo->fontName);
	uS.uppercasestr(tFontName, cfinfo, FALSE);
	if (line[i] == EOS || line[i] == ec)
		return(NULL);
	if (!strcmp(tFontName, "WIN95"))
		finfo->platform = WIN95DATA;
	for (i++; isSpace(line[i]) || line[i] == '\n'; i++) ;
	if (line[i] == EOS || line[i] == ec)
		return(NULL);

	for (j=0; line[i] && line[i] != ':' && line[i] != ec; i++) {
		t[j++] = line[i];
	}
	t[j] = EOS;	
	if (finfo->platform == WIN95DATA) {
		if (j >= 255)
			return(NULL);
		strcpy(finfo->fontName, t);
		strcpy(tFontName, finfo->fontName);
		uS.uppercasestr(tFontName, cfinfo, FALSE);
		if (line[i] == EOS || line[i] == ec)
			return(NULL);
		for (i++; isSpace(line[i]) || line[i] == '\n'; i++) ;
		if (line[i] == EOS || line[i] == ec)
			return(NULL);
		for (j=0; line[i] && line[i] != ':' && line[i] != ec; i++) {
			t[j++] = line[i];
		}
		t[j] = EOS;			
	}

	finfo->fontSize = atol(t);
	if (finfo->fontSize == 0L)
		return(NULL);

	finfo->CharSet = -2;
	if (line[i] == ':') {
		for (i++; isSpace(line[i]) || line[i] == '\n'; i++) ;
		if (line[i] == EOS || line[i] == ec || !isdigit(line[i])) 
			;
		else {
			for (j=0; line[i] && line[i] != ':' && line[i] != ec; i++) {
				t[j++] = line[i];
			}
			t[j] = EOS;						
			finfo->CharSet = atoi(t);
		}
	}

	if (line[i] != ec) {
		if (line[i] == ':') {
			for (; line[i] && line[i] != ec; i++) ;
			if (line[i] != ec)
				return(NULL);
		} else
			return(NULL);
	}
	if (line[i] != EOS)
		for (i++; isSpace(line[i]) || line[i] == '\n'; i++) ;

	IDFont(tFontName, isUTF, finfo);

	return(line+i);
}

extern "C"
{
extern long Dos850ToASCII(FONTARGS);
extern long Dos860ToASCII(FONTARGS);
extern long MonacoToASCII(FONTARGS);
extern long MonacoToWin95courier(FONTARGS);
extern long CourierToWin95courier(FONTARGS);
extern long Win95courierToCourier(FONTARGS);
extern long Win95courierToDos850(FONTARGS);
extern long Dos850ToMonaco(FONTARGS);
extern long Dos860ToCourier(FONTARGS);
extern long Dos860ToWin95courier(FONTARGS);
extern long Dos850ToWin95courier(FONTARGS);
extern long MonacoToDos850(FONTARGS);
extern long CourierToDos860(FONTARGS);
extern long MacCAToWin95CA(FONTARGS);
extern long Win95CAToMacCA(FONTARGS);
extern long MacCourierCEToWin95CourCE(FONTARGS);
extern long Win95CourCEToMacCourierCE(FONTARGS);
}

char FindTTable(NewFontInfo *finfo, short thisPlatform) {
	finfo->fontTable = NULL;
	if (thisPlatform == MACDATA) {
		if (finfo->platform == DOSDATA) {
			if (finfo->fontType == DOS850) {
				strcpy(finfo->fontName, "Monaco");
				finfo->fontPref = "";
				finfo->fontSize = 9;
				finfo->CharSet = 0;
				finfo->fontTable = Dos850ToMonaco;
				return(TRUE);
			} else if (finfo->fontType == DOS860) {
				strcpy(finfo->fontName, "Courier");
				finfo->fontPref = "";
				finfo->fontSize = 10;
				finfo->CharSet = 0;
				finfo->fontTable = Dos860ToCourier;
				return(TRUE);
			}
		} else if (finfo->platform == WIN95DATA) {
			if (finfo->fontType == WIN95cour) {
				strcpy(finfo->fontName, "Courier");
				finfo->fontPref = "";
				finfo->fontSize = 10;
				finfo->CharSet = 0;
				finfo->fontTable = Win95courierToCourier;
				return(TRUE);
			} else if (finfo->fontType == WIN95ipa) {
				strcpy(finfo->fontName, "IPAPhon");
				finfo->fontPref = "";
				finfo->fontSize = 12;
				finfo->CharSet = 0;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == WIN95cain) {
				strcpy(finfo->fontName, "CA");
				finfo->fontPref = "";
				finfo->fontSize = 12;
				finfo->CharSet = 0;
				finfo->fontTable = Win95CAToMacCA;
				return(TRUE);
			} else if (finfo->fontType == WIN95courCE) {
				strcpy(finfo->fontName, "Courier CE");
				finfo->fontPref = "";
				finfo->fontSize = 10;
				finfo->CharSet = 0;
				finfo->fontTable = Win95CourCEToMacCourierCE;
				return(TRUE);
			} else if (finfo->fontType == WINFxdsys) {
				strcpy(finfo->fontName, "Fixedsys Excelsior 2.00");
				finfo->fontPref = "";
				finfo->fontSize = 15;
				finfo->CharSet = 0;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == WINCAFont) {
				strcpy(finfo->fontName, "CAfont");
				finfo->fontPref = "";
				finfo->fontSize = 14;
				finfo->CharSet = 0;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == WINArialUC) {
				strcpy(finfo->fontName, defUniFontName);
				finfo->fontPref = "";
				finfo->fontSize = defUniFontSize;
				finfo->CharSet = 0;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == WINThai) {
				finfo->orgEncod = finfo->CharSet;
				strcpy(finfo->fontName, "LxS SpEcIaL FoNt");
				finfo->fontPref = "";
				finfo->fontSize = 13;
				finfo->CharSet = kTextEncodingMacThai;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == WIN95Latin5) {
				finfo->orgEncod = finfo->CharSet;
				strcpy(finfo->fontName, "LxS SpEcIaL FoNt");
				finfo->fontPref = "";
				finfo->fontSize = 14;
				finfo->CharSet = kTextEncodingWindowsLatin5;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == WINchn) {
				strcpy(finfo->fontName, "Taipei");
				finfo->fontPref = "";
				finfo->fontSize = 12;
				finfo->CharSet = 2;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == WINchn_GB) {
				strcpy(finfo->fontName, "Beijing");
				finfo->fontPref = "";
				finfo->fontSize = 12;
				finfo->CharSet = 25;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == WINchn_B5) {
				strcpy(finfo->fontName, "Taipei");
				finfo->fontPref = "";
				finfo->fontSize = 12;
				finfo->CharSet = 2;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == WINjpn) {
				int i;
				strcpy(finfo->fontName, "Osaka");
				i = strlen(finfo->fontName);
				finfo->fontName[i++] = 0x81;
				finfo->fontName[i++] = 0x7C;
				finfo->fontName[i++] = 0x93;
				finfo->fontName[i++] = 0x99;
				finfo->fontName[i++] = 0x95;
				finfo->fontName[i++] = 0x9D;
				finfo->fontName[i] = EOS;
				finfo->fontPref = "";
				if (finfo->fontSize >= -15)
					finfo->fontSize = 12;
				else if (finfo->fontSize <= -19)
					finfo->fontSize = 16;
				else
					finfo->fontSize = 14;
				finfo->CharSet = 1;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == WINKor_HUNG) {
				strcpy(finfo->fontName, "AppleMyungjo");
				finfo->fontPref = "";
				finfo->fontSize = 10;
				finfo->CharSet = 3;
				finfo->fontTable = NULL;
				return(TRUE);
			}
/*
		} else if (finfo->platform == UNICODEDATA) {
			if (finfo->fontType == UNICYRIL) {
				strcpy(finfo->fontName, "Latinskij");
				finfo->fontPref = "";
				finfo->fontSize = 14;
				finfo->CharSet = 0;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == UNIIPAPH) {
				strcpy(finfo->fontName, "IPAPhon");
				finfo->fontPref = "";
				finfo->fontSize = 12;
				finfo->CharSet = 0;
				finfo->fontTable = NULL;
				return(TRUE);
			}
*/
		}
	} else if (thisPlatform == WIN95DATA) {
		if (finfo->platform == MACDATA) {
			if (finfo->fontType == COURIER) {
				strcpy(finfo->fontName, "Courier");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -14;
				finfo->CharSet = 1;
				finfo->fontTable = CourierToWin95courier;
				return(TRUE);
			} else if (finfo->fontType == MONACO) {
				strcpy(finfo->fontName, "Courier");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -14;
				finfo->CharSet = 1;
				finfo->fontTable = MonacoToWin95courier;
				return(TRUE);
			} else if (finfo->fontType == MACipa) {
				strcpy(finfo->fontName, "IPAPhon");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -14;
				finfo->CharSet = 2;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == MACcain) {
				strcpy(finfo->fontName, "CA");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -12;
				finfo->CharSet = 1;
				finfo->fontTable = MacCAToWin95CA;
				return(TRUE);
			} else if (finfo->fontType == MacFxdsys) {
				strcpy(finfo->fontName, "Fixedsys Excelsior 2.00");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -16;
				finfo->CharSet = 0;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == MacCAFont) {
				strcpy(finfo->fontName, "CAfont");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -15;
				finfo->CharSet = 0;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == MACGenevaCE) {
				strcpy(finfo->fontName, "Courier New");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -13;
				finfo->CharSet = 238;
				finfo->fontTable = MacCourierCEToWin95CourCE;
				return(TRUE);
			} else if (finfo->fontType == MACjpn) {
				strcpy(finfo->fontName, "MS Mincho");
				finfo->fontPref = "Win95:";
				if (finfo->fontSize < 14)
					finfo->fontSize = -15;
				else if (finfo->fontSize > 14)
					finfo->fontSize = -19;
				else
					finfo->fontSize = -16;
				finfo->CharSet = 128;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == MACchn) {
				strcpy(finfo->fontName, "Chn System");
				finfo->fontPref = "Win95:";
				finfo->fontSize = defUniFontSize;
				finfo->CharSet = 1;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == MACchn_GB) {
				strcpy(finfo->fontName, defUniFontName);
				finfo->fontPref = "Win95:";
				finfo->fontSize = defUniFontSize;
				finfo->CharSet = 134;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == MACchn_B5) {
				strcpy(finfo->fontName, defUniFontName);
				finfo->fontPref = "Win95:";
				finfo->fontSize = defUniFontSize;
				finfo->CharSet = 136;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == MACKor_HUNG) {
				strcpy(finfo->fontName, defUniFontName);
				finfo->fontPref = "Win95:";
				finfo->fontSize = defUniFontSize;
				finfo->CharSet = 129;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == MACArialUC) {
				strcpy(finfo->fontName, defUniFontName);
				finfo->fontPref = "Win95:";
				finfo->fontSize = defUniFontSize;
				finfo->CharSet = 0;
				finfo->fontTable = NULL;
				return(TRUE);
			}
/*
		} else if (finfo->platform == UNICODEDATA) {
			if (finfo->fontType == UNICYRIL) {
				strcpy(finfo->fontName, "Courier New");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -13;
				finfo->CharSet = 204;
				finfo->fontTable = NULL;
				return(TRUE);
			} else if (finfo->fontType == UNIIPAPH) {
				strcpy(finfo->fontName, "IPAPhon");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -14;
				finfo->CharSet = 2;
				finfo->fontTable = NULL;
				return(TRUE);
			}
*/
		} else if (finfo->platform == DOSDATA) {
			if (finfo->fontType == DOS850) {
				strcpy(finfo->fontName, "Courier");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -13;
				finfo->CharSet = 1;
				finfo->fontTable = Dos850ToWin95courier;
				return(TRUE);
			} else if (finfo->fontType == DOS860) {
				strcpy(finfo->fontName, "Courier");
				finfo->fontPref = "Win95:";
				finfo->fontSize = -13;
				finfo->CharSet = 1;
				finfo->fontTable = Dos860ToWin95courier;
				return(TRUE);
			}
		}
	} else if (thisPlatform == DOSDATA) {
		if (finfo->platform == MACDATA) {
			if (finfo->fontType == MONACO) {
				strcpy(finfo->fontName, "RealDosCodePage");
				finfo->fontPref = "";
				finfo->fontSize = 850;
				finfo->CharSet = 0;
				finfo->fontTable = MonacoToDos850;
				return(TRUE);
			} else if (finfo->fontType == COURIER) {
				strcpy(finfo->fontName, "RealDosCodePage");
				finfo->fontPref = "";
				finfo->fontSize = 860;
				finfo->CharSet = 0;
				finfo->fontTable = CourierToDos860;
				return(TRUE);
			}
		} else if (finfo->platform == WIN95DATA) {
			if (finfo->fontType == WIN95cour) {
				strcpy(finfo->fontName, "RealDosCodePage");
				finfo->fontPref = "";
				finfo->fontSize = 850;
				finfo->CharSet = 0;
				finfo->fontTable = Win95courierToDos850;
				return(TRUE);
			}
		}
	} else if (thisPlatform == UNIXDATA) {
		if (finfo->platform == MACDATA) {
			if (finfo->fontType == MONACO) {
				strcpy(finfo->fontName, "RealUNIX");
				finfo->fontPref = "";
				finfo->fontSize = 0;
				finfo->CharSet = 0;
				finfo->fontTable = MonacoToASCII;
				return(TRUE);
			}
		} else if (finfo->platform == DOSDATA) {
			if (finfo->fontType == DOS850) {
				strcpy(finfo->fontName, "RealUNIX");
				finfo->fontPref = "";
				finfo->fontSize = 0;
				finfo->CharSet = 0;
				finfo->fontTable = Dos850ToASCII;
				return(TRUE);
			} else if (finfo->fontType == DOS860) {
				strcpy(finfo->fontName, "RealUNIX");
				finfo->fontPref = "";
				finfo->fontSize = 0;
				finfo->CharSet = 0;
				finfo->fontTable = Dos860ToASCII;
				return(TRUE);
			}
		}
	}
	return(FALSE);
}

#define AddStr(s) { strcpy(st, (s)); return(strlen(s)); }

long Dos850ToASCII(FONTARGS) {
	*st = c[*index];
	return(1);
}

long Dos860ToASCII(FONTARGS) {
	*st = c[*index];
	return(1);
}

long MonacoToASCII(FONTARGS) {
	*st = c[*index];
	return(1);
}

long MonacoToWin95courier(FONTARGS) {
	*st = c[*index];
	return(1);
}

long MacCAToWin95CA(FONTARGS) {
	*st = c[*index];
	return(1);
}

long Win95CAToMacCA(FONTARGS) {
	*st = c[*index];
	return(1);
}

long Win95courierToCourier(FONTARGS) {
	*st = c[*index];
	return(1);
}

long CourierToWin95courier(FONTARGS) {
	*st = c[*index];
	return(1);
}

long MacCourierCEToWin95CourCE(FONTARGS) {
	*st = c[*index];
	return(1);
}

long Win95CourCEToMacCourierCE(FONTARGS) {
	*st = c[*index];
	return(1);
}

long Win95courierToDos850(FONTARGS) {
	*st = c[*index];
	return(1);
}

long Dos860ToWin95courier(FONTARGS) {
	*st = c[*index];
	return(1);
}

long Dos850ToWin95courier(FONTARGS) {
	*st = c[*index];
	return(1);
}

long Dos850ToMonaco(FONTARGS) {
	*st = c[*index];
	return(1);
}

long Dos860ToCourier(FONTARGS) {
	*st = c[*index];
	return(1);
}

long MonacoToDos850(FONTARGS) {
	*st = c[*index];
	return(1);
}

long CourierToDos860(FONTARGS) {
	*st = c[*index];
	return(1);
}

