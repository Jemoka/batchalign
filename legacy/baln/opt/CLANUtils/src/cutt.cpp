/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/

#include "cu.h"
#include "mul.h"
#include "check.h"

#include <time.h>

static char VERSION[] = "BATCHALIGN_CLAN";


#define WD_Not_Eq_OD (pathcmp(wd_dir, od_dir))

#if defined(__linux__) || defined(ERRNO)
	#define USE_TERMIO
#endif

#ifdef _DOS
//	#include "stdafx.h"
#endif
#if defined(UNX)
	#ifdef USE_TERMIO
		#include <sys/errno.h>
		#if defined(ERRNO)
			#include <errno.h>
		#endif		
		#ifdef __linux__
			#include <termio.h>
		#else
			#include <sys/termio.h>
		#endif
	#else
		#include <sgtty.h>
	#endif
#endif

#include <sys/stat.h>
#include <dirent.h>

#include <ctype.h>
#include "c_curses.h"
#include "mul.h"

typedef unsigned long  UInt32;

NewFontInfo dFnt = {"", "", 0L, 0, 0, 0, 0, 0, 0, '\0', 0, 0, NULL};
FNType wd_dir[FNSize];
FNType od_dir[FNSize];
FNType lib_dir[FNSize];
FNType mor_lib_dir[FNSize];

#define EXTENDEDARGVLEN 10000
#define NUMLANGUAGESINTABLE 10
#define OPTIONS_EXT_LEN 15

#define MORPHS "-#$~+"

#define UTTLENWORDS struct UttLenWord
UTTLENWORDS {
    char *word;
	char inc;
    UTTLENWORDS *nextWdUttLen;
} ;

#define LANGPARTS struct LangParts
struct LangParts {
    char *wordpart;
	char *pos;
	char *lang0;
	char *lang1;
	char *lang2;
	char *lang3;
	char *suffix0;
	char *suffix1;
	char *suffix2;
} ;

#define ML_CLAUSE struct clauses
ML_CLAUSE {
	char *cl;
	ML_CLAUSE *nextcl;
} ;
static ML_CLAUSE *ml_root_clause;

struct mor_link_struct mor_link;

char isWinMode;	/* controls line counting and window features */

//extern void (*clan_main[]) (int argc, char *argv[]);
extern void (*clan_usage[]) (void);
extern void (*clan_getflag[]) (char *, char *, int *);
extern void (*clan_init[]) (char);
extern void (*clan_call[]) (void);

static char options_already_inited = 0;
static char fontErrorReported = FALSE;

int  IsOutTTY(void);

#define ALL_OPTIONS F_OPTION+RE_OPTION+K_OPTION+P_OPTION+L_OPTION+R_OPTION+SP_OPTION+SM_OPTION+T_OPTION+UP_OPTION+Z_OPTION

#define TESTLIM 6
const char *MonthNames[12] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
			   "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" } ;
char GExt[32];
int  OnlydataLimit;
int  UttlineEqUtterance;
int  chatmode;				/* chatmode =0 - no y switch & errmargin =0;*/
							/*		=1 - y switch & errmargin =0;	   */
							/*		=2 - no y switch & errmargin =0;	*/
							/*		=3 - y switch & errmargin = TESTLIM */
							/*		=4 - no y switch & no testing	*/
int  F_numfiles = 0;


/**************************************************************/
/*	 enviroment control				  */

char tcs = FALSE,			/* 1- if all non-specified speaker tier should be selected*/
	 tch = FALSE,			/* 1- if all non-specified header tier should be selected */
	 tct = FALSE;			/* 1- if all non-specified code tier should be selected   */
char otcs = TRUE,			/* 1- if all non-specified speaker tier should be selected*/
	 otch = TRUE,			/* 1- if all non-specified header tier should be selected */
	 otct = TRUE,			/* 1- if all non-specified code tier should be selected   */
	 otcdt = FALSE;			/* 1- if code tiers were specified by user with +o option */
char tcode = FALSE;			/* 1- if all files with keyword on @Keywords should be included */
static char *targs;			/* array used by bmain; 0 if arg is a file name; otherwise 1  */
static char isSearchMorSym; /* 1 if user searches for symbols: cm|*, end|*, beg|* //2019-04-29, bq|*, eq|* */
static char LanguagesTable[NUMLANGUAGESINTABLE][9];
static char foundUttContinuation = FALSE;
static int  defLanguage;
char GlobalPunctuation[50];
static char morPunctuation[50];
static char cedPunctuation[50];
//2011-01-26 FNType punctFile[FNSize];
//2011-01-26 char punctMess[512];
char ProgDrive;
char OutputDrive;
char IncludeAllDefaultCodes;
char cutt_isMultiFound = FALSE;
char cutt_isCAFound = FALSE;
char cutt_isBlobFound = FALSE;
char cutt_isSearchForQuotedItems = 0;  // 2019-07-17
char isRemoveCAChar[NUMSPCHARS+2];
struct tier *HeadOutTier;	/* points to the list of specified output tier   */
struct tier *headtier;		/* points to the list of specified tier	   */
struct tier *defheadtier;	/* default include/exclude tier list header	   */
struct IDtype *IDField;		/* used to specify speaker names through @ID tier*/
struct IDtype *CODEField;	/* used to filter files based on string on @Keywords tier */
struct IDtype *SPRole;		/* specifies speakers through role on @participants tier */
UTTER *utterance = NULL;
UTTER *lutter;				/* point to current tier in a set of window tiers   */
char *uttline;				/* points to a working version of the turn text	 */
char currentchar= '\0';		/* contains last input character read		   */
AttTYPE currentatt=  '\0';	/* contains attribute of the last input character read */
int aftwin = 0;				/* size of window after the target tier, in tiers   */
int IsModUttDel;
int CLAN_PROG_NUM;
int getc_cr_lc = '\0';
char fgets_cr_lc = '\0';
long MAXOUTCOL;
long option_flags[LAST_CLAN_PROG];
char isKWAL2013;
void (*fatal_error_function)(char IsOutOfMem) = NULL;


char MBF = FALSE;
char C_MBF = FALSE;
static char options_ext[LAST_CLAN_PROG][OPTIONS_EXT_LEN+1];
char org_spName[SPEAKERLEN+2];	      /* stores the name of speaker in org_spTier*/
char org_spTier[UTTLINELEN+2];	      /* used by createMorUttline function for compairing speaker to dep tier*/
char org_depTier[UTTLINELEN+2];/* used by createMorUttline function for compairing two dep tiers*/
char fDepTierName[SPEAKERLEN+2];      /* used by getmaincode, getwholeutter for createMorUttline function to compaire two dep tiers*/
char sDepTierName[SPEAKERLEN+2];      /* used by getmaincode, getwholeutter for createMorUttline function to compaire two dep tiers*/
static char tempTier[UTTLINELEN+2];   /* used by createMorUttline, remove_main_tier_print, cutt_cleanUpLine->expandX, copyFillersToDepTier */
static char tempTier1[UTTLINELEN+2];  /* used by getmaincode, excelHeader, excelCommasStrCell, excelRedStrCell, excelStrCell */
char spareTier1[UTTLINELEN+2];
char spareTier2[UTTLINELEN+2];
char spareTier3[UTTLINELEN+2];
char morWordParse[UTTLINELEN+2];	/* %mor word for parsing in ParseWordMorElems */
unCH templine [UTTLINELEN+2];	/* temporary variable to store code descriptors */ // found uttlinelen
unCH templine1[UTTLINELEN+2];	/* temporary variable to store code descriptors */
unCH templine2[UTTLINELEN+2];	/* temporary variable to store code descriptors */
unCH templine3[UTTLINELEN+2];	/* temporary variable to store code descriptors */
unCH templine4[UTTLINELEN+2];   /* temporary variable to store code descriptors */
unCH templineW[UTTLINELEN+2];	/* temporary variable to store code descriptors */
unCH templineW1[UTTLINELEN+2];	/* temporary variable to store code descriptors */
char templineC [UTTLINELEN+2];	/* used to be: templine  */
char templineC1[UTTLINELEN+2];	/* used to be: templine1 */
char templineC2[UTTLINELEN+2];	/* used to be: templine2 */
char templineC3[UTTLINELEN+2];	/* used to be: templine3 */
char templineC4[UTTLINELEN+2];	/* used to be: templine4 */
AttTYPE tempAtt[UTTLINELEN+2];	/* temporary variable to store code descriptors */
FNType DirPathName[FNSize+2];		/* used for temp storage of path name */
FNType FileName1[FNSize+2];		/* used for temp storage of file name */
FNType FileName2[FNSize+2];		/* used for temp storage of file name */
FNType cMediaFileName[FILENAME_MAX+2];/* used by expend bullets */

int  totalFilesNum;				/* used by CHECK to compute the current progress */
int  postcodeRes;				/* reflects presence of postcodes 0-if non or 4,5,6-if found */
char rightspeaker = '\001';		/* 1 - speaker/code turn is selected by the user*/
char nomain = FALSE;			/* 1 - exclude text from main speaker tiers,   0*/
char getrestline = 1;			/* 0 - cutt_getline() should not get rest of text,  1*/
char skipgetline = FALSE;		/* 1 - cutt_getline() should NOT add "\n"		   0*/
char linkDep2Other = FALSE;		/* connect words on a dependant tier to corresponding items on other tier */
char linkMain2Mor = FALSE;		/* connect words on speaker tier to corresponding items on %mor */
char linkMain2Sin = FALSE;		/* connect words on speaker tier to corresponding items on %sin */
char isCreateFakeMor = 0;	    /* */
char Preserve_dir = TRUE;		/* select directory for output files		   */
char FirstTime = TRUE;			/* 0 if the call() has been called once, dflt 1 */
char *Toldsp;					/* last speaker name, used  for +z#t range	   */
char *TSoldsp;					/* last speaker name, used  for -u range	   */
char isExpendX = TRUE;			/* if = 1, then expend word [x 2] to word [/] word */
char LocalTierSelect;			/* 1 if tier selection is done by programs,dfl 1*/
char FilterTier;				/* 0 if tiers should NOT be filtered, 1 if only default filer, dflt 2*/
char Parans = 1;				/* get(s); 1-gets, 2-get(s), 3-get		   */
char f_override;				/* if user specified -f option, then = 1 */
char y_option;					/* 0 - line at a time; 1 - utterance at a time  */
char *rootmorf = NULL;			/* contains a list of morpheme characters	   */
char RemPercWildCard;			/* if =1 then chars matched by % are removed	   */
char contSpeaker[SPEAKERLEN+1];	/* if +/. found then set it and look for +, */
char OverWriteFile;				/* if =0 then keep output file versions		*/
const char *AddCEXExtension;	/* if =1 then add ".cex" to output file name*/
char isPostcliticUse;			/* if '~' found on %mor tier break it into two words */
char isPrecliticUse;			/* if '$' found on %mor tier break it into two words */
static char restoreXXX, restoreYYY, restoreWWW;
char filterUttLen_cmp;			/* = Equal, < les than, > greater than */
long filterUttLen;				/* filter utterances based on their length */
long FromWU,					/* contains the beginning range number	   */
	 ToWU,						/* contains the end range number		   */
	 WUCounter;					/* current word/utterance count number	   */

/*****************************************************************/
/*	 programs common options						 */
const char *delSkipedFile = NULL;/* if +t@id="" option used and file didn't match, then delete output file */
char isFileSkipped = FALSE;		/* TRUE - if +t@id="" option used and file didn't match */
char replaceFile = FALSE;		/* 1 - replace org data file with the same name file */
int  onlydata = '\0';			/* 1,2,3 - level of amount of data output, dflt 0  */
char puredata = 2;				/* FALSE - the +d output is NOT CHAT legal, dflt 1 */
char outputOnlyData = FALSE;	/* if FALSE, then no file, command line info outputed */
char combinput = '\0';			/* 1 - merge data from all specified file, dflt 0  */
char ByTurn = '\0';				/* if = 1 then each turn is afile set		   */
char nomap = FALSE;				/* 0 - convert all to lower case, dflt 0 	   */
char WordMode = '\0';			/* 'i' - means to include wdptr words, else exclude*/
char MorWordMode = '\0';
char isWOptUsed;				/* TRUE if +/-w option specified */
char R5;						/* take care of [: text]. default leave it in
								   its a hack look for value 1001 by "isExcludeScope" */
char R5_1;						/* take care of [:: text]. default leave it in
								   its a hack look for value 1001 by "isExcludeScope" */
char pauseFound,opauseFound;	/* 1 - if +s#* found on command line				*/
static char *ScopWdPtr[MXWDS];	/* set of scop tokens (<one>[\\]) to be included
								or excluded */
char ScopMode = '\0';			/* 'i' - means to included scop tokens, else excl  */
int  ScopNWds = 1;				/* number of include/exclude scop tokens specified */
char PostCodeMode= '\0';		/* mode for post codes, the same setup as ScopMode */
char R4 = FALSE;				/* if true do not break words at '~' and '$' on %mor tier */
char R6 = TRUE;					/* if true then retraces </>,<//>,<///>,</->,</?> are included   */
char R6_override = FALSE;		/* includes all </>,<//>,<///>,</->,</?> regardless of other settings */
char R6_freq = FALSE;			/* includes all </>,<//>,<///>,</->,</?> regardless, only for FREQ */
char R7Slash = TRUE;			/* if false do not remove '/' from words */
char R7Tilda = TRUE;			/* if false do not remove '~' from words */
char R7Caret = TRUE;			/* if false do not remove '^' from words */
char R7Colon = TRUE;			/* if false do not remove ':' from words */
char isSpecRepeatArrow = FALSE;	/* 1 if left arrow loop character is used in search */
char R8 = FALSE;				/* if true inlcude actual word spoken and/or error code in output from %mor: tier */
char isRecursive=FALSE;			/* if = 1 then run on all sub-directories too	*/
char anyMultiOrder = FALSE;		/* if = 1 then match multi-words in any order found on a tier */
char onlySpecWsFound= FALSE;	/* if = 1 then match only if tier consists solely of all words in multi-word group */
char isMultiMorSearch = FALSE;	/* set if multi-word search is on %mor tier and %mor tier has to be select */
char isMultiGraSearch = FALSE;	/* set if multi-word search is on %gra tier and %gar tier has to be select */
long lineno = 0L;				/* current turn line number (dflt 0)			*/
long tlineno = 1L;				/* line number within the current turn			*/
long deflineno = 0L;			/* default line number							*/
IEWORDS *wdptr;					/* contains words to included/excluded			*/
IEMWORDS *mwdptr;				/* contains multi-words to included/excluded	*/
static MORWDLST *morfptr;		/* contains %mor words to included/excluded		*/
static GRAWDLST *grafptr;		/* contains %gra words to included/excluded		*/
static IEWORDS *defwdptr;		/* contains default words to included/excluded	*/
static IEWORDS *CAptr;			/* contains words to included/excluded			*/

/**************************************************************/
/*	 input output file control 				  */
FILE *fpin;						/* file pointers to input stream		   */
FILE *fpout;					/* file pointers to output stream		   */
FNType *oldfname;
FNType newfname[FNSize];		/* the name of the currently opened file	   */
char stout = TRUE;				/* 0 - if the output is NOT stdout, dflt 1	   */
char stin  = FALSE;				/* 1 - if the input is stdin, dflt 0		   */

char maxwd_which;

static DIR *cDIR;

short my_CharacterByteType(const char *org, short pos, NewFontInfo *finfo) {
	short cType;

	if (finfo->isUTF) {
		cType = 2;
		if (UTF8_IS_SINGLE((unsigned char)org[pos]))
			cType = 0;
		else if (UTF8_IS_LEAD((unsigned char)org[pos]))
			cType = -1;
		else if (UTF8_IS_TRAIL((unsigned char)org[pos])) {
			if (!UTF8_IS_TRAIL((unsigned char)org[pos+1]))
				cType = 1;
			else
				cType = 2;
		}

		return(cType);
	} else
		return(0);
}

static void VersionNumber(char isshortfrmt, FILE *fp) {
	long pnum = 0L, t;
	char *s;
	char pbuf[20];
	extern char VERSION[];

	if (isshortfrmt) {
		fprintf(fp, " (%s)", VERSION);
	} else {
		fprintf(fp, "CLAN version: %s; ", VERSION);
#if defined(_DOS)
		fprintf(fp, "Windows DOS MinGW-w64 gcc\n");
#else
		fprintf(fp, "Unix.\n");
#endif
	}
}

void out_of_mem() {
	fputs("ERROR: Out of memory.\n",stderr);
	cutt_exit(1);
}

int getc_cr(FILE *fp, AttTYPE *att) {
	register int c;

	if (ByTurn == 3) {
		if (att != NULL)
			*att = currentatt;
		return(currentchar);
	}
rep:
	if ((c=getc(fp)) == '\r') {
		if (getc_cr_lc == '\n')
			c = getc(fp);
		else {
			getc_cr_lc = '\r';
			return('\n');
		}
	} else if (c == '\n' && getc_cr_lc == '\r') {
		c = getc(fp);
		if (c == '\r') {
			getc_cr_lc = '\r';
			return('\n');
		}
	}
	getc_cr_lc = c;
	if (c == (int)ATTMARKER) {
		c = getc(fp);
		if (SetTextAtt(NULL, (unCH)c, att))
			goto rep;
		else {
			fprintf(stderr,"*** File \"%s\": line %ld.\n", oldfname, lineno);
			fprintf(stderr,"Illegal text attribute marker '%c'(%d) found\n", c, (int)c);
			cutt_exit(0);
		}
	}
	return(c);
}

char *fgets_cr(char *beg, int size, FILE *fp) {
	register int i = 1;
	register char *buf;

	size--;
	buf = beg;
	*buf = (char)getc(fp);
	if (feof(fp))
		return(NULL);
	do {
		i++;
		if (*buf == '\r') {
			if (fgets_cr_lc != '\n') {
				*buf++ = '\n';
				fgets_cr_lc = '\r';
				break;
			} else
				i--;
		} else if (*buf == '\n') {
			if (fgets_cr_lc != '\r') {
				fgets_cr_lc = '\n';
				buf++;
				break;
			} else
				i--;
		} else
			buf++;
		fgets_cr_lc = '\0';
		if (i >= size)
			break;
		*buf = (char)getc(fp);
		if (feof(fp))
			break;
	} while (1) ;
	*buf = EOS;
	return(beg);
}

char *getCurrDirName(char *dir) {
	int i, t;

	i = strlen(dir) - 1;
	while ((isSpace(dir[i]) || dir[i] == '\n' || dir[i] == PATHDELIMCHR) && i >= 0)
		i--;
	if (i < 0)
		return(dir);
	t = i + 1;
	while (dir[i] != PATHDELIMCHR && i >= 0)
		i--;
	i++;
	t = t - i;
	strncpy(templineC4, dir+i, t);
	templineC4[t] = EOS;
	return(templineC4);
}

void displayOoption(void) {
	puts("     macl  - Mac Latin (German, Spanish ...)");
	puts("     macce - Mac Central Europe");
	puts("     pcl   - PC  Latin (German, Spanish ...)");
	puts("     pcce  - PC  Central Europe\n");

	puts("     macar - Mac Arabic");
	puts("     pcar  - PC  Arabic\n");

	puts("     maccs - Mac Chinese Simplified");
	puts("     macct - Mac Chinese Traditional");
	puts("     pccs  - PC  Chinese Simplified");
	puts("     pcct  - PC  Chinese Traditional\n");

	puts("     maccr - Mac Croatian");

	puts("     maccy - Mac Cyrillic");
	puts("     pccy  - PC  Cyrillic");

	puts("     machb - Mac Hebrew");
	puts("     pchb  - PC  Hebrew\n");

	puts("     macjp - Mac Japanese");
	puts("     pcjp  - PC  Japanese (Shift-JIS)\n");
	puts("     macj1 - Mac Japanese (JIS_X0201_76)");
	puts("     macj2 - Mac Japanese (JIS_X0208_83)");
	puts("     macj3 - Mac Japanese (JIS_X0208_90)");
	puts("     macj4 - Mac Japanese (JIS_X0212_90)");
	puts("     macj5 - Mac Japanese (JIS_C6226_78)");
	puts("     macj6 - Mac Japanese (Shift-JIS_X0213_00)");
	puts("     macj7 - Mac Japanese (ISO_2022_JP)");
	puts("     macj8 - Mac Japanese (ISO_2022_JP_1)");
	puts("     macj9 - Mac Japanese (ISO_2022_JP_2)");
	puts("     macj10- Mac Japanese (ISO_2022_JP_3)");
	puts("     macj11- Mac Japanese (EUC_JP)");
	puts("     macj12- Mac Japanese (Shift-JIS)\n");

	puts("     krn   - Mac and Windows Korean\n");
	puts("     pckr  - PC DOS Korean unified Hangul Code");
	puts("     pckrj - PC DOS Korean Johab");

	puts("     macth - Mac Thai");
	puts("     pcth  - PC  Thai\n");

	puts("     pcturk- PC  Turkish\n");

	puts("     macvt - Mac Vietnamese");
	puts("     pcvt  - PC  Vietnamese");
}

/**************************************************************/
//ML_ CLAUSE ml_ cluase begin
int ml_isclause(void) {
	return(ml_root_clause != NULL);
}

static int ml_IsClauseMarker(char *s) {
	ML_CLAUSE *tc;

	for (tc=ml_root_clause; tc; tc=tc->nextcl) {
		if (uS.partwcmp(s, tc->cl))
			return(TRUE);
	}
	return(FALSE);
}

int ml_UtterClause(char *line, int pos) {
	if (ml_root_clause == NULL) {
		if (*utterance->speaker == '%' && line[pos] == '?' && line[pos+1] == '|')
			;
		else if (uS.IsUtteranceDel(line, pos)) {
			if (!uS.atUFound(line, pos, &dFnt, MBF))
				return(TRUE);
		}
	} else {
		if (ml_IsClauseMarker(line+pos))
			return(TRUE);
	}
	return(FALSE);
} 

static void ml_free_clause(void) {
	ML_CLAUSE *t;

	while (ml_root_clause != NULL) {
		t = ml_root_clause;
		ml_root_clause = ml_root_clause->nextcl;
		if (t->cl != NULL)
			free(t->cl);
		free(t);
	}
}

void ml_AddClause(char opt, char *st) {
	char code[BUFSIZ], t[BUFSIZ];
	ML_CLAUSE *tc;

	if ((tc=NEW(ML_CLAUSE)) == NULL)
		out_of_mem();
	if (st[0] != '[') {
		strcpy(code, "[^");
		strcat(code, st);
		strcat(code, "]");
	} else
		strcpy(code, st);
	strcpy(t, "\001");
	strcat(t, code);
	if (t[1] == '[') {
		int i;
		t[1] = '<';
		for (i=0; t[i]; i++) {
			if (t[i] == ']') {
				t[i] = '>';
				break;
			}
		}
		addword(opt, '\0', t);
		t[1] = '[';
		for (i=0; t[i]; i++) {
			if (t[i] == '>') {
				t[i] = ']';
				break;
			}
		}
		addword(opt, 'i', t);
	}
	tc->cl = (char *)malloc(strlen(code)+1);
	if (tc->cl != NULL)
		strcpy(tc->cl, code);
	tc->nextcl = ml_root_clause;
	ml_root_clause = tc;
}

void ml_AddClauseFromFile(char opt, FNType *fname) {
	FILE *fp;
	char wd[BUFSIZ+1];
	FNType mFileName[FNSize];

	if ((fp=OpenGenLib(fname,"r",TRUE,FALSE,mFileName)) == NULL) {
		fprintf(stderr, "Can't open either one of the delemeter files:\n\t\"%s\", \"%s\"\n", fname, mFileName);
		cutt_exit(0);
	}
	while (fgets_cr(wd, BUFSIZ, fp)) {
		if (uS.isUTF8(wd) || uS.isInvisibleHeader(wd))
			continue;
		uS.remblanks(wd);
		ml_AddClause(opt, wd);
	}
	fclose(fp);
}
//ML_ CLAUSE ml_ cluase end
/**************************************************************/

/**************************************************************/
/*	BEGIN +s@0|, +sg| %gra tier elements parsing BEGIN */

static void graSearchSytaxUsage(char *f) {
	printf("\n%c%cg Followed by \"|\" and by grammatical relations search string\n", f[0], f[1]);
	puts("For example:");
	puts("    +s\"g|DET\"");
	puts("  find \"DET\" grammatical relations");
	puts("    +s\"g|OBJ\"");
	puts("  find \"OBJ\" grammatical relations");
	cutt_exit(0);
}

char isGraPat(char *f) {
	if (!isdigit(*f))
		return(FALSE);
	if (*f == '0' && *(f+1) == '|')
		return(TRUE);
	for (; *f != EOS; f++) {
		if (*f == '|')
			break;
		if (!isdigit(*f))
			return(FALSE);
	}
	if (*f == '|')
		return(TRUE);
	else
		return(FALSE);
}

char isGRASearchOption(char *f, char spc, char ch) {
	if (chatmode == 0)
		return(FALSE);
	if (ch != '-' && ch != '+')
		return(FALSE);
	if (*f != spc)
		return(FALSE);
	f++;
	if (*f == '+' || *f == '~')
		f++;
	if (spc == 'g' && *f == '|')
		return(TRUE);
	return(isGraPat(f));
}

char isWordFromGRATier(char *word) {
	int i, vbCnt;

	if (!isdigit(word[0]))
		return(FALSE);
	vbCnt = 0;
	for (i=1; 1; i++) {
		if (!isdigit(word[i])) {
			if (word[i] != '|') {
				return(FALSE);
			} else {
				vbCnt++;
				if (vbCnt == 2)
					return(TRUE);
			}
		}
	}
	return(FALSE);
}

char isGRASearch(void) {
	if (grafptr == NULL)
		return(FALSE);
	else
		return(TRUE);
}

static GRAWDLST *freeGraWords(GRAWDLST *p) {
	GRAWDLST *t;

	while (p != NULL) {
		t = p;
		p = p->nextword;
		if (t->word != NULL)
			free(t->word);
		free(t);
	}
	return(p);
}

static GRAWDLST *makeGraSeachList(GRAWDLST *root, char *wd, char ch) {
	char *s;
	GRAWDLST *p;

	s = strrchr(wd, '|');
	if (s != NULL)
		wd = s + 1;
	if (*wd == EOS)
		return(root);
	if (root == NULL) {
		p = NEW(GRAWDLST);
		root = p;
	} else {
		for (p=root; p->nextword != NULL; p=p->nextword) ;
		p->nextword = NEW(GRAWDLST);
		p = p->nextword;
	}
	if (p == NULL) {
		root = freeGraWords(root);
		out_of_mem();
	}
	p->nextword = NULL;
	p->word = (char *)malloc(strlen(wd)+1);
	if (p->word == NULL) {
		root = freeGraWords(root);
		out_of_mem();
	}
	strcpy(p->word, wd);
	p->type = ch;
	return(root);
}

void cleanupGRAWord(char *word) {
	char *s;

	s = strrchr(word, '|');
	if (s != NULL)
		strcpy(word, s+1);
}

/*	END +s@0|, +sg| %gra tier elements parsing END */
/**************************************************************/

/**************************************************************/
/*	BEGIN +sm %mor tier elements parsing BEGIN */
#define isStringEmpty(x) (x == NULL || *x == EOS)

static void freeMorFeats(MORFEATURESLIST *p) {
	MORFEATURESLIST *t;

	while (p != NULL) {
		t = p;
		p = p->nextFeat;
		if (t->pat != NULL)
			free(t->pat);
		free(t);
	}
}

MORWDLST *freeMorWords(MORWDLST *p) {
	MORWDLST *t;

	while (p != NULL) {
		t = p;
		p = p->nextMatch;
		freeMorFeats(t->rootFeat);
		free(t);
	}
	isSearchMorSym = FALSE;
	return(p);
}

void freeUpIxes(IXXS *ixes, int max) {
	int i;

	for (i=0; i < max; i++) {
		if (ixes[i].free_ix_s) {
			ixes[i].free_ix_s = FALSE;
			if (ixes[i].ix_s != NULL)
				free(ixes[i].ix_s);
		}
	}
}

static void freeUpCompd(MORFEATS *p) {
	MORFEATS *t;

	while (p != NULL) {
		t = p;
		p = p->compd;
		freeUpIxes(t->prefix, NUM_PREF);
		freeUpIxes(t->suffix, NUM_SUFF);
		freeUpIxes(t->fusion, NUM_FUSI);
		if (t->free_trans)
			free(t->trans);
		if (t->free_repls)
			free(t->repls);
		freeUpIxes(t->error, NUM_ERRS);
		free(t);
	}
}

void freeUpFeats(MORFEATS *c) {
	MORFEATS *p, *t;

	freeUpIxes(c->prefix, NUM_PREF);
	freeUpIxes(c->suffix, NUM_SUFF);
	freeUpIxes(c->fusion, NUM_FUSI);
	if (c->free_trans) {
		c->free_trans   = FALSE;
		free(c->trans);
	}
	if (c->free_repls) {
		c->free_repls   = FALSE;
		free(c->repls);
	}
	freeUpIxes(c->error, NUM_ERRS);
	freeUpCompd(c->compd);
	c->compd = NULL;
	p = c->clitc;
	c->clitc = NULL;
	while (p != NULL) {
		t = p;
		p = p->clitc;
		freeUpCompd(t);
	}
}

static void morSearchSytaxUsage(char *f) {
	NewFontInfo finfo;

	if (f != NULL) {
		finfo.fontName[0] = EOS;
		SetDefaultCAFinfo(&finfo);
		selectChoosenFont(&finfo, TRUE, TRUE);
		printf("\n%c%cm Followed by morpho-syntax\n", f[0], f[1]);
		puts("Morphosyntactic markers specify the nature of the following string");
	}
	puts("  # prefix marker");
	puts("  | part-of-speech marker");
	puts("  ; stem of the word marker");
//	puts("  r- root of the word marker, same as above'");
	puts("  - suffix marker");
	puts("  & nonconcatenated morpheme marker");
	puts("  = English translation for the stem marker");
	puts("  @ error word preceding [: ...] code marker on the main line");
	puts("  * error code inside [* ...] code marker on the main line");
	if (f != NULL) {
		puts("  and then optionally - or + and followed by one of these:");
		puts("      *        find any match");
		puts("      %        erase any match");
		puts("      string   find \"string\"");
		puts("  o erase all other elements not specified by user");
		puts("      o%    erase all other elements");
		puts("      o~    erase postclitic element, if present");
		puts("      o$    erase preclitic element, if present");
		puts("  ,		separates alternative elements");
		puts("Postclitic AND Preclitic exception:");
		puts("  Find postclitics with specific Morphosyntactic marker example:");
		puts("        |*,~|*   OR   ;*,~;*");
		puts("  Find preclitic with specific Morphosyntactic marker example:");
		puts("        |*,$|*   OR   ;*,$;*");
		puts("     *        find any match");
		puts("     string   find \"string\"");
		puts("\nFor example:");
		puts("  find all stems and erase all other markers:");
		puts("    +sm;*,o%");
		puts("  find all stems and erase all other markers including all postclitics, if present:");
		puts("    +sm;*,o%,o~");
		puts("  find all stems of all \"adv\" and erase all other markers:");
		puts("    +sm;*,|adv,o%");
		puts("  find all forms of \"be\" verb:");
		puts("    +sm;be");
		puts("  find all stems and parts-of-speech and erase other markers:");
		puts("    +sm;*,|*,o%");
		puts("  find only stems and parts-of-speech that have suffixes and erase other markers:");
		puts("    +sm;*,|*,-*,o%");
		puts("  find all stems, parts-of-speech and distinguish those with suffix and erase other markers:");
		puts("    +sm;*,|-*,-+*,o-%");
		puts("  find only stems and parts-of-speech that do not have suffixes and erase other markers:");
		puts("    -sm-* +sm;*,|*,o%");
		puts("  find only noun words with \"poss\" parts-of-speech postclitic:");
		puts("    +sm|n,|n:*,~|poss");
		puts("  find all noun words and show postclitics, if they have any:");
		puts("    +sm|n,|n:* +r4");
		puts("  find all noun words and erase postclitics, if they have any:");
		puts("    +sm|n,|n:*,o~");
		cutt_exit(0);
	} else {
		puts("  o erase all other elements not specified by user");
		puts("  ,		separates alternative elements");
		puts("Postclitic AND Preclitic exception:");
		puts("  Find postclitics with specific Morphosyntactic marker example:");
		puts("        |*,~|*   OR   ;*,~;*");
		puts("  Find preclitic with specific Morphosyntactic marker example:");
		puts("        |*,$|*   OR   ;*,$;*");
	}
}

char isEqual(const char *pat, const char *st) {
	if (pat == NULL || st == NULL)
		return(FALSE);
	if (!uS.mStricmp(st, pat))
		return(TRUE);
	return(FALSE);
}

char isnEqual(const char *pat, const char *st, int len) {
	if (pat == NULL || st == NULL)
		return(FALSE);
	if (!uS.mStrnicmp(st, pat, len))
		return(TRUE);
	return(FALSE);
}

char isEqualIxes(const char *pat, IXXS *ixes, int max) {
	int i;

	if (pat == NULL || ixes == NULL)
		return(FALSE);
	for (i=0; i < max; i++) {
		if (ixes[i].ix_s != NULL) {
			if (!uS.mStricmp(ixes[i].ix_s, pat))
				return(TRUE);
		}
	}
	return(FALSE);
}

char isIxesMatchPat(IXXS *ixes, int max, const char *pat) {
	char isMatched;
	int i;

	isMatched = FALSE;
	for (i=0; i < max; i++) {
		if (!isStringEmpty(ixes[i].ix_s)) {
			if (uS.patmat(ixes[i].ix_s, pat)) {
				isMatched = TRUE;
				ixes[i].isIXMatch = TRUE;
			}
		}
	}
	return(isMatched);
}

char isAllv(MORFEATS *feat) {
	if (isEqual("v", feat->pos) ||
		(isEqual("part",feat->pos) &&
		 (isEqualIxes("PASTP",feat->suffix,NUM_SUFF) || isEqualIxes("PRESP",feat->suffix,NUM_SUFF) ||
		  isEqualIxes("PASTP",feat->fusion,NUM_FUSI) || isEqualIxes("PRESP",feat->fusion,NUM_FUSI))))
		return(TRUE);
	return(FALSE);
}

static void checkOmatch(IXXS *ixes, int max, const char *pat) {
	int i;

	for (i=0; i < max; i++) {
		if (!isStringEmpty(ixes[i].ix_s) && ixes[i].isIXMatch == FALSE) {
			if (uS.patmat(ixes[i].ix_s, pat)) {
			}
		}
	}
}

static char isSpecialMorSym(char *sym, char type) {
	if (strcmp(sym, "cm") == 0 || strcmp(sym, "end") == 0 || strcmp(sym, "beg") == 0 ||
		strcmp(sym, "bq") == 0 || strcmp(sym, "eq") == 0)
		return(TRUE);
	else if (type == 'r' && (strcmp(sym, "bq2") == 0 || strcmp(sym, "eq2") == 0))
		return(TRUE);
	return(FALSE);
}

char isMORSearch(void) {
	if (morfptr == NULL)
		return(FALSE);
	else
		return(TRUE);
}

static MORFEATURESLIST *makeMorSearchFeatList(char *wd, char *orgWd, char *isClitic, char *res) {
	char *comma, stem[BUFSIZ], *hyphenI;
	size_t len;
	MORFEATURESLIST *tItem;

	if (*wd != '#' && *wd != '~' && *wd != '$' && *wd != '|' && *wd != 'r' && *wd != ';' && *wd != '-' &&
		*wd != '&' && *wd != '=' && *wd != '@' && *wd != '*' && *wd != 'o') {
		fprintf(stderr, "\n  Illegal symbol \"%c\" used in option \"%s\".\n  Please use one the following symbols:\n", *wd, orgWd);
		morSearchSytaxUsage(NULL);
		if (res != NULL)
			*res = FALSE;
		return(NULL);
	}
	if ((tItem=NEW(MORFEATURESLIST)) == NULL) {
		fprintf(stderr, "Error: out of memory\n");
		return(NULL);
	} else {
		comma = strchr(wd, ',');
		if (comma != NULL)
			*comma = EOS;
		if (*wd == '~') {
			tItem->typeID = '~';
			isPostcliticUse = TRUE;
			*isClitic = TRUE;
			wd++;
		} else if (*wd == '$') {
			tItem->typeID = '$';
			isPrecliticUse = TRUE;
			*isClitic = TRUE;
			wd++;
		} else
			tItem->typeID = ' ';
		tItem->featMatched = NULL;
		if ((tItem->typeID == '~' || tItem->typeID == '$') && *wd == '%') {
			tItem->featType = tItem->typeID;
			tItem->flag = '-';
			len = strlen(wd);
			tItem->pat = (char *)malloc(2);
			if (tItem->pat == NULL) {
				free(tItem);
				if (comma != NULL)
					*comma = ',';
				fprintf(stderr, "Error: out of memory\n");
				return(NULL);
			}
			strcpy(tItem->pat, "%");
		} else {
			if ((tItem->typeID == '~' || tItem->typeID == '$') && *wd == EOS) {
				fprintf(stderr, "\n  Please specify Morphosyntactic marker or '%%' after symbol '%c' in option \"%s\".\n", tItem->typeID, orgWd);
				free(tItem);
				if (comma != NULL)
					*comma = ',';
				if (res != NULL)
					*res = FALSE;
				return(NULL);
			} else if (*wd == EOS) {
				fprintf(stderr, "\n  Please specify all Morphosyntactic marker in option \"%s\".\n", orgWd);
				free(tItem);
				if (comma != NULL)
					*comma = ',';
				if (res != NULL)
					*res = FALSE;
				return(NULL);
			}
			tItem->featType = *wd++;
			if (tItem->featType == ';')
				tItem->featType = 'r';
			if (tItem->featType == '@' || tItem->featType == '*')
				R8 = TRUE;
			if (*wd == '-' || *wd == '+')
				tItem->flag = *wd++;
			else
				tItem->flag = '-';
			len = strlen(wd);
			if (len == 0) {
				fprintf(stderr, "\n  Please specify '*' or '%%' or string after symbol \"%c\" in option \"%s\".\n", tItem->featType, orgWd);
				free(tItem);
				if (comma != NULL)
					*comma = ',';
				if (res != NULL)
					*res = FALSE;
				return(NULL);
			}

// 2019-07-30 change dash to 0x2013
			if (tItem->featType == 'r') {
				strcpy(stem, wd);
				hyphenI = strchr(stem, '-');
				while (hyphenI != NULL) {
//fprintf(stderr, "%s\n", stem);
					uS.shiftright(hyphenI, 2);
					*hyphenI++ = 0xE2;
					*hyphenI++ = 0x80;
					*hyphenI   = 0x93;
					hyphenI = strchr(stem, '-');
				}
				len = strlen(stem);
				wd = stem;
			}

			tItem->pat = (char *)malloc(len+1);
			if (tItem->pat == NULL) {
				free(tItem);
				if (comma != NULL)
					*comma = ',';
				fprintf(stderr, "Error: out of memory\n");
				return(NULL);
			}
			strcpy(tItem->pat, wd);
			if (tItem->featType == 'o' && (tItem->pat[0] == '~' || tItem->pat[0] == '$')) {
				if (tItem->pat[0] == '~') {
					tItem->typeID = '~';
					isPostcliticUse = TRUE;
					*isClitic = TRUE;
				} else {
					tItem->typeID = '$';
					isPrecliticUse = TRUE;
					*isClitic = TRUE;
				}
				tItem->featType = tItem->typeID;
				tItem->flag = '-';
				tItem->pat[0] = '%';
			}
			if (CLAN_PROG_NUM != CORELEX && CLAN_PROG_NUM != KIDEVAL &&
				CLAN_PROG_NUM != IPSYN && CLAN_PROG_NUM != MORTABLE && CLAN_PROG_NUM != FLUCALC) {
				if (!isSearchMorSym) {
					if (isSpecialMorSym(tItem->pat, tItem->featType))
						isSearchMorSym = TRUE;
				}
			}
		}
		tItem->nextFeat = NULL;
		if (comma != NULL) {
			tItem->nextFeat = makeMorSearchFeatList(comma+1, orgWd, isClitic, res);
			if (tItem->nextFeat == NULL) {
				if (tItem->pat != NULL)
					free(tItem->pat);
				free(tItem);
				if (comma != NULL)
					*comma = ',';
				return(NULL);
			}
		}
		if (comma != NULL)
			*comma = ',';
	}
	return(tItem);
}

MORWDLST *makeMorSeachList(MORWDLST *root, char *res, char *wd, char ch) {
	char st[BUFSIZ], colaps[] = "o-%", *wildChr, isFullWildStemFound, isPartWildStemFound;
	char numPref, numPos, numStem, numSuf, numFus, numTran, numRepl, numErr;
	MORWDLST *tItem, *p;
	MORFEATURESLIST *l, *m;

	if (res != NULL)
		*res = TRUE;
	if ((tItem=NEW(MORWDLST)) == NULL) {
		root = freeMorWords(root);
		fprintf(stderr, "Error: out of memory\n");
		return(NULL);
	}
	strncpy(st, wd, BUFSIZ-2);
	st[BUFSIZ-1] = EOS;
	tItem->type = ch;
	tItem->isClitic = FALSE;
	tItem->rootFeat = makeMorSearchFeatList(wd, st, &tItem->isClitic, res);
	tItem->nextMatch = NULL;
	if (tItem->rootFeat == NULL) {
		root = freeMorWords(root);
		return(NULL);
	}
	numPref = 0;
	numPos  = 0;
	numStem = 0;
	numSuf  = 0;
	numFus  = 0;
	numTran = 0;
	numRepl = 0;
	numErr  = 0;
	isFullWildStemFound = FALSE;
	isPartWildStemFound = FALSE;
	for (l=tItem->rootFeat; l != NULL; l=l->nextFeat) {
		if (l->featType == '#') {
			numPref++;
		} else if (l->featType == '|') {
			numPos = 1;
		} else if (l->featType == 'r') {
			if (strcmp(l->pat, "*") == 0)
				isFullWildStemFound = TRUE;
			else if ((wildChr=strchr(l->pat, '*')) != NULL) {
				if (wildChr == l->pat || *(wildChr-1) != '\\')
					isPartWildStemFound = TRUE;
			}
			if (strcmp(l->pat, "*") != 0) {
				if (numStem < 127)
					numStem++;
			}
//			numStem = 1;
		} else if (l->featType == '-') {
			numSuf++;
		} else if (l->featType == '&') {
			numFus++;
		} else if (l->featType == '=') {
			numTran = 1;
		} else if (l->featType == '@') {
			numRepl = 1;
		} else if (l->featType == '*') {
			numErr = 1;
		}
		for (m=l->nextFeat; m != NULL; m=m->nextFeat) {
			if (l->featType == m->featType && m->featType != '|' && m->featType != 'r' && m->featType != '*' &&
				m->featType != '&' && m->featType != '-') {
				fprintf(stderr, "\nPlease do not use the same element twice \"%c-%s\" and \"%c-%s\" in option \"%s\".\n", l->featType, l->pat, m->featType, m->pat, st);
				root = freeMorWords(root);
				if (res != NULL)
					*res = FALSE;
				return(NULL);
			}
		}
	}
	tItem->numPref = numPref;
	tItem->numPos  = numPos;
	if ((isFullWildStemFound && numStem == 0) || (isPartWildStemFound && numStem == 1)) {
		tItem->numStem = 1;
		tItem->isWildStem = TRUE;
	} else if (numStem == 0) {
		tItem->numStem = 0;
		tItem->isWildStem = TRUE;
	} else {
		tItem->numStem = numStem;
		tItem->isWildStem = FALSE;
	}
	tItem->numSuf  = numSuf;
	tItem->numFus  = numFus;
	tItem->numTran = numTran;
	tItem->numRepl = numRepl;
	tItem->numErr  = numErr;
	if (tItem->type != 'i' && tItem->rootFeat != NULL) {
		for (l=tItem->rootFeat; l->nextFeat != NULL; l=l->nextFeat) {
			if (l->featType == 'o')
				break;
		}
		if (l->featType != 'o') {
			l->nextFeat = makeMorSearchFeatList(colaps, st, &tItem->isClitic, res);
			if (l->nextFeat == NULL) {
				root = freeMorWords(root);
				return(NULL);
			}
		}
	}
	if (root == NULL)
		root = tItem;
	else {
		for (p=root; p->nextMatch != NULL; p=p->nextMatch) ;
		p->nextMatch = tItem;
	}
	return(root);
}

static char isPOSFound(char *line) {
	while (*line != EOS && *line != sMarkChr) {
		if (*line == '|')
			return(TRUE);
		line++;
	}
	return(FALSE);
}

static char isRightR8Command(void) {
	if (R8) {
		if (CLAN_PROG_NUM == FREQ || CLAN_PROG_NUM == C_NNLA || CLAN_PROG_NUM == C_QPA || CLAN_PROG_NUM == MORTABLE ||
			CLAN_PROG_NUM == CORELEX || CLAN_PROG_NUM == EVAL || CLAN_PROG_NUM == EVALD || CLAN_PROG_NUM == SUGAR) {
			return(TRUE);
		}
	}
	return(FALSE);
}

void filterMorTier(char *morUtt, char *morLine, char isReplace, char linkTiers) {
	int  i;
	char spaceOut, isDMark;

	if (linkTiers) {
		if (isMORSearch()) {
			isDMark = TRUE;
			for (i=0; morUtt[i] != EOS; i++) {
				if (morUtt[i] == sMarkChr)
					isDMark = FALSE;
				else if (morUtt[i] == dMarkChr)
					isDMark = TRUE;
				if (isDMark && !isPostcliticUse && morUtt[i] == '~' && isPOSFound(morUtt+i+1))
					morUtt[i] = ' ';
				if (isDMark && !isPrecliticUse && morUtt[i] == '$' && isPOSFound(morUtt+i+1))
					morUtt[i] = ' ';
			}
		}
	} else if (isMORSearch() || (R8 && CLAN_PROG_NUM == FREQ)) {
		for (i=0; morUtt[i] != EOS; i++) {
			if (!isPostcliticUse && morUtt[i] == '~')
				morUtt[i] = ' ';
			if (!isPrecliticUse && morUtt[i] == '$')
				morUtt[i] = ' ';
			if (isReplace) {
				if (morUtt[i] == rplChr)
					morUtt[i] = '@';
				else if (morUtt[i] == errChr)
					morUtt[i] = '*';
			}
		}
		if (morLine != NULL) {
			if (isReplace == 1) {
				spaceOut = FALSE;
				for (i=0; morLine[i] != EOS; i++) {
					if (uS.isskip(morLine,i,&dFnt,MBF))
						spaceOut = FALSE;
					if (morLine[i] == rplChr || morLine[i] == errChr)
						spaceOut = TRUE;
					if (spaceOut)
						morLine[i] = ' ';
				}
			} else if (isReplace == 2) {
				for (i=0; morLine[i] != EOS; i++) {
					if (morLine[i] == rplChr)
						morLine[i] ='@';
					else if (morLine[i] == errChr)
						morLine[i] ='*';
				}
			}
		}
	} else if (CLAN_PROG_NUM == FREQ && /* onlydata != 7 && */onlydata != 8) {
		for (i=0; morUtt[i] != EOS; i++) {
			if (!isPostcliticUse && morUtt[i] == '~')
				morUtt[i] = ' ';
			if (!isPrecliticUse && morUtt[i] == '$')
				morUtt[i] = ' ';
		}
	} else if (CLAN_PROG_NUM == COMBO) {
		for (i=0; morUtt[i] != EOS; i++) {
			if (!isPostcliticUse && morUtt[i] == '~')
				morUtt[i] = ' ';
			if (!isPrecliticUse && morUtt[i] == '$')
				morUtt[i] = ' ';
		}
	}
/*
	else if (isRightR8Command()) {
		if (isReplace) {
			for (i=0; morUtt[i] != EOS; i++) {
				if (morUtt[i] == rplChr)
					morUtt[i] = '@';
				else if (morUtt[i] == errChr)
					morUtt[i] = '*';
			}
			if (morLine != NULL) {
				for (i=0; morLine[i] != EOS; i++) {
					if (morLine[i] == rplChr)
						morLine[i] ='@';
					else if (morLine[i] == errChr)
						morLine[i] ='*';
				}
			}
		}
	}
*/
}

char isMorPat(char *f) {
	if (*f == '+' || *f == '~')
		f++;
	if (*f == '#') // prefix
		return(TRUE);
	else if (*f == '~') // postclitic
		return(TRUE);
	else if (*f == '$') // preclitic
		return(TRUE);
	else if (*f == '|') // part of speech
		return(TRUE);
	else if (*f == 'r') {// stem
		f++;
		if (*f == '-' || *f == '+' || *f == '*' || *f == '%')
			return(TRUE);
	} else if (*f == ';') // stem
		return(TRUE);
	else if (*f == '-') // suffix
		return(TRUE);
	else if (*f == '&') // fusion
		return(TRUE);
	else if (*f == '=') // trans
		return(TRUE);
	else if (*f == '@') // repls
		return(TRUE);
	else if (*f == '*') // errors
		return(TRUE);
	else if (*f == 'o') {// other (default *)
		f++;
		if (*f == '-' || *f == '+' || *f == '*' || *f == '%')
			return(TRUE);
	}
	return(FALSE);
}

char isMorSearchOption(char *f, char spc, char ch) {
	if (chatmode == 0)
		return(FALSE);
	if (ch != '-' && ch != '+')
		return(FALSE);
	if (*f != spc)
		return(FALSE);
	f++;
	return(isMorPat(f));
}

static char isMorWord(char c, char isErrReplFound) {
	if (isErrReplFound) {
		if (c != '@' && c != '*' && c != rplChr && c != errChr && c != EOS)
			return(TRUE);			
	} else {
		if (c != '^' && c != '~' && c != '$' && c != '+' && c != '=' && c != '#' && c != '|' && c != '-' && c != '&' &&
			c != '@' && c != '*' && c != rplChr && c != errChr && c != EOS)
			return(TRUE);			
	}
	return(FALSE);
}

static void copyStToIxes(IXXS *to, int max, char *st) {
	int i;

	for (i=0; i < max; i++) {
		if (to[i].ix_s == NULL) {
			to[i].ix_s = (char *)malloc(strlen(st)+1);
			if (to[i].ix_s != NULL) {
				to[i].free_ix_s = TRUE;
				strcpy(to[i].ix_s, st);
			}
			return;
		}
	}
}

static void copyIxes(IXXS *to, IXXS *from, int max) {
	int i;

	for (i=0; i < max; i++) {
		if (from[i].ix_s != NULL) {
			copyStToIxes(to, max, from[i].ix_s);
		}
	}
}

static void copyCompoundElems(MORFEATS *root) {
	MORFEATS *elem;

	for (elem=root->compd; elem != NULL; elem=elem->compd) {
		if (elem->type == NULL || elem->typeID != '+') 
			break;
		copyIxes(root->prefix, elem->prefix, NUM_PREF);
		copyIxes(root->suffix, elem->suffix, NUM_SUFF);
		copyIxes(root->fusion, elem->fusion, NUM_FUSI);
		if (elem->trans != NULL && root->trans == NULL) {
			root->trans = (char *)malloc(strlen(elem->trans)+1);
			if (root->trans != NULL) {
				root->free_trans = TRUE;
				strcpy(root->trans, elem->trans);
			}
		}
		if (elem->repls != NULL && root->repls == NULL) {
			root->repls = (char *)malloc(strlen(elem->repls)+1);
			if (root->repls != NULL) {
				root->free_repls = TRUE;
				strcpy(root->repls, elem->repls);
			}
		}
		copyIxes(root->error, elem->error, NUM_ERRS);
	}
}

void addIxesToSt(char *item, IXXS *ixes, int max, const char *sym, char isAddFront) {
	int i;

	for (i=0; i < max; i++) {
		if (!isStringEmpty(ixes[i].ix_s)) {
			if (isAddFront && sym != NULL)
				strcat(item, sym);
			strcat(item, ixes[i].ix_s);
			if (!isAddFront && sym != NULL)
				strcat(item, sym);
		}
	}
}

static void initIXESMatch(IXXS *ixes, int max) {
	int i;

	for (i=0; i < max; i++) {
		ixes[i].isIXMatch = FALSE;
	}
}

static void resetWordMatch(MORFEATS *word_feat) {
	initIXESMatch(word_feat->prefix, NUM_PREF);
	word_feat->isPosMatch = FALSE;
	word_feat->isStemMatch = FALSE;
	initIXESMatch(word_feat->suffix, NUM_SUFF);
	initIXESMatch(word_feat->fusion, NUM_FUSI);
	word_feat->isTransMatch = FALSE;
	word_feat->isReplsMatch = FALSE;
	initIXESMatch(word_feat->error, NUM_ERRS);
}

static void initIxes(IXXS *ixes, int max) {
	int i;

	for (i=0; i < max; i++) {
		ixes[i].free_ix_s = FALSE;
		ixes[i].ix_s = NULL;
	}
}

static void blankIXESString(IXXS *ixes, int max, char isCheck) {
	int  i;
	char *c;

	for (i=0; i < max; i++) {
		if (!isStringEmpty(ixes[i].ix_s) && (!isCheck || ixes[i].isIXMatch == FALSE)) {
			for (c=ixes[i].ix_s; *c != EOS; c++)
				*c = ' ';
		}
	}
}

static void asignStToIxes(IXXS *to, int max, char *st) {
	int i;

	for (i=0; i < max; i++) {
		if (to[i].ix_s == NULL) {
			to[i].ix_s = st;
			return;
		}
	}
}

static char ParseMorElems(char *item, MORFEATS *word_feats) {
	char *c, type, t, isErrReplFound;

	initIxes(word_feats->prefix, NUM_PREF);
	word_feats->pos     = NULL;
	word_feats->stem    = NULL;
	initIxes(word_feats->suffix, NUM_SUFF);
	initIxes(word_feats->fusion, NUM_FUSI);
	word_feats->free_trans   = FALSE;
	word_feats->trans   = NULL;
	word_feats->free_repls   = FALSE;
	word_feats->repls   = NULL;
	initIxes(word_feats->error, NUM_ERRS);
	word_feats->compd   = NULL;
	resetWordMatch(word_feats);
	for (c=item; *c != EOS && *c != '+' && *c != '*' && *c != '@' && *c != errChr && *c != rplChr; c++) ;
	if (*c == '+') {
		*c = EOS;
		word_feats->compd = NEW(MORFEATS);
		if (word_feats->compd != NULL) {
			word_feats->compd->clitc = NULL;
			word_feats->compd->type = c;
			word_feats->compd->typeID = '+';
			if (ParseMorElems(c+1, word_feats->compd) == FALSE)
				return(FALSE);
		} else {
			return(FALSE);
		}
	}
	for (c=item; *c != EOS && *c != '#' && *c != '*' && *c != '@' && *c != errChr && *c != rplChr; c++) ;
	while (*c == '#') {
		*c = EOS;
		asignStToIxes(word_feats->prefix, NUM_PREF, item);
		item = c + 1;
		for (c=item; *c!=EOS && *c!='#' && *c!='|' && *c!='*' && *c!='@' && *c!=errChr && *c!=rplChr; c++) ;
	}
	for (c=item; *c != EOS && *c != '|' && *c != '*' && *c != '@' && *c != errChr && *c != rplChr; c++) ;
	if (*c == '|') {
		*c = EOS;
		word_feats->pos = item;
		item = c + 1;
	}
	isErrReplFound = FALSE;
	for (c=item; isMorWord(*c, isErrReplFound); c++) {
		if (*c == '*' || *c == '@' || *c == errChr || *c == rplChr)
			isErrReplFound = TRUE;
	}
	type = *c;
	*c = EOS;
	word_feats->stem = item;
	item = c + 1;

	while (type != EOS) {
		while (type == '\n' || isSpace(type))
			type = *item++;
		if (type == '*' || type == '@' || type == errChr || type == rplChr)
			isErrReplFound = TRUE;
		for (c=item; isMorWord(*c, isErrReplFound); c++) ;
		if ((type == '@' && *c == '@') || (type == rplChr && *c == rplChr)) {
			for (c++; isMorWord(*c, isErrReplFound); c++) ;
		}
		t = *c;
		*c = EOS;
		if (type == '-') {
			asignStToIxes(word_feats->suffix, NUM_SUFF, item);
		} else if (type == '&') {
			asignStToIxes(word_feats->fusion, NUM_FUSI, item);
		} else if (type == '=') {
			word_feats->trans = item;
		} else if (type == '@' || type == rplChr) {
			word_feats->repls = item;
		} else if (type == '*' || type == errChr) {
			asignStToIxes(word_feats->error, NUM_ERRS, item);
		} else {
			for (; *item != EOS; item++)
				*item = ' ';
		}
		type = t;
		item = c + 1;
	}
	if (word_feats->typeID == 'R' && word_feats->stem != NULL && word_feats->compd != NULL) {
		if (word_feats->stem[0] == EOS && word_feats->compd->type != NULL && word_feats->compd->typeID == '+') {
			copyCompoundElems(word_feats);
		}
	}
	return(TRUE);
}

char ParseWordMorElems(char *item, MORFEATS *word_feats) {
	char *ab, *ae, at, *cb, *ce, b, e;
	MORFEATS *word_clitc;

	if (item[0] == '^')
		strcpy(item, item+1);
	word_feats->type = NULL;
	word_feats->typeID = 'R';
	word_feats->clitc = NULL;
	ab = item;
	while (*ab != EOS) {
		for (ae=ab; *ae != EOS && *ae != '^'; ae++) ;
		at = *ae;
		*ae = EOS;
		cb = ab;
		b = '\0';
		while (*cb != EOS) {
			for (ce=cb; *ce != EOS && *ce != '$' && *ce != '~'; ce++) ;
			e = *ce;
			*ce = EOS;
			if (b == '~' || e == '$') {
				if (word_feats->clitc == NULL) {
					word_feats->clitc = NEW(MORFEATS);
					word_clitc = word_feats->clitc;
				} else {
					for (word_clitc=word_feats->clitc; word_clitc->clitc != NULL; word_clitc=word_clitc->clitc) ;
					word_clitc->clitc = NEW(MORFEATS);
					word_clitc = word_clitc->clitc;
				}
				if (word_clitc == NULL)
					return(FALSE);
				word_clitc->clitc = NULL;
				word_clitc->type = ce;
				if (e == '$')
					word_clitc->typeID = '$';
				else
					word_clitc->typeID = '~';
				if (ParseMorElems(cb, word_clitc) == FALSE)
					return(FALSE);
			} else {
				if (ParseMorElems(cb, word_feats) == FALSE)
					return(FALSE);
			}
			if (e == '$' || e == '~') {
				b = e;
				cb = ce + 1;
			} else
				break;
		}
		if (at == '^') {
			ab = ae + 1;
			if (word_feats->clitc == NULL) {
				word_feats->clitc = NEW(MORFEATS);
				word_clitc = word_feats->clitc;
			} else {
				for (word_clitc=word_feats->clitc; word_clitc->clitc != NULL; word_clitc=word_clitc->clitc) ;
				word_clitc->clitc = NEW(MORFEATS);
				word_clitc = word_clitc->clitc;
			}
			if (word_clitc == NULL)
				return(FALSE);
			word_clitc->clitc = NULL;
			word_clitc->type = ae;
			word_clitc->typeID = '^';
			word_feats = word_clitc;
		} else
			break;
	}
	return(TRUE);
}

char matchToMorElems(MORWDLST *pat_rootFeat, MORFEATS *word_feats, char skipCompound, char skipClitic, char isClean) {
	char *c, isMatched, isPreCMatched, isPostCMatched, isWildStem;
	char numPref, numPos, numStem, numSuf, numFus, numTran, numRepl, numErr, fPos, fStem;
	char isWordHasPrefix, isWordHasSuffix, isWordHasFusion, isWordHasTrans, isWordHasRepls, isWordHasError;
	char isCliticPat, isTryMatch;
	MORFEATS *word_clitic, *word_feat;
	MORFEATURESLIST *pat_feats, *pat_feat;

	pat_feats = pat_rootFeat->rootFeat;
	isCliticPat = pat_rootFeat->isClitic;
	isWordHasPrefix = FALSE;
	isWordHasSuffix = FALSE;
	isWordHasFusion = FALSE;
	isWordHasTrans = FALSE;
	isWordHasRepls = FALSE;
	isWordHasError = FALSE;
	for (word_clitic=word_feats; word_clitic != NULL; word_clitic=word_clitic->clitc) {
		for (word_feat=word_clitic; word_feat != NULL; word_feat=word_feat->compd) {
			resetWordMatch(word_feat);
			if (word_feat->prefix[0].ix_s != NULL)
				isWordHasPrefix = TRUE;
			if (word_feat->suffix[0].ix_s != NULL)
				isWordHasSuffix = TRUE;
			if (word_feat->fusion[0].ix_s != NULL)
				isWordHasFusion = TRUE;
			if (word_feat->trans != NULL)
				isWordHasTrans = TRUE;
			if (word_feat->repls != NULL)
				isWordHasRepls = TRUE;
			if (word_feat->error[0].ix_s != NULL)
				isWordHasError = TRUE;

		}
	}
	if (isCliticPat) {
		isMatched      = TRUE;
		isPreCMatched  = TRUE;
		isPostCMatched = TRUE;
		fPos = 1;
		fStem = 1;
	} else {
		isMatched = FALSE;
		isPreCMatched  = FALSE;
		isPostCMatched = FALSE;
		fPos = 0;
		fStem = 0;
	}
	for (pat_feat=pat_feats; pat_feat != NULL; pat_feat=pat_feat->nextFeat) {
		pat_feat->featMatched = NULL;
		if (isCliticPat) {
			if (pat_feat->typeID == '~') {
				if (pat_feat->featType != '~' || strcmp(pat_feat->pat, "%"))
					isPostCMatched = FALSE;
			} else if (pat_feat->typeID == '$') {
				if (pat_feat->featType != '$' || strcmp(pat_feat->pat, "%"))
					isPreCMatched = FALSE;
			} else {
				if (pat_feat->featType == '|')
					fPos = 0;
				else if (pat_feat->featType == 'r')
					fStem = 0;
				isMatched = FALSE;
			}
		}
		for (word_clitic=word_feats; word_clitic != NULL; word_clitic=word_clitic->clitc) {
			for (word_feat=word_clitic; word_feat != NULL; word_feat=word_feat->compd) {
				if ((!R7Slash || !R7Tilda || !R7Caret || !R7Colon) && word_feat->typeID == '^')
					break;
				if (pat_feat->featType == '=') {
					if (word_feat->trans != NULL) {
						if (uS.patmat(word_feat->trans, pat_feat->pat)) {
							pat_feat->featMatched = word_clitic;
							word_feat->isTransMatch = TRUE;
						}
					}
				} else if (pat_feat->featType == '@') {
					if (word_feat->repls != NULL) {
						if (uS.patmat(word_feat->repls, pat_feat->pat)) {
							pat_feat->featMatched = word_feats;
							word_feat->isReplsMatch = TRUE;
						}
					}
				} else if (pat_feat->featType == '*') {
					if (isIxesMatchPat(word_feat->error, NUM_ERRS, pat_feat->pat)) {
						pat_feat->featMatched = word_feats;
					}
				}
			}
		}
	}
	isWildStem = pat_rootFeat->isWildStem;
	for (word_clitic=word_feats; word_clitic != NULL; word_clitic=word_clitic->clitc) {
		if ((!R7Slash || !R7Tilda || !R7Caret || !R7Colon) && word_clitic->typeID == '^')
			break;
		if (!isWildStem) {
			numStem = 0;
			for (word_feat=word_clitic; word_feat != NULL; word_feat=word_feat->compd) {
				if (word_feat->stem != NULL && word_feat->stem[0] != EOS && numStem < 127)
					numStem++;
			}
			if (pat_rootFeat->numStem > numStem)
				numStem = pat_rootFeat->numStem;
		}
		if (skipClitic && word_clitic->type != NULL && (word_clitic->typeID == '~' || word_clitic->typeID == '$'))
			continue;
		for (word_feat=word_clitic; word_feat != NULL; word_feat=word_feat->compd) {
			if (skipCompound && word_feat->type != NULL && word_feat->typeID == '+')
				continue;
			for (pat_feat=pat_feats; pat_feat != NULL; pat_feat=pat_feat->nextFeat) {
				if (isCliticPat) {
					if ((word_feat->typeID == '~' || word_feat->typeID == '$') && word_feat->typeID == pat_feat->typeID)
						isTryMatch = TRUE;
					else if (word_feat->typeID != '~' && word_feat->typeID != '$' && pat_feat->typeID != '~' && pat_feat->typeID != '$')
						isTryMatch = TRUE;
					else
						isTryMatch = FALSE;
				} else
					isTryMatch = TRUE;
				if (pat_feat->featType == 'o') {
				} else if (pat_feat->featType == '#') {
//					if (isTryMatch) {
						if (isIxesMatchPat(word_feat->prefix, NUM_PREF, pat_feat->pat)) {
							pat_feat->featMatched = word_feat;
						}
//					}
				} else if (pat_feat->featType == '|') {
					if (isTryMatch) {
						if (word_feat->type != NULL && word_feat->typeID == '+') {
						} else if (word_feat->pos != NULL && uS.patmat(word_feat->pos, pat_feat->pat)) {
							pat_feat->featMatched = word_feat;
							word_feat->isPosMatch = TRUE;
							if (isCliticPat && word_feat->typeID != '~' && word_feat->typeID != '$') {
								fPos = 1;
							}
						}
					}
				} else if (pat_feat->featType == 'r') {
					if (isTryMatch) {
						if (word_feat->stem != NULL &&
							(uS.patmat(word_feat->stem,pat_feat->pat) || (word_feat->stem[0]==EOS && !strcmp(pat_feat->pat,"*")))) {
							pat_feat->featMatched = word_feat;
							word_feat->isStemMatch = TRUE;
							if (isCliticPat && word_feat->typeID != '~' && word_feat->typeID != '$') {
								fStem = 1;
							}
						}
					}
				} else if (pat_feat->featType == '-') {
//					if (isTryMatch) {
						if (isIxesMatchPat(word_feat->suffix, NUM_SUFF, pat_feat->pat)) {
							pat_feat->featMatched = word_feat;
						}
//					}
				} else if (pat_feat->featType == '&') {
//					if (isTryMatch) {
						if (isIxesMatchPat(word_feat->fusion, NUM_FUSI, pat_feat->pat)) {
							pat_feat->featMatched = word_feat;
						}
//					}
				} else if (isCliticPat) {
					if (((pat_feat->featType == '~' && pat_feat->typeID == '~') && word_feat->typeID == '~') ||
						((pat_feat->featType == '$' && pat_feat->typeID == '$') && word_feat->typeID == '$')) {
						if (isIxesMatchPat(word_feat->prefix, NUM_PREF, pat_feat->pat)) {
							pat_feat->featMatched = word_feat;
						}
						if (!isStringEmpty(word_feat->pos)) {
							if (uS.patmat(word_feat->pos, pat_feat->pat)) {
								pat_feat->featMatched = word_feat;
								word_feat->isPosMatch = TRUE;
							}
						}
						if (!isStringEmpty(word_feat->stem)) {
							if (uS.patmat(word_feat->stem, pat_feat->pat)) {
								pat_feat->featMatched = word_feat;
								word_feat->isStemMatch = TRUE;
							}
						}
						if (isIxesMatchPat(word_feat->suffix, NUM_SUFF, pat_feat->pat)) {
							pat_feat->featMatched = word_feat;
						}
						if (isIxesMatchPat(word_feat->fusion, NUM_FUSI, pat_feat->pat)) {
							pat_feat->featMatched = word_feat;
						}
						if (!isStringEmpty(word_feat->trans)) {
							if (uS.patmat(word_feat->trans, pat_feat->pat)) {
								pat_feat->featMatched = word_feat;
								word_feat->isStemMatch = TRUE;
							}
						}
						if (!isStringEmpty(word_feat->repls)) {
							if (uS.patmat(word_feat->repls, pat_feat->pat)) {
								pat_feat->featMatched = word_feat;
								word_feat->isStemMatch = TRUE;
							}
						}
						if (isIxesMatchPat(word_feat->error, NUM_ERRS, pat_feat->pat)) {
							pat_feat->featMatched = word_feat;
						}
					}
				}
			}
			numPref = pat_rootFeat->numPref;
			numPos  = pat_rootFeat->numPos;
			if (isWildStem)
				numStem = pat_rootFeat->numStem;
			numSuf  = pat_rootFeat->numSuf;
			numFus  = pat_rootFeat->numFus;
			numTran = pat_rootFeat->numTran;
			numRepl = pat_rootFeat->numRepl;
			numErr  = pat_rootFeat->numErr;
			for (pat_feat=pat_feats; pat_feat != NULL; pat_feat=pat_feat->nextFeat) {
				if (pat_feat->featType == '#') {
					if (pat_feat->featMatched == word_feat) {
						if (numPref > 0)
							numPref--;
					} else if (isWordHasPrefix == FALSE && pat_feat->flag == '+')
						numPref = 0;
				} else if (pat_feat->featType == '|') {
					if (pat_feat->featMatched == word_feat) {
						if (numPos > 0)
							numPos--;
					}
				} else if (pat_feat->featType == 'r') {
					if (pat_feat->featMatched == word_feat) {
						if (!isWildStem && strcmp(pat_feat->pat, "*") == 0) {
						} else {
							if (numStem > 0)
								numStem--;
						}
					}
				} else if (pat_feat->featType == '-') {
					if (pat_feat->featMatched == word_feat) {
						if (numSuf > 0)
							numSuf--;
					} else if (isWordHasSuffix == FALSE && pat_feat->flag == '+')
						numSuf = 0;
				} else if (pat_feat->featType == '&') {
					if (pat_feat->featMatched == word_feat) {
						if (numFus > 0)
							numFus--;
					} else if (isWordHasFusion == FALSE && pat_feat->flag == '+')
						numFus = 0;
				} else if (pat_feat->featType == '=') {
					if (pat_feat->featMatched == word_feat) {
						if (numTran > 0)
							numTran--;
					} else if (isWordHasTrans == FALSE && pat_feat->flag == '+')
						numTran = 0;
				} else if (pat_feat->featType == '@') {
					if (pat_feat->featMatched == word_feat) {
						if (numRepl > 0)
							numRepl--;
					} else if (isWordHasRepls == FALSE && pat_feat->flag == '+')
						numRepl = 0;
				} else if (pat_feat->featType == '*') {
					if (pat_feat->featMatched == word_feat) {
						if (numErr > 0)
							numErr--;
					} else if (isWordHasError == FALSE && pat_feat->flag == '+')
						numErr = 0;
				}
				if (pat_feat->featType == 'o') {
/*
					if ((pat_feat->pat[0] == '~' && pat_feat->pat[1] == EOS && word_feat->typeID == '~') ||
						(pat_feat->pat[0] == '$' && pat_feat->pat[1] == EOS && word_feat->typeID == '$')) {
						checkOmatch(word_feat->prefix, NUM_PREF, "%");
						if (!isStringEmpty(word_feat->pos)) {
							if (uS.patmat(word_feat->pos, "%")) {
							}
						}
						if (!isStringEmpty(word_feat->stem)) {
							if (uS.patmat(word_feat->stem, "%")) {
							}
						}
						checkOmatch(word_feat->suffix, NUM_SUFF, "%");
						checkOmatch(word_feat->fusion, NUM_FUSI, "%");
						if (!isStringEmpty(word_feat->trans)) {
							if (uS.patmat(word_feat->trans, "%")) {
							}
						}
						if (!isStringEmpty(word_feat->repls)) {
							if (uS.patmat(word_feat->repls, "%")) {
							}
						}
						checkOmatch(word_feat->error, NUM_ERRS, "%");
					} else {
*/
						checkOmatch(word_feat->prefix, NUM_PREF, pat_feat->pat);
						if (!isStringEmpty(word_feat->pos) && word_feat->isPosMatch == FALSE) {
							if (uS.patmat(word_feat->pos, pat_feat->pat)) {
							}
						}
						if (!isStringEmpty(word_feat->stem) && word_feat->isStemMatch == FALSE) {
							if (uS.patmat(word_feat->stem, pat_feat->pat)) {
							}
						}
						checkOmatch(word_feat->suffix, NUM_SUFF, pat_feat->pat);
						checkOmatch(word_feat->fusion, NUM_FUSI, pat_feat->pat);
						if (!isStringEmpty(word_feat->trans) && word_feat->isTransMatch == FALSE) {
							if (uS.patmat(word_feat->trans, pat_feat->pat)) {
							}
						}
						if (!isStringEmpty(word_feat->repls) && word_feat->isReplsMatch == FALSE) {
							if (uS.patmat(word_feat->repls, pat_feat->pat)) {
							}
						}
						checkOmatch(word_feat->error, NUM_ERRS, pat_feat->pat);
//					}
				}
				pat_feat->featMatched = NULL;
			}
			if (numPref == 0 && numPos == 0 && numStem == 0 && numSuf == 0 && numFus == 0 && numTran == 0 && numRepl == 0 && numErr == 0) {
				if (isCliticPat) {
					if (word_feat->typeID == '~') {
						isPostCMatched = TRUE;
						if (fPos == 1 || fStem == 1)
							isMatched = TRUE;
					} else if (word_feat->typeID == '$') {
						isPreCMatched = TRUE;
						if (fPos == 1 || fStem == 1)
							isMatched = TRUE;
					} else {
						isMatched = TRUE;
					}
				} else {
					isMatched = TRUE;
				}
			} else if (isPrecliticUse || isPostcliticUse) {
			} else if (isClean && word_feat->typeID != '+' && (word_feat->compd == NULL || word_feat->compd->typeID != '+')) {
				blankIXESString(word_feat->prefix, NUM_PREF, FALSE);
				if (word_feat->pos != NULL) {
					for (c=word_feat->pos; *c != EOS; c++)
						*c = ' ';
				}
				if (word_feat->stem != NULL) {
					for (c=word_feat->stem; *c != EOS; c++)
						*c = ' ';
				}
				blankIXESString(word_feat->suffix, NUM_SUFF, FALSE);
				blankIXESString(word_feat->fusion, NUM_FUSI, FALSE);
				if (word_feat->isTransMatch == FALSE && word_feat->trans != NULL) {
					for (c=word_feat->trans; *c != EOS; c++)
						*c = ' ';
				}
				if (word_feat->isReplsMatch == FALSE && word_feat->repls != NULL) {
					for (c=word_feat->repls; *c != EOS; c++)
						*c = ' ';
				}
				blankIXESString(word_feat->error, NUM_ERRS, TRUE);
			}
		}
	}
	if (isCliticPat) {
		if (isMatched && isPreCMatched && isPostCMatched)
			return(TRUE);
		else
			return(FALSE);
	} else
		return(isMatched);
}

static void cleanUpMorExcludeWord(char *word, char *matched) {
	char isFoundTransOrRepls, isErrReplFound;
	long i, j, c;

	isFoundTransOrRepls = FALSE;
	for (i=0; word[i] != EOS; i++) {
		if (!isSpace(matched[i]) && matched[i] != EOS) {
			for (j=i; j >= 0; j--) {
				if ((word[j] == '^' || word[j] == '~' || word[j] == '$') && !isFoundTransOrRepls)
					break;
				if (word[j] == '=' || word[j] == '@' || word[j] == '*')
					isFoundTransOrRepls = TRUE;
				word[j] = ' ';
			}
			for (; word[i] != EOS; i++) {
				if ((word[i]=='^' || word[i]=='~' || word[i]=='$' || word[i]=='=' || word[i]=='@' || word[i]=='*') && !isFoundTransOrRepls)
					break;
				word[i] = ' ';
			}
			if (word[i] == EOS)
				break;
		}
	}
	for (i=0; word[i] != EOS && isSpace(word[i]); i++) ;
	if ((word[i] == '@' || word[i] == '*') && (isSpace(matched[i]) || matched[i] == EOS) && isSpace(matched[i+1])) {
		for (; word[i] != EOS; i++) {
			if (isSpace(matched[i]) || matched[i] == EOS)
				word[i] = ' ';
		}
	}
	for (i=0; word[i] != EOS; ) {
		if (isSpace(word[i]))
			strcpy(word+i, word+i+1);
		else
			i++;
	}
	if (word[0] == '@' || word[0] == '*')
		isErrReplFound = TRUE;
	else
		isErrReplFound = FALSE;
	while (word[0] != EOS && !isMorWord(word[0], isErrReplFound)) {
		if (word[0] == '@' || word[0] == '*')
			isErrReplFound = TRUE;
		strcpy(word, word+1);
	}
	for (c=0; word[c] != EOS && word[c] != '@' && word[c] != '*'; c++) ;
	if (word[c] == '@' || word[c] == '*')
		isErrReplFound = TRUE;
	else
		isErrReplFound = FALSE;
	for (i=strlen(word)-1; i >= 0 && !isMorWord(word[i], isErrReplFound); i--) {
		if (c == i)
			isErrReplFound = FALSE;
		strcpy(word+i, word+i+1);
	}
	isErrReplFound = FALSE;
	for (i=0; word[i] != EOS; ) {
		if (word[i]=='@' || word[i]=='*')
			isErrReplFound = TRUE;
		if ((word[i]=='~' || word[i]=='$' || word[i]=='^' || word[i]=='+' || word[i]=='=' || word[i]=='@' || word[i]=='*') && !isMorWord(word[i+1], isErrReplFound))
			strcpy(word+i, word+i+1);
		else
			i++;
	}
	if (word[0] == '=' || word[0] == '@' || word[0] == '*') {
		while (word[0] != EOS)
			strcpy(word, word+1);
	}
}

static char isMorAllBeforeSpace(char *matched, char *word, long i) {
	while (i >= 0) {
		if (!isSpace(matched[i]) && matched[i] != EOS)
			return(FALSE);
		if (word[i] == '+' || word[i] == '^' || word[i] == '~' || word[i] == '$')
			break;
		i--;
	}
	return(TRUE);
}

static void cleanUpMorWord(char *word, char *matched) {
	char isErrReplFound;
	long i, c;

	isErrReplFound = FALSE;
	for (i=0; word[i] != EOS; i++) {
		if (word[i]=='@' || word[i]=='*')
			isErrReplFound = TRUE;
		if (isSpace(matched[i]) || matched[i] == EOS) {
			if (isMorWord(word[i], isErrReplFound))
				word[i] = ' ';
			else if ((word[i] == '#' || word[i] == '|') && isMorAllBeforeSpace(matched, word, i-1))
				word[i] = ' ';
			else if (matched[i] == EOS && word[i] == '|' && matched[i+1]==EOS && word[i+1] == '+')
				;
			else if (word[i]!='+' && word[i]!='^' && word[i]!='~' && word[i]!='$' && word[i]!='#' && (isSpace(matched[i+1]) || matched[i+1]==EOS))
				word[i] = ' ';
		}
	}
	for (i=0; word[i] != EOS; ) {
		if (isSpace(word[i]))
			strcpy(word+i, word+i+1);
		else
			i++;
	}
	if (word[0] == '@' || word[0] == '*')
		isErrReplFound = TRUE;
	else
		isErrReplFound = FALSE;
	while (word[0] != EOS && !isMorWord(word[0], isErrReplFound)) {
		if (word[0] == '@' || word[0] == '*')
			isErrReplFound = TRUE;
		strcpy(word, word+1);
	}
	for (c=0; word[c] != EOS && word[c] != '@' && word[c] != '*'; c++) ;
	if (word[c] == '@' || word[c] == '*')
		isErrReplFound = TRUE;
	else
		isErrReplFound = FALSE;
	for (i=strlen(word)-1; i >= 0 && !isMorWord(word[i], isErrReplFound); i--) {
		if (c == i)
			isErrReplFound = FALSE;
		strcpy(word+i, word+i+1);
	}
	isErrReplFound = FALSE;
	for (i=0; word[i] != EOS; ) {
		if (word[i]=='@' || word[i]=='*')
			isErrReplFound = TRUE;
		if ((word[i] == '~' || word[i] == '$' || word[i] == '^' || word[i] == '+') && !isMorWord(word[i+1], isErrReplFound))
			strcpy(word+i, word+i+1);
		else
			i++;
	}
}

char isWordFromMORTier(char *word) {
	char *s;

	s = strchr(word, '|');
	if (s != NULL) {
		if (!isdigit(word[0]) || isWordFromGRATier(word) == FALSE)
			return(TRUE);
	}
	return(FALSE);
}

void combineMainDepWords(char *line, char isColor) {
	int  i;
	char isErrorStarted;

	if (isColor) {
		isErrorStarted = FALSE;
		for (i=0; line[i] != EOS; i++) {
			if (line[i] == dMarkChr) {
				if (isErrorStarted) {
					line[i-1] = ' ';
					line[i]   = (char)ATTMARKER;
					line[i+1] = (char)error_end;
				} else {
					strcpy(line+i, line+i+2);
				}
				isErrorStarted = FALSE;
			} else if (line[i] == sMarkChr) {
				line[i-1] = (char)'-';
				line[i]   = (char)ATTMARKER;
				line[i+1] = (char)error_start;
				isErrorStarted = TRUE;
			}
		}
		if (isErrorStarted) {
			line[i++]   = (char)ATTMARKER;
			line[i++] = (char)error_end;
			line[i] = EOS;
		}
	} else {
		for (i=0; line[i] != EOS; i++) {
			if (line[i] == dMarkChr) {
				strcpy(line+i, line+i+2);
			} else if (line[i] == sMarkChr) {
				line[i-1] = (char)0xE2;
				line[i]   = (char)0x87;
				line[i+1] = (char)0x94;
			}
		}
	}
}

void removeBlankPairs(char *line) {
	int b, e;
	char dataFound;

	if (strchr(line, dMarkChr) == NULL)
		return;
	b = 0;
	while (line[b] != EOS) {
		if (line[b] == dMarkChr) {
			dataFound = FALSE;
			for (e=b+1; line[e] != EOS && line[e] != dMarkChr; e++) {
				if (!isSpace(line[e]) && line[e] != '-' && line[e] != sMarkChr)
					dataFound = TRUE;
			}
			if (!dataFound) {
				for (; b < e; b++)
					line[b] = ' ';
			} else
				b = e;
		} else
			b++;
	}
}

void removeMainTierWords(char *line) {
	int b, e, t;
	char dataFound;

	if (strchr(line, dMarkChr) == NULL)
		return;
	for (b=0; isSpace(line[b]); b++) ;
	while (line[b] != EOS) {
		if (line[b] == dMarkChr) {
			for (e=b+1; line[e] != EOS && line[e] != sMarkChr; e++) ;
			if (line[e] == sMarkChr) {
				t = e;
				dataFound = FALSE;
				for (e++; line[e] != EOS && line[e] != dMarkChr; e++) {
					if (!isSpace(line[e]))
						dataFound = TRUE;
				}
				line[b] = ' ';
				if (!dataFound) {
					t = b + 1;
				}
				for (b=t; b < e; b++)
					line[b] = ' ';
			} else {
				line[b] = ' ';
				b = e;
			}
		} else
			b++;
	}
}

void removeDepTierItems(char *line) {
	int  b, e;
	char dataFound;

	if (strchr(line, dMarkChr) == NULL)
		return;
	for (b=0; isSpace(line[b]); b++) ;
	while (line[b] != EOS) {
		if (line[b] == dMarkChr) {
			dataFound = FALSE;
			for (e=b+1; line[e] != EOS && line[e] != sMarkChr; e++) {
				if (!isSpace(line[e]))
					dataFound = TRUE;
			}
			if (!dataFound) {
				for (; line[e] != EOS && line[e] != dMarkChr; e++) ;
			}
			if (line[e] == sMarkChr)
				e += 1;
			for (; b < e; b++)
				line[b] = ' ';
		} else
			b++;
	}
}

int getNextDepTierPair(char *line, char *morItem, char *spWord, int *wi, int i) {
	int j, k;

	morItem[0] = EOS;
	spWord[0] = EOS;
	for (; isSpace(line[i]) || line[i] == '\n'; i++) ;
	if (line[i] == EOS)
		return(0);
	if (line[i] == dMarkChr) {
		i++;
		for (; isSpace(line[i]) || line[i] == '\n'; i++) ;
	}
	if (line[i] == EOS)
		return(0);
	if (wi != NULL)
		*wi = i;
	k = 0;
	for (; line[i] != sMarkChr && !isSpace(line[i]) && line[i] != '\n' && line[i] != EOS; i++) {
		morItem[k++] = line[i];
	}
	morItem[k] = EOS;
	uS.remFrontAndBackBlanks(morItem);
	for (; isSpace(line[i]) || line[i] == '\n'; i++) ;
	if (line[i] == EOS)
		return(i);
	j = i + 1;
	if (line[i] != sMarkChr) {
		for (; line[j] != sMarkChr && line[j] != EOS; j++) ;
		if (line[j] == sMarkChr)
			j++;
	}
	k = 0;
	for (; line[j] != dMarkChr && line[j] != EOS; j++) {
		spWord[k++] = line[j];
	}
	spWord[k] = EOS;
	uS.remFrontAndBackBlanks(spWord);
	if (line[i] == sMarkChr)
		i = j;
	return(i);
}
/* 2015-06-27 ["] <\">
static int LinkQuoteMorScope(char *wline, int pos, char isFirst) {
	int tPos;
	char isLetFound;

	if (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) && pos >= 0) {
		while (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) && pos >= 0) {
			if (isFirst)
				wline[pos] = ' ';
			pos--;
		}
		if (uS.isRightChar(wline, pos, '[', &dFnt, MBF)) {
			if (isFirst)
				wline[pos] = ' ';
		}
	}
	if (pos < 0) {
		fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
		fprintf(stderr,"Missing '[' character\n");
		fprintf(stderr,"text=%s->%s\n",wline,wline+pos);
		return(-2);
	} else
		pos--;
	while (!uS.isRightChar(wline, pos, '>', &dFnt, MBF) && uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0) {
		if (uS.isRightChar(wline, pos, ']', &dFnt, MBF) || uS.isRightChar(wline, pos, '<', &dFnt, MBF))
			break;
		else
			pos--;
	}
	if (uS.isRightChar(wline, pos, '>', &dFnt, MBF)) {
		isLetFound = FALSE;
		while (!uS.isRightChar(wline, pos, '<', &dFnt, MBF) && pos >= 0) {
			if (uS.isRightChar(wline, pos, ']', &dFnt, MBF))
				pos = LinkQuoteMorScope(wline,pos,FALSE);
			else {
				if (isFirst) {
					if (!uS.isskip(wline,pos,&dFnt,MBF))
						isLetFound = TRUE;
					if (isSpace(wline[pos]) && isLetFound)
						wline[pos] = fSpaceChr;
				}
				pos--;
			}
		}
		if (pos < 0) {
			fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno+tlineno);
			fprintf(stderr,"Missing '<' character\n");
			fprintf(stderr,"text=%s\n",wline);
			return(-2);
		} else {
			tPos = pos;
			if (wline[tPos] == '<')
				tPos++;
			for (; isSpace(wline[tPos]) || wline[tPos] == fSpaceChr; tPos++)
				wline[tPos] = ' ';
		}
	} else if (pos < 0) ;
	else if (uS.isRightChar(wline, pos, ']', &dFnt, MBF)) {
		pos = LinkQuoteMorScope(wline,pos,FALSE);
	} else {
		while (!uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0) {
			if (uS.isRightChar(wline, pos, ']', &dFnt, MBF))
				pos = LinkQuoteMorScope(wline,pos,FALSE);
			else {
				pos--;
			}
		}
	}
	return(pos);
}
*/
static char isQuotes(char *wline, int pos) {
	if (UTF8_IS_LEAD((unsigned char)wline[pos-2])) {
		if (wline[pos-2] == (char)0xE2 && wline[pos-1] == (char)0x80 && wline[pos] == (char)0x9C) {
			return(TRUE);
		} else if (wline[pos-2] == (char)0xE2 && wline[pos-1] == (char)0x80 && wline[pos] == (char)0x9D) {
			return(TRUE);
		}
	}
	return(FALSE);
}

static int isPauseMarker(char *wline, int pos) {
	int t;

	while (pos >= 0 && !uS.isRightChar(wline, pos, '(', &dFnt, MBF))
		pos--;
	if (uS.isRightChar(wline, pos, '(', &dFnt, MBF)) {
		if (uS.isPause(wline, pos, NULL, &t))
			return(pos);
	}
	return(-1);
}

static int ExcludeMORScope(char *wline, int pos, char isblankit, char isReplaceWord) {
	int pauseBeg;

	if (isReplaceWord) {
		if (CLAN_PROG_NUM != FREQ && CLAN_PROG_NUM != FREQPOS)
			isReplaceWord = FALSE;
	}
	if (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) && pos >= 0) {
		while (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) && pos >= 0) {
			if (isblankit) {
				if (isQuotes(wline, pos)) {
					pos -= 2;
				} else
					wline[pos] = ' ';
			}
			pos--;
		}
		if (uS.isRightChar(wline, pos, '[', &dFnt, MBF)) {
			if (isblankit)
				wline[pos] = ' ';
		}
	}
	if (pos < 0) {
		fprintf(stderr,"\rMissing '[' character in file %s\n",oldfname);
		fprintf(stderr,"In tier on line: %ld.\n", lineno);
		fprintf(stderr,"text=%s->%s\n",wline,wline+pos);
		return(-2);
	} else
		pos--;
	while (!uS.isRightChar(wline, pos, '>', &dFnt, MBF) && uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0) {
		if (uS.isRightChar(wline, pos, ']', &dFnt, MBF) || uS.isRightChar(wline, pos, '<', &dFnt, MBF))
			break;
		else
			pos--;
	}
	if (uS.isRightChar(wline, pos, '>', &dFnt, MBF)) {
		while (!uS.isRightChar(wline, pos, '<', &dFnt, MBF) && pos >= 0) {
			if (uS.isRightChar(wline, pos, ']', &dFnt, MBF))
				pos = ExcludeMORScope(wline,pos,isblankit,isReplaceWord);
			else {
				if (isblankit) {
					if (isQuotes(wline, pos)) {
						pos -= 2;
					} else if (isReplaceWord && !uS.isskip(wline,pos,&dFnt,MBF))
						wline[pos] = '\003';
					else
						wline[pos] = ' ';
				}
				pos--;
			}
		}
		if (pos < 0) {
			fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno+tlineno);
			fprintf(stderr,"Missing '<' character\n");
			fprintf(stderr,"text=%s\n",wline);
			return(-2);
		} else if (isblankit) {
			if (isReplaceWord && !uS.isskip(wline,pos,&dFnt,MBF))
				wline[pos] = '\003';
			else
				wline[pos] = ' ';
		}
	} else if (pos < 0) ;
	else if (uS.isRightChar(wline, pos, ']', &dFnt, MBF)) {
		pos = ExcludeMORScope(wline,pos,isblankit,isReplaceWord);
	} else {
		while (!uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0) {
			if (uS.isRightChar(wline, pos, ')', &dFnt, MBF)) {
				pauseBeg = isPauseMarker(wline, pos);
				if (pauseBeg != -1) {
					pos = pauseBeg - 1;
					while (uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0)
						pos--;
				}
			}
			if (uS.isRightChar(wline, pos, ']', &dFnt, MBF))
				pos = ExcludeMORScope(wline,pos,isblankit,isReplaceWord);
			else {
				if (isblankit) {
					if (isQuotes(wline, pos)) {
						pos -= 2;
					} else if (isReplaceWord && !uS.isskip(wline,pos,&dFnt,MBF))
						wline[pos] = '\003';
					else
						wline[pos] = ' ';
				}
				pos--;
			}
		}
	}
	return(pos);
}

static void getRepls(char *repls, char *errors, int beg, int end, char *wline) {
	int  ri, ei;

	ri = 0;
	repls[ri++] = ' ';
	repls[ri++] = rplChr;
	ei = 0;
	errors[ei++] = ' ';
	while (beg < end) {
		for (; beg < end && isspace(wline[beg]); beg++) ;
		if (beg >= end)
			break;
		if (wline[beg] == '[' && wline[beg+1] == '*' && isSpace(wline[beg+2])) {
			errors[ei++] = errChr;
			for (beg=beg+2; beg < end && wline[beg] != ']'; beg++) {
				if (wline[beg] == (char)0xE2 && wline[beg+1] == (char)0x80 && 
					(wline[beg+2] == (char)0x9C || wline[beg+2] == (char)0x9D)) {
						beg += 2;
					} else if (wline[beg] == ',' && !isdigit(wline[beg+1])) {
					} else if (!isSpace(wline[beg]))
					errors[ei++] = wline[beg];
			}
			if (beg < end)
				beg++;
		} else if (wline[beg] == '[') {
			for (beg++; beg < end && wline[beg] != ']'; beg++) ;
			if (beg < end)
				beg++;
		} else if (wline[beg] == '&' || wline[beg] == '0' || wline[beg] == '+' || wline[beg] == '-') {
			for (; beg < end && !isspace(wline[beg]); beg++) ;
		} else if (!isspace(wline[beg]) && wline[beg] != '<' && wline[beg] != '>') {
			if (ri > 2) {
				repls[ri++] = (char)0xe2;
				repls[ri++] = (char)0x80;
				repls[ri++] = (char)0xaf;
			}
			for (; beg < end && !isspace(wline[beg]); beg++) {
				if (wline[beg] == (char)0xE2 && wline[beg+1] == (char)0x80 && 
					(wline[beg+2] == (char)0x9C || wline[beg+2] == (char)0x9D)) {
						beg += 2;
				} else if (wline[beg] == ',' && !isdigit(wline[beg+1])) {
				} else
					repls[ri++] = wline[beg];
			}
		} else
			beg++;
	}
	repls[ri] = EOS;
	errors[ei] = EOS;
}

static void addRepls(char *repls, int beg, int end, char *wline) {
	int i, j;

	while (beg < end) {
		for (; beg < end && isspace(wline[end]); end--) ;
		if (beg < end && !isspace(wline[end])) {
			uS.shiftright(wline+end+1, strlen(repls));
			for (i=end+1,j=0; repls[j] != EOS; i++, j++)
				wline[i] = repls[j];
			for (; beg < end && !isspace(wline[end]); end--) ;
		}
	}
}

static int countWords(char *wline, int beg, int end) {
	int  cnt;
	char sq, counted;

	cnt = 0;
	counted = FALSE;
	if (uS.isRightChar(wline, beg, '[', &dFnt, MBF))
		sq = TRUE;
	else
		sq = FALSE;
	for (; beg < end; beg++) {
		if (uS.isRightChar(wline, beg, '[', &dFnt, MBF))
			sq = TRUE;
		if (!uS.isskip(wline,beg,&dFnt,MBF) && !counted && !sq) {
			cnt++;
			counted = TRUE;
		}
		if (isSpace(wline[beg]) && !sq)
			counted = FALSE;
		if (uS.isRightChar(wline, beg, ']', &dFnt, MBF))
			sq = FALSE;
	}
	return(cnt);
}

static IEWORDS *InsertCASearch(IEWORDS *CAptr, char *str) {
	IEWORDS *t;

	if ((t=NEW(IEWORDS)) == NULL) {
		fprintf(stderr,"No more space left in core.\n");
		cutt_exit(1);
	}			
	t->word = (char *)malloc(strlen(str)+1);
	if (t->word == NULL) {
		fprintf(stderr,"No more space left in core.\n");
		cutt_exit(1);
	}
	strcpy(t->word, str);
	t->nextword = CAptr;
	CAptr = t;
	return(CAptr);
}

void IsSearchCA(char *wline) {
	char CAc[20];
	int cur, end, matchType;

	cur = 0;
	while (wline[cur]) {
		if ((end=uS.HandleCAChars(wline+cur, &matchType)) != 0) {
			if (matchType == NOTCA_LEFT_ARROW_CIRCLE) {
				isSpecRepeatArrow = TRUE;
			} else if (matchType == NOTCA_OPEN_QUOTE) { // 2019-07-17
				if (wline[end] != EOS) {
					if (cutt_isSearchForQuotedItems == 0)
						cutt_isSearchForQuotedItems = 1;
				}
			} else if (matchType == NOTCA_CLOSE_QUOTE) { // 2019-07-17
				if (cur > 0 && wline[cur-1] != '+' && wline[cur-1] != '\\' ) {
					if (cutt_isSearchForQuotedItems == 0)
						cutt_isSearchForQuotedItems = 1;
					else if (cutt_isSearchForQuotedItems == 1)
						cutt_isSearchForQuotedItems = 2;
				}
			}
			strncpy(CAc, wline+cur, end);
			CAc[end] = EOS;
			CAptr = InsertCASearch(CAptr, CAc);
			cur += end;
		} else
			cur++;
	}
}

static int isCAsearched(char *word) {
	IEWORDS *twd;

	for (twd=CAptr; twd != NULL; twd = twd->nextword) {
		if (strcmp(word, twd->word) == 0) {
			return(TRUE);
		}
	}
	return(FALSE);
}

static void removeRepeatSegments(char *wline, int beg, int len) {
	int i;

	for (i=beg+1; wline[i] != EOS; i++) {
		if (wline[i] == (char)0xE2 && wline[i+1] == (char)0x86 && wline[i+2] == (char)0xAB)
			break;
	}
	if (wline[i] == EOS) {
		strcpy(wline+beg,wline+beg+len);
	} else {
		strcpy(wline+beg,wline+i+3);
	}
}

int isMorExcludePlus(char *w) {
//	if (w[0] == '+') {
	if ((w[1] == '"' || w[1] == '^' || w[1] == ',' || w[1] == '+' || w[1] == '<') && w[2] == EOS)
		return(TRUE);
	if (w[1] == EOS)
		return(TRUE);
//	}
	return(FALSE);
}

/* excludeMorTierMorDef(word) determines if %mor: item only "word" is to be included in
 the analyses or not by compairing the "word" to the list of words in "wdptr". It
 returns 1 if the word should be included, and 0 otherwise.
 */
static int excludeMorTierMorDef(char *word) {
	IEWORDS *twd;

	for (twd=defwdptr; twd != NULL; twd = twd->nextword) {
		if (strchr(twd->word, '|') != NULL && uS.patmat(word, twd->word)) {
			if (word[0] != '-') {
				if (uS.patmat(word, "cm|*") || uS.patmat(word, "end|*") || uS.patmat(word, "beg|*")
//2019-04-29					|| uS.patmat(word, "bq|*") || uS.patmat(word, "eq|*")
					) {
				} else
					return(FALSE);
			} else if (uS.isToneUnitMarker(word)) {
				return(FALSE);
			}
		}
	}
	if (word[0] == '&') {
		for (twd=defwdptr; twd != NULL; twd = twd->nextword) {
			if (uS.patmat(word, twd->word)) {
				if (word[0] != '-')
					return(FALSE);
				else if (uS.isToneUnitMarker(word))
					return(FALSE);
			}
		}
		for (twd=wdptr; twd != NULL; twd=twd->nextword) {
			if (uS.patmat(word, twd->word)) {
				if (WordMode == 'i' || WordMode == 'I') {
					return(TRUE);
				} else
					return(FALSE);
			}
		}
		return(FALSE);
	}
	if (uS.mStricmp(word, "unk|xxx") == 0 || uS.mStricmp(word, "unk|yyy") == 0 || uS.patmat(word, "*|www") ||
		uS.patmat(word, "*|xx") || uS.patmat(word, "*|yy")) {
		return(FALSE);
	} else if (word[0] == '+') {
		if (isMorExcludePlus(word))
			return(FALSE);
	}
	return(TRUE);
}

/* excludeSpTierMorDef(word) determines if %mor: item only "word" is to be included in
 the analyses or not by compairing the "word" to the list of words in "wdptr". It
 returns 1 if the word should be included, and 0 otherwise.
 */
static int excludeSpTierMorDef(char *word) {
	IEWORDS *twd;

	if (uS.mStricmp(word, "xx") == 0 || uS.mStricmp(word, "xxx") == 0 || strcmp(word, "0") == 0 ||
		uS.mStricmp(word, "yy") == 0 || uS.mStricmp(word, "yyy") == 0 || uS.mStricmp(word, "www") == 0 ||
		uS.mStrnicmp(word,"xxx@s",5) == 0 || uS.mStrnicmp(word,"yyy@s",5) == 0 || uS.mStrnicmp(word,"www@s",5) == 0 ||
		word[0] == '#' || word[0] == '-') {
//		if (isCreateFakeMor && isNextRep()) {
		return(FALSE);
	} else if (uS.patmat(word, "(*.*)")) {
		if (isCreateFakeMor == 0)
			return(FALSE);
	} else if (word[0] == '[') {
		if (isCreateFakeMor == 0) {
			return(FALSE);
		} else if (!strcmp(word, "[/]") || !strcmp(word, "[//]") || !strcmp(word, "[///]") ||
				   !strcmp(word, "[/?]") || !strcmp(word, "[/-]") || !strcmp(word, "[e]")) {
			return(TRUE);
		} else {
			return(FALSE);
		}
	} else if (word[0] == '&') {
		if (isCreateFakeMor) { // 2018-10-22 beg Nan duplicates // lxs
			return(TRUE);
		} else {
			for (twd=defwdptr; twd != NULL; twd = twd->nextword) {
				if (uS.patmat(word, twd->word)) {
					if (word[0] != '-')
						return(FALSE);
					else if (uS.isToneUnitMarker(word))
						return(FALSE);
				}
			}
			for (twd=wdptr; twd != NULL; twd=twd->nextword) {
				if (uS.patmat(word, twd->word)) {
					if (WordMode == 'i' || WordMode == 'I') {
						return(TRUE);
					} else
						return(FALSE);
				}
			}
			return(FALSE);
		}
	} else if (word[0] == '+') {
		if (isMorExcludePlus(word))
			return(FALSE);
	}
	return(TRUE);
}

static void filterMORScop(char *wline, char linkTiers) {
	char repls[BUFSIZ+1], errors[BUFSIZ+1], CAc[20];
	int i, pos, res, lastNonSkip, cur, end, begRepls, sqCnt, matchType, org_word_cnt;

	cur = 0;
	while (wline[cur]) {
		if ((end=uS.HandleCAChars(wline+cur, &matchType)) != 0) {
			if (isRemoveCAChar[matchType] == TRUE &&
				matchType != NOTCA_DOUBLE_COMMA && matchType != NOTCA_VOCATIVE) {
//2019-04-29			   && matchType != NOTCA_OPEN_QUOTE   && matchType != NOTCA_CLOSE_QUOTE) {
				strncpy(CAc, wline+cur, end);
				CAc[end] = EOS;
				if (!isCAsearched(CAc)) {
					if (matchType == NOTCA_LEFT_ARROW_CIRCLE)
						removeRepeatSegments(wline, cur, end);
					else
						strcpy(wline+cur,wline+cur+end);
				} else
					cur += end;
			} else
				cur += end;
			continue;
		}
		cur++;
	}
	pos = strlen(wline) - 1;
	lastNonSkip = pos;
	while (pos >= 0) {
		if (wline[pos] == HIDEN_C) {
			end = pos;
			for (pos--; wline[pos] != HIDEN_C && pos >= 0; pos--) ;
			if (wline[pos] == HIDEN_C && pos >= 0) {
				for (; end >= pos; end--)
					wline[end] = ' ';
			}
		} else if (uS.isRightChar(wline, pos, ']', &dFnt, MBF)) {
			sqCnt = 0;
			end = pos;
			for (pos--; (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) || sqCnt > 0) && pos >= 0; pos--) {
				if (uS.isRightChar(wline, pos, ']', &dFnt, MBF))
					sqCnt++;
				else if (uS.isRightChar(wline, pos, '[', &dFnt, MBF))
					sqCnt--;
			}
			if (pos < 0) {
				if (chatmode) {
					if (!cutt_isBlobFound) {
						fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno+tlineno);
						fprintf(stderr,"Missing '[' character\n");
						fprintf(stderr,"text=%s\n",wline);
					}
					pos = end - 1;
					if (isRecursive || cutt_isCAFound || cutt_isBlobFound)
						continue;
					else
						cutt_exit(1);
				} else
					break;
			}
			wline[end] = EOS;
			uS.isSqCodes(wline+pos+1, templineC3, &dFnt, TRUE);
			wline[end] = ']';
			uS.lowercasestr(templineC3, &dFnt, MBF);
			res = isExcludeScope(templineC3);
			if (!strcmp(templineC3, "/") || !strcmp(templineC3, "//")  || !strcmp(templineC3, "///") ||
				!strcmp(templineC3, "/?") || !strcmp(templineC3, "/-") ||
				!strcmp(templineC3, "e") || !strcmp(templineC3, "E")) {
				if (isCreateFakeMor == 0) {
					ExcludeMORScope(wline, pos, TRUE, FALSE);
					for (; !uS.isRightChar(wline, end, '[', &dFnt, MBF); end--)
						wline[end] = ' ';
					wline[end] = ' ';
					for (; end < lastNonSkip; end++) {
						if (!isSpace(wline[end]) && wline[end] != '\n')
							wline[end] = ' ';
					}
				}
			}
/* 2015-06-27 ["] <\">
			else if (!strcmp(templineC3, "\"") ) {
				LinkQuoteMorScope(wline, pos, TRUE);
				for (; !uS.isRightChar(wline, end, '[', &dFnt, MBF); end--)
					wline[end] = ' ';
				wline[end] = ' ';
				for (; end < lastNonSkip; end++) {
					if (!isSpace(wline[end]) && wline[end] != '\n')
						wline[end] = ' ';
				}
			}
*/
			 else if (linkTiers) {
				if (uS.patmat(templineC3,":: *")) {
					if (!R5_1) {
						for (; !uS.isRightChar(wline, end, '[', &dFnt, MBF); end--)
							wline[end] = ' ';
						wline[end] = ' ';
					} else {
						begRepls = ExcludeMORScope(wline, pos, FALSE, FALSE);
						if (isSpace(wline[begRepls]))
							begRepls++;
						org_word_cnt = countWords(wline, begRepls, pos);
						for (cur=begRepls; cur < pos; cur++)
							wline[cur] = ' ';
						wline[end] = ' ';
						cur = pos;
						lastNonSkip = pos;
						wline[cur++] = ' ';
						if (wline[cur] == ':')
							wline[cur++] = ' ';
						if (wline[cur] == ':')
							wline[cur++] = ' ';
						for (cur=pos; cur < end && isSpace(wline[cur]); cur++) ;
						for (; cur < end && isSpace(wline[end]); end--) ;
						end++;
						i = 0;
						for (; cur < end; cur++) {
							if (isSpace(wline[cur]))
								wline[cur] = fSpaceChr;
							if (i < BUFSIZ)
								repls[i++] = wline[cur];
						}
						repls[i] = EOS;
						if (org_word_cnt > 1) {
							strcpy(wline+begRepls, wline+end);
							i = (org_word_cnt * (strlen(repls) + 1)) - 1;
							pos = begRepls;
							lastNonSkip = pos;
							cur = begRepls;
							uS.shiftright(wline+cur, i);
							for (; org_word_cnt > 0; org_word_cnt--) {
								for (i=0; repls[i] != EOS; i++) {
									wline[cur++] = repls[i];
								}
								if (org_word_cnt > 1)
									wline[cur++] = ' ';
							}
						}
					}
				} else if (uS.patmat(templineC3,": *") || uS.patmat(templineC3,":\\* *") || uS.patmat(templineC3,":=_ *")) {
					if (!R5) {
						begRepls = ExcludeMORScope(wline, pos, FALSE, FALSE);
						if (isSpace(wline[begRepls]))
							begRepls++;
						wline[end] = ' ';
						cur = pos;
						wline[cur++] = ' ';
						if (wline[cur] == ':')
							wline[cur++] = ' ';
						if (wline[cur] == '*')
							wline[cur++] = ' ';
						if (wline[cur] == '=') {
							wline[cur++] = ' ';
							wline[cur++] = ' ';
						}
						lastNonSkip = begRepls;
						org_word_cnt = countWords(wline, pos, end);
						for (; cur < end; cur++)
							wline[cur] = ' ';
						for (cur=begRepls; cur < pos && isSpace(wline[cur]); cur++) ;
						for (; cur < pos && isSpace(wline[pos]); pos--) ;
						pos++;
						i = 0;
						for (; cur < pos; cur++) {
							if (isSpace(wline[cur]))
								wline[cur] = fSpaceChr;
							if (i < BUFSIZ)
								repls[i++] = wline[cur];
						}
						repls[i] = EOS;
						if (org_word_cnt > 1) {
							strcpy(wline+begRepls, wline+end);
							i = (org_word_cnt * (strlen(repls) + 1)) - 1;
							pos = begRepls;
							lastNonSkip = pos;
							cur = begRepls;
							uS.shiftright(wline+cur, i);
							for (; org_word_cnt > 0; org_word_cnt--) {
								for (i=0; repls[i] != EOS; i++) {
									wline[cur++] = repls[i];
								}
								if (org_word_cnt > 1)
									wline[cur++] = ' ';
							}
						}
					} else {
						ExcludeMORScope(wline, pos, TRUE, FALSE);
						wline[end] = ' ';
						cur = pos;
						lastNonSkip = pos;
						wline[cur++] = ' ';
						if (wline[cur] == ':')
							wline[cur++] = ' ';
						if (wline[cur] == '*')
							wline[cur++] = ' ';
						if (wline[cur] == '=') {
							wline[cur++] = ' ';
							wline[cur++] = ' ';
						}
					}
				} else {
					if (res == 0 || res == 100/* || res == 2*/)
						ExcludeMORScope(wline,pos,(char)TRUE, TRUE);
					for (; !uS.isRightChar(wline, end, '[', &dFnt, MBF); end--)
						wline[end] = ' ';
					wline[end] = ' ';
				}
			} else if (R8 && uS.patmat(templineC3,"\\* *-*")) {
/*
				for (; !uS.isRightChar(wline, end, '[', &dFnt, MBF); end--)
					wline[end] = ' ';
				wline[end] = ' ';
*/
				cur = end;
				for (; !uS.isRightChar(wline, cur, '[', &dFnt, MBF); cur--) {
					if (wline[cur] == '-')
						wline[cur] = '_';
				}
				if (pos <= cur) {
					wline[end] = ' ';
					wline[cur++] = ' ';
					while (cur < end) {
						if (wline[cur] == '*')
							wline[cur] = errChr;
						if (isspace(wline[cur])) {
							strcpy(wline+cur, wline+cur+1);
							end--;
						} else
							cur++;
					}
				}
			} else if (R8 && uS.patmat(templineC3,"\\* *")) {
				cur = end;
				for (; !uS.isRightChar(wline, cur, '[', &dFnt, MBF); cur--) ;
				if (pos <= cur) {
					wline[end] = ' ';
					wline[cur++] = ' ';
					while (cur < end) {
						if (wline[cur] == '*')
							wline[cur] = errChr;
						if (isspace(wline[cur])) {
							strcpy(wline+cur, wline+cur+1);
							end--;
						} else
							cur++;
					}
				}
			} else if (R8 && !strcmp(templineC3,"*")) {
				cur = end;
				for (; !uS.isRightChar(wline, cur, '[', &dFnt, MBF); cur--) ;
				if (pos <= cur) {
					wline[end] = '_';
					if (!isSpace(wline[end+1]) && wline[end+1] != EOS && wline[end+1] != '\n' && wline[end+1] != '\r') {
						uS.shiftright(wline+end+1, 1);
						wline[end+1] = ' ';
					}
					wline[cur++] = ' ';
					while (cur < end) {
						if (wline[cur] == '*')
							wline[cur] = errChr;
						if (isspace(wline[cur])) {
							strcpy(wline+cur, wline+cur+1);
							end--;
						} else
							cur++;
					}
				}
			} else if (uS.patmat(templineC3,": *") || uS.patmat(templineC3,":: *") || uS.patmat(templineC3,":\\* *") || uS.patmat(templineC3,":=_ *")) {
				begRepls = ExcludeMORScope(wline,pos,FALSE,FALSE);
				if (begRepls > -2) {
					if (begRepls < 0)
						begRepls = 0;
					cur = end;
					wline[cur] = ' ';
					for (; !uS.isRightChar(wline, cur, '[', &dFnt, MBF); cur--) ;
					lastNonSkip = cur;
					wline[cur] = ' ';
					wline[cur+1] = ' ';
					if (wline[cur+2] == ':')
						wline[cur+2] = ' ';
					if (wline[cur+2] == '*')
						wline[cur+2] = ' ';
					if (wline[cur+2] == '=' && !isSpace(wline[cur+3])) {
						wline[cur+2] = ' ';
						wline[cur+3] = ' ';
					}
					if (pos <= cur) {
						if (uS.patmat(templineC3,":: *")) {
							getRepls(repls, errors, cur, end, wline);
						} else
							getRepls(repls, errors, begRepls, cur, wline);
						templineC3[0] = EOS;
						if (repls[0] != EOS)
							strcat(templineC3, repls);
						if (errors[0] != EOS)
							strcat(templineC3, errors);
						if (templineC3[0] != EOS)
							addRepls(templineC3, cur, end, wline);
					}
					for (; begRepls < cur; begRepls++)
						wline[begRepls] = ' ';
				}
			} else if (uS.patmat(templineC3,"\"")) {
//				ExcludeMORScope(wline,pos,TRUE,FALSE);
				for (; !uS.isRightChar(wline, end, '[', &dFnt, MBF); end--)
					wline[end] = ' ';
				wline[end] = ' ';
			} else {
				if (res == 0 || res == 100/* || res == 2*/)
					ExcludeMORScope(wline,pos,(char)TRUE, TRUE);
				for (; !uS.isRightChar(wline, end, '[', &dFnt, MBF); end--)
					wline[end] = ' ';
				wline[end] = ' ';
			}
		} else {
			if (!uS.isskip(wline,pos,&dFnt,MBF) && !uS.isRightChar(wline,pos,'[',&dFnt,MBF) && !uS.isRightChar(wline,pos,']',&dFnt,MBF))
				lastNonSkip = pos;
			else if (uS.isRightChar(wline,pos,',',&dFnt,MBF))
				lastNonSkip = pos;
			pos--;
		}
	}
/* lxs +s"<\% n>"
	if (ScopMode == 'I') {
		for (pos=0; pos < lastNonSkip; pos++)
			wline[pos] = ' ';
	}
*/
}

static void replaceCRwithSpace(char *st) {
	while (*st != EOS) {
		if (*st == '\n')
			*st = ' ';
		st++;
	}
}

static char isRightMorUttDelim(char *w, char linkTiers) {
	int i;

	if (linkTiers && w[0] == '+') {
		for (i=0; w[i] != EOS; i++) {
			if (uS.IsUtteranceDel(w, i))
				return(TRUE);
		}
	}
	return(FALSE);
}

static void ChangeBackSpace(char *s) {
	for (; *s != EOS; s++) {
		if (*s == fSpaceChr)
			*s = ' ';
	}
}

static void copyFillersToDepTier(char *spTier, char *mor_tier) {
	int  i, j, t, repWord, res;
	char spWord[BUFSIZ], MorWord[BUFSIZ], isMorDone;

	isMorDone = FALSE;
	i = 0;
	j = 0;
	tempTier[0] = EOS;
	do {
		while ((i=getword("*\001", spTier, spWord, NULL, i))) {
			if (uS.isRightChar(spWord, 0, '(', &dFnt, MBF) && uS.isPause(spWord, 0, NULL,  &t)) {
			} else if (isRightMorUttDelim(spWord, TRUE)) {
				break;
			} else if (spWord[0] == '[' || spWord[0] == HIDEN_C) {
			} else if (spWord[0] != rplChr && spWord[0] != errChr) {
				if ((res=uS.HandleCAChars(spWord, &t)) != 0) {
					if (isRemoveCAChar[t] == FALSE)
						break;
					if (spWord[res] != EOS)
						break;
				} else
					break;
			}
		}
		if (i != 0) {
			repWord = j;
			j = getword("%", mor_tier, MorWord, NULL, j);
			if (j != 0) {
				if (!strcmp(spWord, ",") && uS.mStrnicmp(MorWord, "cm|", 3) != 0) {
					j = repWord;
				} else if (spWord[0] == '&' && uS.mStricmp(spWord, MorWord) != 0) {
					strcat(tempTier, " ");
					strcat(tempTier, spWord);
					j = repWord;
				} else {
					strcat(tempTier, " ");
					strcat(tempTier, MorWord);
				}
			} else {
				isMorDone = TRUE;
				if (spWord[0] == '&') {
					strcat(tempTier, " ");
					strcat(tempTier, spWord);
				}
			}
		}
	} while (i != 0) ;
	if (!isMorDone) {
		while ((j=getword("%", mor_tier, MorWord, NULL, j))) {
			strcat(tempTier, " ");
			strcat(tempTier, MorWord);
		}
	}
	if (tempTier[0] == ' ')
		strcpy(tempTier, tempTier+1);
	if (tempTier[0] != EOS)
		strcpy(mor_tier, tempTier);
}

// 2018-10-22 beg Nan duplicates
#define morWsMax 100 // 1024

struct morWsCom {
	int  morWsI;
	const char *morWs[morWsMax];
	char isDumWs[morWsMax];
} ;

static const char *getMorWs(struct morWsCom *morWsC, int pos, int offset) {
	int i;

	offset--;
	for (i=pos-1; i > 0 && morWsC->isDumWs[i] == TRUE; i--) ;
	return(morWsC->morWs[i-offset]);
}

static int skipSubCodeWs(char *wline, int pos) {
	if (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) && pos >= 0) {
		while (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) && pos >= 0) {
			pos--;
		}
	}
	if (pos < 0) {
		fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
		fprintf(stderr,"Missing '[' character\n");
		fprintf(stderr,"text=%s->%s\n",wline,wline+pos);
		return(-1);
	} else {
		pos--;
	}
	while (!uS.isRightChar(wline, pos, '>', &dFnt, MBF) && uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0) {
		if (uS.isRightChar(wline, pos, ']', &dFnt, MBF) || uS.isRightChar(wline, pos, '<', &dFnt, MBF))
			break;
		else
			pos--;
	}
	if (uS.isRightChar(wline, pos, '>', &dFnt, MBF)) { // lxs
		while (!uS.isRightChar(wline, pos, '<', &dFnt, MBF) && pos >= 0) {
			if (uS.isRightChar(wline, pos, ']', &dFnt, MBF)) {
				pos = skipSubCodeWs(wline, pos);
			} else {
				pos--;
			}
		}
		if (pos < 0) {
			fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno+tlineno);
			fprintf(stderr,"Missing '<' character\n");
			fprintf(stderr,"text=%s\n",wline);
			return(-1);
		} else {
			pos--;
		}
	} else if (pos >= 0) {
		while (!uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0) {
			pos--;
		}
	}
	return(pos);
}

static void internalError(char *spTier) {
	fprintf(stderr,"\n*** File \"%s\": ", oldfname);
	fprintf(stderr,"line %ld.\n", lineno);
	fprintf(stderr,"ERROR: exceeded number of words per utterance (morW) limit of %d.\n", morWsMax);
	fprintf(stderr,"%s", org_spName);
	printoutline(stderr, org_spTier);
	if (fatal_error_function != NULL)
		(*fatal_error_function)(FALSE);
	else
		cutt_exit(1);
}

static int duplicateCodeMorWs(char *wline, int pos, char isRepeat, struct morWsCom *morWsC) {
	int t, tempPos, angWsCnt, lastMorWsPos;

	if (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) && pos >= 0) {
		while (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) && pos >= 0) {
			pos--;
		}
	}
	if (pos < 0) {
		fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
		fprintf(stderr,"Missing '[' character\n");
		fprintf(stderr,"text=%s->%s\n",wline,wline+pos);
		return(-1);
	} else {
		if (uS.isRightChar(wline, pos, '[', &dFnt, MBF)) {
			if (!strncmp(wline+pos, "[/]", 3)   || !strncmp(wline+pos, "[//]", 4) ||
					   !strncmp(wline+pos, "[///]", 5) || !strncmp(wline+pos, "[/?]", 4) ||
					   !strncmp(wline+pos, "[/-]", 4)  ||
					   !strncmp(wline+pos, "[e]", 3)   || !strncmp(wline+pos, "[E]", 3)) {
				if (strncmp(wline+pos, "[/]", 3) != 0)
					isRepeat = FALSE;
				if (isRepeat && isCreateFakeMor == 2) {
					tempPos = pos - 1;
					while (!uS.isRightChar(wline, tempPos, '>', &dFnt, MBF) && uS.isskip(wline,tempPos,&dFnt,MBF) && tempPos >= 0) {
						if (uS.isRightChar(wline, tempPos, ']', &dFnt, MBF) || uS.isRightChar(wline, tempPos, '<', &dFnt, MBF))
							break;
						else
							tempPos--;
					}
					if (uS.isRightChar(wline, tempPos, '>', &dFnt, MBF) || uS.isRightChar(wline, tempPos, ']', &dFnt, MBF) ||
						uS.isRightChar(wline, tempPos, '<', &dFnt, MBF))
						isRepeat = FALSE;
				}
				if (morWsC->morWsI > 1) {
					if (morWsC->morWsI >= morWsMax) {
						internalError(wline);
					}
					if (isRepeat) {
						morWsC->morWs[morWsC->morWsI] = getMorWs(morWsC, morWsC->morWsI, 1);
					} else {
						morWsC->morWs[morWsC->morWsI] = morWsC->morWs[0];
					}
					morWsC->isDumWs[morWsC->morWsI] = TRUE;
					morWsC->morWsI++;
				}
				if (isRepeat && isCreateFakeMor == 2) {
					isRepeat = FALSE;
				}
			}
		}
		pos--;
	}
	while (!uS.isRightChar(wline, pos, '>', &dFnt, MBF) && uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0) {
		if (uS.isRightChar(wline, pos, ']', &dFnt, MBF) || uS.isRightChar(wline, pos, '<', &dFnt, MBF))
			break;
		else
			pos--;
	}
	if (uS.isRightChar(wline, pos, '>', &dFnt, MBF)) { // lxs
		tempPos = pos;
		angWsCnt = 0;
		lastMorWsPos = morWsC->morWsI;

		while (!uS.isRightChar(wline, tempPos, '<', &dFnt, MBF) && tempPos >= 0) {
			if (uS.isRightChar(wline, tempPos, ']', &dFnt, MBF)) {
				tempPos = skipSubCodeWs(wline, tempPos);
			} else {
				if (!uS.isskip(wline,tempPos,&dFnt,MBF) && (tempPos == 0 || uS.isskip(wline,tempPos-1,&dFnt,MBF))) {
					if (wline[tempPos] == '&' ||
						(uS.isRightChar(wline, tempPos, '(', &dFnt, MBF) && uS.isPause(wline, tempPos, NULL, &t))) {
					} else
						angWsCnt++;
				}
				tempPos--;
			}
		}
		
		while (!uS.isRightChar(wline, pos, '<', &dFnt, MBF) && pos >= 0) {
			if (uS.isRightChar(wline, pos, ']', &dFnt, MBF)) {
				pos = duplicateCodeMorWs(wline, pos, isRepeat, morWsC);
			} else {
				if (!uS.isskip(wline,pos,&dFnt,MBF) && (pos == 0 || uS.isskip(wline,pos-1,&dFnt,MBF))) {
					if (wline[pos] == '&' ||
						(uS.isRightChar(wline, pos, '(', &dFnt, MBF) && uS.isPause(wline, pos, NULL, &t))) {
						if (morWsC->morWsI >= morWsMax) {
							internalError(wline);
						}
						if (isRepeat) {
							morWsC->morWs[morWsC->morWsI] = getMorWs(morWsC, morWsC->morWsI, 1);
						} else {
							morWsC->morWs[morWsC->morWsI] = morWsC->morWs[0];
						}
						morWsC->isDumWs[morWsC->morWsI] = TRUE;
						morWsC->morWsI++;
					} else if (isRepeat && angWsCnt > 0 && morWsC->morWsI > angWsCnt) {
						if (morWsC->morWsI >= morWsMax) {
							internalError(wline);
						}
						morWsC->morWs[morWsC->morWsI] = getMorWs(morWsC, lastMorWsPos, angWsCnt);
						morWsC->isDumWs[morWsC->morWsI] = FALSE;
						morWsC->morWsI++;
						angWsCnt--;
					} else if (!isRepeat && angWsCnt > 0) {
						if (morWsC->morWsI >= morWsMax) {
							internalError(wline);
						}
						morWsC->morWs[morWsC->morWsI] = morWsC->morWs[0];
						morWsC->isDumWs[morWsC->morWsI] = TRUE;
						morWsC->morWsI++;
						angWsCnt--;
					}
				}
				pos--;
			}
		}
		if (pos < 0) {
			fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno+tlineno);
			fprintf(stderr,"Missing '<' character\n");
			fprintf(stderr,"text=%s\n",wline);
			return(-1);
		} else {
			pos--;
		}
	} else if (pos >= 0) {
		while (!uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0) {
			pos--;
		}
		if (morWsC->morWsI > 1) {
			if (morWsC->morWsI >= morWsMax) {
				internalError(wline);
			}
			if (isRepeat) {
				morWsC->morWs[morWsC->morWsI] = getMorWs(morWsC, morWsC->morWsI, 1);
				morWsC->isDumWs[morWsC->morWsI] = FALSE;
			} else {
				morWsC->morWs[morWsC->morWsI] = morWsC->morWs[0];
				morWsC->isDumWs[morWsC->morWsI] = TRUE;
			}
			morWsC->morWsI++;
		}
	}
	return(pos);
}

static void createFullFakeMorForSpeaker(char *spTier, char *mor_tier) {
	char isSkipMor, isRepeatFound;
	int  i, t, posSp, posMor, sqCnt;
	struct morWsCom morWsC;

	uS.remFrontAndBackBlanks(spTier);
	replaceCRwithSpace(spTier);
	removeExtraSpace(spTier);
	uS.remFrontAndBackBlanks(mor_tier);
	replaceCRwithSpace(mor_tier);
	removeExtraSpace(mor_tier);

	isRepeatFound = FALSE;
	morWsC.morWs[0] = "<NO_POS>";
	morWsC.morWsI = 1;
	posSp = strlen(spTier) - 1;
	posMor = strlen(mor_tier) - 1;
	while (posSp >= 0) {
		isSkipMor = FALSE;
		if (posSp >= 0 && uS.isRightChar(spTier,posSp,',',&dFnt,MBF)) {
			posSp--;
		} else {
			for (; posSp >= 0 && !uS.isskip(spTier, posSp, &dFnt, MBF); posSp--) ;
		}
		if (uS.isRightChar(spTier, posSp+1, '&', &dFnt, MBF) ||
			(uS.isRightChar(spTier, posSp+1, '(', &dFnt, MBF) && uS.isPause(spTier, posSp+1, NULL, &t))) {
			isSkipMor = TRUE;
			if (morWsC.morWsI > 0) {
				if (morWsC.morWsI >= morWsMax) {
					internalError(spTier);
				}
				if (isCreateFakeMor == 2)
					morWsC.morWs[morWsC.morWsI] = morWsC.morWs[0];
				else
					morWsC.morWs[morWsC.morWsI] = getMorWs(&morWsC, morWsC.morWsI, 1);
				morWsC.isDumWs[morWsC.morWsI] = TRUE;
				morWsC.morWsI++;
			}
		}
		if (posSp >= 0 && uS.isRightChar(spTier,posSp,',',&dFnt,MBF))
			posSp--;
		for (; posSp >= 0 && uS.isskip(spTier,posSp,&dFnt,MBF) && !uS.isRightChar(spTier,posSp,']',&dFnt,MBF) &&
			                                                      !uS.isRightChar(spTier,posSp,',',&dFnt,MBF); posSp--) ;
		if (posMor >= 0 && !isSkipMor) {
			for (; posMor >= 0 && !isSpace(mor_tier[posMor]); posMor--) ;
			if (posMor >= 0) {
				mor_tier[posMor] = EOS;
				t = posMor + 1;
				posMor--;
			} else
				t = posMor + 1;
			if (morWsC.morWsI >= morWsMax) {
				internalError(spTier);
			}
			morWsC.morWs[morWsC.morWsI] = mor_tier + t;
			morWsC.isDumWs[morWsC.morWsI] = FALSE;
			morWsC.morWsI++;
			for (; posMor >= 0 && isSpace(mor_tier[posMor]); posMor--) ;
		}
		isRepeatFound = FALSE;
repeatSQ:
		if (posSp >= 0 && uS.isRightChar(spTier, posSp, ']', &dFnt, MBF)) {
			sqCnt = 0;
			for (posSp--; (!uS.isRightChar(spTier, posSp, '[', &dFnt, MBF) || sqCnt > 0) && posSp >= 0; posSp--) {
				if (uS.isRightChar(spTier, posSp, ']', &dFnt, MBF))
					sqCnt++;
				else if (uS.isRightChar(spTier, posSp, '[', &dFnt, MBF))
					sqCnt--;
			}
			if (posSp < 0) {
				fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno+tlineno);
				fprintf(stderr,"Missing '[' character\n");
				fprintf(stderr,"text=%s\n",spTier);
				cutt_exit(1);
			}
			if (!strncmp(spTier+posSp, "[/]", 3)) {
				if (isCreateFakeMor == 2) {
					posSp = duplicateCodeMorWs(spTier, posSp, !isRepeatFound, &morWsC);
				} else {
					posSp = duplicateCodeMorWs(spTier, posSp, TRUE, &morWsC);
				}
				isRepeatFound = TRUE;
			} else if (!strncmp(spTier+posSp, "[//]", 4) ||
				!strncmp(spTier+posSp, "[///]", 5) || !strncmp(spTier+posSp, "[/?]", 4) ||
				!strncmp(spTier+posSp, "[/-]", 4)  ||
				!strncmp(spTier+posSp, "[e]", 3)   || !strncmp(spTier+posSp, "[E]", 3)) {
				posSp = duplicateCodeMorWs(spTier, posSp, FALSE, &morWsC);
				isRepeatFound = FALSE;
			} else {
				sqCnt = 0;
				isRepeatFound = FALSE;
			}
			for (; posSp >= 0 && uS.isskip(spTier,posSp,&dFnt,MBF) && !uS.isRightChar(spTier,posSp,']',&dFnt,MBF) &&
				 !uS.isRightChar(spTier,posSp,',',&dFnt,MBF); posSp--) ;
			if (posSp >= 0 && uS.isRightChar(spTier,posSp,']',&dFnt,MBF))
				goto repeatSQ;
		} else
			isRepeatFound = FALSE;
	}
	while (posMor >= 0) {
		for (; posMor >= 0 && !isSpace(mor_tier[posMor]); posMor--) ;
		if (posMor >= 0) {
			mor_tier[posMor] = EOS;
			t = posMor + 1;
			posMor--;
		} else
			t = posMor + 1;
		if (morWsC.morWsI >= morWsMax) {
			internalError(spTier);
		}
		morWsC.morWs[morWsC.morWsI] = mor_tier + t;
		morWsC.isDumWs[morWsC.morWsI] = FALSE;
		morWsC.morWsI++;
		for (; posMor >= 0 && isSpace(mor_tier[posMor]); posMor--) ;
	}
	tempTier[0] = EOS;
	for (i=morWsC.morWsI-1; i > 0; i--) {
		strcat(tempTier, morWsC.morWs[i]);
		strcat(tempTier, " ");
	}
	uS.remblanks(tempTier);
	if (tempTier[0] != EOS)
		strcpy(mor_tier, tempTier);
}

static void prepSpTier(char *line) {
	int i, j, XLen;
	int ab, sq;

	sq = 0;
	ab = 0;
	for (i=0; line[i] != EOS; i++) {
		if (line[i] == '[') {
			sq++;
		} else if (line[i] == ']') {
			if (sq > 0)
				sq--;
		}
		if (sq == 0) {
			if (line[i] == '<') {
				ab++;
			} else if (line[i] == '>') {
				if (ab > 0)
					ab--;
			}
		}
		if (i == 0 || isSpace(line[i])) {
			if (i != 0)
				i++;
			XLen = 0;
			if (strncmp(line+i, "xxx", 3) == 0 && (isSpace(line[i+3]) || line[i+3] == '\n'))
				XLen = 3;
			else if (strncmp(line+i, "xx", 2) == 0 && (isSpace(line[i+2]) || line[i+2] == '\n'))
				XLen = 2;
			if (XLen > 0) {
				if (ab > 0) {
					for (j=i; line[j] != '>' && line[j] != '[' && line[j] != EOS; j++) ;
					if (line[j] == '>')
						for (j++; line[j] == '>' || isSpace(line[j]); j++) ;
				} else {
					for (j=i+XLen; isSpace(line[j]); j++) ;
				}
				if ((line[j] == '[' && line[j+1] == '/' && line[j+2] == ']') ||
					(line[j] == '[' && line[j+1] == '/' && line[j+2] == '/' && line[j+3] == ']')) {
					line[i] = 'N';
					line[i+1] = 'A';
					if (XLen == 3) {
						line[i+2] = ' ';
					}
				}
			} else if (i > 0)
				i--;
		}
	}
}
// 2018-10-22 end Nan duplicates

static void rewrapComboTier(char *line) {
	register long colnum;
	register long splen = 0;
	char *pos, *s, first = TRUE, sb = FALSE, isBulletFound, BulletFound;

	splen = ((5 / TabSize) + 1) * TabSize;;
	colnum = splen;
	BulletFound = FALSE;
	while (*line != EOS) {
		pos = line;
		colnum++;
		if (*line == '[')
			sb = TRUE;
		if (BulletFound) {
			BulletFound = FALSE;
			if (!cutt_isMultiFound)
				colnum = MAXOUTCOL + 1;
		}
		isBulletFound = FALSE;
		if (*line == dMarkChr)
			line += 2;
		while ((!isSpace(*line) || sb || isBulletFound) && *line != EOS) {
			if (*line == HIDEN_C) {
				isBulletFound = !isBulletFound;
				if (isBulletFound && isdigit(line[1]))
					BulletFound = TRUE;
			}
			line++;
			if (*line == dMarkChr) {
				line += 2;
			} else if (*line == ' ' && *(line+1) == sMarkChr) {
				line += 3;
				colnum++;
			} else if ((*line == '\n' && sb) || *line == '\t') {
				*line = ' ';
			} else if (*line == '[') {
				sb = TRUE;
			} else if (*line == ']') {
				sb = FALSE;
			}
			if (!isBulletFound && (UTF8_IS_SINGLE((unsigned char)*line) || UTF8_IS_LEAD((unsigned char)*line)))
				colnum++;
		}
		if (colnum > MAXOUTCOL) {
			if (first)
				first = FALSE;
			else {
				uS.shiftright(pos, 2);
				line += 2;
				*pos++ = '\n';
				*pos++ = '\t';
				colnum = splen;
				isBulletFound = FALSE;
				for (s=pos; s != line; s++) {
					if (*s == HIDEN_C)
						isBulletFound = !isBulletFound;
					if (*s == dMarkChr) {
						s += 2;
					} else if (*s == ' ' && *(s+1) == sMarkChr) {
						s += 3;
						colnum++;
					} else if (!isBulletFound && (UTF8_IS_SINGLE((unsigned char)*s) || UTF8_IS_LEAD((unsigned char)*s)))
						colnum++;
				}
			}
		} else if (!first) {
		} else
			first = FALSE;
		while (isSpace(*line) || *line == '\n') {
			line++;
		}
	}
	*line++ = '\n';
	*line = EOS;
}

/**
 * Sets the vocative marks to not be removed; i.e. double commas, etc.
 * Houjun Liu 04/30/2023 for Batchalign
 **/
void setVocative() {
	isRemoveCAChar[NOTCA_DOUBLE_COMMA] = FALSE;
	isRemoveCAChar[NOTCA_VOCATIVE] = FALSE;
}

void createMorUttline(char *new_mor_tier, char *spTier, const char *dcode, char *mor_tier, char isFilterSP, char linkTiers) {
	int  i, j, t, matchType, res;
	char spWord[BUFSIZ], MorWord[BUFSIZ], spTierDone, dMark[4], sMark[4], missMatch;
	char tBEG, tEND;
//2019-04-29	char tOQ, tCQ;

	if (linkTiers) {
		if (morPunctuation[0] == EOS) {
			strcpy(morPunctuation, GlobalPunctuation);
			for (i=0; morPunctuation[i]; ) {
				if (morPunctuation[i] == '!' ||
					morPunctuation[i] == '?' ||
					morPunctuation[i] == '.') 
					strcpy(morPunctuation+i,morPunctuation+i+1);
				else
					i++;
			}
		}
		punctuation = morPunctuation;
	}
/*2019-04-29
	tOQ  = isRemoveCAChar[NOTCA_OPEN_QUOTE];
	isRemoveCAChar[NOTCA_OPEN_QUOTE] = FALSE;
	tCQ  = isRemoveCAChar[NOTCA_CLOSE_QUOTE];
	isRemoveCAChar[NOTCA_CLOSE_QUOTE] = FALSE;
*/
	tBEG  = isRemoveCAChar[NOTCA_VOCATIVE];
	isRemoveCAChar[NOTCA_VOCATIVE] = FALSE;
	tEND  = isRemoveCAChar[NOTCA_DOUBLE_COMMA];
	isRemoveCAChar[NOTCA_DOUBLE_COMMA] = FALSE;
	if (isFilterSP) {
		if (linkTiers && isCreateFakeMor)
			prepSpTier(spTier);
		filterMORScop(spTier, linkTiers);
		filterwords("*", spTier, excludeSpTierMorDef);
		if (linkTiers) {
			if (isCreateFakeMor) {
				createFullFakeMorForSpeaker(spTier, mor_tier);  // 2018-10-22 beg Nan duplicates
			} else if (CLAN_PROG_NUM == COMBO && strchr(spTier, '&') != NULL) {
				copyFillersToDepTier(spTier, mor_tier);
			}
		}
	}
	filterwords(dcode, mor_tier, excludeMorTierMorDef);
/*
	addword('\0','\0',"+</?>");
	addword('\0','\0',"+</->");
	addword('\0','\0',"+<///>");
	addword('\0','\0',"+<//>");
	addword('\0','\0',"+</>");

	addword('\0','\0',"+xxx");
	addword('\0','\0',"+yyy");
	addword('\0','\0',"+www");
	addword('\0','\0',"+xxx@s*");
	addword('\0','\0',"+yyy@s*");
	addword('\0','\0',"+www@s*");

	addword('\0','\0',"+0");
	addword('\0','\0',"+&*");
	addword('\0','\0',"+-*");
	addword('\0','\0',"+#*");
	addword('\0','\0',"+(*.*)");
*/
	sMark[0] = ' ';
	sMark[1] = sMarkChr;
	sMark[2] = ' ';
	sMark[3] = EOS;
	dMark[0] = ' ';
	dMark[1] = dMarkChr;
	dMark[2] = ' ';
	dMark[3] = EOS;
	missMatch = FALSE;
	i = 0;
	j = 0;
	new_mor_tier[0] = EOS;
	if (linkTiers || R8)
		spTierDone = FALSE;
	else
		spTierDone = TRUE;
	tempTier[0] = EOS;
	t = -1;
	do {
		while ((j=getword("%", mor_tier, MorWord, NULL, j))) {
			if (!uS.mStrnicmp(MorWord,"tag|",4) && MorWord[4]== (char)0xE2 && MorWord[5]== (char)0x80 && MorWord[6]== (char)0x9E) {
			} else if (MorWord[0] == HIDEN_C) {
			} else
				break;
		}
		if (j != 0) {
			if (linkTiers)
				strcat(new_mor_tier, dMark);
			else
				strcat(new_mor_tier, " ");
			strcat(new_mor_tier, MorWord);
			if ((MorWord[0] != '+' || isRightMorUttDelim(MorWord, linkTiers)) && MorWord[0] != '-' && !spTierDone) {
				while ((i=getword("*\001", spTier, spWord, NULL, i))) {
					if (uS.isRightChar(spWord, 0, '(', &dFnt, MBF) && uS.isPause(spWord, 0, NULL,  &t)) {
						if (isCreateFakeMor)
							break;
					} else if (!strcmp(spWord, ",") && uS.mStrnicmp(MorWord, "cm|", 3) && isFilterSP) {
					} else if (isRightMorUttDelim(spWord, linkTiers)) {
						break;
					} else if (spWord[0] != rplChr && spWord[0] != errChr) {
						if ((res=uS.HandleCAChars(spWord, &matchType)) != 0) {
							if (isRemoveCAChar[matchType] == FALSE)
								break;
							if (spWord[res] != EOS)
								break;
						} else
							break;
					}
				}
				if (i == 0) {
					spTierDone = TRUE;
					missMatch = TRUE;
				} else {
					if (linkTiers) {
						ChangeBackSpace(spWord);
						strcat(new_mor_tier, sMark);
						strcat(new_mor_tier, spWord);
					}
					t = i;
					while ((t=getword("*\001", spTier, spWord, NULL, t))) {
						ChangeBackSpace(spWord);
						if (spWord[0] == rplChr) {
							if (R8)
								strcat(new_mor_tier, spWord);
						} else if (spWord[0] == errChr) {
							if (R8)
								strcat(new_mor_tier, spWord);
						} else if (spWord[0] != '[' || isCreateFakeMor) {
							if ((res=uS.HandleCAChars(spWord, &matchType)) != 0) {
								if (isRemoveCAChar[matchType] == FALSE)
									break;
								if (spWord[res] != EOS)
									break;
							} else
								break;
						} else if (spWord[0] == '[' && (spWord[1] == '+' || spWord[1] == '-')) {
							break;
						} else if (linkTiers) {
							if (!ml_IsClauseMarker(spWord)) {
								strcat(new_mor_tier, " ");
								strcat(new_mor_tier, spWord);
							}
						}
					}
					if (ml_isclause()) {
						t = i;
						while ((t=getword("*\001", spTier, spWord, NULL, t))) {
							ChangeBackSpace(spWord);
							if (uS.isRightChar(spWord, 0, '(', &dFnt, MBF) && uS.isPause(spWord, 0, NULL,  &t)) {
								if (isCreateFakeMor)
									break;
							} else if (isRightMorUttDelim(spWord, linkTiers)) {
								break;
							} else if (spWord[0] != rplChr && spWord[0] != errChr) {
								if ((res=uS.HandleCAChars(spWord, &matchType)) != 0) {
									if (isRemoveCAChar[matchType] == FALSE)
										break;
									if (spWord[res] != EOS)
										break;
								} else
									break;
							} else if (linkTiers) {
								if (ml_IsClauseMarker(spWord)) {
									strcat(tempTier, spWord);
									strcat(tempTier, " ");
								}
							}
						}
					}
				}
			} else if (spTierDone)
				missMatch = TRUE;
			else if (MorWord[0] == '+' && (spWord[0] == '+' || t == -1)) {
				if (t == -1) {
					t = 0;
					while ((t=getword("*\001", spTier, spWord, NULL, t))) {
						ChangeBackSpace(spWord);
						if (spWord[0] == rplChr) {
							if (R8)
								strcat(new_mor_tier, spWord);
						} else if (spWord[0] == errChr) {
							if (R8)
								strcat(new_mor_tier, spWord);
						} else if (spWord[0] != '[') {
							if ((res=uS.HandleCAChars(spWord, &matchType)) != 0) {
								if (isRemoveCAChar[matchType] == FALSE)
									break;
								if (spWord[res] != EOS)
									break;
							} else
								break;
						} else if (spWord[0] == '[' && (spWord[1] == '+' || spWord[1] == '-')) {
							break;
						} else if (linkTiers) {
							if (!ml_IsClauseMarker(spWord)) {
								strcat(new_mor_tier, " ");
								strcat(new_mor_tier, spWord);
							}
						}
					}
				}
				if (spWord[0] == '+') {
					if (uS.mStricmp(MorWord, spWord) == 0) {
						i = t;
					}
				}
			}
			if (tempTier[0] != EOS) {
				uS.remblanks(tempTier);
				strcat(new_mor_tier, dMark);
				strcat(new_mor_tier, tempTier);
				strcat(new_mor_tier, sMark);
				strcat(new_mor_tier, tempTier);
				tempTier[0] = EOS;
			}
		}
	} while (j != 0) ;
	if (tempTier[0] != EOS) {
		uS.remblanks(tempTier);
		strcat(new_mor_tier, dMark);
		strcat(new_mor_tier, tempTier);
		strcat(new_mor_tier, sMark);
		strcat(new_mor_tier, tempTier);
		tempTier[0] = EOS;
	}
	if (!missMatch && i != 0) {
		while ((i=getword("*", spTier, spWord, NULL, i))) {
			if (uS.isRightChar(spWord, 0, '(', &dFnt, MBF) && uS.isPause(spWord, 0, NULL,  &t)) {
			} else if (!strcmp(spWord, ",") && uS.mStrnicmp(MorWord, "cm|", 3) && isFilterSP) {
			} else if (isRightMorUttDelim(spWord, linkTiers)) {
				break;
			} else if (spWord[0] != rplChr && spWord[0] != errChr) {
				if ((res=uS.HandleCAChars(spWord, &matchType)) != 0) {
					if (isRemoveCAChar[matchType] == FALSE)
						break;
					if (spWord[res] != EOS)
						break;
				} else
					break;
			}
		}
		if (i != 0)
			missMatch = TRUE;
	}
//2019-04-29	isRemoveCAChar[NOTCA_OPEN_QUOTE]    = tOQ;
//2019-04-29	isRemoveCAChar[NOTCA_CLOSE_QUOTE]   = tCQ;
	isRemoveCAChar[NOTCA_VOCATIVE]      = tBEG;
	isRemoveCAChar[NOTCA_DOUBLE_COMMA]  = tEND;
	if (missMatch && linkTiers) {
		mor_link.error_found = TRUE;
		strcpy(mor_link.fname, oldfname);
		mor_link.lineno = lineno;
		strcpy(new_mor_tier, mor_tier);
	}
	if (new_mor_tier[0] == ' ')
		strcpy(new_mor_tier, new_mor_tier+1);
	if (CLAN_PROG_NUM == COMBO) {
		if (isFilterSP == TRUE && strchr(new_mor_tier, dMarkChr) != NULL) {
			removeMainTierWords(new_mor_tier);
		}
		removeExtraSpace(new_mor_tier);
		uS.remFrontAndBackBlanks(new_mor_tier);
		rewrapComboTier(new_mor_tier);
	}
	punctuation = GlobalPunctuation;
}

/*	END %mor tier elements parsing END */
/**************************************************************/

/**************************************************************/
/*	 keyword and punctuation files					*/
void freedefwdptr(char *st) {
	IEWORDS *t, *tt;

	t = defwdptr;
	tt = t;
	do {
		if (uS.patmat(st,tt->word)) {
			if (tt == defwdptr) {
				defwdptr = tt->nextword;
				t = defwdptr;
			} else
				t->nextword = tt->nextword;
			free(tt->word);
			free(tt);
			tt = t;
		} else if (strchr(st,(int)'%') == NULL) {
			if (uS.patmat(tt->word,st)) {
				if (tt == defwdptr) {
					defwdptr = tt->nextword;
					t = defwdptr;
				} else
					t->nextword = tt->nextword;
				free(tt->word);
				free(tt);
				tt = t;
			}
		}
		t = tt;
		tt = tt->nextword;
	} while (tt != NULL) ;
}

/* addword(opt,ch,wd) is used to specify the words and scop items to be excluded
   or include. If the first character of string "wd" is "+" then the string
   will be added to the existing list of words or scop tokens. "ch"
   character spesifies whether the string is to be included "i" or excluded
   "e". If the ScopMode or WordMode are equal to 0 then the items will be
   excluded. If the first or the first after "+" character is either "<" or
   "[" then the "wd" is a scop token, otherwise it is a word.
*/
IEWORDS *InsertWord(IEWORDS *tw, IEWORDS *wptr) {
	IEWORDS *t, *tt;

	if (wptr == NULL) {
		wptr = tw;
		wptr->nextword = NULL;
	} else if (strcmp(tw->word,wptr->word) > 0) {
		tw->nextword = wptr;
		wptr = tw;
	} else if (strcmp(tw->word,wptr->word) == 0) {
		free(tw->word);
		free(tw);
	} else {
		t = wptr;
		tt = wptr->nextword;
		while (tt != NULL) {
			if (strcmp(tw->word,tt->word) > 0) break;
			t = tt;
			tt = tt->nextword;
		}
		if (tt == NULL) {
			t->nextword = tw;
			tw->nextword = NULL;
		} else {
			tw->nextword = tt;
			t->nextword = tw;
		}
	}
	return(wptr);
}

void cleanupMultiWord(char *st) {
	char *s;

	s = st + strlen(st) + 1;
	*s = EOS;
	s = st;
	while ((s=strchr(s, ' ')) != NULL) {
		*s = EOS;
		s++;
	}
}

IEMWORDS *InsertMulti(IEMWORDS *mroot, char *st, char searchTier) {
	char *e, t, res;
	IEMWORDS *p;

	if (st[0] == EOS)
		return(mroot);
	if (mroot == NULL) {
		mroot = NEW(IEMWORDS);
		p = mroot;
	} else {
		for (p=mroot; p->nextword != NULL; p=p->nextword) ;
		p->nextword = NEW(IEMWORDS);
		p = p->nextword;
	}
	if (p == NULL)
		out_of_mem();
	p->nextword = NULL;
	p->total = 0;
	while (*st != EOS) {
		for (; isSpace(*st) || *st == '^'; st++) ;
		for (e=st; *e != '^' && *e != ' ' && *e != EOS; e++) ;
		t = *e;
		if (*e == '^' || *e == ' ') {
			*e = EOS;
		}
		if (searchTier == 1) {
			p->morwords_arr[p->total] = makeMorSeachList(NULL, &res, st, 'i');
			if (p->morwords_arr[p->total] == NULL) {
				if (res == FALSE)
					cutt_exit(0);
				else
					out_of_mem();
			}
		} else {
			p->morwords_arr[p->total] = NULL;
		}
		if (searchTier == 2) {
			p->grafptr_arr[p->total] = makeGraSeachList(NULL, st, 'i');
			if (p->grafptr_arr[p->total] == NULL)
				out_of_mem();
		} else {
			p->grafptr_arr[p->total] = NULL;
		}
		p->word_arr[p->total] = (char *)malloc(strlen(st)+1);
		if(p->word_arr[p->total] == NULL)
			out_of_mem();
		strcpy(p->word_arr[p->total], st);
		p->matched_word_arr[p->total][0] = EOS;
		p->context_arr[p->total][0] = EOS;
		p->isMatch[p->total] = FALSE;
		p->total++;
		if (t == EOS)
			break;
		else {
			*e = t;
			st = e + 1;
		}
		if (p->total >= MULTIWORDMAX)
			break;
	}
	p->cnt = 0;
	if (searchTier == 1)
		isMultiMorSearch = TRUE;
	if (searchTier == 2)
		isMultiGraSearch = TRUE;
	return(mroot);
}

static char isTierSpecifiedByUser(const char *code) {
	struct tier *p;

	for (p=headtier; p != NULL; p=p->nexttier) {
		if (uS.partcmp(code, p->tcode, p->pat_match, FALSE)) {
			if (p->include)
				return(TRUE);
			else
				break;
		}
	}
	return(FALSE);
}

static char isCodeSpecified(int num) {
	int  res;
	char code[SPEAKERLEN];
	IEWORDS *twd;

	for (twd=wdptr; twd != NULL; twd = twd->nextword) {
		if (uS.isSqCodes(twd->word, code, &dFnt, FALSE)) {
			if (code[0] == '[') {
				res = strlen(code) - 1;
				code[res] = EOS;
				if (!uS.mStricmp(ScopWdPtr[num]+3, code+1)) {
					ScopWdPtr[num][0] = 5;
					ScopWdPtr[num][1] = 2;
					return(TRUE);
				}
			}
		}
	}
	return(FALSE);
}

void IsSearchR7(char *w) {
	for (; *w; w++) {
		if (*w == '@')
			break;
		if (*w == '/') {
			R7Slash = FALSE;
		} else if (*w == '~') {
			R7Tilda = FALSE;
		} else if (*w == '^') {
			R7Caret = FALSE;
		} else if (*w == ':') {
			R7Colon = FALSE;
		}
	}
}

static char SpecialPlus(char *wd, char opt, char ch) {
	if (wd[0] == '+') {
		if (opt == '\0' && ch == '\0')
			return(TRUE);
		else if (!strcmp(wd+1, "...") || !strcmp(wd+1, "..?") ||  !strcmp(wd+1, "!?")  || !strcmp(wd+1, "/.")  ||
			!strcmp(wd+1, "/?")  || !strcmp(wd+1, "//.") || !strcmp(wd+1, "//?")  || !strcmp(wd+1, ".")   ||
			!strcmp(wd+1, "+.")  || !strcmp(wd+1, "=.")  || !strcmp(wd+1, "\"/.") || !strcmp(wd+1, "\".") ||
			!strcmp(wd+1, "\"")  || !strcmp(wd+1, "^")   || !strcmp(wd+1, "<")    || !strcmp(wd+1, ",")   ||
			!strcmp(wd+1, "+"))
			return(FALSE);
		else
			return(TRUE);
	} else
		return(FALSE);
}

static int cutt_isPause(char *st, int posO) {
	int pos;

	for (pos=posO+1; isdigit(st[pos]) || st[pos]== '.' || st[pos]== ':' || st[pos]== '*' || st[pos]== '%' || st[pos]== '_'; pos++) ;
	if (!uS.isRightChar(st,pos,')',&dFnt,MBF))
		return(FALSE);
	return(TRUE);
}

static void cleanCode(char *code) {
	char *e;

	while (*code != EOS) {
		if ((*code == '<' || *code == '[') && (*(code+1) == '-' || *(code+1) == '+')) {
			code += 2;
			if (!isSpace(*code)) {
				if (*code != ']' && *code != '>') {
					uS.shiftright(code, 1);
					*code = ' ';
					code++;
				}
			} else {
				*code = ' ';
				code++;
				for (e=code; isSpace(*e); e++) ;
				if (code != e)
					strcpy(code, e);
			}
		} else if ((*code == '<' || *code == '[') && *(code+1) == '\\' && *(code+2) == '*') {
			code += 3;
			if (!isSpace(*code)) {
				if (*code != ']' && *code != '>' && *code != '*'  && *code != '%' && *code != '_') {
					uS.shiftright(code, 1);
					*code = ' ';
					code++;
				}
			} else {
				*code = ' ';
				code++;
				for (e=code; isSpace(*e); e++) ;
				if (code != e)
					strcpy(code, e);
			}
		} else
			code++;
	}
}

void addword(char opt, char ch, const char *wdO) {
	int i, j, offset, JustScop = 0;
	char wd[3200+2];
	IEWORDS *tempwd;

	if (strlen(wdO) > 3200)
		return;
	if (!strcmp(wdO, "<*>"))
		strcpy(wd, "<\\*>");
	else if (chatmode && !strcmp(wdO, "[*]"))
		strcpy(wd, "[\\*]");
	else if (!strcmp(wdO, "[\\\\*]"))
		strcpy(wd, "[*]");
	else
		strcpy(wd, wdO);
	if (wd[0] == '[' || wd[0] == '<' || wd[1] == '[' || wd[1] == '<')
		cleanCode(wd);
	for (i=strlen(wd)-1; (wd[i]== ' ' || wd[i]== '\t' || wd[i]== '\n') && i >= 0; i--) ;
	if (i == -1)
		return;
	wd[++i] = EOS;
	if (SpecialPlus(wd, opt, ch)) {
		JustScop = -1;
		i = 1;
	} else if (wd[0] == '~') {
		JustScop = 1;
		i = 1;
	} else if (wd[0] == '\001') {
		i = 1;
	} else {
 		if (*wd == '\\' && *(wd+1) == '+')
 			strcpy(wd, wd+1);
 		i = 0;
	}
	for (; wd[i] == ' ' || wd[i] == '\t'; i++) ;
	if (wd[i] == EOS)
		return;

	if (uS.isRightChar(wd, i, '(', &dFnt, MBF) && cutt_isPause(wd, i)) {
		if (opt == 's' && ch == 'i')
			pauseFound = FALSE;
		else {
			pauseFound = TRUE;
			return;
		}
	}
	if (uS.isRightChar(wd, i, ',', &dFnt, C_MBF)) {
		for (j=0; GlobalPunctuation[j]; ) {
			if (GlobalPunctuation[j] == ',')
				strcpy(GlobalPunctuation+j,GlobalPunctuation+j+1);
			else
				j++;
		}
	}
	if (!nomap)
		uS.lowercasestr(wd+i, &dFnt, C_MBF);
	for (offset=i; wd[offset] == '\\'; offset++) ;
	if (uS.isRightChar(wd, offset, '<', &dFnt, C_MBF) || uS.isRightChar(wd, offset, '[', &dFnt, C_MBF)) {
		if (isPostCodeMark(wd[i+1], wd[i+2])) {
			if (!PostCodeMode) {
				PostCodeMode = ch;
//				if (uS.isRightChar(wd, offset, '<', &dFnt, C_MBF))
//					PostCodeMode = toupper(PostCodeMode);
				if (JustScop == -1 && uS.isRightChar(wd, offset, '[', &dFnt, C_MBF)) {
					if (ch == 'i')
						PostCodeMode = 'I';
				}
			} else if ((char)toupper((unsigned char)PostCodeMode) != (char)toupper((unsigned char)ch)) {
				if (opt == '\0')
					opt = ' ';
				fprintf(stderr,"-%c@ or -%c option can not be used together with either option +%c@ or +%c.\n",opt,opt,opt,opt);
				fprintf(stderr,"Offending word is: %s\n", wd+i);
				cutt_exit(0);
			}
			if (JustScop != 1 && uS.isRightChar(wd, offset, '[', &dFnt, C_MBF)) {
				wd[offset] = '<';
			} else if (uS.isRightChar(wd, offset, '<', &dFnt, C_MBF)) {
				wd[offset] = '[';
			}
			if (opt != 'P')
				JustScop = 0;
		} else {
			if (!ScopMode || (opt == '\0' && ch == '\0')) {
				if (wd[offset] == '<') {
					if (JustScop == 0 && ch == 'i')
						ScopMode = 'I';
				} else if ((opt != '\0' || ch != '\0') && wd[0] != '\001')
					ScopMode = ch;
			} else if (ScopMode != ch) {
				if (ScopMode != 'I' || ch != 'i') {
					if (opt == '\0')
						opt = ' ';
					fprintf(stderr,"-%c@ or -%c option can not be used together with either option +%c@ or +%c.\n",opt,opt,opt,opt);
					fprintf(stderr,"Offending word is: %s\n", wd+i);
					cutt_exit(0);
				}
			}
		}

		if (ScopNWds >= MXWDS) {
			fprintf(stderr,"Maximum of %d words allowed.\n",MXWDS);
			cutt_exit(0);
		} else {
			int  num, k;
			char *t, isCodeMatch;

			for (; wd[i] == '\\'; i++) ;
			i++;
			j = strlen(wd+i)-1+i;
			if ((wd[j] == ']' || wd[j] == '>') && j >= i) j--;
			for (; (wd[j] == ' ' || wd[j] == '\t') && j >= i; j--) ;
			wd[++j] = EOS;
			uS.isSqCodes(wd+i, templineC3, &dFnt, TRUE);
			uS.lowercasestr(templineC3, &dFnt, MBF);
			isCodeMatch = FALSE;
			if (strcmp(templineC3, "*") != 0) {
				for (num=1; num < ScopNWds; num++) {
					if (opt == '\0' && ch == '\0')
						k = (strcmp(ScopWdPtr[num]+3, templineC3) == 0);
					else if (templineC3[0] == '-' && strcmp(ScopWdPtr[num]+3, "- *") == 0) {
						t = ScopWdPtr[num];
						for (k=num,ScopNWds--; k < ScopNWds; k++)
							ScopWdPtr[k] = ScopWdPtr[k+1];
						free(t);
						k = 0;
					} else
						k = uS.patmat(ScopWdPtr[num]+3, templineC3);
					if (k) {
						if (wd[i-1] == '<') {
							if (ch == 'i' && JustScop != 0) {
								if (!isCodeSpecified(num)) {
									t = ScopWdPtr[num];
									for (k=num,ScopNWds--; k < ScopNWds; k++)
										ScopWdPtr[k] = ScopWdPtr[k+1];
									free(t);
								}
							} else {
								if (ScopWdPtr[num][0] != 98 && ScopWdPtr[num][2] != 'i' && ScopWdPtr[num][2] != 'I') {
									ScopWdPtr[num][0] = 1;
									ScopWdPtr[num][2] = ch;
								}
							}
						} else {
							if (JustScop == 1 || wd[0] == '\001')
								ScopWdPtr[num][0] = 1;
							else
								ScopWdPtr[num][0] = 98;
							ScopWdPtr[num][1] = 2;
							ScopWdPtr[num][2] = ch;
						}
						isCodeMatch = TRUE;
					}
				}
			}
			if (isCodeMatch)
				goto cont;
			if (isPostCodeMark(wd[i], wd[i+1])) ;
			else if (wd[i-1] == '<' && ch == 'i' && JustScop != 0)
				return;
			if (ScopWdPtr[0] != NULL) {
				if (!strcmp(templineC3,ScopWdPtr[0]+3)) {
					if (wd[i-1] == '<')
						ScopWdPtr[0][0] = 1;
					else if (ch == 'e' && ScopWdPtr[0][1] == 2) {
						if (ScopWdPtr[0][0] == 0) {
							free(ScopWdPtr[0]);
							ScopWdPtr[0] = NULL;
							goto cont;
						} else
							ScopWdPtr[0][1] = 0;
					} else
						ScopWdPtr[0][1] = 2;
					ScopWdPtr[0][2] = ch;
					goto cont;
				}
			}
			if (JustScop > 0) {
				JustScop = ScopNWds;
				ScopNWds = 0;
				if (ScopWdPtr[ScopNWds] != NULL) {
					free(ScopWdPtr[ScopNWds]);
				}
			}
			ScopWdPtr[ScopNWds] = (char *)malloc(strlen(templineC3)+4);
			if (ScopWdPtr[ScopNWds] == NULL)
				out_of_mem();
			if (wd[i-1] == '<') {
				ScopWdPtr[ScopNWds][0] = 1;
				ScopWdPtr[ScopNWds][1] = 0;
			} else {
				if ((!strcmp(templineC3, "/") || !strcmp(templineC3, "//") || !strcmp(templineC3, "///") ||
					 !strcmp(templineC3, "/-") || !strcmp(templineC3, "/?")) && R6) {
					ScopWdPtr[num][0] = 5;
					ScopWdPtr[num][1] = 2;
				} else {
					if (JustScop == -1)
						ScopWdPtr[ScopNWds][0] = 1;
					else
						ScopWdPtr[ScopNWds][0] = 0;
					ScopWdPtr[ScopNWds][1] = 2;
				}
			}
			ScopWdPtr[ScopNWds][2] = ch;
			strcpy(ScopWdPtr[ScopNWds]+3,templineC3);
			if (JustScop > 0)
				ScopNWds = JustScop;
			else
				ScopNWds++;
cont:
			i--;
			if (uS.isRightChar(wd, offset, '[', &dFnt, C_MBF))
				wd[j++] = ']';
			else
				wd[j++] = '>';
			wd[j] = EOS;
		}
	}
	if (wd[offset] != '<' && JustScop < 1 && wd[0] != '\001') {
		if (wd[i] != '\002' && wd[i] != '\003' && wd[i] != '\004') {
			if (WordMode && WordMode != ch && JustScop != -1) {
				if (opt == 's' && ch =='e') {
					opt = '\0';
					ch  = '\0';
					JustScop = -1;
				} else {
					if (opt == '\0')
						opt = ' ';
					fprintf(stderr,"-%c@ or -%c option can not be used together with either option +%c@ or +%c.\n",opt,opt,opt,opt);
					fprintf(stderr,"Offending word is: %s\n", wd+i);
					cutt_exit(0);
				}
			}
			if (ch && defwdptr != NULL)
				freedefwdptr(wd+i);
			if (JustScop != -1)
				WordMode = ch;
			else if (ch == 'i')
				WordMode = 'I';
		} else {
			if (WordMode && WordMode != ch && ch == 'i') {
				fprintf(stderr," Please place all -%c@ or -%c options after any +%c@ or +%c option.\n",opt,opt,opt,opt);
				cutt_exit(0);
			} else if (ch == 'i') {
				if (JustScop == -1)
					WordMode = 'I';
				else
					WordMode = ch;
			}
			if (JustScop != -1)
				MorWordMode = ch;
			else if (ch == 'i')
				MorWordMode = 'I';
		}
		if (wd[i] == '\002') {
			morfptr = makeMorSeachList(morfptr, NULL, wd+i+1, ch);
			if (morfptr == NULL)
				cutt_exit(0);
		} else if (wd[i] == '\004') {
			grafptr = makeGraSeachList(grafptr, wd+i, ch);
			if (grafptr == NULL)
				cutt_exit(0);
		} else if ((tempwd=NEW(IEWORDS)) == NULL)
			out_of_mem();
		else {
			tempwd->word = (char *)malloc(strlen(wd+i)+1);
			if (tempwd->word == NULL) {
				out_of_mem();
			}
			strcpy(tempwd->word, wd+i);
			IsSearchR7(tempwd->word);
			IsSearchCA(tempwd->word);
			if (strchr(tempwd->word, '$') != NULL) {
				if (strchr(tempwd->word, '|') != NULL && isTierSpecifiedByUser("%mor:")) {
					isPrecliticUse = TRUE;
				}
			}
			if (strchr(tempwd->word, '~') != NULL) {
				if (strchr(tempwd->word, '|') != NULL && isTierSpecifiedByUser("%mor:")) {
					isPostcliticUse = TRUE;
				}
			}
			if ((JustScop == -1 && WordMode != 'I') || !ch) {
				tempwd->nextword = defwdptr;
				defwdptr = tempwd;
			} else {
				wdptr = InsertWord(tempwd,wdptr);
			}
		}
	}
}

static char isMultiWordSearch(char *wd, char isDepTier) {
	if (strchr(wd, '[') == NULL && strchr(wd, '<') == NULL) {
		if (anyMultiOrder || onlySpecWsFound) {
			if (CLAN_PROG_NUM != FREQ) {
				fprintf(stderr,"Multi-words are only allowed with FREQ command.\n");
				cutt_exit(0);
			}
			return(TRUE);
		}
		if (isDepTier && strchr(wd, '^') != NULL) {
			if (CLAN_PROG_NUM != FREQ) {
				fprintf(stderr,"Multi-words are only allowed with FREQ command.\n");
				cutt_exit(0);
			}
			return(TRUE);
		}
		if (strchr(wd, ' ') != NULL) {
			if (CLAN_PROG_NUM == UNIQ) {
				return(FALSE);
			}
			if (CLAN_PROG_NUM != FREQ) {
				fprintf(stderr,"Multi-words are only allowed with FREQ command.\n");
				cutt_exit(0);
			}
			return(TRUE);
		}
	}
	return(FALSE);
}

/* rdexclf(opt,ch,fname) opens file fname and send its content, one line at the
   time, to the addword function. "ch" character spesifies whether the string
   is to be included "i" or excluded "e".
*/
void rdexclf(char opt, char ch, const FNType *fname) {
	FILE *efp;
	char wd[1024];
	int  len;
	FNType mFileName[FNSize];

	if (*fname == '+') {
		*wd = '+';
		fname++;
	} else if (*fname == '~') {
		*wd = '~';
		fname++;
	} else
		*wd = ' ';

	if (*fname == EOS) {
		fprintf(stderr,	"No %s file specified.\n", ((ch=='i') ? "include" : "exclude"));
		cutt_exit(0);
	}

	if ((efp=OpenGenLib(fname,"r",TRUE,TRUE,mFileName)) == NULL) {
		fprintf(stderr, "Can't open either one of the %s-files:\n\t\"%s\", \"%s\"\n",
				((ch=='i') ? "include" : "exclude"), fname, mFileName);
		cutt_exit(0);
	}
	fprintf(stderr, "    Using search file: %s\n", mFileName);
	while (fgets_cr(wd+1, 1024, efp)) {
		if (uS.isUTF8(wd+1) || uS.isInvisibleHeader(wd+1))
			continue;
		if (wd[1] == ';' && wd[2] == '%' && wd[3] == '*' && wd[4] == ' ')
			continue;
		if (wd[1] == '#' && wd[2] == ' ')
			continue;
		for (len=strlen(wd)-1; (wd[len]== ' ' || wd[len]== '\t' || wd[len]== '\n') && len >= 0; len--) ;
		if (len < 0)
			continue;
		wd[len+1] = EOS;
		if ((wd[1] == '@' || wd[1] == 'm') && isMorSearchOption(wd+1, wd[1], '+'))
			wd[1] = '\002';
		else if ((wd[1] == '@' || wd[1] == 'g') && isGRASearchOption(wd+1, wd[1], '+'))
			wd[1] = '\004';
		else if (wd[1] == '"' && wd[len] == '"') {
			wd[len] = EOS;
			strcpy(wd+1, wd+2);
		}
		if (wd[1] == '\'' && wd[len] == '\'') {
			wd[len] = EOS;
			strcpy(wd+1, wd+2);
		}
		removeExtraSpace(wd+1);
		uS.remFrontAndBackBlanks(wd+1);
		if (ch == 'e') {
			if (strchr(wd+1, '[') == NULL && strchr(wd+1, '<') == NULL && strchr(wd+1, ' ') != NULL) {
				fprintf(stderr,"Multi-words are not allowed with option: -%c\n", opt);
				fprintf(stderr,"    Multi-words are search patterns with space character(s) in-between.\n");
				fprintf(stderr,"    If you did not mean to have space characters in-between words,\n");
				fprintf(stderr,"        then please remove space characters and try again.\n");
				cutt_exit(0);
			} else
				addword(opt,ch,wd);
		} else {
			if (isMultiWordSearch(wd+1, (char)((wd[1]=='\002') || (wd[1]=='\004')))) {
				if (wd[1] == '\002')
					mwdptr = InsertMulti(mwdptr, wd+2, 1);
				else if (wd[1] == '\004')
					mwdptr = InsertMulti(mwdptr, wd+2, 2);
				else
					mwdptr = InsertMulti(mwdptr, wd+1, 0);
			} else
				addword(opt,ch,wd);
		}
	}
	fclose(efp);
}

/**************************************************************/
/*	 strings manipulation routines                            */
/*															  */
/* String coping functions
*/
static void att_copy(long pos, char *desSt, const char *srcSt, AttTYPE *desAtt, AttTYPE *srcAtt) {
	long i;

	for (i=0; srcSt[i]; i++, pos++) {
		desSt[pos] = srcSt[i];
		desAtt[pos] = srcAtt[i];
	}
	desSt[pos] = EOS;
}

static void att_copy_same(long pos, char *desSt, const char *srcSt, AttTYPE *desAtt) {
	AttTYPE att;
	long i;

	if (pos > 0L)
		att = desAtt[pos-1];
	else
		att = 0;
	for (i=0; srcSt[i]; i++, pos++) {
		desSt[pos] = srcSt[i];
		desAtt[pos] = att;
	}
	desSt[pos] = EOS;
}

void att_cp(long pos, char *desSt, const char *srcSt, AttTYPE *desAtt, AttTYPE *srcAtt) {
	if (srcAtt != NULL)
		att_copy(pos, desSt, srcSt, desAtt, srcAtt);
	else
		att_copy_same(pos, desSt, srcSt, desAtt);
}

void att_shiftright(char *srcSt, AttTYPE *srcAtt, long num) {
	long i;

	for (i=strlen(srcSt); i >= 0L; i--) {
		srcSt[i+num] = srcSt[i];
		srcAtt[i+num] = srcAtt[i];
	}
}

/**************************************************************/
/* isPostCodeOnUtt(char *line, const char *postcode) determines if the code token in
   "postcode" is located on utterance "line"
*/
char isPostCodeOnUtt(char *line, const char *postcode) {
	int i, pos;
	char buf[BUFSIZ+1];

	for (pos=0; line[pos] != EOS; pos++) {
		if (line[pos] == '[' && line[pos+1] == '+') {
			i = 0;
			for (; line[pos] != EOS && i < BUFSIZ; pos++) {
				if (line[pos] == '\t' || line[pos] == '\n')
					buf[i++] = ' ';
				else
					buf[i++] = line[pos];
				if (line[pos] == ']')
					break;
			}
			if (line[pos] == EOS)
				return(FALSE);
			buf[i] = EOS;
			removeExtraSpace(buf);
			if (uS.mStricmp(buf, postcode) == 0)
				return(TRUE);
		}
	}
	return(FALSE);
}

/**************************************************************/
/* isExcludePostcode(str) determines if the code token in "str" is to be included
 in the analyses or not by comparing the "str" to the list of words in
 "ScopWdPtr".
*/
int isExcludePostcode(char *str) {
	int i;

	if (!isPostCodeMark(str[0], str[1]))
		return(0);
	for (i=1; i < ScopNWds; i++) {
		if (isPostCodeMark(ScopWdPtr[i][3],ScopWdPtr[i][4])) {
			if (uS.patmat(str,ScopWdPtr[i]+3)) {
				if (PostCodeMode == 'i' || PostCodeMode == 'I') {
					if (ScopWdPtr[i][1] == 2)
						return(6);
					else
						return(4);
				} else
					return(5);
			}
		}
	}
	return(0);
}

/**************************************************************/
/* isPostCodeFound(utt) determines if there is a PostCode on utterance utt that
 was selected by speaker using +/-s option by comparing found PostCode to the
 list of words in "ScopWdPtr".
*/
int isPostCodeFound(const char *sp, char *line) {
	long i, b, e;

	for (b=0L; line[b] != EOS; b++) {
		if (uS.isRightChar(line, b, '[', &dFnt, MBF)) {
			b++;
			for (e=b; !uS.isRightChar(line,e,']',&dFnt,MBF) && !uS.isRightChar(line,e,'[',&dFnt,MBF) && line[e] != EOS; e++) ;
			if (uS.isRightChar(line,e,']',&dFnt,MBF)) {
				line[e] = EOS;
				uS.isSqCodes(line+b, templineC3, &dFnt, TRUE);
				line[e] = ']';
				if (isPostCodeMark(templineC3[0], templineC3[1])) {
					uS.lowercasestr(templineC3, &dFnt, MBF);
					if (uS.isRightChar(line,e,']',&dFnt, MBF)) {
						for (i=1L; i < ScopNWds; i++) {
							if (isPostCodeMark(ScopWdPtr[i][3],ScopWdPtr[i][4])) {
								if (uS.patmat(templineC3,ScopWdPtr[i]+3)) {
									if (PostCodeMode == 'i' || PostCodeMode == 'I') {
										if (ScopWdPtr[i][1] == 2)
											return(6);
										else
											return(4);
									} else
										return(5);
								}
							}
						}
						b = e;
					}
				}

			}
		}
	}
	if (sp[0] == '%') {
		if (postcodeRes != 0)
			return(postcodeRes);
	}
	if (PostCodeMode == 'i' || PostCodeMode == 'I')
		return(1);
	else
		return(0);
}

/**************************************************************/
/* isExcludeScope(str) determines if the scop token in "str" is to be included
 in the analyses or not by compairing the "str" to the list of words in
 "ScopWdPtr". It returns 1 if the scop token should be included, and 0
 otherwise except for is uS.patmat true.
 */
int isExcludeScope(char *str) {
	int i;

	if (R6_override) {
		if (uS.patmat(str, "/") || uS.patmat(str, "//") || uS.patmat(str, "///") || uS.patmat(str, "/-") || uS.patmat(str, "/?"))
			goto skip_check;
	} else if (R6_freq) {
		if (uS.patmat(str, "/") || uS.patmat(str, "//") || uS.patmat(str, "///") || uS.patmat(str, "/-") || uS.patmat(str, "/?"))
			return(7);
	}
	if (isPostCodeMark(str[0],str[1]) && (PostCodeMode == 'i' || PostCodeMode == 'I')) {
		for (i=1; i < ScopNWds; i++) {
			if (isPostCodeMark(ScopWdPtr[i][3],ScopWdPtr[i][4]) && uS.patmat(str, ScopWdPtr[i]+3)) {
				if (ScopWdPtr[i][1] == 2)
					return(6);
				else
					return(4);
			}
		}
	}
	for (i=1; i < ScopNWds; i++) {
		if (uS.patmat(str, ScopWdPtr[i]+3)) {
			if (isPostCodeMark(str[0],str[1]) && isPostCodeMark(ScopWdPtr[i][3],ScopWdPtr[i][4])) {
				if (PostCodeMode == 'i' || PostCodeMode == 'I') {
					if (ScopWdPtr[i][1] == 2)
						return(6);
					else
						return(4);
				} else
					return(5);
			}
			if (ScopWdPtr[i][2] == 'i' || ScopWdPtr[i][2] == 'I') {
				return(ScopWdPtr[i][0] + ScopWdPtr[i][1]);
			} else if (CLAN_PROG_NUM == KWAL && ScopWdPtr[i][0] != 1 && ScopWdPtr[i][2] == 'e') {
				isKWAL2013 = TRUE;
				return(2013);
			} else
				return((ScopWdPtr[i][0] == 1) ? 0 : 1);
		}
	}
	if (IncludeAllDefaultCodes)
		return(2009);
	if (R5 == TRUE) {
		if (uS.patmat(str,": *"))
			return(1001);
		if (uS.patmat(str,":\\* *"))
			return(1001);
		if (uS.patmat(str,":=_ *"))
			return(1001);
	}
	if (R5_1 == TRUE) {
		if (uS.patmat(str,":: *"))
			return(1001);
	}
skip_check:
	if (ScopWdPtr[0] == NULL) {
		if (ScopMode == 'i' || ScopMode == 'I') {
			if (ScopMode == 'I')
				return(0);
			else if (isPostCodeMark(str[0], str[1]))
				return(1);
			else
				return(1);
		} else
			return(1);
	} else {
		if (!uS.patmat(str,ScopWdPtr[0]+3)) {
			if (ScopWdPtr[0][2] == 'i' || ScopWdPtr[0][2] == 'I') {
				if (isPostCodeMark(str[0], str[1]))
					return(1);
				else
					return(1);
			} else
				return(1);
		} else
			return(ScopWdPtr[0][0] + ScopWdPtr[0][1]);
	}
}

char isMorPatMatchedWord(MORWDLST *pats, char *word) {
//	char wordT[BUFSIZ];
	MORFEATS word_feats;
	MORWDLST *pat_rootFeat;

	if (pats == NULL)
		return(1);
	for (pat_rootFeat=pats; pat_rootFeat != NULL; pat_rootFeat=pat_rootFeat->nextMatch) {
		if (pat_rootFeat->type == 'i')
			continue;
		strcpy(morWordParse, word);
		if (ParseWordMorElems(morWordParse, &word_feats) == FALSE)
			return(2);
		if (matchToMorElems(pat_rootFeat, &word_feats, TRUE, FALSE, TRUE)) {
//			strcpy(wordT, word);
//			cleanUpMorExcludeWord(wordT, morWordParse);
			freeUpFeats(&word_feats);
//			if (wordT[0] == EOS)
			return(0);
		}
		freeUpFeats(&word_feats);
	}
	for (pat_rootFeat=pats; pat_rootFeat != NULL; pat_rootFeat=pat_rootFeat->nextMatch) {
		if (pat_rootFeat->type != 'i')
			continue;
		strcpy(morWordParse, word);
		if (ParseWordMorElems(morWordParse, &word_feats) == FALSE)
			return(2);
		if (matchToMorElems(pat_rootFeat, &word_feats, TRUE, FALSE, TRUE)) {
			freeUpFeats(&word_feats);
			return(1);
		} 
		freeUpFeats(&word_feats);
	}
	return(0);
}

/* exclude(word) determines if the "word" is to be included in the analyses
   or not by compairing the "word" to the list of words in "wdptr". It returns
   1 if the word should be included, and 0 otherwise.
*/
int excludedef(char *word) {
	int i;
	IEWORDS *twd;

	if (word[0] == '\003') {
		for (i=0; word[i] == '\003'; i++) ;
		if (word[i] == EOS)
			return(FALSE);
	}
	if (isSearchMorSym) {
		if (uS.patmat(word, "cm|*") ||uS.patmat(word, "end|*") || uS.patmat(word, "beg|*")
//2019-04-29			|| uS.patmat(word, "bq|*") || uS.patmat(word, "eq|*") ||
			) {
			return(TRUE);
		}
	}
	for (twd=defwdptr; twd != NULL; twd = twd->nextword) {
		if (uS.patmat(word, twd->word)) {
			if (word[0] != '-')
				return(FALSE);
			else if (uS.isToneUnitMarker(word))
				return(FALSE);
		}
	}
	return(TRUE);
}

int exclude(char *word) {
	int res;
	char code[SPEAKERLEN], *at, isOnlyCodeSearched, isSpecialWordFound, wType;
	IEWORDS *twd;

	if (linkMain2Mor || linkDep2Other) {
		if (isWordFromGRATier(word)) {
			wType = 'g';
		} else if (isWordFromMORTier(word)) {
			wType = '|';
		} else {
			wType = 'w';
		}
	} else
		wType = 'w';
	if (word[0] != '-' && word[0] != '+') {
		if (grafptr != NULL && isWordFromGRATier(word)) {
			GRAWDLST *gra_item;

			strcpy(templineC1, word);
			cleanupGRAWord(templineC1);
			for (gra_item=grafptr; gra_item != NULL; gra_item=gra_item->nextword) {
				if (gra_item->type != 'i') {
					if (uS.patmat(templineC1, gra_item->word))
						return(FALSE);
				}
			}
			for (gra_item=grafptr; gra_item != NULL; gra_item=gra_item->nextword) {
				if (gra_item->type == 'i') {
					if (uS.patmat(templineC1, gra_item->word)) {
						if (CLAN_PROG_NUM == FREQ) {
							for (res=0; templineC1[res] != EOS && word[res] != EOS; res++) {
								word[res] = templineC1[res];
							}
							for (; word[res] != EOS; res++) {
								word[res] = ' ';
							}
						}
						return(TRUE);
					}
				}
			}
			if (CLAN_PROG_NUM == FREQ && linkDep2Other) {
				for (res=0; templineC1[res] != EOS && word[res] != EOS; res++) {
					word[res] = templineC1[res];
				}
				for (; word[res] != EOS; res++) {
					word[res] = ' ';
				}
			}
		} else if (morfptr != NULL && isWordFromMORTier(word)) {
			MORFEATS word_feats;
			MORWDLST *pat_rootFeat;

			for (pat_rootFeat=morfptr; pat_rootFeat != NULL; pat_rootFeat=pat_rootFeat->nextMatch) {
				if (pat_rootFeat->type == 'i')
					continue;
				strcpy(morWordParse, word);
				if (ParseWordMorElems(morWordParse, &word_feats) == FALSE)
					out_of_mem();
				if (matchToMorElems(pat_rootFeat, &word_feats, FALSE, FALSE, TRUE)) {
					cleanUpMorExcludeWord(word, morWordParse);
					if (word[0] == EOS) {
						freeUpFeats(&word_feats);
						return(FALSE);
					}
				}
				freeUpFeats(&word_feats);
			}
			for (pat_rootFeat=morfptr; pat_rootFeat != NULL; pat_rootFeat=pat_rootFeat->nextMatch) {
				if (pat_rootFeat->type != 'i')
					continue;
				strcpy(morWordParse, word);
				if (ParseWordMorElems(morWordParse, &word_feats) == FALSE)
					out_of_mem();
				if (matchToMorElems(pat_rootFeat, &word_feats, FALSE, FALSE, TRUE)) {
					cleanUpMorWord(word, morWordParse);
					freeUpFeats(&word_feats);
					return(TRUE);
				}
				freeUpFeats(&word_feats);
			}
		}
	}
	if ((WordMode == 'i' || WordMode == 'I') && wdptr != NULL) {
		isOnlyCodeSearched = TRUE;
		for (twd=wdptr; twd != NULL; twd = twd->nextword) {
			if (uS.isSqCodes(twd->word, code, &dFnt, FALSE)) {
				res = strlen(code) - 1;
				code[res] = EOS;
				if (isExcludeScope(code+1) != 3) {
					isOnlyCodeSearched = FALSE;
					break;
				}
			} else {
				isOnlyCodeSearched = FALSE;
				break;
			}
		}
		if (isOnlyCodeSearched)
			return(TRUE);
	}
	if (word[0] == '0' || word[0] == '&' || word[0] == '+' || word[0] == '-' || word[0] == '#')
		isSpecialWordFound = word[0];
	else
		isSpecialWordFound = 0;
	for (twd=wdptr; twd != NULL; twd=twd->nextword) {
		if (uS.patmat(word, twd->word)) {
			if (WordMode == 'i' || WordMode == 'I') {
				res = strlen(twd->word) - 1;
				if ((twd->word[res-2] == '-' || twd->word[res-2] == '&' || twd->word[res-2] == '~') && 
					twd->word[res-1] == '%' && twd->word[res] == '%') {
					for (twd=twd->nextword; twd != NULL; twd=twd->nextword) {
						res = strlen(twd->word) - 1;
						if ((twd->word[res-2] == '-' || twd->word[res-2] == '&' || twd->word[res-2] == '~') && 
							twd->word[res-1] == '%' && twd->word[res] == '%')
							uS.patmat(word, twd->word);
					}
				}
				return(TRUE);
			} else
				return(FALSE);
		}
		if (isSpecialWordFound == twd->word[0])
			isSpecialWordFound = 1;
	}
	if (WordMode == 'i' || (WordMode == 'I' && isSpecialWordFound == 1)) {
		if (linkMain2Mor || linkDep2Other) {
			if (wType == 'w' && wdptr == NULL) {
				if ((morfptr == NULL && grafptr != NULL) || (morfptr != NULL && grafptr == NULL)) {
					return(TRUE);
				}
			} else if (wType == '|' && morfptr == NULL) {
				if ((wdptr != NULL && grafptr == NULL) || (wdptr == NULL && grafptr != NULL)) {
					return(TRUE);
				}
			} else if (wType == 'g' && grafptr == NULL) {
				if ((morfptr == NULL && wdptr != NULL) || (morfptr != NULL && wdptr == NULL)) {
					if (CLAN_PROG_NUM == FREQ && strchr(word, '|') != NULL) {
						strcpy(templineC1, word);
						cleanupGRAWord(templineC1);
						for (res=0; templineC1[res] != EOS && word[res] != EOS; res++) {
							word[res] = templineC1[res];
						}
						for (; word[res] != EOS; res++) {
							word[res] = ' ';
						}
					}
					return(TRUE);
				}
			}
		}
		return(FALSE);
	} else
		return(TRUE);
}

static char isTierLabel(char *line, char isFirstChar) {
	if (isSpeaker(*line) && (*(line-1) =='\n' || *(line-1) == '\r' || isFirstChar)) {
		for (; *line != ':' && *line != '\n' && *line != '\r' && *line != EOS; line++) ;
		if (*line == ':')
			return(TRUE);
	}
	return(FALSE);
}

void getMediaName(char *line, FNType *tMediaFileName, long size) {
	char qf, *s;
	int i;

	if (line != NULL) {
		strncpy(tMediaFileName, line, size-1);
		tMediaFileName[size-1] = EOS;
	}
	for (i=0; isSpace(tMediaFileName[i]); i++) ;
	if (i > 0)
		strcpy(tMediaFileName, tMediaFileName+i);
	qf = FALSE;
	for (i=0; tMediaFileName[i] != EOS; i++) {
		if (tMediaFileName[i] == '"')
			qf = !qf;
		if (tMediaFileName[i] == ',' || (!qf && isSpace(tMediaFileName[i]))) {
			tMediaFileName[i] = EOS;
			break;
		}
	}
	uS.remFrontAndBackBlanks(tMediaFileName);
	s = strrchr(tMediaFileName, '.');
	if (s != NULL) {
		i = strlen(tMediaFileName) - 1;
		if (tMediaFileName[i] == '"')
			*s++ = '"';
		*s = EOS;
	}
}

/* getMediaTagInfo(*line, *tag, *fname, *Beg, *End) get information in the bullets
   associated with tag name "tag". If tag == NULL, then get information for both "%snd:" and "%mov:".
*/
char getMediaTagInfo(char *line, long *Beg, long *End) {
	int i;
	long beg = 0L, end = 0L;
	char s[256+2];

	if (*line == HIDEN_C)
		line++;

	while (*line && (isSpace(*line) || *line == '_'))
		line++;
	if (*line == EOS)
		return(FALSE);

	while (*line && !isdigit(*line) && *line != HIDEN_C)
		line++;
	if (!isdigit(*line))
		return(FALSE);
	for (i=0; *line && isdigit(*line) && i < 256; line++)
		s[i++] = *line;
	s[i] = EOS;
	beg = atol(s);
	if (beg == 0)
		beg = 1;
	while (*line && !isdigit(*line) && *line != HIDEN_C)
		line++;
	if (!isdigit(*line))
		return(FALSE);
	for (i=0; *line && isdigit(*line) && i < 256; line++)
		s[i++] = *line;
	s[i] = EOS;
	end = atol(s);
	*Beg = beg;
	*End = end;
	return(TRUE);
}

/* getOLDMediaTagInfo(*line, *tag, *fname, *Beg, *End) same as above for older version of bullets
*/
char getOLDMediaTagInfo(char *line, const char *tag, FNType *fname, long *Beg, long *End) {
	int i;
	long beg = 0L, end = 0L;
	char s[256+2];

	if (*line == HIDEN_C)
		line++;
	for (i=0; *line && *line != ':' && *line != HIDEN_C && i < 256; line++)
		s[i++] = *line;
	if (*line != ':')
		return(FALSE);
	s[i++] = *line;
	s[i] = EOS;
	line++;
	if (tag == NULL) {
		if (!uS.partcmp(s, SOUNDTIER, FALSE, FALSE) && !uS.partcmp(s, REMOVEMOVIETAG, FALSE, FALSE))
			return(FALSE);
	} else {
		if (!uS.partcmp(s, tag, FALSE, FALSE))
			return(FALSE);
	}

	while (*line && (isSpace(*line) || *line == '_'))
		line++;
	if (*line != '"')
		return(FALSE);

	line++;
	if (*line == EOS)
		return(FALSE);
	for (i=0; *line && *line != '"' && *line != HIDEN_C  && i < 256; line++)
		s[i++] = *line;
	s[i] = EOS;
	if (fname != NULL)
		uS.str2FNType(fname, 0L, s);
	while (*line && !isdigit(*line) && *line != HIDEN_C)
		line++;
	if (!isdigit(*line))
		return(FALSE);
	for (i=0; *line && isdigit(*line) && i < 256; line++)
		s[i++] = *line;
	s[i] = EOS;
	beg = atol(s);
	if (beg == 0)
		beg = 1;
	while (*line && !isdigit(*line) && *line != HIDEN_C)
		line++;
	if (!isdigit(*line))
		return(FALSE);
	for (i=0; *line && isdigit(*line) && i < 256; line++)
		s[i++] = *line;
	s[i] = EOS;
	end = atol(s);
	*Beg = beg;
	*End = end;
	return(TRUE);
}

/* getword(sp, cleanLine, word, *wi, i) extracts a word from the "cleanLine" starting at "i"
   position and stores it into "word". It return "i", the first position
   after the word and "wi" first pos of word, or 0 if end of string. It removes "(", ")"
   characters from the word.
*/
int getword(const char *sp, register char *cleanLine, register char *orgWord, int *wi, int i) {
	int  si, temp;
	char sq, isGetWholeQuoteItem;
	char *word;

	word = orgWord;
	if (chatmode && *sp == '%') {
		if (cleanLine[i] == EOS)
			return(0);
		if (cleanLine[i] == (char)0xE2 || UTF8_IS_TRAIL((unsigned char)cleanLine[i])) {
		} else if (i > 0) {
			if (CLAN_PROG_NUM != MOR_P) {
				if (UTF8_IS_TRAIL((unsigned char)cleanLine[i-1])) {
				} else if (!uS.isRightChar(cleanLine, i, '(', &dFnt, MBF) && !uS.isRightChar(cleanLine,i,'[',&dFnt,MBF))
					i--;
			}
		}
		while ((*word=cleanLine[i]) != EOS && uS.isskip(cleanLine,i,&dFnt,MBF) && !uS.isRightChar(cleanLine,i,'[',&dFnt,MBF)) {
			if (cleanLine[i] == '?' && cleanLine[i+1] == '|')
				break;
			i++;
			if (*word == '<') {
				temp = i;
				for (i++; cleanLine[i] != '>' && cleanLine[i]; i++) {
					if (isdigit(cleanLine[i])) ;
					else if (cleanLine[i]== ' ' || cleanLine[i]== '\t' || cleanLine[i]== '\n') ;
					else if ((i-1 == temp+1 || !isalpha(cleanLine[i-1])) && cleanLine[i] == '-' && !isalpha(cleanLine[i+1])) ;
					else if ((i-1 == temp+1 || !isalpha(cleanLine[i-1])) && (cleanLine[i] == 'u' || cleanLine[i] == 'U') &&
							 !isalpha(cleanLine[i+1])) ;
					else if ((i-1 == temp+1 || !isalpha(cleanLine[i-1])) && (cleanLine[i] == 'w' || cleanLine[i] == 'W') &&
							 !isalpha(cleanLine[i+1])) ;
					else if ((i-1 == temp+1 || !isalpha(cleanLine[i-1])) && (cleanLine[i] == 's' || cleanLine[i] == 'S') &&
							 !isalpha(cleanLine[i+1])) ;
					else
						break;
				}
				if (cleanLine[i] == '>')
					i++;
				else
					i = temp;
			}
		}
	} else {
		while ((*word=cleanLine[i]) != EOS && uS.isskip(cleanLine,i,&dFnt,MBF) && !uS.isRightChar(cleanLine,i,'[',&dFnt,MBF)) {
// 2012-11-1 2012-10-16 comma to %mor
			if (*sp == '*' && sp[1] == '\001' && uS.isRightChar(cleanLine,i,',',&dFnt,MBF))
				break;
//
			i++;
		}
	}
	if (*word == EOS)
		return(0);
	si = i;
	if (uS.isRightChar(cleanLine, i, '[',&dFnt,MBF)) {
		if (uS.isSqBracketItem(cleanLine, i+1, &dFnt, MBF))
			sq = TRUE;
		else
			sq = FALSE;
	} else
		sq = FALSE;
	if (wi != NULL)
		*wi = i;
	isGetWholeQuoteItem = FALSE;
getword_rep:

	if ((*word == '+' || *word == '-') && !sq) {
		while ((*++word=cleanLine[++i]) != EOS && (!uS.isskip(cleanLine,i,&dFnt,MBF) ||
			uS.isRightChar(cleanLine,i,'/',&dFnt,MBF) || uS.isRightChar(cleanLine,i,'<',&dFnt,MBF) ||
			uS.isRightChar(cleanLine,i,'.',&dFnt,MBF) || uS.isRightChar(cleanLine,i,'!',&dFnt,MBF) ||
			uS.isRightChar(cleanLine,i,'?',&dFnt,MBF) || uS.isRightChar(cleanLine,i,',',&dFnt,MBF))) {
			if (i-si >= BUFSIZ-1) {
				fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
				fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
				fprintf(fpout,"%s\n", cleanLine);
				cutt_exit(0);
			}
			if (uS.isRightChar(cleanLine, i, ']', &dFnt, MBF)) {
				if (i-si >= BUFSIZ-1) {
					fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
					fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
					fprintf(fpout,"%s\n", cleanLine);
					cutt_exit(0);
				}
				*++word = EOS;
				return(i+1);
			}
		}
	} else if (uS.atUFound(cleanLine, i, &dFnt, MBF)) {
		while ((*++word=cleanLine[++i]) != EOS && (!uS.isskip(cleanLine,i,&dFnt,MBF) ||
			uS.isRightChar(cleanLine,i,'.',&dFnt,MBF) || uS.isRightChar(cleanLine,i,'!',&dFnt,MBF) ||
			uS.isRightChar(cleanLine,i,'?',&dFnt,MBF) || uS.isRightChar(cleanLine,i,',',&dFnt,MBF))) {
			if (i-si >= BUFSIZ-1) {
				fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
				fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
				fprintf(fpout,"%s\n", cleanLine);
				cutt_exit(0);
			}
			if (uS.isRightChar(cleanLine, i, ']', &dFnt, MBF)) {
				if (i-si >= BUFSIZ-1) {
					fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
					fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
					fprintf(fpout,"%s\n", cleanLine);
					cutt_exit(0);
				}
				*++word = EOS;
				return(i+1);
			}
		}
	} else {
		if (UTF8_IS_LEAD((unsigned char)cleanLine[i])) {
			if (cleanLine[i] == (char)0xE2 && cleanLine[i+1] == (char)0x80 && cleanLine[i+2] == (char)0x9C) {
				if (i-si >= BUFSIZ-4) {
					fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
					fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
					fprintf(fpout,"%s\n", cleanLine);
					cutt_exit(0);
				}
				if (cutt_isSearchForQuotedItems != 0) { // 2019-07-17
					if (cutt_isSearchForQuotedItems == 2 && sq == FALSE)
						isGetWholeQuoteItem = TRUE;
				} else {
					*++word = (char)0x80;
					*++word = (char)0x9C;
					*++word = EOS;
					return(i+3);
				}
			} else if (cleanLine[i] == (char)0xE2 && cleanLine[i+1] == (char)0x80 && cleanLine[i+2] == (char)0x9D) {
				if (i-si >= BUFSIZ-4) {
					fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
					fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
					fprintf(fpout,"%s\n", cleanLine);
					cutt_exit(0);
				}
				if (cutt_isSearchForQuotedItems != 0) { // 2019-07-17
					if (cutt_isSearchForQuotedItems == 2 && sq == FALSE)
						isGetWholeQuoteItem = FALSE;
				} else {
					*++word = (char)0x80;
					*++word = (char)0x9D;
					*++word = EOS;
					return(i+3);
				}
			}
		} else if (cleanLine[i] == ',') {
			if (i-si >= BUFSIZ-1) {
				fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
				fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
				fprintf(fpout,"%s\n", cleanLine);
				cutt_exit(0);
			}
			*++word = EOS;
			return(i+1);
		}
		if (i-si >= BUFSIZ-1) {
			fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
			fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
			fprintf(fpout,"%s\n", cleanLine);
			cutt_exit(0);
		}
		while ((*++word=cleanLine[++i]) != EOS && (!uS.isskip(cleanLine,i,&dFnt,MBF) || sq || isGetWholeQuoteItem)) {
			if (i-si >= BUFSIZ-1) {
				fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
				fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
				fprintf(fpout,"%s\n", cleanLine);
				cutt_exit(0);
			}
			if (uS.isRightChar(cleanLine, i, ']', &dFnt, MBF)) {
				if (i-si >= BUFSIZ-1) {
					fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
					fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
					fprintf(fpout,"%s\n", cleanLine);
					cutt_exit(0);
				}
				*++word = EOS;
				if (*orgWord == '[')
					uS.cleanUpCodes(orgWord, &dFnt, MBF);
				return(i+1);
			} else if (uS.isRightChar(cleanLine, i, '(', &dFnt, MBF) && uS.isPause(cleanLine, i, NULL,  &temp)) {
				*word = EOS;
				return(i);
			} else if (uS.isRightChar(cleanLine, i, '.', &dFnt, MBF) && uS.isPause(cleanLine, i, NULL, NULL)) {
				while ((*++word=cleanLine[++i]) != EOS && !uS.isRightChar(cleanLine, i, ')', &dFnt, MBF)) {
					if (i-si >= BUFSIZ-1) {
						fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
						fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
						fprintf(fpout,"%s\n", cleanLine);
						cutt_exit(0);
					}
				}
				if (i-si >= BUFSIZ-1) {
					fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
					fprintf(fpout,"ERROR: One word on the tier below has exceded 1024 characters in lenght\n");
					fprintf(fpout,"%s\n", cleanLine);
					cutt_exit(0);
				}
				*++word = EOS;
				return(i+1);
			} else if (uS.IsUtteranceDel(cleanLine, i) == 2) {
				*word = EOS;
				return(i);
			} else if (UTF8_IS_LEAD((unsigned char)cleanLine[i])) {
				if (i > 0 && cleanLine[i-1] == '|' && *sp == '%') {
				} else if (cleanLine[i] == (char)0xE2 && cleanLine[i+1] == (char)0x80 && cleanLine[i+2] == (char)0x9C) {
					if (cutt_isSearchForQuotedItems != 0) { // 2019-07-17
						if (cutt_isSearchForQuotedItems == 2 && sq == FALSE)
							isGetWholeQuoteItem = TRUE;
					} else {
						*word = EOS;
						return(i);
					}
				} else if (cleanLine[i] == (char)0xE2 && cleanLine[i+1] == (char)0x80 && cleanLine[i+2] == (char)0x9D) {
					if (cutt_isSearchForQuotedItems != 0) { // 2019-07-17
						if (cutt_isSearchForQuotedItems == 2 && sq == FALSE)
							isGetWholeQuoteItem = FALSE;
					} else {
						*word = EOS;
						return(i);
					}
				}
			} else if (cleanLine[i] == ',' && !isdigit(cleanLine[i+1])) {
				*word = EOS;
				return(i);
			}
		}
	}
	if (uS.isRightChar(cleanLine, i, '[', &dFnt, MBF)) {
		if (!uS.isSqBracketItem(cleanLine, i+1, &dFnt, MBF))
			goto getword_rep;
		else
			i--;
	}
	*word = EOS;
	if (cleanLine[i] != EOS) {
// 2012-11-1 2012-10-16 comma to %mor
		if (*sp == '*' && sp[1] == '\001' && uS.isRightChar(cleanLine,i,',',&dFnt,MBF)) {
		} else
//
			i++;
	}
	return(i);
}

/**************************************************************/
/*	 filter whole tier					  */
/* isSelectedScope(char st, int pos) determines if the scop token was selected
 by a user with +s option.
 */
static char isSelectedScope(char *wline, int pos) {
//	long i;

	return(FALSE);
/* Should a code be excluded even if user is searching for it and it is encompassed by exclude code:

 *PAR:	&uh <a hat [: cat] [* p:w-ret]> [//] a cat [/] cat up there looking up

 Following source code will still include code [* p:w-ret] in output if +s"[\* p*]" option specified even
 if it within exclude code [//].
*/
/*
	if (ScopMode != 'i' && ScopMode != 'I')
		return(FALSE);
	i = pos;
	for (pos--; !uS.isRightChar(wline, pos, '[', &dFnt, MBF) && !uS.isRightChar(wline, pos, ']', &dFnt, MBF) && pos >= 0; pos--) ;
	wline[i] = EOS;
	uS.isSqCodes(wline+pos+1, templineC3, &dFnt, TRUE);
	wline[i] = ']';
	uS.lowercasestr(templineC3, &dFnt, MBF);
	for (i=1; i < ScopNWds; i++) {
		if (uS.patmat(templineC3,ScopWdPtr[i]+3)) {
			if (isPostCodeMark(templineC3[0], templineC3[1]))
				return(FALSE);
			if (ScopWdPtr[i][2] == 'i' || ScopWdPtr[i][2] == 'I') {
				return(TRUE);
			} else
				return(FALSE);
		}
	}
	return(FALSE);
 */
}

/* ExcludeScope(wline,pos,isblankit) replaces all the character of the scoped
 data with the space character. All the scoped data within the given one
 is left untouched, i.e. <one <two> [*] three> [//] <two> [*] is left
 untouched. It return the pointer to the first character of the given
 scoped data. "beg" is a begining of the string marker, "wline" is a
 pointer to a scoped data, "isblankit" if 1 means replace data with space
 character and if 0 means leave it untouched.
 */
int ExcludeScope(char *wline, int pos, char isblankit) {
	if (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) && pos >= 0) {
		while (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) && pos >= 0) {
			if (isblankit) {
				if (isQuotes(wline, pos)) {
					pos -= 2;
				} else
					wline[pos] = ' ';
			}
			pos--;
		}
		if (uS.isRightChar(wline, pos, '[', &dFnt, MBF)) {
			if (isblankit)
				wline[pos] = ' ';
		}
	}
	if (pos < 0) {
		fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
		fprintf(stderr,"Missing '[' character\n");
		fprintf(stderr,"text=%s->%s->%d\n",wline,wline+pos,isblankit);
		return(-1);
	} else
		pos--;
	while (!uS.isRightChar(wline, pos, '>', &dFnt, MBF) && uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0) {
		if (uS.isRightChar(wline, pos, ']', &dFnt, MBF) || uS.isRightChar(wline, pos, '<', &dFnt, MBF))
			break;
		else
			pos--;
	}
	if (uS.isRightChar(wline, pos, '>', &dFnt, MBF)) {
		while (!uS.isRightChar(wline, pos, '<', &dFnt, MBF) && pos >= 0) {
			if (uS.isRightChar(wline, pos, ']', &dFnt, MBF))
				pos = ExcludeScope(wline,pos,!isSelectedScope(wline, pos));
			else if (isblankit) {
				if (isQuotes(wline, pos)) {
					pos -= 3;
				} else
					wline[pos--] = ' ';
			} else
				pos--;
		}
		if (pos < 0) {
			if (chatmode) {
				fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno+tlineno);
				fprintf(stderr,"Missing '<' character\n");
				fprintf(stderr,"text=%s\n",wline);
			}
			return(-1);
		} else if (isblankit) {
			wline[pos] = ' ';
		} else
			pos--;
	} else if (pos < 0) {
	} else if (uS.isRightChar(wline, pos, ']', &dFnt, MBF)) {
		pos = ExcludeScope(wline,pos,!isSelectedScope(wline, pos));
	} else {
		while (!uS.isskip(wline,pos,&dFnt,MBF) && pos >= 0) {
			if (uS.isRightChar(wline, pos, ']', &dFnt, MBF))
				pos = ExcludeScope(wline,pos,!isSelectedScope(wline, pos));
			else if (isblankit) {
				if (isQuotes(wline, pos)) {
					pos -= 3;
				} else
					wline[pos--] = ' ';
			} else
				pos--;
		}
	}
	return(pos);
}

void HandleParans(char *s, int beg, int end) {
	int temp, i;
	char buf[BUFSIZ+1], tParans, isXXXFound;

	temp = 0;
	tParans = 0;
	for (i=beg; s[i] != EOS; i++) {
		if (temp >= BUFSIZ)
			break;
		if (!uS.isRightChar(s, i, '(', &dFnt, MBF) && !uS.isRightChar(s, i, ')', &dFnt, MBF)) {
			buf[temp++] = s[i];
		}
	}
	isXXXFound = FALSE;
	if (temp < BUFSIZ) {
		buf[temp] = EOS;
		if (uS.mStricmp(buf, "xxx") == 0 || uS.mStricmp(buf, "yyy") == 0 || uS.mStricmp(buf, "www") == 0) {
			tParans = Parans;
			Parans = 1;
			isXXXFound = TRUE;
		}
	}
	while (s[beg]) {
		if (uS.isRightChar(s, beg, '(', &dFnt, MBF) || uS.isRightChar(s, beg, ')', &dFnt, MBF)) {
			if (Parans == 1) {
				if (uS.isRightChar(s, beg, '(', &dFnt, MBF) && s[beg+1] == '*') {
					for (temp=beg; s[temp] && !uS.isRightChar(s, temp, ')', &dFnt, MBF); temp++) ;
					if (s[temp])
						strcpy(s+beg,s+temp+1);
					else
						beg++;
				} else
					strcpy(s+beg,s+beg+1);
			} else { /* if (Parans == 3) */
				for (temp=beg; s[temp] && !uS.isRightChar(s, temp, ')', &dFnt, MBF); temp++ );
				if (s[temp])
					strcpy(s+beg,s+temp+1);
				else
					beg++;
			}
		} else
			beg++;
	}
	if (isXXXFound)
		Parans = tParans;
	if (end != 0) {
		for (; beg < end; beg++)
			s[beg] = ' ';
	}
}

static void HandleMorCAs(char *s, int beg, int end, char isAlwaysRemove) {
	int  res, i;
	char CAc[20];
	int matchType;

	while (s[beg]) {
		if ((res=uS.HandleCAChars(s+beg, &matchType)) != 0) {
			if (isRemoveCAChar[matchType] == TRUE) {
				strncpy(CAc, s+beg, res);
				CAc[res] = EOS;
				if (!isCAsearched(CAc) || isAlwaysRemove) {
					if (matchType == NOTCA_LEFT_ARROW_CIRCLE)
						removeRepeatSegments(s, beg, res);
					else
						strcpy(s+beg,s+beg+res);
					i = beg - 1;
					if (s[i] == '|') {
						for (; i >= 0 && !uS.isskip(s,i,&dFnt,MBF); i--) {
							s[i] = ' ';
						}
					}
				} else
					beg += res;
			} else
				beg += res;
		} else
			beg++;
	}
	if (end != 0) {
		for (; beg < end; beg++)
			s[beg] = ' ';
	}
}

static void HandleSpCAs(char *s, int beg, int end, char isAlwaysRemove) {
	int  res, len;
	char CAc[20];
	int matchType;

	while (s[beg]) {
		if ((res=uS.HandleCAChars(s+beg, &matchType)) != 0) {
			if (isRemoveCAChar[matchType] == TRUE) {
				strncpy(CAc, s+beg, res);
				CAc[res] = EOS;
				if (!isCAsearched(CAc) || isAlwaysRemove) {
					if (matchType == NOTCA_LEFT_ARROW_CIRCLE)
						removeRepeatSegments(s, beg, res);
					else
						strcpy(s+beg,s+beg+res);
				} else
					beg += res;
			} else
				beg += res;
		} else
			beg++;
	}
	if (end != 0) {
		len = strlen(s);
		if (end > len)
			end = len;
		for (; beg < end; beg++)
			s[beg] = ' ';
	}
}

static void HandleSlash(char *s, int beg, int end) {
	int  res;
	char sq, isATFound, spSymFound, CAc[20];
	int matchType;

	sq = FALSE;
	isATFound = FALSE;
	spSymFound = FALSE;
	if (s[beg] == '+' || s[beg] == '-')
		isATFound = TRUE;
	while (s[beg]) {
		if ((res=uS.HandleCAChars(s+beg, &matchType)) != 0) {
			if (isRemoveCAChar[matchType] == TRUE) {
				strncpy(CAc, s+beg, res);
				CAc[res] = EOS;
				if (!isCAsearched(CAc)) {
					if (matchType == NOTCA_LEFT_ARROW_CIRCLE)
						removeRepeatSegments(s, beg, res);
					else
						strcpy(s+beg,s+beg+res);
				} else
					beg += res;
			} else
				beg += res;
			continue;
		}
		if (s[beg] == '@' || s[beg] == '$')
			spSymFound = TRUE;
		if (uS.isRightChar(s, beg, '@', &dFnt, MBF) && !sq)
			isATFound = TRUE;
		if (uS.isRightChar(s, beg, '[', &dFnt, MBF))
			sq = TRUE;
		else if (uS.isRightChar(s, beg, ']', &dFnt, MBF))
			sq = FALSE;
		if (!sq && !isATFound && !spSymFound) {
			if (uS.isRightChar(s,beg,'^',&dFnt,MBF) && R7Caret) {
				strcpy(s+beg,s+beg+1);
			} else if (uS.isRightChar(s,beg,':',&dFnt,MBF) && R7Colon) {
				strcpy(s+beg,s+beg+1);
			} else if (uS.isRightChar(s,beg,'~',&dFnt,MBF) && R7Tilda) {
				beg++;
//				strcpy(s+beg,s+beg+1);
			} else if (uS.isRightChar(s,beg,'/',&dFnt,MBF) && R7Slash) {
				if ((uS.isRightChar(s,beg-1,'+',&dFnt,MBF) && beg > 0) ||
					  (uS.isRightChar(s,beg-1,'/',&dFnt,MBF) && uS.isRightChar(s,beg-2,'+',&dFnt,MBF) && beg > 1) ||
					  (uS.isRightChar(s,beg-1,'"',&dFnt,MBF) && uS.isRightChar(s,beg-2,'+',&dFnt,MBF) && beg > 1))
					beg++;
				else
					strcpy(s+beg,s+beg+1);
			} else
				beg++;
		} else
			beg++;
	}
	if (end != 0) {
		for (; beg < end; beg++)
			s[beg] = ' ';
	}
}

void findWholeWord(char *line, int wi, char *word) {
	int i;

	i = 0;
	if (wi > 0 && !uS.isskip(line,wi-1,&dFnt,MBF) && (line[wi-1] == '~' || line[wi-1] == '$')) {
		for (; wi > 0 && line[wi] != ']' && !uS.isskip(line,wi,&dFnt,MBF); wi--) ;
		if (wi < 0)
			wi = 0;
		if (line[wi] == ']' || uS.isskip(line,wi,&dFnt,MBF))
			wi++;
	}
	while (line[wi] != EOS && line[wi] != '[' && !uS.isskip(line,wi,&dFnt,MBF)) {
		word[i++] = line[wi++];
	}
	word[i] = EOS;
}

char isUttDel(char *s) {
	int i;

	if (s[0] == '+') {
		for (i=0; s[i] != EOS; i++) {
			if (uS.IsUtteranceDel(s, i))
				return(TRUE);
		}
	} else if (uS.IsUtteranceDel(s, 0))
		return(TRUE);
	return(FALSE);
}

void findWholeScope(char *line, int wi, char *word) {
	int sqb, ang, isword;
	int i, ti, eg;
	char tNOTCA_CROSSED_EQUAL, tNOTCA_LEFT_ARROW_CIRCLE;

	i = 0;
	word[i] = EOS;
	for (ti=wi; ti >= 0 && line[ti] != '[' && line[ti] != ']'; ti--) ;
	if (ti >= 0 && line[ti] == '[' && line[ti+1] == ':') {
		if (line[ti+2] == ':' && line[ti+3] == ' ') {
			wi = ti + 4;
		} else if (line[ti+2] == ' ') {
			wi = ti + 3;
		}
	}
	if ((wi > 3 && line[wi-4] == '[' && line[wi-3] == ':' && line[wi-2] == ':' && line[wi-1] == ' ') ||
		(wi > 2 && line[wi-3] == '[' && line[wi-2] == ':' && line[wi-1] == ' ')) {
		for (; line[wi] != EOS && line[wi] != ']' && line[wi] != '['; wi++) ;
		if (line[wi] == EOS || line[wi] == '[') {
			word[0] = EOS;
			return;
		}
		eg = wi+1;
		sqb = 0;
		for (; line[eg] != EOS; eg++) {
			if (line[eg] == ']')
				sqb--;
			else if (line[eg] == '[') {
				if (sqb > 0)
					break;
				sqb++;
			} else if (line[eg] == '<' && line[eg-1] != '+' && sqb <= 0) {
				eg--;
				break;
			} else if ((!uS.isskip(line,eg,&dFnt,MBF) || isUttDel(line+eg) || line[eg] == ',') && sqb <= 0) {
				eg--;
				break;
			}
		}
		if (line[eg] == '[') {
			word[0] = EOS;
			return;
		}
		sqb = 0;
		ang = 0;
		isword = 0;
		for (; wi >= 0; wi--) {
			if (line[wi] == ']')
				sqb++;
			else if (line[wi] == '[')
				sqb--;
			else if (line[wi] == '>' && line[wi-1] != '+' && sqb <= 0)
				ang++;
			else if (line[wi] == '<' && line[wi-1] != '+' && sqb <= 0)
				ang--;
			else if (uS.isskip(line,wi,&dFnt,MBF) && sqb <= 0 && ang <= 0 && isword > 0)
				break;
			else if (!uS.isskip(line,wi,&dFnt,MBF) && sqb <= 0 && isword <= 0)
				isword++;
		}
		if (wi < 0)
			wi = 0;
		while (line[wi] != EOS && wi <= eg && uS.isskip(line,wi,&dFnt,MBF))
			wi++;
		i = 0;
		if (wi <= eg) {
			for (; wi <= eg; wi++) {
				if (line[wi] == '[' && line[wi+1] == '/') {
					for (; wi <= eg && line[wi] != ']'; wi++) ;
				} else if (line[wi] != '<' && line[wi] != '>')
					word[i++] = line[wi];
			}
		}
		word[i] = EOS;
	} else if (line[wi] != '[') {
		sqb = 0;
		ang = 0;
		i = 0;
		if (!uS.isskip(line,wi,&dFnt,MBF)) {
			ti = wi;
			for (wi--; wi >= 0 && line[wi] != ']' && uS.isskip(line,wi,&dFnt,MBF); wi--) {
				if (line[wi] == ']')
					sqb++;
				else if (line[wi] == '[')
					sqb--;
				else if (line[wi] == '>' && line[wi-1] != '+' && sqb <= 0)
					ang++;
				else if (line[wi] == '<' && line[wi-1] != '+' && sqb <= 0)
					ang--;
			}
			if (ang != 0) {
				for (wi++; line[wi] != EOS && line[wi] != '<' && uS.isskip(line,wi,&dFnt,MBF); wi++) ;
				while (line[wi] != EOS && uS.isskip(line,wi,&dFnt,MBF)) {
					word[i++] = line[wi++];
				}
			} else
				wi = ti;
		}
		while (line[wi] != EOS && line[wi] != '[' && !uS.isskip(line,wi,&dFnt,MBF)) {
			word[i++] = line[wi++];
		}
		ti = i;
		while (line[wi] != EOS && uS.isskip(line,wi,&dFnt,MBF) && line[wi] != '[') {
			word[i++] = line[wi++];
		}
		if (line[wi] == '[' && line[wi+1] == '*') {
			word[i++] = line[wi++];
			while (line[wi] != EOS && line[wi] != ']' && line[wi] != '[') {
				word[i++] = line[wi++];
			}
			if (line[wi] == EOS || line[wi] == '[')
				i = ti;
			else
				word[i++] = line[wi++];
		} else
			i = ti;
		word[i] = EOS;
		if (ang != 0) {
			sqb = 0;
			ang = 0;
			for (i=0; word[i] != EOS; i++) {
				if (word[i] == ']')
					sqb++;
				else if (word[i] == '[')
					sqb--;
				else if (word[i] == '>' && sqb <= 0)
					ang++;
				else if (word[i] == '<' && sqb <= 0)
					ang--;
			}
			if (ang != 0) {
				for (i=0; word[i] != EOS; i++) {
					if (word[i] == '>' && sqb <= 0)
						word[i] = ' ';
					else if (word[i] == '<' && sqb <= 0)
						word[i] = ' ';
				}
			}
		}
	} else {
		for (wi++; line[wi] != EOS && line[wi] != ']' && line[wi] != '['; wi++) ;
		if (line[wi] == EOS || line[wi] == '[') {
			word[0] = EOS;
			return;
		}
		eg = wi+1;
		sqb = 0;
		for (; line[eg] != EOS; eg++) {
			if (line[eg] == ']')
				sqb--;
			else if (line[eg] == '[') {
				if (sqb > 0)
					break;
				sqb++;
			} else if (line[eg] == '<' && line[eg-1] != '+' && sqb <= 0) {
				eg--;
				break;
			} else if (!uS.isskip(line,eg,&dFnt,MBF) && sqb <= 0) {
				eg--;
				break;
			}
		}
		if (line[eg] == '[') {
			word[0] = EOS;
			return;
		}
		sqb = 0;
		ang = 0;
		isword = 0;
		for (; wi >= 0; wi--) {
			if (line[wi] == ']')
				sqb++;
			else if (line[wi] == '[')
				sqb--;
			else if (line[wi] == '>' && line[wi-1] != '+' && sqb <= 0)
				ang++;
			else if (line[wi] == '<' && line[wi-1] != '+' && sqb <= 0)
				ang--;
			else if (uS.isskip(line,wi,&dFnt,MBF) && sqb <= 0 && ang <= 0 && isword > 0)
				break;
			else if (!uS.isskip(line,wi,&dFnt,MBF) && sqb <= 0 && isword <= 0)
				isword++;
		}
		if (wi < 0)
			wi = 0;
		while (line[wi] != EOS && wi <= eg && uS.isskip(line,wi,&dFnt,MBF))
			wi++;
		i = 0;
		if (wi <= eg) {
			for (; wi <= eg; wi++) {
				if (line[wi] == '[' && line[wi+1] == '/') {
					for (; wi <= eg && line[wi] != ']'; wi++) ;
				} else if (line[wi] != '<' && line[wi] != '>')
					word[i++] = line[wi];
			}
		}
		word[i] = EOS;
	}
	tNOTCA_CROSSED_EQUAL = isRemoveCAChar[NOTCA_CROSSED_EQUAL];
	tNOTCA_LEFT_ARROW_CIRCLE = isRemoveCAChar[NOTCA_LEFT_ARROW_CIRCLE];
	isRemoveCAChar[NOTCA_CROSSED_EQUAL] = FALSE;
	isRemoveCAChar[NOTCA_LEFT_ARROW_CIRCLE] = FALSE;
	HandleSpCAs(word, 0, strlen(word), FALSE);
	isRemoveCAChar[NOTCA_CROSSED_EQUAL] = tNOTCA_CROSSED_EQUAL;
	isRemoveCAChar[NOTCA_LEFT_ARROW_CIRCLE] = tNOTCA_LEFT_ARROW_CIRCLE;
	uS.remFrontAndBackBlanks(word);
}

static void filterPause(char *wline, char opauseFound, char pauseFound) {
	int pos, temp, i;
	char res, pa;

	pos = 0;
	do {
		for (; uS.isskip(wline,pos,&dFnt,MBF) && wline[pos] && !uS.isRightChar(wline,pos,'[',&dFnt,MBF); pos++) ;
		pa = FALSE;
		if (wline[pos]) {
			temp = pos;
			if (uS.isRightChar(wline, pos, '[', &dFnt, MBF) && uS.isSqBracketItem(wline,pos+1,&dFnt,MBF)) {
					for (pos++; !uS.isRightChar(wline, pos, ']', &dFnt, MBF) && wline[pos]; pos++) ;
				if (wline[pos])
					pos++;
			} else if (uS.isRightChar(wline, pos, '(', &dFnt, MBF) && uS.isPause(wline, pos, NULL,  &i)) {
				pos = i + 1;
				pa = TRUE;
			} else {
				do {
					for (pos++; !uS.isskip(wline,pos,&dFnt,MBF) && wline[pos]; pos++) {
						if (uS.isRightChar(wline, pos, '.', &dFnt, MBF)) {
							if (uS.isPause(wline, pos, &i, NULL)) {
								pos = i;
								if (temp > pos)
									temp = pos;
								break;
							}
						}
					}
					if (uS.isRightChar(wline, pos, '[', &dFnt, MBF)) {
						if (uS.isSqBracketItem(wline, pos+1, &dFnt, MBF))
								break;
					} else
						break;
				} while (1) ;
			}
			res = wline[pos];
			wline[pos] = EOS;
			if (opauseFound && uS.patmat(wline+temp, "#*")) {
				for (; temp != pos; temp++)
					wline[temp] = ' ';
			} else if (pa && pauseFound) {
				for (; temp != pos; temp++)
					wline[temp] = ' ';
			}
			wline[pos] = res;
		}
	} while (wline[pos]) ;
}

static void filterscop(const char *sp, char *wline, char isExcludeThisScope) {
	int  pos, temp, t1, t2, LastPos, res, sqCnt;
	char tc;

	pos = strlen(wline) - 1;
	LastPos = pos + 1;
	while (pos >= 0) {
		if (wline[pos] == HIDEN_C) {
			temp = pos;
			for (pos--; wline[pos] != HIDEN_C && pos >= 0; pos--) ;
			if (wline[pos] == HIDEN_C && pos >= 0) {
				for (; temp >= pos; temp--)
					wline[temp] = ' ';
			}
		} else if (uS.isRightChar(wline, pos, ']', &dFnt, MBF)) {
			sqCnt = 0;
			temp = pos;
			for (pos--; (!uS.isRightChar(wline, pos, '[', &dFnt, MBF) || sqCnt > 0) && pos >= 0; pos--) {
				if (uS.isRightChar(wline, pos, ']', &dFnt, MBF))
					sqCnt++;
				else if (uS.isRightChar(wline, pos, '[', &dFnt, MBF))
					sqCnt--;
			}
			if (pos < 0) {
				if (chatmode) {
					if (!cutt_isBlobFound) {
						fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno+tlineno);
						fprintf(stderr,"Missing '[' character\n");
						fprintf(stderr,"text=%s\n",wline);
					}
					pos = temp - 1;
					if (isRecursive || cutt_isCAFound || cutt_isBlobFound)
						continue;
					else
						cutt_exit(1);
				} else
					break;
			}
			wline[temp] = EOS;
			uS.isSqCodes(wline+pos+1, templineC3, &dFnt, TRUE);
			wline[temp] = ']';
			uS.lowercasestr(templineC3, &dFnt, MBF);
			res = isExcludeScope(templineC3);
			if (isExcludeThisScope && (res == 0 || res == 100/* || res == 2*/))
				ExcludeScope(wline,pos,(char)TRUE);
			if ((res == 1 || res == 3) && ScopMode == 'I') {
				for (t2=temp+1; t2 < LastPos; t2++)
					wline[t2] = ' ';
				if (uS.isRightChar(wline, pos, '[', &dFnt, MBF))
					pos--;
				while (pos >= 0 && (sqCnt > 0 || (uS.isskip(wline,pos,&dFnt,MBF) && !uS.isRightChar(wline,pos,'>',&dFnt,MBF)))) {
					if (uS.isRightChar(wline,pos,']',&dFnt,MBF))
						sqCnt++;
					else if (uS.isRightChar(wline,pos,'[',&dFnt,MBF))
						sqCnt--;

					pos--;
				}
				if (pos >= 0) {
					if (uS.isRightChar(wline, pos, '>', &dFnt, MBF)) {
						while (pos >= 0 && !uS.isRightChar(wline, pos, '<', &dFnt, MBF)) pos--;
						if (pos < 0) {
							if (chatmode) {
								fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno+tlineno);
								fprintf(stderr,"Missing '<' character\n");
								fprintf(stderr,"text=%s\n",wline);
								cutt_exit(1);
							}
						} else
							pos--;
					} else {
						while (pos >= 0 && !uS.isskip(wline,pos,&dFnt,MBF) && !uS.isRightChar(wline,pos,']',&dFnt,MBF))
							pos--;
					}
					for (t2=temp; t2 > pos && !uS.isRightChar(wline, t2, '[', &dFnt, MBF); t2--) ;
					if (t2 > pos && wline[t2] == '[') {
						for (t2--; t2 > pos && !uS.isRightChar(wline, t2, ']', &dFnt, MBF); t2--) ;
						if (t2 > pos && wline[t2] == ']') {
							t2++;
							tc = wline[t2];
							wline[t2] = EOS;
							t1 = pos;
							if (t1 < 0)
								t1 = 0;
							if (t1 < t2)
								filterscop(sp, wline+t1, FALSE);
							wline[t2] = tc;
						}
					}
				}
				LastPos = pos + 1;
			}
			if (res == 2 && ScopMode == 'I') {
				for (t2=temp+1; t2 < LastPos; t2++)
					wline[t2] = ' ';
				LastPos = pos;
			} else if (res < 2) {
				if (strncmp(sp, "%mor:", 5) == 0 && strcmp(templineC3, "/") == 0 && isCreateFakeMor == 2) {
// 2019-08-30					res = res;
				} else {
					for (; !uS.isRightChar(wline, temp, '[', &dFnt, MBF); temp--)
						wline[temp] = ' ';
					wline[temp] = ' ';
				}
			} else if (res == 2013) {
				for (pos=0; wline[pos]; pos++)
					wline[pos] = ' ';
				return;
			} else if (res == 1001) {
				ExcludeScope(wline,pos,(char)TRUE);
				wline[temp] = ' ';
				for (; !uS.isRightChar(wline, temp, '[', &dFnt, MBF); temp--) ;
				wline[temp] = ' ';
				wline[temp+1] = ' ';
				if (wline[temp+2] == ':')
					wline[temp+2] = ' ';
				if (wline[temp+2] == '*')
					wline[temp+2] = ' ';
				if (wline[temp+2] == '=' && !isSpace(wline[temp+3])) {
					wline[temp+2] = ' ';
					wline[temp+3] = ' ';
				}
			} else if (res == 4) {
				if (postcodeRes != 5)
					postcodeRes = 4;
				t1 = isExcludePostcode(templineC3);
				if (t1 == 0 || t1 == 5) {
					for (; !uS.isRightChar(wline, temp, '[', &dFnt, MBF); temp--)
						wline[temp] = ' ';
					wline[temp] = ' ';
				}
			} else if (res == 5) {
				postcodeRes = 5;
				for (pos=0; wline[pos]; pos++)
					wline[pos] = ' ';
				return;
			} else if (res == 6) {
				if (postcodeRes != 5)
					postcodeRes = 6;
			} else if (res == 7) {
//				both [...] and <...> are preserved and if +s[...] used, then it is searched for
			}
		} else
			pos--;
	}
	if (ScopMode == 'I' && isExcludeThisScope) {
		for (pos=0; pos < LastPos; pos++)
			wline[pos] = ' ';
	}
	if (PostCodeMode == 'i' && isExcludeThisScope) {
		if (postcodeRes == 0) {
			for (pos=0; wline[pos]; pos++)
				wline[pos] = ' ';
		}
	}
	if (*sp == '%' && postcodeRes == 5) {
		for (pos=0; wline[pos]; pos++)
			wline[pos] = ' ';
	}
}

void filterwords(const char *sp, char *wline, int (*excludeP)(char *)) {
	int pos, temp, i, pc;
	char res, pa, tc, isGetWholeQuoteItem;

	if (*sp == '@') {
		if (uS.mStrnicmp(sp+1, "ID:", 3) == 0)
			return;
	}
	pos = 0;
	do {
		if (sp[0] == '%' && uS.mStrnicmp(sp+1, "mor", 3) == 0) {
			for (; uS.isskip(wline,pos,&dFnt,MBF) && (wline[pos] != '?' || wline[pos+1] != '|') && wline[pos] && wline[pos] != '[' && wline[pos] != ','; pos++) {
			}
		} else if (sp[0] == '%' && (uS.mStrnicmp(sp+1, "gra", 3) == 0 || uS.mStrnicmp(sp+1, "grt", 3) == 0)) {
			for (; uS.isskip(wline,pos,&dFnt,MBF) && (wline[pos] != '?' || wline[pos+1] != '|') && wline[pos] && wline[pos] != '[' && wline[pos] != ','; pos++) {
			}
		} else {
			for (; uS.isskip(wline,pos,&dFnt,MBF) && wline[pos] && !uS.isRightChar(wline,pos,'[',&dFnt,MBF); pos++) ;
		}
		pa = FALSE;
		if (wline[pos]) {
			temp = pos;
			if (!nomap) {
				if (MBF) {
					if (my_CharacterByteType(wline, (short)pos, &dFnt) == 0)
						wline[pos] = (char)tolower((unsigned char)wline[pos]);
				} else {
					wline[pos] = (char)tolower((unsigned char)wline[pos]);
				}
			}
			if (uS.isRightChar(wline, pos, '[', &dFnt, MBF) && uS.isSqBracketItem(wline,pos+1,&dFnt,MBF)) {
				pc = pos;
				if (MBF) {
					for (pos++; !uS.isRightChar(wline, pos, ']', &dFnt, MBF) && wline[pos]; pos++) {
						if (!nomap && my_CharacterByteType(wline, (short)pos, &dFnt) == 0)
							wline[pos] = (char)tolower((unsigned char)wline[pos]);
					}
				} else {
					for (pos++; !uS.isRightChar(wline, pos, ']', &dFnt, MBF) && wline[pos]; pos++) {
						if (!nomap)
							wline[pos] = (char)tolower((unsigned char)wline[pos]);
					}
				}
// 2014-10 POSTCODES
//				if (CLAN_PROG_NUM == FREQ) {
					if (wline[pos]) {
						tc = wline[pos];
						wline[pos] = EOS;
						uS.isSqCodes(wline+pc+1, templineC3, &dFnt, TRUE);
						wline[pos] = tc;
						pos++;
						uS.lowercasestr(templineC3, &dFnt, MBF);
						if (isExcludePostcode(templineC3) == 4) {
							for (; pc < pos; pc++) {
								wline[pc] = ' ';
							}
						}
					}
//				}
			} else if (uS.isRightChar(wline, pos, '+', &dFnt, MBF) || uS.isRightChar(wline, pos, '-', &dFnt, MBF)) {
				do {
					for (pos++; wline[pos] != EOS && (!uS.isskip(wline,pos,&dFnt,MBF) ||
						uS.isRightChar(wline,pos,'/',&dFnt,MBF) || uS.isRightChar(wline,pos,'<',&dFnt,MBF) ||
						uS.isRightChar(wline,pos,'.',&dFnt,MBF) || uS.isRightChar(wline,pos,'!',&dFnt,MBF) ||
						uS.isRightChar(wline,pos,'?',&dFnt,MBF) || uS.isRightChar(wline,pos,',',&dFnt,MBF)); pos++) {
					}
					if (uS.isRightChar(wline, pos, '[', &dFnt, MBF)) {
						if (uS.isSqBracketItem(wline, pos+1, &dFnt, MBF))
							break;
					} else
						break;
				} while (1) ;
			} else if (uS.isRightChar(wline, pos, '(', &dFnt, MBF) && uS.isPause(wline, pos, NULL,  &i)) {
				pos = i + 1;
				pa = TRUE;
			} else {
				isGetWholeQuoteItem = FALSE;
				do {
					if (UTF8_IS_LEAD((unsigned char)wline[pos])) {
						if (wline[pos] == (char)0xE2 && wline[pos+1] == (char)0x80 && wline[pos+2] == (char)0x9C) {
							if (cutt_isSearchForQuotedItems != 0) { // 2019-07-17
								if (cutt_isSearchForQuotedItems == 2)
									isGetWholeQuoteItem = TRUE;
							} else { // 2019-07-17
								pos += 3;
								break;
							}
						} else if (wline[pos] == (char)0xE2 && wline[pos+1] == (char)0x80 && wline[pos+2] == (char)0x9D) {
							if (cutt_isSearchForQuotedItems != 0) { // 2019-07-17
								if (cutt_isSearchForQuotedItems == 2)
									isGetWholeQuoteItem = FALSE;
							} else { // 2019-07-17
								pos += 3;
								break;
							}
						}
					} else if (wline[pos] == ',') {
						pos += 1;
						break;
					}
					for (pos++; (!uS.isskip(wline,pos,&dFnt,MBF) || isGetWholeQuoteItem) && wline[pos]; pos++) {
						if (uS.isRightChar(wline, pos, '.', &dFnt, MBF) && uS.isPause(wline, pos, &i, NULL)) {
							pos = i;
							if (temp > pos)
								temp = pos;
							break;
						} else if (uS.IsUtteranceDel(wline, pos) == 2) {
							if (temp > pos)
								temp = pos;
							break;
						} else if (UTF8_IS_LEAD((unsigned char)wline[pos])) {
							if (pos > 0 && wline[pos-1] == '|' && *sp == '%') {
							} else if (wline[pos] == (char)0xE2 && wline[pos+1] == (char)0x80 && wline[pos+2] == (char)0x9C) {
								if (cutt_isSearchForQuotedItems != 0) { // 2019-07-17
									if (cutt_isSearchForQuotedItems == 2)
										isGetWholeQuoteItem = TRUE;
								} else { // 2019-07-17
									break;
								}
							} else if (wline[pos] == (char)0xE2 && wline[pos+1] == (char)0x80 && wline[pos+2] == (char)0x9D) {
								if (cutt_isSearchForQuotedItems != 0) { // 2019-07-17
									if (cutt_isSearchForQuotedItems == 2)
										isGetWholeQuoteItem = FALSE;
								} else { // 2019-07-17
									break;
								}
							} 
						} else if (wline[pos] == ',' && !isdigit(wline[pos+1])) {
							break;
						}
						if (!nomap) {
							if (MBF) {
								 if (my_CharacterByteType(wline, (short)pos, &dFnt) == 0)
									wline[pos] = (char)tolower((unsigned char)wline[pos]);
							} else {
								wline[pos] = (char)tolower((unsigned char)wline[pos]);
							}
						}
					}
					if (uS.isRightChar(wline, pos, '[', &dFnt, MBF)) {
						if (uS.isSqBracketItem(wline, pos+1, &dFnt, MBF))
							break;
					} else
						break;
				} while (1) ;
			}
			res = wline[pos];
			wline[pos] = EOS;
			if (!uS.isToneUnitMarker(wline+temp) && !pa) {
				if (isMainSpeaker(*sp)) {
					if (Parans != 2) {
						HandleParans(wline,temp,pos);
						if (pos > 0 && pos > temp && isSpace(wline[pos-1])) {
							wline[pos] = res;
							for (pos--; pos > temp-1 && isSpace(wline[pos]); pos--) ;
							pos++;
							res = wline[pos];
							wline[pos] = EOS;
						}
					}
					if (R7Slash || R7Tilda || R7Caret || R7Colon) {
						if (wline[temp] != '\002' || !R8) {
							if (isWinMode) {
								if (!isTierLabel(wline+temp, (temp <= 0)))
									HandleSlash(wline,temp,pos);
							} else
								HandleSlash(wline,temp,pos);
							if (pos > 0 && pos > temp && isSpace(wline[pos-1])) {
								wline[pos] = res;
								for (pos--; pos > temp-1 && isSpace(wline[pos]); pos--) ;
								pos++;
								res = wline[pos];
								wline[pos] = EOS;
							}
						}
					} else {
						HandleSpCAs(wline,temp,pos,FALSE);
						if (pos > 0 && pos > temp && isSpace(wline[pos-1])) {
							wline[pos] = res;
							for (pos--; pos > temp-1 && isSpace(wline[pos]); pos--) ;
							pos++;
							res = wline[pos];
							wline[pos] = EOS;
						}
					}
				} else {
					if (uS.partcmp(sp,"%mor:",FALSE, FALSE)) {
						HandleMorCAs(wline,temp,pos,FALSE);
						if (pos > 0 && pos > temp && isSpace(wline[pos-1])) {
							wline[pos] = res;
							for (pos--; pos > temp-1 && isSpace(wline[pos]); pos--) ;
							pos++;
							res = wline[pos];
							wline[pos] = EOS;
						}
					}
				}
			}
			if (excludeP != NULL) {
				if (!(*excludeP)(wline+temp)) {
					for (; temp < pos; temp++)
						wline[temp] = ' ';
				} else {
					for (; temp < pos; temp++) {
						if (wline[temp] == EOS)
							wline[temp] = ' ';
					}
				}
			}
			wline[pos] = res;
		}
	} while (wline[pos]) ;
}

/* filterData(sp,wline) filters out all the wrong data from the string pointed
   to by "wline". "sp" is the first character in a speaker string.
*/
void filterData(const char *sp, char *wline) {
	if (*sp != '@') {
		if (opauseFound || pauseFound)
			filterPause(wline, opauseFound, pauseFound);
		filterscop(sp, wline, TRUE);
	}
	filterwords(sp,wline,excludedef);
}

FILE *OpenGenLib(const FNType *fname, const char *mode, char checkWD, char checkSubDir, FNType *mFileName) {
	FILE *fp;
	int  t;
	FNType mDirPathName[FNSize];
	struct dirent *dp;
	struct stat sb;
	DIR *cDIR;

    #ifdef _DOS
        fp = fopen(fname, mode);
        strcpy(mFileName, fname);
    #else
        if (!isRefEQZero(fname)) {
            fp = fopen(fname, mode);
            strcpy(mFileName, fname);
        } else if (checkWD) {
            strcpy(mFileName, wd_dir);
            addFilename2Path(mFileName, fname);
            fp = fopen(mFileName, mode);
        } else {
            mFileName[0] = EOS;
            fp = NULL;
        }	
        if (!checkWD) {
            strcpy(mFileName, wd_dir);
            addFilename2Path(mFileName, fname);
            fp = fopen(mFileName, mode);
        }
        if (fp == NULL) {
            getcwd(mDirPathName, FNSize);
            strcpy(mFileName,lib_dir);
            t = strlen(mFileName);
            addFilename2Path(mFileName, fname);
            fp = fopen(mFileName, mode);
            if (checkSubDir && fp == NULL) {
                SetNewVol(lib_dir);
                if ((cDIR=opendir(".")) != NULL) {
                    while ((dp=readdir(cDIR)) != NULL) {
                        if (stat(dp->d_name, &sb) == 0) {
                            if (!S_ISDIR(sb.st_mode)) {
                                continue;
                            }
                        } else
                            continue;
                        if (dp->d_name[0] == '.')
                            continue;
                        mFileName[t] = EOS;
                        addFilename2Path(mFileName, dp->d_name);
                        addFilename2Path(mFileName, fname);
                        fp = fopen(mFileName, mode);
                        if (fp != NULL) {
                            break;
                        }
                    }
                    closedir(cDIR);
                }
            }
            SetNewVol(mDirPathName);
            if (fp == NULL) {
                strcpy(mFileName,lib_dir);
                addFilename2Path(mFileName, fname);
            }
        }
    #endif
    return(fp);
}

FILE *OpenMorLib(const FNType *fname, const char *mode, char checkWD, char checkSubDir, FNType *mFileName) {
	FILE *fp;
	int  t;
	struct dirent *dp;
	struct stat sb;
	DIR *cDIR;

	if (checkWD) {
		strcpy(mFileName, wd_dir);
		addFilename2Path(mFileName, fname);
		fp = fopen(mFileName, mode);
	} else {
		mFileName[0] = EOS;
		fp = NULL;
	}

	if (fp == NULL) {
		strcpy(mFileName, mor_lib_dir);
		t = strlen(mFileName);
		addFilename2Path(mFileName, fname);
		fp = fopen(mFileName, mode);
		if (fp == NULL) {
			strcpy(mFileName,mor_lib_dir);
			addFilename2Path(mFileName, fname);
		}
	}
	return(fp);
}

void init_punct(char which) {
	if (which == 0)
		strcpy(cedPunctuation, PUNCTUATION_SET);
	else
		strcpy(cedPunctuation, PUNCT_PHO_MOD_SET);
	punctuation = cedPunctuation;
	morPunctuation[0] = EOS;
}
/**************************************************************/
/*	 main initialization routines				  */
void mmaininit(void) {
	int i;

	for (i=0; i <= NUMSPCHARS; i++) // up-arrow
		isRemoveCAChar[i] = TRUE;

	isRemoveCAChar[NOTCA_STRESS] = FALSE;
	isRemoveCAChar[NOTCA_GLOTTAL_STOP] = FALSE;
	isRemoveCAChar[NOTCA_HEBREW_GLOTTAL] = FALSE;
	isRemoveCAChar[NOTCA_CARON] = FALSE;
	isRemoveCAChar[NOTCA_VOWELS_COLON] = FALSE;
	Parans = 1;
	nomain = FALSE;
	FromWU = 0;
	fpin = NULL;
	fpout = NULL;
	wdptr = NULL;
	defwdptr = NULL;
	mwdptr = NULL;
	morfptr = NULL;
	grafptr = NULL;
	CAptr = NULL;
	linkDep2Other = FALSE;
	linkMain2Mor = FALSE;
	isCreateFakeMor = 0;
	isExpendX = TRUE;
	isKWAL2013 = FALSE;
	anyMultiOrder = FALSE;
	onlySpecWsFound = FALSE;
	isPostcliticUse = FALSE;
	isPrecliticUse = FALSE;
	isMultiMorSearch = FALSE;
	isMultiGraSearch = FALSE;
	cutt_isMultiFound = FALSE;
	cutt_isCAFound = FALSE;
	cutt_isBlobFound = FALSE;
	cutt_isSearchForQuotedItems = FALSE; // 2019-07-17
	ml_root_clause = NULL;
	totalFilesNum = 0; // 2019-04-18 TotalNumFiles
	ByTurn = 0;
	filterUttLen_cmp = 0;
	filterUttLen = 0L;
	restoreXXX = FALSE;
	restoreYYY = FALSE;
	restoreWWW = FALSE;
	mor_link.error_found = FALSE;
	mor_link.fname[0] = EOS;
	mor_link.lineno = 0L;
	y_option = 0;
	f_override = FALSE;
	isWOptUsed = FALSE;
	Toldsp = NULL;
	TSoldsp = NULL;
	MAXOUTCOL = 76L;
	R4 = FALSE;
	if (CLAN_PROG_NUM == WDLEN)
		R5 = FALSE;
	else
		R5 = TRUE;
	R5_1 = FALSE;
	if (CLAN_PROG_NUM == MLT || CLAN_PROG_NUM == MLU || CLAN_PROG_NUM == MODREP || CLAN_PROG_NUM == REPEAT ||
		CLAN_PROG_NUM == FLO || CLAN_PROG_NUM == FREQ || CLAN_PROG_NUM == DSS || CLAN_PROG_NUM == RETRACE || CLAN_PROG_NUM == MOR_P ||
		CLAN_PROG_NUM==CMDI_P || CLAN_PROG_NUM==LAB2CHAT ||
		CLAN_PROG_NUM == MORTABLE || CLAN_PROG_NUM == CORELEX || CLAN_PROG_NUM == IPSYN ||
		CLAN_PROG_NUM == EVAL || CLAN_PROG_NUM == EVALD || CLAN_PROG_NUM == KIDEVAL || CLAN_PROG_NUM == MAXWD ||
		CLAN_PROG_NUM == SCRIPT_P || CLAN_PROG_NUM == TIMEDUR || CLAN_PROG_NUM == WDLEN ||
		CLAN_PROG_NUM == CHAT2CONLL || CLAN_PROG_NUM == CONLL2CHAT || CLAN_PROG_NUM == FLUCALC || CLAN_PROG_NUM == C_NNLA ||
		CLAN_PROG_NUM == C_QPA || CLAN_PROG_NUM == SUGAR)
		R6 = FALSE;
	else
		R6 = TRUE;
	R6_override = FALSE;
	R6_freq = FALSE;
	R7Slash = TRUE;
	R7Tilda = TRUE;
	R7Caret = TRUE;
	R7Colon = TRUE;
	R8 = FALSE;
	if (CLAN_PROG_NUM == FREQ || CLAN_PROG_NUM == DSS || CLAN_PROG_NUM == PHONFREQ ||
		CLAN_PROG_NUM == RELY || CLAN_PROG_NUM == CP2UTF || CLAN_PROG_NUM == LOWCASE || CLAN_PROG_NUM == ORT ||
		CLAN_PROG_NUM == MOR_P || CLAN_PROG_NUM == TRNFIX || CLAN_PROG_NUM == POSTMORTEM ||
		CLAN_PROG_NUM == CMDI_P || CLAN_PROG_NUM == LAB2CHAT || CLAN_PROG_NUM == MORTABLE || CLAN_PROG_NUM == CORELEX ||
		CLAN_PROG_NUM == FLO || CLAN_PROG_NUM == IPSYN || CLAN_PROG_NUM == SCRIPT_P || CLAN_PROG_NUM == CHAT2SRT ||
		CLAN_PROG_NUM == TIMEDUR || CLAN_PROG_NUM == EVAL || CLAN_PROG_NUM == EVALD || CLAN_PROG_NUM == KIDEVAL ||
		CLAN_PROG_NUM == CHAT2CONLL || CLAN_PROG_NUM == CONLL2CHAT || CLAN_PROG_NUM == FLUCALC ||
		CLAN_PROG_NUM == VOCD || CLAN_PROG_NUM == CHIP || CLAN_PROG_NUM == C_NNLA || CLAN_PROG_NUM == C_QPA ||
		CLAN_PROG_NUM == SUGAR || CLAN_PROG_NUM == CODES_P)
		nomap = TRUE;
	else
		nomap = FALSE;
	isSpecRepeatArrow = FALSE;
	MBF = FALSE;
	isSearchMorSym = FALSE;
	WUCounter = 0L;
	contSpeaker[0] = EOS;
	OverWriteFile = FALSE;
	AddCEXExtension = ".cex";
	fontErrorReported = FALSE;
	IncludeAllDefaultCodes = FALSE;
	FilterTier = 2;
	SPRole = NULL;
	IDField = NULL;
	CODEField = NULL;
	headtier = NULL;
	ProgDrive = '\0';
	OutputDrive = '\0';
	HeadOutTier = NULL;
	combinput = FALSE;
	isRecursive = FALSE;
	ScopWdPtr[0] = NULL;
	Preserve_dir = TRUE;
	opauseFound = FALSE;
	pauseFound = FALSE;
	RemPercWildCard = TRUE;
	LocalTierSelect = FALSE;
	cMediaFileName[0] = EOS;
//2011-01-26	uS.str2FNType(punctFile, 0L, "punct.cut");
//2011-01-26	punctMess[0] = EOS;
	strcpy(GlobalPunctuation, PUNCTUATION_SET);
	punctuation = GlobalPunctuation;
	morPunctuation[0] = EOS;
	rootmorf = (char *)malloc(strlen(MORPHS)+1);
	if (rootmorf == NULL) {
		fprintf(stderr,"No more space left in core.\n");
		cutt_exit(0);
	}
	strcpy(rootmorf,MORPHS);
	fDepTierName[0] = EOS;
	sDepTierName[0] = EOS;
	fatal_error_function = NULL;
	utterance = NEW(UTTER);
	if (utterance == NULL) {
		fprintf(stderr,"No more space left in core.\n");
		cutt_exit(0);
	}
	*utterance->speaker	= EOS;
	*utterance->attSp	= EOS;
	*utterance->line	= EOS;
	*utterance->attLine	= EOS;
	utterance->nextutt = utterance;
	if (isWinMode) {
		lutter = utterance;
		uttline = utterance->tuttline;
	} else {
		if (UttlineEqUtterance)
			uttline = utterance->line;
		else
			uttline = utterance->tuttline;
	}
	if (!chatmode)
		defheadtier = NULL;
	else {
		defheadtier = NEW(struct tier);
		defheadtier->include = FALSE;
		defheadtier->pat_match = FALSE;
		strcpy(defheadtier->tcode,"@");
		if (chatmode != 2 && chatmode != 4) {
			defheadtier->nexttier = NEW(struct tier);
			strcpy(defheadtier->nexttier->tcode,"%");
			defheadtier->nexttier->include = FALSE;
			defheadtier->nexttier->pat_match = FALSE;
			defheadtier->nexttier->nexttier = NULL;
		} else
			defheadtier->nexttier = NULL;
	}
}
/* ///
// global init
void globinit(void) {
	int i;

	for (i=0; i <= NUMSPCHARS; i++) // up-arrow
		isRemoveCAChar[i] = TRUE;
	isRemoveCAChar[NOTCA_STRESS] = FALSE;
	isRemoveCAChar[NOTCA_GLOTTAL_STOP] = FALSE;
	isRemoveCAChar[NOTCA_HEBREW_GLOTTAL] = FALSE;
	isRemoveCAChar[NOTCA_CARON] = FALSE;
	isRemoveCAChar[NOTCA_VOWELS_COLON] = FALSE;
	tcs = FALSE;
	tch = FALSE;
	tct = FALSE;
	otcs = TRUE;
	otch = TRUE;
	otct = TRUE;
	otcdt = FALSE;
	tcode = FALSE;
	MBF = FALSE;
	isSearchMorSym = FALSE;
	IncludeAllDefaultCodes = FALSE;
	isKWAL2013 = FALSE;
	fDepTierName[0] = EOS;
	sDepTierName[0] = EOS;
	fatal_error_function = NULL;
	utterance = NULL;
	currentchar = 0;
	currentatt = 0;
	getc_cr_lc = '\0';
	fgets_cr_lc = '\0';
	aftwin = 0;
	rightspeaker = 1;
	mor_link.error_found = FALSE;
	mor_link.fname[0] = EOS;
	mor_link.lineno = 0L;
	f_override = FALSE;
	isWOptUsed = FALSE;
	cutt_isMultiFound = FALSE;
	cutt_isCAFound = FALSE;
	cutt_isBlobFound = FALSE;
	cutt_isSearchForQuotedItems = FALSE; // 2019-07-17
	ml_root_clause = NULL;
	if (CLAN_PROG_NUM == FREQ || CLAN_PROG_NUM == DSS || CLAN_PROG_NUM == PHONFREQ ||
		CLAN_PROG_NUM == RELY || CLAN_PROG_NUM == CP2UTF || CLAN_PROG_NUM == LOWCASE || CLAN_PROG_NUM == ORT ||
		CLAN_PROG_NUM == MOR_P || CLAN_PROG_NUM == TRNFIX || CLAN_PROG_NUM == POSTMORTEM ||
		CLAN_PROG_NUM == CMDI_P || CLAN_PROG_NUM == LAB2CHAT || CLAN_PROG_NUM == MORTABLE || CLAN_PROG_NUM == CORELEX ||
		CLAN_PROG_NUM == FLO || CLAN_PROG_NUM == IPSYN || CLAN_PROG_NUM == SCRIPT_P || CLAN_PROG_NUM == CHAT2SRT ||
		CLAN_PROG_NUM == TIMEDUR || CLAN_PROG_NUM == EVAL || CLAN_PROG_NUM == EVALD || CLAN_PROG_NUM == KIDEVAL ||
		CLAN_PROG_NUM == CHAT2CONLL || CLAN_PROG_NUM == CONLL2CHAT || CLAN_PROG_NUM == FLUCALC ||
		CLAN_PROG_NUM == VOCD || CLAN_PROG_NUM == CHIP || CLAN_PROG_NUM == C_NNLA || CLAN_PROG_NUM == C_QPA ||
		CLAN_PROG_NUM == SUGAR || CLAN_PROG_NUM == CODES_P)
		nomap = TRUE;
	else
		nomap = FALSE;
	isSpecRepeatArrow = FALSE;
	getrestline = 1;
	isExpendX = TRUE;
	skipgetline = FALSE;
	linkDep2Other = FALSE;
	linkMain2Mor = FALSE;
	isCreateFakeMor = 0;
	FirstTime = TRUE;
	delSkipedFile = NULL;
	isFileSkipped = FALSE;
	replaceFile = FALSE;
	restoreXXX = FALSE;
	restoreYYY = FALSE;
	restoreWWW = FALSE;
	totalFilesNum = 0; // 2019-04-18 TotalNumFiles
	onlydata = 0;
	puredata = 2;
	outputOnlyData = FALSE;
	WordMode = '\0';
	MorWordMode = '\0';
	ScopMode = '\0';
	ScopNWds = 1;
	PostCodeMode = '\0';
	lineno = 0L;
	tlineno = 1L;
	deflineno = 0L;
	stout = TRUE;
	stin  = FALSE;
	targs = NULL;
}
*/

IEWORDS *freeIEWORDS(IEWORDS *ptr) {
	IEWORDS *t;

	while (ptr != NULL) {
		t = ptr;
		ptr = ptr->nextword;
		if (t->word)
			free(t->word);
		free(t);
	}
	return(ptr);
}

IEMWORDS *freeIEMWORDS(IEMWORDS *ptr) {
	int i;
	IEMWORDS *t;

	while (ptr != NULL) {
		t = ptr;
		ptr = ptr->nextword;
		for (i=0; i < t->total; i++) {
			if (t->word_arr[i])
				free(t->word_arr[i]);
			if (t->morwords_arr[i])
				t->morwords_arr[i] = freeMorWords(t->morwords_arr[i]);
			if (t->grafptr_arr[i])
				t->grafptr_arr[i] = freeGraWords(t->grafptr_arr[i]);
		}
		free(t);
	}
	return(ptr);
}

void clean_s_option(void) {
	int num;

	if (ScopWdPtr[0] != NULL)
		free(ScopWdPtr[0]);
	ScopWdPtr[0] = NULL;
	for (num=1; num < ScopNWds; num++)
		free(ScopWdPtr[num]);
	morfptr = freeMorWords(morfptr);
	grafptr = freeGraWords(grafptr);
	wdptr = freeIEWORDS(wdptr);
	mwdptr = freeIEMWORDS(mwdptr);
	defwdptr = freeIEWORDS(defwdptr);
	CAptr = freeIEWORDS(CAptr);
	ScopNWds = 1;
	pauseFound = FALSE;
	PostCodeMode = '\0';
	ScopMode = '\0';
	WordMode = '\0';
	MorWordMode = '\0';
}

void main_cleanup(void) {
	struct tier *tt;
	struct IDtype *tID;
	UTTER *ttt;

	clean_s_option();
	if (targs) {
		free(targs);
		targs = NULL;
	}
	if (rootmorf) {
		free(rootmorf);
		rootmorf = NULL;
	}
	ml_free_clause();
	if (Toldsp != NULL) {
		free(Toldsp);
		Toldsp = NULL;
	}
	if (TSoldsp != NULL) {
		free(TSoldsp);
		TSoldsp = NULL;
	}
	while (defheadtier != NULL) {
		tt = defheadtier;
		defheadtier = defheadtier->nexttier;
		free(tt);
	}
	while (headtier != NULL) {
		tt = headtier;
		headtier = headtier->nexttier;
		free(tt);
	}
	while (IDField != NULL) {
		tID = IDField;
		IDField = IDField->next_ID;
		free(tID);
	}
	while (CODEField != NULL) {
		tID = CODEField;
		CODEField = CODEField->next_ID;
		free(tID);
	}
	while (SPRole != NULL) {
		tID = SPRole;
		SPRole = SPRole->next_ID;
		free(tID);
	}
	while (HeadOutTier != NULL) {
		tt = HeadOutTier;
		HeadOutTier = HeadOutTier->nexttier;
		free(tt);
	}
	if (utterance != NULL) {
		ttt = utterance->nextutt;
		utterance->nextutt = NULL;
		utterance = ttt;
		while (utterance != NULL) {
			ttt = utterance;
			utterance = utterance->nextutt;
			free(ttt);
		}
	}
}

void CleanUpTempIDSpeakers(void) {
	struct tier *t, *tt;

	t = headtier;
	tt = t;
	while (t != NULL) {
		if (t->used == '\002') {
			if (t == headtier) {
				headtier = t->nexttier;
				tt = headtier;
				free(t);
				t = headtier;
			} else {
				tt->nexttier = t->nexttier;
				free(t);
				t = tt->nexttier;
			}
		} else {
			tt = t;
			t = t->nexttier;
		}
	}
}

void cutt_exit(int i) {
	if (i != 1 && i != -1) {
		if (fpin != NULL && fpin != stdin) {
			fclose(fpin);
			fpin = NULL;
		}
		main_cleanup();
	}
	if (fpout != NULL && fpout != stderr && fpout != stdout) {
		if (i != -1) {
			fclose(fpout);
/*			unlink(newfname); */
			fpout = NULL;
		}
		if (CLAN_PROG_NUM != RELY)
			fprintf(stderr,"\nCURRENT OUTPUT FILE \"%s\" IS INCOMPLETE.\n",newfname);
	}
	exit(i);
}

void cleanUpIDTier(char *st) {
	int i;

	uS.remFrontAndBackBlanks(st);
	for (i=0; st[i] != EOS; ) {
		if (st[i] == '\t' || st[i] == '\n') {
			if (i > 0 && isalpha(st[i-1])) {
				st[i] = '_';
				i++;
			}
			while (st[i] == '\t' || st[i] == '\n')
				strcpy(st+i, st+i+1);
		} else
			i++;
	}
}

static void AddToCode(const char *ID, char inc) {
	int j;
	struct IDtype *tID;

	if (CODEField == NULL) {
		tID = NEW(struct IDtype);
		if (tID == NULL)
			out_of_mem();
		CODEField = tID;
	} else {
		for (tID=CODEField; tID->next_ID != NULL; tID=tID->next_ID) {
//			if (tID->inc != inc) {
//			}
		}
//		if (tID->inc != inc) {
//		}
		tID->next_ID = NEW(struct IDtype);
		if (tID->next_ID == NULL)
			out_of_mem();
		tID = tID->next_ID;
	}
	tID->next_ID = NULL;
	for (j=0; ID[j] == ' ' || ID[j] == '\t'; j++) ;
	strcpy(tID->ID, ID+j);
	uS.remblanks(tID->ID);
	uS.lowercasestr(tID->ID, &dFnt, FALSE);
	tID->inc = inc;
	tID->used = FALSE;
	if (inc == '+')
		tcode = TRUE;
}

static char CheckCodeNumber(char *line) {
	int  i, j;
	char t, keyword[1025];
	struct IDtype *tID;

	strcpy(templineC, line);
	uS.remblanks(templineC);
	uS.lowercasestr(templineC, &dFnt, FALSE);

	i = 0;
	while (templineC[i]) {
		for (; uS.isskip(templineC, i, &dFnt, MBF) || templineC[i] == '\n' || templineC[i] == '.'; i++) ;
		if (templineC[i] == EOS)
			break;
		for (j=i; !uS.isskip(templineC, j, &dFnt, MBF) && templineC[j] != '.' && templineC[j] != '\n' && templineC[j] != EOS; j++) ;
		t = templineC[j];
		templineC[j] = EOS;
		strncpy(keyword, templineC+i, 1024);

		for (tID=CODEField; tID != NULL; tID=tID->next_ID) {
			if (uS.patmat(keyword, tID->ID)) {
				if (i > 0)
					strcpy(templineC, templineC+i);
				tID->used = TRUE;
				return(TRUE);
			}
		}

		templineC[j] = t;
		i = j;
	}

	return(FALSE);
}

char isAge(char *b, int *agef, int *aget) {
	int  ageNum;
	char *s;

	for (s=b; *s != EOS; s++) {
		if (!isdigit(*s) && *s != ';' && *s != '.' && *s != '~' && *s != '-' && *s != ' ')
			return(FALSE);
	}
	*agef = 0;
	*aget = 0;
	for (s=b; isSpace(*s) || *s == '~'; s++) ;
	if (*s == EOS)
		return(TRUE);
	ageNum = atoi(s) * 12;
	for (; *s != ';' && *s != '-' && *s != EOS; s++) ;
	if (*s == ';') {
		s++;
		if (isdigit(*s))
			ageNum = ageNum + atoi(s);
		for (; *s != '-' && *s != EOS; s++) ;
	}
	*agef = ageNum;
	if (*s == EOS)
		return(TRUE);
	for (s++; isSpace(*s) || *s == '~'; s++) ;
	ageNum = atoi(s) * 12;
	for (; *s != ';' && *s != '-' && *s != EOS; s++) ;
	if (*s == ';') {
		s++;
		if (isdigit(*s))
			ageNum = ageNum + atoi(s);
		else
			ageNum = ageNum + 11;
	} else
		ageNum = ageNum + 11;
	*aget = ageNum;
	return(TRUE);
}

void breakIDsIntoFields(struct IDparts *idTier, char *IDs) {
	char *s;

	for (s=IDs; isSpace(*s); s++) ;
	idTier->lang = s;
	idTier->corp = NULL;
	idTier->code = NULL;
	idTier->ages = NULL;
	idTier->sex = NULL;
	idTier->group = NULL;
	idTier->SES = NULL;
	idTier->role = NULL;
	idTier->edu = NULL;
	idTier->UNQ = NULL;
	if ((s=strchr(idTier->lang, '|')) != NULL) {
		*s++ = EOS;
		idTier->corp = s;
		if ((s=strchr(idTier->corp, '|')) != NULL) {
			*s++ = EOS;
			idTier->code = s;
			if ((s=strchr(idTier->code, '|')) != NULL) {
				*s++ = EOS;
				idTier->ages = s;
				if ((s=strchr(idTier->ages, '|')) != NULL) {
					*s++ = EOS;
					idTier->sex = s;
					if ((s=strchr(idTier->sex, '|')) != NULL) {
						*s++ = EOS;
						idTier->group = s;
						if ((s=strchr(idTier->group, '|')) != NULL) {
							*s++ = EOS;
							idTier->SES = s;
							if ((s=strchr(idTier->SES, '|')) != NULL) {
								*s++ = EOS;
								idTier->role = s;
								if ((s=strchr(idTier->role, '|')) != NULL) {
									*s++ = EOS;
									idTier->edu = s;
									if ((s=strchr(idTier->edu, '|')) != NULL) {
										*s++ = EOS;
										idTier->UNQ = s;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

static void AddToID(const char *ID, char inc) {
	int j;
	struct IDtype *tID;

	if (IDField == NULL && inc != '-')
		maketierchoice("*:", '+', '\001');
	tID = NEW(struct IDtype);
	if (tID == NULL)
		out_of_mem();
	tID->next_ID = IDField;
	IDField = tID;
	for (j=0; ID[j] == ' ' || ID[j] == '\t'; j++) ;
	strcpy(tID->ID, ID+j);
	uS.remblanks(tID->ID);
	uS.lowercasestr(tID->ID, &dFnt, FALSE);
	tID->inc = inc;
	tID->used = FALSE;
}

void SetIDTier(char *line) {
	int i, j;
	struct IDtype *tID;

	strcpy(templineC2, line);
	for (j=0; templineC2[j] != '|' && templineC2[j] != EOS; j++) ;
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	for (j++; templineC2[j] != '|' && templineC2[j] != EOS; j++) ;
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	uS.remblanks(templineC2);
	uS.lowercasestr(templineC2, &dFnt, FALSE);
	for (i=0,j++; templineC2[j] != '|' && templineC2[j] != EOS; j++, i++) {
		templineC3[i] = templineC2[j];
	}
	templineC3[i] = EOS;
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	for (tID=IDField; tID != NULL; tID=tID->next_ID) {
		if (uS.patmat(templineC2, tID->ID)) {
			for (j=0; isSpace(templineC3[j]); j++) ;
			maketierchoice(templineC3+j, tID->inc, '\002');
			tID->used = TRUE;
		}
	}
}

static char CheckIDNumber(char *line, char *copyST) {
	struct IDtype *tID;
//	struct IDparts IDTier;

	strcpy(copyST, line);
	uS.remblanks(copyST);
	uS.lowercasestr(copyST, &dFnt, FALSE);
//	breakIDsIntoFields(&IDTier, copyST);
	for (tID=IDField; tID != NULL; tID=tID->next_ID) {
		if (uS.patmat(copyST, tID->ID)) {
			strcpy(copyST, line);
			uS.remblanks(copyST);
			return(TRUE);
		}
	}
	return(FALSE);
}

static void AddToSPRole(const char *ID, char inc) {
	int j;
	struct IDtype *tID;

	tID = NEW(struct IDtype);
	if (tID == NULL)
		out_of_mem();
	tID->next_ID = SPRole;
	SPRole = tID;
	for (j=0; ID[j] == ' ' || ID[j] == '\t'; j++) ;
	strcpy(tID->ID, ID+j);
	uS.remblanks(tID->ID);
	uS.uppercasestr(tID->ID, &dFnt, FALSE);
	tID->inc = inc;
	tID->used = FALSE;
	if (inc == '+')
		tcs = TRUE;
}

void SetSPRoleIDs(char *line) {
	int i, j;
	char sp[SPEAKERLEN];
	struct IDtype *tID;

	strcpy(templineC2, line);
	for (j=0; templineC2[j] != '|' && templineC2[j] != EOS; j++) ;
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	for (j++; templineC2[j] != '|' && templineC2[j] != EOS; j++) ;
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	for (i=0,j++; templineC2[j] != '|' && templineC2[j] != EOS; j++, i++) {
		sp[i] = templineC2[j];
	}
	sp[i] = EOS;
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	for (j++; templineC2[j] != '|' && templineC2[j] != EOS; j++) ;
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	for (j++; templineC2[j] != '|' && templineC2[j] != EOS; j++) ;
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	for (j++; templineC2[j] != '|' && templineC2[j] != EOS; j++) ;
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	for (j++; templineC2[j] != '|' && templineC2[j] != EOS; j++) ;
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	for (i=0,j++; templineC2[j] != '|' && templineC2[j] != EOS; j++, i++) {
		templineC[i] = templineC2[j];
	}
	templineC[i] = EOS;
	uS.remblanks(templineC);
	uS.uppercasestr(templineC, &dFnt, C_MBF);
	if (templineC2[j] == EOS) {
		uS.remblanks(templineC2);
		fprintf(stderr,"Illegal @ID field format: %s. In file %s\n", templineC2, oldfname);
		cutt_exit(1);
	}
	for (tID=SPRole; tID != NULL; tID=tID->next_ID) {
		if (uS.patmat(templineC, tID->ID)) {
			maketierchoice(sp, tID->inc, '\002');
			tID->used = TRUE;
		}
	}
}

void SetSPRoleParticipants(char *line) {
	int b, e, t;
	char sp[SPEAKERLEN];
	struct IDtype *tID;

	e = strlen(line) - 1;
	while (e >= 0) {
		for (; (isSpace(line[e]) || line[e] == '\n' || line[e] == ',') && e >= 0; e--) ;
		if (e < 0)
			break;
		for (b=e; !isSpace(line[b]) && line[b] != '\n' && line[b] != ',' && b >= 0; b--) ;
		strncpy(templineC, line+b+1, e-b);
		templineC[e-b] = EOS;
		uS.remblanks(templineC);
		uS.uppercasestr(templineC, &dFnt, C_MBF);
		for (; (isSpace(line[b]) || line[b] == '\n') && b >= 0; b--) ;
		if (line[b] == ',' || b < 0) {
			fprintf(stderr,"Corrupted \"%s\" tier in file %s\n", PARTICIPANTS, oldfname);
			cutt_exit(1);
		}
		e = b;
		for (; !isSpace(line[b]) && line[b] != '\n' && line[b] != ',' && b >= 0; b--) ;
		for (t=b; (isSpace(line[t]) || line[t] == '\n') && t >= 0; t--) ;
		if (line[t] == ',' || t < 0) {
			strncpy(sp, line+b+1, e-b);
			sp[e-b] = EOS;
			uS.remblanks(sp);
			uS.uppercasestr(sp, &dFnt, FALSE);
			e = t - 1;
		} else {
			e = t;
			b = t;
			for (; !isSpace(line[b]) && line[b] != '\n' && line[b] != ',' && b >= 0; b--) ;
			for (t=b; (isSpace(line[t]) || line[t] == '\n') && t >= 0; t--) ;
			if (line[t] != ',' && t >= 0) {
				fprintf(stderr,"Corrupted \"%s\" tier in file %s\n", PARTICIPANTS, oldfname);
				cutt_exit(1);
			}
			strncpy(sp, line+b+1, e-b);
			sp[e-b] = EOS;
			uS.remblanks(sp);
			uS.uppercasestr(sp, &dFnt, FALSE);
			e = t - 1;
		}
		for (tID=SPRole; tID != NULL; tID=tID->next_ID) {
			if (uS.patmat(templineC, tID->ID)) {
				maketierchoice(sp, tID->inc, '\002');
				tID->used = TRUE;
			}
		}
	}
}

void InitOptions(void) {
	register int i;

	if (options_already_inited)
		return;
	options_already_inited = 1;
	for (i=0; i < LAST_CLAN_PROG; i++) {
		options_ext[i][0] = EOS;
		option_flags[i] = F_OPTION + T_OPTION;
	}
	option_flags[CHAINS] = F_OPTION+RE_OPTION+K_OPTION+P_OPTION+SP_OPTION+SM_OPTION+T_OPTION+UP_OPTION+Z_OPTION+D_OPTION+L_OPTION;
	strncpy(options_ext[CHAINS], ".chain", OPTIONS_EXT_LEN); options_ext[CHAINS][OPTIONS_EXT_LEN] = EOS;
	option_flags[CHIP] = F_OPTION+RE_OPTION+D_OPTION+K_OPTION+P_OPTION+R_OPTION+T_OPTION+UP_OPTION+UM_OPTION+L_OPTION;
	strncpy(options_ext[CHIP], ".chip", OPTIONS_EXT_LEN); options_ext[CHIP][OPTIONS_EXT_LEN] = EOS;
	option_flags[C_NNLA] = F_OPTION+RE_OPTION+SM_OPTION+T_OPTION+UP_OPTION;
	strncpy(options_ext[C_NNLA], ".cnl", OPTIONS_EXT_LEN); options_ext[C_NNLA][OPTIONS_EXT_LEN] = EOS;
	option_flags[CODES_P] = F_OPTION+RE_OPTION+SP_OPTION+SM_OPTION+T_OPTION+FR_OPTION+UP_OPTION;
	strncpy(options_ext[CODES_P], ".coded", OPTIONS_EXT_LEN); options_ext[CODES_P][OPTIONS_EXT_LEN] = EOS;
	option_flags[COMBO] = F_OPTION+RE_OPTION+K_OPTION+P_OPTION+R_OPTION+T_OPTION+UP_OPTION+D_OPTION+W_OPTION+Y_OPTION+Z_OPTION+O_OPTION+L_OPTION;
	strncpy(options_ext[COMBO], ".combo", OPTIONS_EXT_LEN); options_ext[COMBO][OPTIONS_EXT_LEN] = EOS;
	option_flags[COMPOUND] = 0L;
	strncpy(options_ext[COMPOUND], ".cmpnd", OPTIONS_EXT_LEN); options_ext[COMPOUND][OPTIONS_EXT_LEN] = EOS;
	option_flags[COOCCUR] = ALL_OPTIONS+D_OPTION+Y_OPTION;
	strncpy(options_ext[COOCCUR], ".coocr", OPTIONS_EXT_LEN); options_ext[COOCCUR][OPTIONS_EXT_LEN] = EOS;
	option_flags[CORELEX] = F_OPTION+RE_OPTION+SP_OPTION+SM_OPTION+T_OPTION+FR_OPTION+UP_OPTION;
	strncpy(options_ext[CORELEX], ".corelex", OPTIONS_EXT_LEN); options_ext[CORELEX][OPTIONS_EXT_LEN] = EOS;
	option_flags[DATES] = FR_OPTION+RE_OPTION;
	strncpy(options_ext[DATES], ".date", OPTIONS_EXT_LEN); options_ext[DATES][OPTIONS_EXT_LEN] = EOS;
	option_flags[DIST] = ALL_OPTIONS+UM_OPTION+D_OPTION;
	strncpy(options_ext[DIST], ".dist", OPTIONS_EXT_LEN); options_ext[DIST][OPTIONS_EXT_LEN] = EOS;
	option_flags[DSS] = F_OPTION+RE_OPTION+SP_OPTION+SM_OPTION+T_OPTION+UP_OPTION+UM_OPTION;
	strncpy(options_ext[DSS], ".tbl", OPTIONS_EXT_LEN); // This has to be ".tbl" look at dss.cpp program!!!
		options_ext[DSS][OPTIONS_EXT_LEN] = EOS;
	option_flags[EVAL] = F_OPTION+RE_OPTION+SM_OPTION+T_OPTION+UP_OPTION;
	strncpy(options_ext[EVAL], ".eval", OPTIONS_EXT_LEN); options_ext[EVAL][OPTIONS_EXT_LEN] = EOS;
	option_flags[EVALD] = F_OPTION+RE_OPTION+SM_OPTION+T_OPTION+UP_OPTION;
	strncpy(options_ext[EVALD], ".evald", OPTIONS_EXT_LEN); options_ext[EVALD][OPTIONS_EXT_LEN] = EOS;
	option_flags[FLUCALC] = F_OPTION+RE_OPTION+SP_OPTION+SM_OPTION+T_OPTION+Z_OPTION;
	strncpy(options_ext[FLUCALC], ".flucalc", OPTIONS_EXT_LEN); options_ext[FLUCALC][OPTIONS_EXT_LEN] = EOS;
	option_flags[FREQ] = ALL_OPTIONS+UM_OPTION+D_OPTION+Y_OPTION;
	strncpy(options_ext[FREQ], ".frq", OPTIONS_EXT_LEN); options_ext[FREQ][OPTIONS_EXT_LEN] = EOS;
	option_flags[FREQPOS] = F_OPTION+RE_OPTION+K_OPTION+P_OPTION+R_OPTION+SM_OPTION+T_OPTION+UP_OPTION+UM_OPTION+Z_OPTION+D_OPTION+Y_OPTION+L_OPTION;
	strncpy(options_ext[FREQPOS], ".frqpos", OPTIONS_EXT_LEN); options_ext[FREQPOS][OPTIONS_EXT_LEN] = EOS;
	option_flags[GEM] = ALL_OPTIONS+UM_OPTION+D_OPTION;
	strncpy(options_ext[GEM], ".gem", OPTIONS_EXT_LEN); options_ext[GEM][OPTIONS_EXT_LEN] = EOS;
	option_flags[GEMFREQ] = ALL_OPTIONS+UM_OPTION+D_OPTION;
	strncpy(options_ext[GEMFREQ], ".gemfrq", OPTIONS_EXT_LEN); options_ext[GEMFREQ][OPTIONS_EXT_LEN] = EOS;
	option_flags[GEMLIST] = F_OPTION+RE_OPTION+T_OPTION+D_OPTION+L_OPTION;
	strncpy(options_ext[GEMLIST], ".gemlst", OPTIONS_EXT_LEN); options_ext[GEMLIST][OPTIONS_EXT_LEN] = EOS;
	option_flags[IPSYN] = F_OPTION+RE_OPTION+T_OPTION+UP_OPTION;
	strncpy(options_ext[IPSYN], ".ipsyn", OPTIONS_EXT_LEN); options_ext[IPSYN][OPTIONS_EXT_LEN] = EOS;
	option_flags[KEYMAP] = ALL_OPTIONS+UM_OPTION;
	strncpy(options_ext[KEYMAP], ".keymap", OPTIONS_EXT_LEN); options_ext[KEYMAP][OPTIONS_EXT_LEN] = EOS;
	option_flags[KIDEVAL] = F_OPTION+RE_OPTION+SP_OPTION+SM_OPTION+T_OPTION;
	strncpy(options_ext[KIDEVAL], ".kideval", OPTIONS_EXT_LEN); options_ext[KIDEVAL][OPTIONS_EXT_LEN] = EOS;
	option_flags[KWAL] = ALL_OPTIONS+D_OPTION+W_OPTION+Y_OPTION+O_OPTION+FR_OPTION;
	strncpy(options_ext[KWAL], ".kwal", OPTIONS_EXT_LEN); options_ext[KWAL][OPTIONS_EXT_LEN] = EOS;
	option_flags[MAXWD] = ALL_OPTIONS+UM_OPTION+D_OPTION+Y_OPTION+O_OPTION;
	strncpy(options_ext[MAXWD], ".mxwrd", OPTIONS_EXT_LEN); options_ext[MAXWD][OPTIONS_EXT_LEN] = EOS;
	option_flags[MEGRASP] = RE_OPTION+FR_OPTION+T_OPTION+SP_OPTION+SM_OPTION;
	strncpy(options_ext[MEGRASP], ".mgrasp", OPTIONS_EXT_LEN); options_ext[MEGRASP][OPTIONS_EXT_LEN] = EOS;
	option_flags[MLT] = ALL_OPTIONS+UM_OPTION+D_OPTION;
	strncpy(options_ext[MLT], ".mlt", OPTIONS_EXT_LEN); options_ext[MLT][OPTIONS_EXT_LEN] = EOS;
	option_flags[MLU] = ALL_OPTIONS+UM_OPTION+D_OPTION+Y_OPTION;
	strncpy(options_ext[MLU], ".mlu", OPTIONS_EXT_LEN); options_ext[MLU][OPTIONS_EXT_LEN] = EOS;
	option_flags[MODREP] = F_OPTION+RE_OPTION+K_OPTION+P_OPTION+R_OPTION+T_OPTION+UP_OPTION+UM_OPTION+Z_OPTION+L_OPTION;
	strncpy(options_ext[MODREP], ".mdrep", OPTIONS_EXT_LEN); options_ext[MODREP][OPTIONS_EXT_LEN] = EOS;
	option_flags[MOR_P] = F_OPTION+RE_OPTION+T_OPTION+SP_OPTION+SM_OPTION;
	strncpy(options_ext[MOR_P],	".mor", OPTIONS_EXT_LEN); options_ext[MOR_P][OPTIONS_EXT_LEN] = EOS;
	option_flags[MORTABLE] = F_OPTION+RE_OPTION+SP_OPTION+SM_OPTION+T_OPTION+FR_OPTION+UP_OPTION;
	strncpy(options_ext[MORTABLE], ".mrtbl", OPTIONS_EXT_LEN); options_ext[MORTABLE][OPTIONS_EXT_LEN] = EOS;
	option_flags[PHONFREQ] = ALL_OPTIONS+UM_OPTION+D_OPTION;
	strncpy(options_ext[PHONFREQ], ".phofrq", OPTIONS_EXT_LEN); options_ext[PHONFREQ][OPTIONS_EXT_LEN] = EOS;
	option_flags[POST] = F_OPTION+RE_OPTION+SP_OPTION+SM_OPTION+T_OPTION+FR_OPTION;
	strncpy(options_ext[POST], ".pst", OPTIONS_EXT_LEN); options_ext[POST][OPTIONS_EXT_LEN] = EOS; // it is set in post program, ignored
	option_flags[POSTLIST] = 0L;
	strncpy(options_ext[POSTLIST], ".poslst", OPTIONS_EXT_LEN); options_ext[POSTLIST][OPTIONS_EXT_LEN] = EOS;
	option_flags[POSTMODRULES] = 0L;
	strncpy(options_ext[POSTMODRULES], ".pmr", OPTIONS_EXT_LEN); options_ext[POSTMODRULES][OPTIONS_EXT_LEN] = EOS;
	option_flags[POSTMORTEM]= FR_OPTION+RE_OPTION+T_OPTION+SP_OPTION+SM_OPTION;
	strncpy(options_ext[POSTMORTEM], ".pmortm", OPTIONS_EXT_LEN); options_ext[POSTMORTEM][OPTIONS_EXT_LEN] = EOS;
	option_flags[POSTTRAIN] = RE_OPTION+SP_OPTION+SM_OPTION+T_OPTION;
	strncpy(options_ext[POSTTRAIN], ".ptrain", OPTIONS_EXT_LEN); options_ext[POSTTRAIN][OPTIONS_EXT_LEN] = EOS;
	option_flags[C_QPA] = F_OPTION+RE_OPTION+SM_OPTION+T_OPTION+UP_OPTION;
	strncpy(options_ext[C_QPA], ".cqp", OPTIONS_EXT_LEN); options_ext[C_QPA][OPTIONS_EXT_LEN] = EOS;
	option_flags[RELY] = F_OPTION+K_OPTION+P_OPTION+RE_OPTION+SP_OPTION+T_OPTION+L_OPTION;
	strncpy(options_ext[RELY], ".rely", OPTIONS_EXT_LEN); options_ext[RELY][OPTIONS_EXT_LEN] = EOS;
	option_flags[SCRIPT_P] = F_OPTION+RE_OPTION+R_OPTION+T_OPTION+UP_OPTION;
	strncpy(options_ext[SCRIPT_P], ".script", OPTIONS_EXT_LEN); options_ext[SCRIPT_P][OPTIONS_EXT_LEN] = EOS;
	option_flags[SUGAR] = F_OPTION+RE_OPTION+SM_OPTION+T_OPTION+UP_OPTION;
	strncpy(options_ext[SUGAR], ".sug", OPTIONS_EXT_LEN); options_ext[SUGAR][OPTIONS_EXT_LEN] = EOS;
	option_flags[TIMEDUR] = F_OPTION+RE_OPTION+K_OPTION+P_OPTION+R_OPTION+SP_OPTION+SM_OPTION+T_OPTION+UP_OPTION+Z_OPTION;
	strncpy(options_ext[TIMEDUR], ".timdur", OPTIONS_EXT_LEN); options_ext[TIMEDUR][OPTIONS_EXT_LEN] = EOS;
	option_flags[VOCD] = F_OPTION+K_OPTION+L_OPTION+P_OPTION+R_OPTION+RE_OPTION+SP_OPTION+SM_OPTION+T_OPTION+UP_OPTION;
	strncpy(options_ext[VOCD], ".vocd", OPTIONS_EXT_LEN); options_ext[VOCD][OPTIONS_EXT_LEN] = EOS;
	option_flags[WDLEN] = ALL_OPTIONS+D_OPTION+UM_OPTION+Y_OPTION;
	strncpy(options_ext[WDLEN], ".wdlen", OPTIONS_EXT_LEN); options_ext[WDLEN][OPTIONS_EXT_LEN] = EOS;


	option_flags[CHSTRING] = F_OPTION+RE_OPTION+P_OPTION+T_OPTION+Y_OPTION+FR_OPTION;
	strncpy(options_ext[CHSTRING], ".chstr", OPTIONS_EXT_LEN); options_ext[CHSTRING][OPTIONS_EXT_LEN] = EOS;
	option_flags[ANVIL2CHAT]= F_OPTION+RE_OPTION;
	strncpy(options_ext[ANVIL2CHAT], ".anvil", OPTIONS_EXT_LEN); options_ext[ANVIL2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[CHAT2ANVIL]= F_OPTION+RE_OPTION+P_OPTION+L_OPTION;
	strncpy(options_ext[CHAT2ANVIL], ".c2anvl", OPTIONS_EXT_LEN); options_ext[CHAT2ANVIL][OPTIONS_EXT_LEN] = EOS;
	option_flags[CHAT2CA] = F_OPTION+RE_OPTION+L_OPTION;
	strncpy(options_ext[CHAT2CA], ".c2ca", OPTIONS_EXT_LEN); options_ext[CHAT2CA][OPTIONS_EXT_LEN] = EOS;
	option_flags[CHAT2CONLL] = RE_OPTION+T_OPTION+SM_OPTION+UP_OPTION;
	strncpy(options_ext[CHAT2CONLL], ".conll", OPTIONS_EXT_LEN); options_ext[CHAT2CONLL][OPTIONS_EXT_LEN] = EOS;
	option_flags[CHAT2ELAN] = F_OPTION+RE_OPTION+P_OPTION+L_OPTION;
	strncpy(options_ext[CHAT2ELAN], ".c2elan", OPTIONS_EXT_LEN); options_ext[CHAT2ELAN][OPTIONS_EXT_LEN] = EOS;
	option_flags[CHAT2PRAAT]= F_OPTION+RE_OPTION+P_OPTION+L_OPTION+T_OPTION;
	strncpy(options_ext[CHAT2PRAAT], ".c2praat", OPTIONS_EXT_LEN); options_ext[CHAT2PRAAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[CHAT2SRT] = F_OPTION+RE_OPTION+T_OPTION;
	strncpy(options_ext[CHAT2SRT], ".srt", OPTIONS_EXT_LEN); options_ext[CHAT2SRT][OPTIONS_EXT_LEN] = EOS;
	option_flags[CHAT2XMAR] = F_OPTION+RE_OPTION+P_OPTION+L_OPTION;
	strncpy(options_ext[CHAT2XMAR], ".c2xmar", OPTIONS_EXT_LEN); options_ext[CHAT2XMAR][OPTIONS_EXT_LEN] = EOS;
	option_flags[CHECK_P] = F_OPTION+RE_OPTION+D_OPTION+P_OPTION+T_OPTION+L_OPTION;
	strncpy(options_ext[CHECK_P], ".chck", OPTIONS_EXT_LEN); options_ext[CHECK_P][OPTIONS_EXT_LEN] = EOS;
	option_flags[CONLL2CHAT] = RE_OPTION;
	strncpy(options_ext[CONLL2CHAT], ".conll", OPTIONS_EXT_LEN); options_ext[CONLL2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[COMBTIER] = F_OPTION+RE_OPTION+K_OPTION+P_OPTION+L_OPTION;
	strncpy(options_ext[COMBTIER], ".cmbtr", OPTIONS_EXT_LEN); options_ext[COMBTIER][OPTIONS_EXT_LEN] = EOS;
	option_flags[CP2UTF] = F_OPTION+RE_OPTION+FR_OPTION+T_OPTION+L_OPTION+Y_OPTION;
	strncpy(options_ext[CP2UTF], ".cp2utf", OPTIONS_EXT_LEN); options_ext[CP2UTF][OPTIONS_EXT_LEN] = EOS;
	option_flags[DATACLEANUP]= F_OPTION+RE_OPTION+T_OPTION+P_OPTION+Y_OPTION+L_OPTION;
	strncpy(options_ext[DATACLEANUP], ".dtcl", OPTIONS_EXT_LEN); options_ext[DATACLEANUP][OPTIONS_EXT_LEN] = EOS;
	option_flags[DELIM] = F_OPTION+RE_OPTION+FR_OPTION+K_OPTION+P_OPTION+T_OPTION+L_OPTION;
	strncpy(options_ext[DELIM], ".delim", OPTIONS_EXT_LEN); options_ext[DELIM][OPTIONS_EXT_LEN] = EOS;
	option_flags[ELAN2CHAT] = F_OPTION+RE_OPTION+L_OPTION;
	strncpy(options_ext[ELAN2CHAT], ".elan", OPTIONS_EXT_LEN); options_ext[ELAN2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[FIXBULLETS]= F_OPTION+RE_OPTION+T_OPTION+FR_OPTION+L_OPTION+Y_OPTION;
	strncpy(options_ext[FIXBULLETS], ".fxblts", OPTIONS_EXT_LEN); options_ext[FIXBULLETS][OPTIONS_EXT_LEN] = EOS;
	option_flags[FIXIT] = F_OPTION+RE_OPTION+FR_OPTION+L_OPTION;
	strncpy(options_ext[FIXIT], ".fixit", OPTIONS_EXT_LEN); options_ext[FIXIT][OPTIONS_EXT_LEN] = EOS;
	option_flags[FIXMP3] = P_OPTION+FR_OPTION+L_OPTION;
	strncpy(options_ext[FIXMP3], ".fixmp3", OPTIONS_EXT_LEN); options_ext[FIXMP3][OPTIONS_EXT_LEN] = EOS;
	option_flags[FLO] = F_OPTION+RE_OPTION+D_OPTION+SP_OPTION+SM_OPTION+T_OPTION+FR_OPTION+L_OPTION+R_OPTION+Z_OPTION;
	strncpy(options_ext[FLO], ".flo", OPTIONS_EXT_LEN); options_ext[FLO][OPTIONS_EXT_LEN] = EOS;
	option_flags[CMDI_P] = RE_OPTION;
	strncpy(options_ext[CMDI_P], ".cmdi", OPTIONS_EXT_LEN); options_ext[CMDI_P][OPTIONS_EXT_LEN] = EOS;
	option_flags[INDENT] = F_OPTION+RE_OPTION+FR_OPTION+L_OPTION;
	strncpy(options_ext[INDENT], ".indnt", OPTIONS_EXT_LEN); options_ext[INDENT][OPTIONS_EXT_LEN] = EOS;
	option_flags[LAB2CHAT] = RE_OPTION;
	strncpy(options_ext[LAB2CHAT], ".lab", OPTIONS_EXT_LEN); options_ext[LAB2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[LENA2CHAT] = RE_OPTION;
	strncpy(options_ext[LENA2CHAT], "", OPTIONS_EXT_LEN); options_ext[LENA2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[LIPP2CHAT] = F_OPTION+RE_OPTION;
	strncpy(options_ext[LIPP2CHAT], ".lipp", OPTIONS_EXT_LEN); options_ext[LIPP2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[LONGTIER] = F_OPTION+RE_OPTION+FR_OPTION+L_OPTION;
	strncpy(options_ext[LONGTIER], ".longtr", OPTIONS_EXT_LEN); options_ext[LONGTIER][OPTIONS_EXT_LEN] = EOS;
	option_flags[LOWCASE] = F_OPTION+RE_OPTION+FR_OPTION+P_OPTION+T_OPTION+L_OPTION;
	strncpy(options_ext[LOWCASE], ".lowcas", OPTIONS_EXT_LEN); options_ext[LOWCASE][OPTIONS_EXT_LEN] = EOS;
	option_flags[MAKEMOD] = F_OPTION+RE_OPTION+T_OPTION+L_OPTION;
	strncpy(options_ext[MAKEMOD], ".mkmod", OPTIONS_EXT_LEN); options_ext[MAKEMOD][OPTIONS_EXT_LEN] = EOS;
	option_flags[MEDIALINE]= F_OPTION+RE_OPTION+FR_OPTION;
	strncpy(options_ext[MEDIALINE], ".media", OPTIONS_EXT_LEN); options_ext[MEDIALINE][OPTIONS_EXT_LEN] = EOS;
	option_flags[OLAC_P] = RE_OPTION;
	strncpy(options_ext[OLAC_P], ".xml", OPTIONS_EXT_LEN); options_ext[OLAC_P][OPTIONS_EXT_LEN] = EOS;
	option_flags[ORT] = 0L;
	strncpy(options_ext[ORT], ".ort", OPTIONS_EXT_LEN); options_ext[ORT][OPTIONS_EXT_LEN] = EOS;
	option_flags[PLAY2CHAT] = F_OPTION+RE_OPTION;
	strncpy(options_ext[PLAY2CHAT], ".play", OPTIONS_EXT_LEN); options_ext[PLAY2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[PRAAT2CHAT]= F_OPTION+RE_OPTION;
	strncpy(options_ext[PRAAT2CHAT], ".praat", OPTIONS_EXT_LEN); options_ext[PRAAT2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[QUOTES] = F_OPTION+RE_OPTION+FR_OPTION+UP_OPTION+L_OPTION;
	strncpy(options_ext[QUOTES], ".qotes", OPTIONS_EXT_LEN); options_ext[QUOTES][OPTIONS_EXT_LEN] = EOS;
	option_flags[REPEAT]	= F_OPTION+RE_OPTION+P_OPTION+L_OPTION;
	strncpy(options_ext[REPEAT], ".rpeat", OPTIONS_EXT_LEN); options_ext[REPEAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[RETRACE] = F_OPTION+RE_OPTION+K_OPTION+P_OPTION+L_OPTION+FR_OPTION;
	strncpy(options_ext[RETRACE], ".retrace", OPTIONS_EXT_LEN); options_ext[RETRACE][OPTIONS_EXT_LEN] = EOS;
	option_flags[RTF2CHAT] = F_OPTION+RE_OPTION+K_OPTION+P_OPTION+L_OPTION;
	strncpy(options_ext[RTF2CHAT], ".rtf", OPTIONS_EXT_LEN); options_ext[RTF2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[SALT2CHAT] = F_OPTION+RE_OPTION+K_OPTION+L_OPTION;
	strncpy(options_ext[SALT2CHAT], ".sltin", OPTIONS_EXT_LEN); options_ext[SALT2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[SEGMENT] = F_OPTION+FR_OPTION+RE_OPTION;
	strncpy(options_ext[SEGMENT], ".seg", OPTIONS_EXT_LEN); options_ext[SEGMENT][OPTIONS_EXT_LEN] = EOS;
	option_flags[SILENCE_P] = RE_OPTION+K_OPTION+SP_OPTION;
	strncpy(options_ext[SILENCE_P], ".aif", OPTIONS_EXT_LEN); options_ext[SILENCE_P][OPTIONS_EXT_LEN] = EOS;
	option_flags[SRT2CHAT] = F_OPTION+RE_OPTION+T_OPTION;
	strncpy(options_ext[SRT2CHAT], ".cha", OPTIONS_EXT_LEN); options_ext[SRT2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[TEXT2CHAT] = F_OPTION+RE_OPTION;
	strncpy(options_ext[TEXT2CHAT], ".txtin", OPTIONS_EXT_LEN); options_ext[TEXT2CHAT][OPTIONS_EXT_LEN] = EOS;
	option_flags[TIERORDER] = F_OPTION+RE_OPTION+P_OPTION+Y_OPTION+FR_OPTION+L_OPTION;
	strncpy(options_ext[TIERORDER], ".tordr", OPTIONS_EXT_LEN); options_ext[TIERORDER][OPTIONS_EXT_LEN] = EOS;
	option_flags[TRNFIX] = F_OPTION+RE_OPTION+T_OPTION+P_OPTION+Y_OPTION+FR_OPTION+L_OPTION;
	strncpy(options_ext[TRNFIX], ".trnfx", OPTIONS_EXT_LEN); options_ext[TRNFIX][OPTIONS_EXT_LEN] = EOS;
	option_flags[UNIQ] = F_OPTION+RE_OPTION+D_OPTION+K_OPTION+SP_OPTION+SM_OPTION+UP_OPTION+UM_OPTION+L_OPTION;
	strncpy(options_ext[UNIQ], ".uniq", OPTIONS_EXT_LEN); options_ext[UNIQ][OPTIONS_EXT_LEN] = EOS;
	option_flags[USEDLEX] = RE_OPTION;
	strncpy(options_ext[USEDLEX], ".tmp", OPTIONS_EXT_LEN); options_ext[USEDLEX][OPTIONS_EXT_LEN] = EOS;
	option_flags[VALIDATEMFA] = F_OPTION+FR_OPTION+RE_OPTION+T_OPTION;
	strncpy(options_ext[VALIDATEMFA], ".mfa", OPTIONS_EXT_LEN); options_ext[VALIDATEMFA][OPTIONS_EXT_LEN] = EOS;


	option_flags[TEMPLATE] = F_OPTION;
	strncpy(options_ext[TEMPLATE], ".tmp", OPTIONS_EXT_LEN); options_ext[TEMPLATE][OPTIONS_EXT_LEN] = EOS;
	option_flags[TEMP] = F_OPTION+FR_OPTION+RE_OPTION+T_OPTION;
	strncpy(options_ext[TEMP], ".tmp", OPTIONS_EXT_LEN); options_ext[TEMP][OPTIONS_EXT_LEN] = EOS;


	option_flags[DOS2UNIX]  = F_OPTION+RE_OPTION+P_OPTION+Y_OPTION+L_OPTION;
	strncpy(options_ext[DOS2UNIX], ".dos2unx", OPTIONS_EXT_LEN); options_ext[DOS2UNIX][OPTIONS_EXT_LEN] = EOS;
	option_flags[GPS] = RE_OPTION;
	strncpy(options_ext[GPS], ".gps", OPTIONS_EXT_LEN); options_ext[GPS][OPTIONS_EXT_LEN] = EOS;
	option_flags[LINES_P] = F_OPTION+RE_OPTION+T_OPTION+Y_OPTION+L_OPTION;
	strncpy(options_ext[LINES_P], ".line", OPTIONS_EXT_LEN); options_ext[LINES_P][OPTIONS_EXT_LEN] = EOS;
	option_flags[PP] = 0L;
	strncpy(options_ext[PP], ".pp", OPTIONS_EXT_LEN); options_ext[PP][OPTIONS_EXT_LEN] = EOS;
}

void maininitwords() {
	addword('\0','\0',"+0*");
	addword('\0','\0',"+&*");
	addword('\0','\0',"++*");
	addword('\0','\0',"+-*");
	addword('\0','\0',"+#*");
	addword('\0','\0',"+(*.*)");
// 2014-07-17	addword('\0','\0',"+,");
}

void mor_initwords() {
	addword('\0','\0',"+end|*");
	addword('\0','\0',"+beg|*");
	addword('\0','\0',"+cm|*");
	addword('\0','\0',"+bq|*");//2019-04-29
	addword('\0','\0',"+eq|*");//2019-04-29
	addword('\0','\0',"+bq2|*");//2019-04-29
	addword('\0','\0',"+eq2|*");//2019-04-29
}

void gra_initwords() {
	addword('\0','\0',"+*|*|BEGP");
	addword('\0','\0',"+*|*|ENDP");
	addword('\0','\0',"+*|*|LP");
	addword('\0','\0',"+*|*|PUNCT");
}

/* makewind(w) increases the size of the utterance loop by "w" elements to
   accomodate the data for the window of the specified size.
*/
void makewind(int w) {
	UTTER *ttt;

	for (; w > 0; w--) {
		ttt = NEW(UTTER);
		if (ttt == NULL) out_of_mem();
		ttt->speaker[0]	= EOS;
		ttt->attSp[0]	= EOS;
		ttt->line[0]	= EOS;
		ttt->attLine[0]	= EOS;
		ttt->nextutt = utterance->nextutt;
		utterance->nextutt = ttt;
	}
}

char *mainflgs() {
	*templineC = EOS;
	if (option_flags[CLAN_PROG_NUM] & D_OPTION) {
		if (OnlydataLimit == 1) strcat(templineC,"d ");
		else if (OnlydataLimit > 1) strcat(templineC,"dN ");
	}
	if (option_flags[CLAN_PROG_NUM] & F_OPTION)  strcat(templineC,"fS ");
	if (option_flags[CLAN_PROG_NUM] & K_OPTION)  strcat(templineC,"k ");
	if (option_flags[CLAN_PROG_NUM] & L_OPTION)  strcat(templineC,"lN ");
	if (option_flags[CLAN_PROG_NUM] & O_OPTION)  strcat(templineC,"oS ");
	if (option_flags[CLAN_PROG_NUM] & P_OPTION)  strcat(templineC,"pS ");
	if (option_flags[CLAN_PROG_NUM] & R_OPTION)  strcat(templineC,"rN ");
	if (option_flags[CLAN_PROG_NUM] & RE_OPTION)  strcat(templineC,"re ");
	if (option_flags[CLAN_PROG_NUM] & SP_OPTION || option_flags[CLAN_PROG_NUM] & SM_OPTION) {
		if (CLAN_PROG_NUM == MOR_P) {
			strcat(templineC,"sS ");
		} else if (CLAN_PROG_NUM == GEM || CLAN_PROG_NUM == GEMFREQ) {
			strcat(templineC,"sS ");
		} else if (CLAN_PROG_NUM == SILENCE_P) {
			strcat(templineC,"sS  s@F ");
		} else if (CLAN_PROG_NUM == EVAL || CLAN_PROG_NUM == EVALD || CLAN_PROG_NUM == C_NNLA ||
				   CLAN_PROG_NUM == C_QPA || CLAN_PROG_NUM == SUGAR) {
			strcat(templineC,"sS ");
		} else {
			strcat(templineC,"sS  s@F  sgS  smS ");
		}
	}
	if (option_flags[CLAN_PROG_NUM] & T_OPTION)  strcat(templineC,"tS ");
	if (option_flags[CLAN_PROG_NUM] & UP_OPTION || option_flags[CLAN_PROG_NUM] & UM_OPTION) strcat(templineC,"u ");
	if (option_flags[CLAN_PROG_NUM] & W_OPTION)  strcat(templineC,"wN ");
	if (option_flags[CLAN_PROG_NUM] & Z_OPTION)  strcat(templineC,"xN ");
	if (option_flags[CLAN_PROG_NUM] & Y_OPTION)  strcat(templineC,"yN ");
	if (option_flags[CLAN_PROG_NUM] & Z_OPTION)  strcat(templineC,"zN ");
	if (option_flags[CLAN_PROG_NUM] & FR_OPTION)  strcat(templineC,"1  ");
	if (option_flags[CLAN_PROG_NUM] & F_OPTION)  strcat(templineC,"2  ");
	return(templineC);
}

/* mainusage(char isQuit) displays the extended options list.
*/
void mainusage(char isQuit) {
	if (option_flags[CLAN_PROG_NUM] & D_OPTION) {
		if (CLAN_PROG_NUM == CHECK_P) {
			puts("+d : attempts to suppress repeated warnings of the same type");
			puts("+d1: suppress ALL repeated warnings of the same type");
//  2017-04-11			puts("+d2: display ONLY error messages");
		} else if (CLAN_PROG_NUM == CHIP) {
			puts("+d : outputs only coding tiers");
			puts("+d1: outputs only summary statistics");
			puts("+d2: outputs only summary statistics in SPREADSHEET format");
		} else if (CLAN_PROG_NUM == COMBO) {
		} else if (CLAN_PROG_NUM == COOCCUR) {
			puts("+d : output matches without frequency count");
			puts("+d1: output results with line numbers and file names");
		} else if (CLAN_PROG_NUM == DIST)
			puts("+d:  Output sdata in a form suitable for statistical analysis.");
		else if (CLAN_PROG_NUM == FLO) {
		} else if (CLAN_PROG_NUM == FREQ) {
			puts("+dCN:output only words used by <, <=, =, => or > than N percent of speakers");
			puts("+d : outputs all selected words, corresponding frequencies, and line numbers");
			puts("+d1: outputs word with no frequency information. Usable for +s@filename option.");
			puts("+d2: outputs in SPREADSHEET format");
			puts("+d3: output only type/token information in SPREADSHEET format");
			puts("+d4: outputs only type/token information");
			puts("+d5: outputs all words selected with +s option, including the ones with 0 frequency count");
			puts("+d6: outputs words and frequencies with limited searched word surrounding context");
			puts("+d7: outputs frequencies of words linked between one dependent tier and speaker or another dependent tier");
			puts("+d8: outputs words and frequencies of cross tabulation of one dependent tier with another");
		} else if (CLAN_PROG_NUM == GEM) {
			puts("+d : outputs legal CHAT format");
			puts("+d1: outputs legal CHAT format plus file names, line numbers, and @ID codes");
			puts("+d2: outputs only selected @Bg and @Eg header tiers without any nested header tiers (if present) ");
		} else if (CLAN_PROG_NUM == GEMLIST) {
			puts("+d : only the data between @Bg and @Eg is displayed");
		} else if (CLAN_PROG_NUM == KWAL) {
			puts("+d : outputs legal CHAT format");
			puts("+d1: outputs legal CHAT format plus file names and line numbers");
			puts("+d2: outputs file names once per file only");
			puts("+d3: outputs ONLY matched items");
			puts("+d30: outputs ONLY matched items without any defaults removed");
			puts("+d31: outputs ONLY matched items and includes words associated with target codes [...]");
			puts("+d4: outputs in SPREADSHEET format");
			puts("+d40: outputs in SPREADSHEET format and repeat the same tier for every keyword match");
//			puts("+d41: outputs in SPREADSHEET format with each matched keyword listed one per column");
			puts("+d7: search words linked between one dependent tier and speaker or another dependent tier");
			puts("+d90: same as +d99, but include all speaker tiers in output");
			puts("+d99: convert \"word [x 2]\" to \"word [/] word\" and so on");
		} else if (CLAN_PROG_NUM == MAXWD) {
			puts("+d : outputs one line for the length level and the next line for the word");
			puts("+d1: NO +g option - produces longest words, one per line, without any other information");
			puts("+d1: WITH +g option - outputs longest utterances in legal CHAT format ");
		} else if (CLAN_PROG_NUM == MLT) {
			puts("+d : output in SPREADSHEET format. Must include speaker specifications");
			puts("+d1: output data ONLY.");
		} else if (CLAN_PROG_NUM == MLU) {
			puts("+d : output in SPREADSHEET format. Must include speaker specifications");
			puts("+d1: output data ONLY.");
			puts("+d2: output in SPREADSHEET format each utterance and number of morphemes in it");
		} else if (CLAN_PROG_NUM == PHONFREQ) {
			puts("+d : actual words matched will be written to the output");
		} else if (CLAN_PROG_NUM == VOCD) {
		} else if (CLAN_PROG_NUM == UNIQ) {
			puts("+d : outputs lines with no frequency information");
		} else if (CLAN_PROG_NUM == WDLEN) {
			puts("+d : output in SPREADSHEET format");
		} else if (OnlydataLimit == 1) {
			puts("+d : output only level 0 data");
		} else if (OnlydataLimit > 1) {
			puts("+dN: output only level N data");
		}
	}
	if (option_flags[CLAN_PROG_NUM] & F_OPTION) {
		puts("+fS: send output to file (program will derive filename)");
		puts("-f : send output to the screen");
	}
	if (option_flags[CLAN_PROG_NUM] & K_OPTION) {
		if (!nomap)
			puts("+k : treat upper and lower case as different");
		else
			puts("+k : treat upper and lower case as the same");
	}
	if (option_flags[CLAN_PROG_NUM] & L_OPTION) {
		puts("+l : add language tag \"@s:...\" to every word");
		puts("+l1: add language pre-code \"[- ...]\" to every utterance that doesn't have one");
	}
	if (option_flags[CLAN_PROG_NUM] & O_OPTION) {
		puts("+oS: include additional tier code S for output purposes ONLY");
		puts("-oS: exclude tier code S from an additional inclusion in an output");
	}
	if (option_flags[CLAN_PROG_NUM] & P_OPTION)
		puts("+pS: add S to word delimiters. (+p_ will break New_York into two words)");
	if (option_flags[CLAN_PROG_NUM] & R_OPTION) {
		puts("+rN: if N=1 then \"get(s)\" = \"gets\"(default), 2 = \"get(s)\", 21 = \"get(s)\" & \"gets\", 3 = \"get\"");
		puts("     4- do not break words into two at post/pre clitics '~' and '$' on %mor tier");
		if (R5)
			printf("     5- no text replacement: [: *], ");
		else
			printf("     5- perform text replacement: [: *, ]");
		if (R5_1)
			puts("   50- no text replacement: [:: *]");
		else
			puts("   50- perform text replacement: [:: *]");
		if (R6)
			puts("     6- exclude repetitions: </>, <//>, <///>, </-> and </?>. (default: include)");
		else
			puts("     6- include repetitions: </>, <//>, <///>, </-> and </?>. (default: exclude)");
		puts("     7- do not remove prosodic symbols in words '/', '~', '^' and ':'");
		puts("     8- combine %mor: tier items with spoken word [: ...] and error code [*...], if any, from speaker tier");
		if (CLAN_PROG_NUM == COMBO) {
			puts("-r4: break words into two at post/pre clitics '~' and '$' on %mor tier (default: if +d4 option used)");
		}
	}
	if (option_flags[CLAN_PROG_NUM] & RE_OPTION)
		puts("+re: run program recursively on all sub-directories.");
	if (option_flags[CLAN_PROG_NUM] & SP_OPTION) {
		if (CLAN_PROG_NUM == MOR_P || CLAN_PROG_NUM == POSTMORTEM || CLAN_PROG_NUM == MEGRASP) {
			puts("+sS: select language to analyze with \"[- ...]\" precode.");
		} else if (CLAN_PROG_NUM == FLUCALC || CLAN_PROG_NUM == KIDEVAL) {
			puts("+sS: select utterance to analyze with \"[+ ...]\" postcode or \"[- ...]\" precode.");
		} else if (CLAN_PROG_NUM == DSS) {
			puts("+sS: to override default function of [+ dss] or [+ dsse] postcode");
		} else if (CLAN_PROG_NUM == GEM || CLAN_PROG_NUM == GEMFREQ) {
			puts("+sS: select gems which are labeled by either label S or labels in file @S");
		} else if (CLAN_PROG_NUM == C_NNLA || CLAN_PROG_NUM == C_QPA || CLAN_PROG_NUM == EVAL || CLAN_PROG_NUM == EVALD ||
				   CLAN_PROG_NUM == SUGAR) {
			puts("+sS: select utterance to analyze with \"[+ ...]\" postcode.");
		} else {
			puts("+sS: search for word S in an input file.");
			puts("    \"<...>\" for data associated with code [...], \"[...]\" for code itself");
			puts("    \"[+ ...]\" for data tier with postcode on it, \"<+ ...>\" for postcode itself");
			puts("    to find fillers \"&um\", but not Simple Events \"&=eats\": +s&* -s&=*");
			puts("    to find word sequences use space character to separate words, For example: +s\"in the tree\"");
			puts("+s@F: search for words or morphological markers or grammatical relations specified in file F.");
			if (CLAN_PROG_NUM == FREQ) {
				puts("    for example: to count number of *PAR: speaker utterances and to create SPREADSHEET use options:");
				puts("        +s\\*PAR: +d2");
				puts("    if you want result for file that does not have any speakers use options:");
				puts("        +s\\*PAR: +d2 +d5");
			}
			if (CLAN_PROG_NUM == MLU || CLAN_PROG_NUM == MLT) {
				puts("+sxxx: to include utterances that contain \"xxx\" in the count.");
				puts("+syyy: to include utterances that contain \"xxx\" in the count.");
			}
			if (CLAN_PROG_NUM != SILENCE_P) {
				puts("+sgS: search for grammatical relations S on \"%gra:\" tier in an input file");
				puts("     freq +sg: for more information.");
				puts("+smS: search for morphological markers S on \"%mor:\" tier in an input file.");
				puts("     freq +sm: for more information.");
			}
		}
	}
	if (option_flags[CLAN_PROG_NUM] & SM_OPTION) {
		if (CLAN_PROG_NUM == MOR_P || CLAN_PROG_NUM == POSTMORTEM || CLAN_PROG_NUM == MEGRASP) {
			puts("-sS: select language to exclude from analyze with \"[- ...]\" precode.");
		} else if (CLAN_PROG_NUM == FLUCALC || CLAN_PROG_NUM == KIDEVAL) {
			puts("-sS: select utterance to exclude from analyze with \"[+ ...]\" postcode  or \"[- ...]\" precode.");
		} else if (CLAN_PROG_NUM == DSS) {
			puts("-sS: exclude utterance from analyze with \"[+ ...]\" postcode.");
		} else if (CLAN_PROG_NUM == GEM || CLAN_PROG_NUM == GEMFREQ) {
			puts("-sS: select gems which are NOT labeled by either label S or labels in file @S");
		} else if (CLAN_PROG_NUM == C_NNLA || CLAN_PROG_NUM == C_QPA || CLAN_PROG_NUM == EVAL || CLAN_PROG_NUM == EVALD ||
				   CLAN_PROG_NUM == SUGAR) {
			puts("-sS: do not select utterance to analyze with \"[+ ...]\" postcode.");
			puts("-sS: -sm@* - exclude target replaced words, -sm** - exclude error words");
		} else {
			puts("-sS: exclude word S from an input file.");
			puts("-s@F: exclude words or morphological markers or grammatical relations in file F from an input file.");
			puts("-sgS: exclude grammatical relations S on \"%gra:\" tier from an input file, (\"freq -sg\" for more info)");
			puts("-smS: exclude morphological markers S on \"%mor:\" tier from an input file (\"freq -sm\" for more info).");
		}
	}
	if (option_flags[CLAN_PROG_NUM] & T_OPTION) {
		puts("+tS: include tier code S");
		puts("-tS: exclude tier code S");
		puts("    -t* - exclude data on main speaker tiers from analysis");
		if (CLAN_PROG_NUM == FREQ)
			puts("    +t* - include data on main speaker tiers in analysis");
		puts("    +/-t#Target_Child - select target child's tiers");
		puts("    +/-t@id=\"*|Mother|*\" - select mother's tiers");
	}
	if (!combinput && ByTurn == '\0') {
		if (option_flags[CLAN_PROG_NUM] & UP_OPTION) {
			if (CLAN_PROG_NUM == QUOTES || CLAN_PROG_NUM == VOCD || CLAN_PROG_NUM == EVAL || CLAN_PROG_NUM == EVALD ||
				CLAN_PROG_NUM == MORTABLE || CLAN_PROG_NUM == CORELEX ||
				CLAN_PROG_NUM == SCRIPT_P || CLAN_PROG_NUM == KIDEVAL || CLAN_PROG_NUM == TIMEDUR ||
				CLAN_PROG_NUM == CHAT2CONLL || CLAN_PROG_NUM == FLUCALC || CLAN_PROG_NUM == C_NNLA || CLAN_PROG_NUM == CODES_P)
				puts("+u : send output to just one output file.");
			else
				puts("+u : merge all specified files together.");
		}
		if (option_flags[CLAN_PROG_NUM] & UM_OPTION)
			puts("-u : compute result for each turn separately.");
	}
	if (option_flags[CLAN_PROG_NUM] & W_OPTION) {
		puts("+wN: display N number of utterance AFTER the given one");
		puts("-wN: display N number of utterance BEFORE the given one");
	}
	if (option_flags[CLAN_PROG_NUM] & Z_OPTION) {
		puts("+xCN: include only utterances which are C (>, <, =) than N items (w, c, m), \"+x=0w\" for zero words");
		puts("\tw - words, c - characters or m - morphemes");
		puts("+xS: specify items to include in +xCN: count (Example: +xxxx +xyyy)");
		puts("-xS: specify items to exclude from +xCN: count");
	}
	if (option_flags[CLAN_PROG_NUM] & Y_OPTION) {
		puts("+y : work on TEXT format files one line at the time");
		puts("+y1: work on TEXT format files one utterance at the time");
	}
	if (option_flags[CLAN_PROG_NUM] & Z_OPTION)
		puts("+zN: compute statistics on a specified range of input data");
	if (option_flags[CLAN_PROG_NUM] & FR_OPTION) {
		if (replaceFile)
			puts("-1 : do not replace original data file with new one");
		else
			puts("+1 : replace original data file with new one");
	}
	if (option_flags[CLAN_PROG_NUM] & F_OPTION)
		puts("+/-2: -2 do not create different versions of output file names / +2 create them");
	if (CLAN_PROG_NUM != LENA2CHAT)
		puts("\n    \"filename(s)\" can be \"*.cha\" or a @:filename with a list of data filenames \"@:myfile.cut\"\n");
	puts("-ver(sion): show version number");
	puts("  ");
	VersionNumber(FALSE, stdout);
	puts("  ");
	if (isQuit)
		cutt_exit(0);
}

char *getfarg(char *f, char *f1, int *i) {
	return(f);
/*
	if (*f) return(f);
	(*i)++;
	if (f1 == NULL) {
		fputs("Unexpected ending.\n",stderr); cutt_exit(0);
	} else if (*f1 == '-')
		fprintf(stderr, "*** WARNING: second string %s looks like a switch\n",f1);
	return(f1);
*/
}

void no_arg_option(char *f) {
	if (*f) {
		fprintf(stderr,"Option \"%s\" does not take any arguments.\n", f-2);
		cutt_exit(0);
	}
}

/* maingetflag(f,f1,i) executes apropriate commands assosiated with the
   specified option found in "f" variable.
*/
void maingetflag(char *f, char *f1, int *i) { /* sets up options */
	UTTER *temp;
	char *ts, ch;
	int  tc;
	char wd[1024+2];

	f++;
	if (*f == 'v') {
		no_arg_option(++f);
	} else if (*f == '0') {
	} else if (*f == 'd' && option_flags[CLAN_PROG_NUM] & D_OPTION) {
		f++;
		if (*f != EOS && !isdigit(*f)) {
			fprintf(stderr, "Illegal character \"%c\" used with %s option.\n", *f, f-2);
			if (OnlydataLimit == 1)
				fprintf(stderr, "The only +d level allowed is 0.\n");
			else
				fprintf(stderr, "The only +d levels allowed are 0-%d.\n", OnlydataLimit-1);
			cutt_exit(0);
		}
		onlydata = (char)(atoi(getfarg(f,f1,i))+1);
		if (onlydata < 0 || onlydata > OnlydataLimit) {
			if (CLAN_PROG_NUM == FREQ) {
				fprintf(stderr,"Invalid argument for option: %s\n", f-2);
				fprintf(stderr,"Please specify <, <=, =, =>, > followed by percentage value\n    OR\n");
			}
			if (OnlydataLimit == 1)
				fprintf(stderr, "The only +d level allowed is 0.\n");
			else
				fprintf(stderr, "The only +d levels allowed are 0-%d.\n", OnlydataLimit-1);
			cutt_exit(0);
		}
/* 2019-04-17
		if (chatmode == 0 && (onlydata == 3 || onlydata == 4)) {
			fprintf(stderr, "+d%d option can't be used with +y option.\n", onlydata-1);
			cutt_exit(0);
		}
*/
		if (CLAN_PROG_NUM == CHECK_P) {
			if (onlydata == 3)
				puredata = 2;
			else
				puredata = 0;
		} else if (CLAN_PROG_NUM == CHIP) {
			if (onlydata == 2)
				puredata = 0;
		} else if (CLAN_PROG_NUM == COOCCUR) {
		} else if (CLAN_PROG_NUM == DIST) {
		} else if (CLAN_PROG_NUM == FLO) {
		} else if (CLAN_PROG_NUM == FREQ) {
			if (onlydata == 5 || onlydata == 6) {
				puredata = 0;
			} else if (onlydata == 3 || onlydata == 4) {
				if (ByTurn != '\0') {
					fprintf(stderr, "+d%d option can't be used with -u option.\n", onlydata-1);
					cutt_exit(0);
				}
//				OverWriteFile = TRUE;
			} else if (onlydata == 1 || onlydata == 7) {
				if (onlydata == 7)
					OverWriteFile = TRUE;
				puredata = 1;
			} else if (onlydata == 8 || onlydata == 9) {
				puredata = 1;
			}
		} else if (CLAN_PROG_NUM == GEM) {
		} else if (CLAN_PROG_NUM == GEMFREQ) {
		} else if (CLAN_PROG_NUM == KWAL) {
			if (onlydata == 3)
				puredata = 0;
		} else if (CLAN_PROG_NUM == MAXWD) {
			if (onlydata == 1)
				puredata = 0;
		} else if (CLAN_PROG_NUM == MLT) {
		} else if (CLAN_PROG_NUM == MLU) {
		} else if (CLAN_PROG_NUM == PHONFREQ) {
			puredata = 0;
		} else if (CLAN_PROG_NUM == VOCD) {
		} else if (CLAN_PROG_NUM == WDLEN) {
		}
	} else if (*f == 'k' && option_flags[CLAN_PROG_NUM] & K_OPTION) {
		no_arg_option(++f);
	} else if (*f == 'r' && f[1] == 'e' && option_flags[CLAN_PROG_NUM] & RE_OPTION) {
		f += 2;
		no_arg_option(f);
		isRecursive = TRUE;
	} else if (*f == 'r' && option_flags[CLAN_PROG_NUM] & R_OPTION) {
		f++;
		tc = atoi(getfarg(f,f1,i));
		if (tc == 0 && !*f)
			tc = 1;
		else if (tc < 1 || tc > 3) {
			if (tc == 21 || tc == 20) {
				Parans = (char)tc;
			} else if (tc == 4) {
				R4 = TRUE;
				isPrecliticUse = TRUE;
				isPostcliticUse = TRUE;
				if (CLAN_PROG_NUM == COMBO) {
					if (*(f-2) == '-') {
						R4 = FALSE;
						isPrecliticUse = FALSE;
						isPostcliticUse = FALSE;
					}
				}
			} else if (*f == '5') {
				if (*(f+1) == '0')
					R5_1 = !R5_1;
				else
					R5 = !R5;
			} else if (tc == 6) {
				R6 = !R6;
				if (R6) {
					addword('r','i',"+</?>");
					addword('r','i',"+</->");
					addword('r','i',"+<///>");
					addword('r','i',"+<//>");
					addword('r','i',"+</>");
				} else {
					addword('\0','\0',"+</?>");
					addword('\0','\0',"+</->");
					addword('\0','\0',"+<///>");
					addword('\0','\0',"+<//>");
					addword('\0','\0',"+</>");
				}
			} else if (tc == 7) {
				R7Slash = FALSE;
				R7Tilda = FALSE;
				R7Caret = FALSE;
				R7Colon = FALSE;
				isRemoveCAChar[NOTCA_CROSSED_EQUAL] = FALSE;
				isRemoveCAChar[NOTCA_LEFT_ARROW_CIRCLE] = FALSE;
			} else if (tc == 8) {
				R8 = TRUE;
			} else {
				fputs("Choose N for +r option to be between 1 - 7\n",stderr);
				cutt_exit(0);
			}
		} else
			Parans = (char)tc;
	} else if (*f == 's') {
		f++;
		ch = *(f-2);
		if (ch == '+' && option_flags[CLAN_PROG_NUM] & SP_OPTION) {
			if (*f) {
				strncpy(wd, f, 1024);
				wd[1024] = EOS;
				removeExtraSpace(wd);
				uS.remFrontAndBackBlanks(wd);
				if (wd[0] == '+' || wd[0] == '~') {
					if (wd[1] == '@' || (wd[1] == 'm' && isMorSearchOption(wd+1, wd[1], ch)) || (wd[1] == 'g' && isGRASearchOption(wd+1, wd[1], ch))) {
						wd[1] = wd[0];
						wd[0] = *(f+1);
					}
				}
				if ((wd[0] == '@' || wd[0] == 'm') && isMorSearchOption(wd, wd[0], ch)) {
					if (wd[0] == '@') {
						fprintf(stderr,"Please use new format of +s@ option: +sm\n");
						fprintf(stderr,"For more information type \"freq +sm\"\n");
						cutt_exit(0);
					}
					if (isMultiWordSearch(wd, TRUE)) {
						mwdptr = InsertMulti(mwdptr, wd+1, 1);
					} else {
						if (wd[1] == '+' || wd[1] == '~') {
							wd[0] = wd[1];
							wd[1] = '\002';
						} else
							wd[0] = '\002';
						addword('s','i',wd);
					}
				} else if ((wd[0] == '@' || wd[0] == 'g') && isGRASearchOption(wd, wd[0], ch)) {
					if (wd[0] == '@') {
						fprintf(stderr,"Please use new format of +s@ option: +sg\n");
						fprintf(stderr,"For more information type \"freq +sg\"\n");
						cutt_exit(0);
					}
					if (isMultiWordSearch(wd, TRUE)) {
						mwdptr = InsertMulti(mwdptr, wd+1, 2);
					} else {
						if (wd[1] == '+' || wd[1] == '~') {
							wd[0] = wd[1];
							wd[1] = '\004';
						} else
							wd[0] = '\004';
						addword('s','i',wd);
					}
				} else if (wd[0] == '@') {
					rdexclf('s','i',wd+1);
				} else {
					if (wd[0] == '\\' && (wd[1] == '@' || wd[1] == 'g' || wd[1] == 'm'))
						strcpy(wd, wd+1);
					if ((wd[0] == '+' || wd[0] == '~') && wd[1] == '\\' && (wd[2] == '@' || wd[2] == 'g' || wd[2] == 'm'))
						strcpy(wd+1, wd+2);
					if (isMultiWordSearch(wd, FALSE))
						mwdptr = InsertMulti(mwdptr, wd, 0);
					else
						addword('s','i',wd);
				}
			} else {
				fprintf(stderr,"Missing argument to option: %s\n", f-2);
				cutt_exit(0);
			}
		} else if (option_flags[CLAN_PROG_NUM] & SM_OPTION) {
			if (*f) {
				strncpy(wd, f, 1024);
				wd[1024] = EOS;
				removeExtraSpace(wd);
				uS.remFrontAndBackBlanks(wd);
				if (wd[0] == '+' || wd[0] == '~') {
					if (wd[1] == '@' || (wd[1] == 'm' && isMorSearchOption(wd+1, wd[1], ch)) || (wd[1] == 'g' && isGRASearchOption(wd+1, wd[1], ch))) {
						wd[1] = wd[0];
						wd[0] = *(f+1);
					}
				}
				if ((wd[0] == '@' || wd[0] == 'm') && isMorSearchOption(wd, wd[0], ch)) {
					if (wd[0] == '@') {
						fprintf(stderr,"Please use new format of -s@ option: -sm\n");
						fprintf(stderr,"For more information type \"freq -sm\"\n");
						cutt_exit(0);
					}
					if (wd[1] == '+' || wd[1] == '~') {
						wd[0] = wd[1];
						wd[1] = '\002';
					} else
						wd[0] = '\002';
					addword('s','e',wd);
				} else if ((wd[0] == '@' || wd[0] == 'g') && isGRASearchOption(wd, wd[0], ch)) {
					if (wd[1] == '+' || wd[1] == '~') {
						wd[0] = wd[1];
						wd[1] = '\004';
					} else
						wd[0] = '\004';
					addword('s','e',wd);
				} else if (wd[0] == '@') {
					rdexclf('s','e',wd+1);
				} else {
					if (wd[0] == '\\' && (wd[1] == '@' || wd[1] == 'g' || wd[1] == 'm'))
						strcpy(wd, wd+1);
					if ((wd[0] == '+' || wd[0] == '~') && wd[1] == '\\' && (wd[2] == '@' || wd[2] == 'g' || wd[2] == 'm'))
						strcpy(wd+1, wd+2);
					if (strchr(wd, '[') == NULL && strchr(wd, '<') == NULL && strchr(wd, ' ') != NULL) {
						fprintf(stderr,"Multi-words are not allowed with option: \"%s\"\n", f-2);
						fprintf(stderr,"    Multi-words are search patterns with space character(s) in-between.\n");
						fprintf(stderr,"    If you did not mean to have space characters in-between words,\n");
						fprintf(stderr,"        then please remove space characters and try again.\n");
						cutt_exit(0);
					} else
						addword('s','e',wd);
				}
			} else {
				fprintf(stderr,"Missing argument to option: %s\n", f-2);
				cutt_exit(0);
			}
		} else {
			fprintf(stderr,"Invalid option: %s\n", f-2);
			cutt_exit(0);
		}
	} else  if (*f == 'f' && option_flags[CLAN_PROG_NUM] & F_OPTION) {
		if (*(f-1) == '+') {
			if (!IsOutTTY()) {
				fprintf(stderr,"Option \"%s\" can't be used with file redirect \">\".\n", f-1);
				cutt_exit(0);
			}
			f++;
			stout = FALSE;
			if (*f) {
				if (*f != '.')
					strcpy(GExt, ".");
				else
					GExt[0] = EOS;
				strncat(GExt, f, 31);
				GExt[31] = EOS;
//				AddCEXExtension = "";
			}
		} else {
			f_override = TRUE;
			stout = TRUE;
		}
	} else if (*f == 't' && option_flags[CLAN_PROG_NUM] & T_OPTION) {
		f++;
		if (chatmode) {
			if (*f == '@') {
				if ((*(f+1) == 'I' || *(f+1) == 'i') && (*(f+2) == 'D' || *(f+2) == 'd') && (*(f+3) == '=') && *(f+4) != EOS) {
					AddToID(getfarg(f,f1,i)+4, *(f-2));
				} else if ((*(f+1) == 'K' || *(f+1) == 'k') && (*(f+2) == 'E' || *(f+2) == 'e') && (*(f+3) == 'Y' || *(f+3) == 'y') && (*(f+4) == '=') && *(f+5) != EOS) {
					AddToCode(getfarg(f,f1,i)+5, *(f-2));
				} else
					maketierchoice(getfarg(f,f1,i), *(f-2), FALSE);
			} else if (*f == '#')
				AddToSPRole(getfarg(f,f1,i)+1, *(f-2));
			else
				maketierchoice(getfarg(f,f1,i), *(f-2), FALSE);
		} else {
			fputs("+/-t option is not allowed with TEXT format\n",stderr);
			cutt_exit(0);
		}
	} else if (*f == 'u') {
/* FREQ +d2 +d3
		if (onlydata == 3 || onlydata == 4) {
			fprintf(stderr, "+/-u option can't be used with +d%d option.\n", onlydata-1);
			cutt_exit(0);
		}
*/
		if ((option_flags[CLAN_PROG_NUM] & UP_OPTION) && *(f-1) == '+') {
			if (ByTurn) {
				fputs("+u option can't be used with -u option.\n",stderr);
				cutt_exit(0);
			}
			combinput = TRUE;
		} else if ((option_flags[CLAN_PROG_NUM] & UM_OPTION) && *(f-1)== '-') {
			if (chatmode == 0) {
				fputs("-u option can't be used in nonCHAT mode.\n",stderr);
				cutt_exit(0);
			}
			if (combinput) {
				fputs("-u option can't be used with +u option.\n",stderr);
				cutt_exit(0);
			}
			if (TSoldsp != NULL) {
				free(TSoldsp);
				TSoldsp = NULL;
			}
			if (TSoldsp == NULL) {
				TSoldsp = (char *) malloc(SPEAKERLEN+1);
				*TSoldsp = EOS;
			}
			ByTurn = 1;
		} else {
			fprintf(stderr,"Invalid option: %s\n", f-1);
			cutt_exit(0);
		}
		no_arg_option(++f);
	} else if (*f == 'p' && option_flags[CLAN_PROG_NUM] & P_OPTION) {
		int i, j;
		f++;
		if (*f == EOS) {
		   fputs("Please specify word delimiter characters with +p option.\n",stderr);
		   cutt_exit(0);
		}
		j = strlen(GlobalPunctuation);
		for (; *f != EOS; f++) {
			if (!isSpace(*f)) {
				for (i=0; GlobalPunctuation[i] != EOS; i++) {
					if (*f == GlobalPunctuation[i])
						break;
				}
				if (GlobalPunctuation[i] == EOS) {
					GlobalPunctuation[j++] = *f;
				}
			}
		}
		GlobalPunctuation[j] = EOS;
		punctuation = GlobalPunctuation;
//2011-01-26		uS.str2FNType(punctFile, 0L, getfarg(f,f1,i));
	} else if (*f == 'y' && option_flags[CLAN_PROG_NUM] & Y_OPTION) {
		f++;
		if (headtier != NULL || IDField != NULL || CODEField != NULL || SPRole != NULL) {
		   fputs("ERROR: +y option can't be used with +/-t option.\n",stderr);
		   cutt_exit(0);
		}
		if (ByTurn) {
		   fputs("ERROR: +y option can't be used with -u option.\n",stderr);
		   cutt_exit(0);
		}
		if (onlydata == 3 || onlydata == 4) {
		   fputs("+y option can't be used with +d2 or +d3 option.\n",stderr);
		   cutt_exit(0);
		}
		chatmode = 0;
		nomain = FALSE;
		temp = utterance;
		do {
			*temp->speaker = EOS;
			temp = temp->nextutt;
		} while (temp != utterance) ;
		for (ts=GlobalPunctuation; *ts; ) {
			if (*ts == ':')
				strcpy(ts,ts+1);
			else
				ts++;
		}
		if (*f == EOS)
			y_option = 0;
		else
			y_option = (char)(atoi(getfarg(f,f1,i)));
		if (y_option < 0 || y_option > 1) {
			fputs("Choose N for +y option to be between 0 - 1\n",stderr);
			cutt_exit(0);
		}
	} else if (*f == 'w' && option_flags[CLAN_PROG_NUM] & W_OPTION) {
		f++;
		if (*(f-2) == '-')
			makewind(atoi(getfarg(f,f1,i)));
		else
			makewind(aftwin=atoi(getfarg(f,f1,i)));
		isWOptUsed = TRUE;
	} else if (*f == 'o' && option_flags[CLAN_PROG_NUM] & O_OPTION) {
		f++;
		MakeOutTierChoice(getfarg(f,f1,i), *(f-2));
	} else if (*f == '1' && option_flags[CLAN_PROG_NUM] & FR_OPTION) {
		f++;
		no_arg_option(f);
		if (replaceFile) {
			if (*(f-2) == '-')
				replaceFile = FALSE;
		} else {
			if (*(f-2) == '+')
				replaceFile = TRUE;
		}
	} else if (*f == '2') {
		f++;
		no_arg_option(f);
		if (*(f-2) == '+')
			OverWriteFile = FALSE;
		else
			OverWriteFile = TRUE;
	} else {
		fprintf(stderr,"Invalid option: %s\n", f-1);
		cutt_exit(0);
	}
}

// @Time Duration:	@t<hh:mm-hh:mm> @t<hh:mm:ss-hh:mm:ss> @t<hh:mm:ss>

float getTimeDuration(char *st) {
	char *col, *time1S, *time2S;
	float h, m, s, time1, time2;

	while (isSpace(*st))
		st++;
	col = strchr(st,'-');
	if (col != NULL) {
		*col = EOS;
		time1S = st;
		time2S = col + 1;
		while (isSpace(*time2S))
			time2S++;
	} else {
		time1S = st;
		time2S = NULL;
	}
	h = atof(time1S);
	col = strchr(time1S,':');
	if (col != NULL) {
		*col = EOS;
		col++;
		m = atof(col);
		col = strchr(col,':');
		if (col != NULL) {
			*col = EOS;
			col++;
			s = atof(col);
		} else
			s = 0.0;
	} else {
		m = 0.0;
		s = 0.0;
	}
	time1 = (h * 3600.0000) + (m * 60.0000) + s;
	if (time2S != NULL) {
		h = atof(time2S);
		col = strchr(time2S,':');
		if (col != NULL) {
			*col = EOS;
			col++;
			m = atof(col);
			col = strchr(col,':');
			if (col != NULL) {
				*col = EOS;
				col++;
				s = atof(col);
			} else
				s = 0.0;
		} else {
			m = 0.0;
			s = 0.0;
		}
		time2 = (h * 3600.0000) + (m * 60.0000) + s;
		time1 = time2 - time1;
	}
	return(time1);
}

float getPauseTimeDuration(char *st) {
	char *col, *dot, *time1S;
	float h, m, s, ms, time1;

	while (isSpace(*st) || *st == '(')
		st++;
	time1S = st;
	dot = strchr(time1S,'.');
	if (dot != NULL) {
		*dot = EOS;
		dot++;
		ms = atof(dot);
		if (ms < 10.0)
			ms = ms / 10.0;
		else // if (ms < 100.0)
			ms = ms / 100.0;
	} else
		ms = 0.0;
	h = atof(time1S);
	col = strchr(time1S,':');
	if (col != NULL) {
		*col = EOS;
		col++;
		m = atof(col);
		col = strchr(col,':');
		if (col != NULL) {
			*col = EOS;
			col++;
			s = atof(col);
		} else {
			s = m;
			m = h;
			h = 0.0;
		}
	} else {
		s = h;
		h = 0.0;
		m = 0.0;
	}
	time1 = (h * 3600.0000) + (m * 60.0000) + s + ms;
	return(time1);
}

void Secs2Str(float tm, char *st, char isMsecs) {
	int   i;
	char  isNeg;
	float h, m, s, ms;

	isNeg = FALSE;
	if (tm < 0.0) {
		isNeg = TRUE;
		tm = -tm;
	}
	i = tm;
	ms = tm - i;
	if (ms > 0.000) {
		ms = ms * 1000.000;
		i = ms;
		ms = i;
	}
	h = tm / 3600.0;
	i = h;
	h = i;
	tm = tm - (h * 3600.0);
	m = tm / 60;
	i = m;
	m = i;
	tm = tm - (m * 60.0);
	i = tm;
	s = i;
	st[0] = EOS;
	if (isNeg)
		strcat(st, "-");
	i = strlen(st);
	if (h >= 1.0)
		sprintf(st+i, "%.0f:", h);
	else
		sprintf(st+i, "00:");
	i = strlen(st);
	if (m < 10.00)
		sprintf(st+i, "0%.0f:", m);
	else
		sprintf(st+i, "%.0f:", m);
	i = strlen(st);
	if (s < 10.00)
		sprintf(st+i, "0%.0f", s);
	else
		sprintf(st+i, "%.0f", s);
	if (isMsecs && ms > 0.000) {
		i = strlen(st);
		if (ms < 10.00)
			sprintf(st+i, ".00%.0f", ms);
		else if (ms < 100.00)
			sprintf(st+i, ".0%.0f", ms);
		else
			sprintf(st+i, ".%.0f", ms);
	}
}

void RoundSecsStr(float tm, char *st) {
	char  isNeg;

	isNeg = FALSE;
	if (tm < 0.0) {
		isNeg = TRUE;
		tm = -tm;
	}
	if (isNeg)
		sprintf(st, "-%.0f", tm);
	else
		sprintf(st, "%.0f", tm);
}

AttTYPE set_color_num(char num, AttTYPE att) {	// bits 8=128, 9=256, 10=512
	AttTYPE t;

	t = 0;
	t = (AttTYPE)num;
	t = t & (AttTYPE)0x7;
	t = t << 7;
	att = zero_color_num(att);
	att = att | t;
	return(att);
}

char get_color_num(AttTYPE att) {			// bits 8=128, 9=256, 10=512
	AttTYPE t;
	char num;

	t = att;
	t = t & (AttTYPE)0x380;
	t = t >> 7;
	num = (char)t;
	return(num);
}

char SetTextAtt(char *st, unCH c, AttTYPE *att) {
	char found = FALSE;

	if (st != NULL)
		c = st[1];
	if (c == underline_start) {
		if (att != NULL)
			*att = set_underline_to_1(*att);
		found = TRUE;
	} else if (c == underline_end) {
		if (att != NULL)
			*att = set_underline_to_0(*att);
		found = TRUE;
	} else if (c == italic_start) {
		if (att != NULL)
			*att = set_italic_to_1(*att);
		found = TRUE;
	} else if (c == italic_end) {
		if (att != NULL)
			*att = set_italic_to_0(*att);
		found = TRUE;
	} else if (c == bold_start) {
		if (att != NULL)
			*att = set_bold_to_1(*att);
		found = TRUE;
	} else if (c == bold_end) {
		if (att != NULL)
			*att = set_bold_to_0(*att);
		found = TRUE;
	} else if (c == error_start) {
		if (att != NULL)
			*att = set_error_to_1(*att);
		found = TRUE;
	} else if (c == error_end) {
		if (att != NULL)
			*att = set_error_to_0(*att);
		found = TRUE;
	} else if (c == blue_start) {
		if (att != NULL)
			*att = set_color_num(blue_color, *att);
		found = TRUE;
	} else if (c == red_start) {
		if (att != NULL)
			*att = set_color_num(red_color, *att);
		found = TRUE;
	} else if (c == green_start) {
		if (att != NULL)
			*att = set_color_num(green_color, *att);
		found = TRUE;
	} else if (c == magenta_start) {
		if (att != NULL)
			*att = set_color_num(magenta_color, *att);
		found = TRUE;
	} else if (c == color_end) {
		if (att != NULL)
			*att = zero_color_num(*att);
		found = TRUE;
	}

	if (found) {
		if (st != NULL)
			strcpy(st, st+2);
		return(TRUE);
	} else {
		return(FALSE);
	}
}

void copyFontInfo(FONTINFO *des, FONTINFO *src, char isUse) {
	SetFontName(des->FName, src->FName);
	des->FSize   = src->FSize;
	des->CharSet = src->CharSet;
	des->Encod = src->Encod;
	des->FHeight = src->FHeight;
}

void copyNewToFontInfo(FONTINFO *des, NewFontInfo *src) {
}

void copyNewFontInfo(NewFontInfo *des, NewFontInfo *src) {
	strcpy(des->fontName, src->fontName);
	des->fontPref = src->fontPref;
	des->fontSize = src->fontSize;
	des->CharSet  = src->CharSet;
	des->charHeight  = src->charHeight;
	des->platform = src->platform;
	des->fontType = src->fontType;
	des->orgFType = src->orgFType;
	des->fontId = src->fontId;
	des->isUTF = src->isUTF;
	des->Encod = src->Encod;
	des->orgEncod = src->orgEncod;
	des->fontTable = src->fontTable;
}

/************************************************************/
/*	 Find and set the current font of the tier			*/
void SetDefaultThaiFinfo(NewFontInfo *finfo) {
	strcpy(finfo->fontName, "Arial Unicode MS");
	finfo->fontSize = 12L;
	finfo->fontPref = "";
	finfo->fontType = getFontType(finfo->fontName, FALSE);
	finfo->orgFType = finfo->fontType;
	finfo->fontId = 0;
	finfo->charHeight = 12;
	finfo->fontTable = NULL;
	finfo->CharSet = 0;
}

void SetDefaultCAFinfo(NewFontInfo *finfo) {
	strcpy(finfo->fontName, "Arial Unicode MS");
	finfo->fontSize = 12L;
	finfo->fontPref = "";
	finfo->fontType = getFontType(finfo->fontName, FALSE);
	finfo->orgFType = finfo->fontType;
	finfo->fontId = 0;
	finfo->charHeight = 12;
	finfo->fontTable = NULL;
	finfo->CharSet = 0;
}

char SetDefaultUnicodeFinfo(NewFontInfo *finfo) {
// DO NOT SET finfo->Encod HERE
	short lOrgFType = finfo->orgFType;
	strcpy(finfo->fontName, "Arial Unicode MS");
	finfo->fontSize = 12L;
	finfo->fontPref = "";
	finfo->fontType = getFontType("Arial Unicode MS", FALSE);
	finfo->orgFType = finfo->fontType;
	finfo->fontId = 0;
	finfo->charHeight = 12;
	finfo->fontTable = NULL;
	finfo->CharSet = 0;
	return(lOrgFType == MACArialUC);
}

char selectChoosenFont(NewFontInfo *finfo, char isForced, char isKeepSize) {
	char isUnicodeFont = FALSE;

	if (!stout && !isForced)
		return(TRUE);
	return(TRUE);
}

char cutt_SetNewFont(const char *st, char ec) {
	NewFontInfo finfo;
	int  i;

	if (ec != EOS) {
		for (i=0; st[i] && st[i] != ':' && st[i] != ec; i++) ;
		if (st[i] != ':')
			return(FALSE);
		i++;
	} else
		i = 0;

	if (GetDatasFont(st+i, ec, &dFnt, dFnt.isUTF, &finfo) == NULL)
		return(FALSE);

	dFnt.orgFType = finfo.orgFType;
	return(FALSE);
}

void textToXML(char *an, const char *bs, const char *es) {
	long i;

	for (; (isSpace(*bs) || *bs == '\n') && bs < es; bs++) ;
	for (es--; (isSpace(*es) || *es == '\n') && bs <= es; es--) ;
	es++;
	i = 0L;
	for (; bs < es; bs++) {
		if (*bs == '&') {
			strcpy(an+i, "&amp;");
			i = strlen(an);
		} else if (*bs == '"') {
			strcpy(an+i, "&quot;");
			i = strlen(an);
		} else if (*bs == '\'') {
			strcpy(an+i, "&apos;");
			i = strlen(an);
		} else if (*bs == '<') {
			strcpy(an+i, "&lt;");
			i = strlen(an);
		} else if (*bs == '>') {
			strcpy(an+i, "&gt;");
			i = strlen(an);
		} else if (*bs == '\n')
			an[i++] = ' ';
		else if (*bs == '\t')
			an[i++] = ' ';
		else if (*bs >= 0 && *bs < 32) {
			sprintf(an+i,"{0x%x}", *bs);
			i = strlen(an);
		} else
			an[i++] = *bs;
	}
	an[i] = EOS;
}

// Excel Output BEGIN
void excelTextToXML(char *an, const char *bs, const char *es) {
	long i;

	for (; (isSpace(*bs) || *bs == '\n') && bs < es; bs++) ;
	for (es--; (isSpace(*es) || *es == '\n') && bs <= es; es--) ;
	es++;
	i = 0L;
	for (; bs < es; bs++) {
		if (*bs == '&') {
			strcpy(an+i, "&amp;");
			i = strlen(an);
		} else if (*bs == '"') {
			strcpy(an+i, "&quot;");
			i = strlen(an);
/*
		} else if (*bs == '\'') {
			strcpy(an+i, "&apos;");
			i = strlen(an);
*/
		} else if (*bs == '<') {
			strcpy(an+i, "&lt;");
			i = strlen(an);
		} else if (*bs == '>') {
			strcpy(an+i, "&gt;");
			i = strlen(an);
		} else if (*bs == '\n')
			an[i++] = ' ';
		else if (*bs == '\t')
			an[i++] = ' ';
		else if (*bs >= 0 && *bs < 32) {
			sprintf(an+i,"{0x%x}", *bs);
			i = strlen(an);
		} else
			an[i++] = *bs;
	}
	an[i] = EOS;
}

void excelHeader(FILE *fp, char *name, int colSize) {
	char *sName;

	fprintf(fp, "<?xml version=\"1.0\"?>\n");
	fprintf(fp, "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n");
	fprintf(fp, " xmlns:o=\"urn:schemas-microsoft-com:office:office\"\n");
	fprintf(fp, " xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n");
	fprintf(fp, " xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n");
	fprintf(fp, " xmlns:html=\"http://www.w3.org/TR/REC-html40\">\n");
	fprintf(fp, " <ExcelWorkbook xmlns=\"urn:schemas-microsoft-com:office:excel\">\n");
	fprintf(fp, "  <WindowHeight>15260</WindowHeight>\n");
	fprintf(fp, "  <WindowWidth>25600</WindowWidth>\n");
	fprintf(fp, "  <Date1904/>\n");
	fprintf(fp, "  <ProtectStructure>False</ProtectStructure>\n");
	fprintf(fp, "  <ProtectWindows>False</ProtectWindows>\n");
	fprintf(fp," </ExcelWorkbook>\n");
	fprintf(fp, " <Styles>\n");
	fprintf(fp, "  <Style ss:ID=\"RedText\">\n");
	fprintf(fp, "    <Font ss:FontName=\"Calibri\" ss:Size=\"12\" ss:Color=\"#FF0000\"/>\n");
	fprintf(fp, "  </Style>\n");
	fprintf(fp, "  <Style ss:ID=\"TallText\">\n");
	fprintf(fp, "    <Alignment ss:Vertical=\"Bottom\" ss:WrapText=\"1\"/>\n");
	fprintf(fp, "  </Style>\n");
	fprintf(fp, " </Styles>\n");

	if (name[0] == EOS) {
		strcpy(tempTier1, "sheet 1");
	} else {
		sName = strrchr(name, PATHDELIMCHR);
		if (sName != NULL) {
			sName++;
			excelTextToXML(tempTier1, sName, sName+strlen(sName));
		} else {
			excelTextToXML(tempTier1, name, name+strlen(name));
		}
	}
	if (strlen(tempTier1) > 30)
		tempTier1[30] = EOS;
	fprintf(fp, " <Worksheet ss:Name=\"%s\">\n", tempTier1);
	fprintf(fp, "  <Table ss:DefaultColumnWidth=\"%d\" ss:DefaultRowHeight=\"15\">\n", colSize); // 85
}

void excelFooter(FILE *fp) {
	fprintf(fp, "  </Table>\n");
	fprintf(fp, " </Worksheet>\n");
	fprintf(fp, "</Workbook>\n");
}

static char isAllDigit(const char *st) {
	char digitFound;

	digitFound = FALSE;
	for (; *st != EOS; st++) {
		if (*st != '.' && !isdigit(*st))
			return(FALSE);
		else if (isdigit(*st))
			digitFound = TRUE;
	}
	if (digitFound)
		return(TRUE);
	return(FALSE);
}

void excelOutputID(FILE *fp, char *IDtier) {
	int  cnt;
	char *b, *e, *comma;

	cnt = 0;
	b = IDtier;
	while (*b != EOS) {
		e = strchr(b, '|');
		if (e != NULL)
			*e = EOS;
		if (*b == EOS) {
			fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", ".");
			if (cnt == 6)
				fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", ".");
		} else {
			if (isAllDigit(b))
				fprintf(fp, "    <Cell><Data ss:Type=\"Number\">%s</Data></Cell>\n", b);
			else if (cnt == 6) {
				comma = strchr(b, ',');
				if (comma != NULL) {
					*comma = EOS;
					fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", b);
					fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", comma+1);
					*comma = ',';
				} else if (isupper(*b) && isupper(*(b+1)) && (*(b+2) == EOS || isSpace(*(b+2)))) {
					fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", ".");
					fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", b);
				} else {
					fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", b);
					fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", ".");
				}
			} else
				fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", b);
		}
		if (e != NULL) {
			cnt++;
			*e = '|';
			b = e + 1;
		} else
			break;
	}
	if (cnt < 10)
		fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", ".");
}

void excelCommasStrCell(FILE *fp, const char *st) {
	int  i, j;
	char buf[BUFSIZ];

	if (st != NULL) {
		j = 0;
		for (i=0; st[i] != EOS; i++) {
			if (st[i] == ',') {
				buf[j] = EOS;
				if (buf[0] != EOS) {
					if (isAllDigit(buf))
						fprintf(fp, "    <Cell><Data ss:Type=\"Number\">%s</Data></Cell>\n", buf);
					else {
						excelTextToXML(tempTier1, buf, buf+strlen(buf));
						fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", tempTier1);
					}
				}
				j = 0;
			} else
				buf[j++] = st[i];
		}
		buf[j] = EOS;
		if (buf[0] != EOS) {
			if (isAllDigit(buf))
				fprintf(fp, "    <Cell><Data ss:Type=\"Number\">%s</Data></Cell>\n", buf);
			else {
				excelTextToXML(tempTier1, buf, buf+strlen(buf));
				fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", tempTier1);
			}
		}
	}
}

void excelRedStrCell(FILE *fp, const char *st) {
	if (st != NULL) {
		if (*st == ',')
			st++;
		excelTextToXML(tempTier1, st, st+strlen(st));
		fprintf(fp, "    <Cell ss:StyleID=\"RedText\"><Data ss:Type=\"String\">%s</Data></Cell>\n", tempTier1);
	}
}

void excelStrCell(FILE *fp, const char *st) {
	if (st != NULL) {
		if (*st == ',')
			st++;
		if (isAllDigit(st))
			fprintf(fp, "    <Cell><Data ss:Type=\"Number\">%s</Data></Cell>\n", st);
		else {
			excelTextToXML(tempTier1, st, st+strlen(st));
			fprintf(fp, "    <Cell><Data ss:Type=\"String\">%s</Data></Cell>\n", tempTier1);
		}
	}
}

void excelNumCell(FILE *fp, const char *format, float num) {
	if (*format == ',')
		format++;
	fprintf(fp, "    <Cell><Data ss:Type=\"Number\">");
	if (num == 0.0)
		fprintf(fp, "0");
	else
		fprintf(fp, format, num);
	fprintf(fp, "</Data></Cell>\n");
}

void excelLongNumCell(FILE *fp, const char *format, long num) {
	if (*format == ',')
		format++;
	fprintf(fp, "    <Cell><Data ss:Type=\"Number\">");
	if (num == 0.0)
		fprintf(fp, "0");
	else
		fprintf(fp, format, num);
	fprintf(fp, "</Data></Cell>\n");
}

void excelRow(FILE *fp, int type) {
	if (type == ExcelRowStart) {
		fprintf(fp, "   <Row>\n");
	} else if (type == ExcelRowEnd) {
		fprintf(fp, "   </Row>\n");
	} else if (type == ExcelRowEmpty) {
		fprintf(fp, "   <Row>\n");
		fprintf(fp, "   </Row>\n");
	}
}

void excelHeightRowStart(FILE *fp, int height) {
	fprintf(fp, "   <Row ss:Height=\"%d\" ss:StyleID=\"TallText\">\n", height);
}

void excelRowOneStrCell(FILE *fp, int isRed, const char *st) {
	excelRow(fp, ExcelRowStart);
	if (isRed == ExcelRedCell)
		excelRedStrCell(fp, st);
	else
		excelStrCell(fp, st);
	excelRow(fp, ExcelRowEnd);
}
// Excel Output END
// commas Excel Output BEGIN
void outputIDForExcel(FILE *fp, char *IDtier, short isComma) {
	int  cnt;
	char *b, *e, *comma;

	if (isComma < 0)
		fprintf(fp, ",");
	cnt = 0;
	b = IDtier;
	while (*b != EOS) {
		e = strchr(b, '|');
		if (e != NULL)
			*e = EOS;
		if (*b == EOS) {
			fprintf(fp, ".");
			if (cnt == 6) {
				fprintf(fp, ",");
				fprintf(fp, ".");
			}
		} else if (cnt == 6) {
			comma = strchr(b, ',');
			if (comma != NULL) {
				*comma = EOS;
				fprintf(fp, "\"");
				fprintf(fp, "%s", b);
				fprintf(fp, "\"");
				fprintf(fp, ",");
				fprintf(fp, "\"");
				fprintf(fp, "%s", comma+1);
				fprintf(fp, "\"");
				*comma = ',';
			} else if (isupper(*b) && isupper(*(b+1)) && (*(b+2) == EOS || isSpace(*(b+2)))) {
				fprintf(fp, ".");
				fprintf(fp, ",");
				fprintf(fp, "\"");
				fprintf(fp, "%s", b);
				fprintf(fp, "\"");
			} else {
				fprintf(fp, "\"");
				fprintf(fp, "%s", b);
				fprintf(fp, "\"");
				fprintf(fp, ",");
				fprintf(fp, ".");
			}
		} else if (strchr(b, ',')) {
			fprintf(fp, "\"");
			fprintf(fp, "%s", b);
			fprintf(fp, "\"");
		} else
			fprintf(fp, "%s", b);
		if (e != NULL) {
			cnt++;
			if (cnt < 10)
				fprintf(fp, ",");
			*e = '|';
			b = e + 1;
		} else
			break;
	}
	if (isComma > 0)
		fprintf(fp, ",");
}

void outputStringForExcel(FILE *fp, const char *st, short isComma) {
	if (isComma < 0)
		fprintf(fp, ",");
	if (strchr(st, ',')) {
		fprintf(fp, "\"");
		fprintf(fp, "%s", st);
		fprintf(fp, "\"");
	} else
		fprintf(fp, "%s", st);
	if (isComma > 0)
		fprintf(fp, ",");
}
// commas Excel Output END

char isIDSpeakerSpecified(char *IDTier, char *code, char isReportErr) {
	char *s, *codeP, isSpSpecified;
	struct tier *p;

	if (IDTier != code)
		strcpy(code, IDTier);
	s = code;
	if ((s=strchr(s, '|')) != NULL) {
		s++;
		if ((s=strchr(s, '|')) != NULL) {
			s++;
			codeP = s;
			if ((s=strchr(s, '|')) != NULL) {
				*s = EOS;
				if (*codeP != '*' && *codeP != EOS) {
					code[0] = '*';
					strcpy(code+1, codeP);
				} else {
					strcpy(code, codeP);
				}
				if (*codeP != EOS)
					strcat(code, ":");
				if (checktier(code) && *code != EOS)
					return(TRUE);
				isSpSpecified = FALSE;
				for (p=headtier; p != NULL; p=p->nexttier) {
					if (*p->tcode == '*') {
						isSpSpecified = TRUE;
					}
				}
				if (IDField == NULL && SPRole == NULL && isSpSpecified == FALSE)
					return(TRUE);
			} else if (isReportErr) {
				uS.remblanks(code);
				fprintf(stderr,"Malformed @ID tier format: %s. In file %s\n", code, oldfname);
				cutt_exit(1);
			}
		} else if (isReportErr) {
			uS.remblanks(code);
			fprintf(stderr,"Malformed @ID tier format: %s. In file %s\n", code, oldfname);
			cutt_exit(1);
		}
	} else if (isReportErr) {
		uS.remblanks(code);
		fprintf(stderr,"Malformed @ID tier format: %s. In file %s\n", code, oldfname);
		cutt_exit(1);
	}
	return(FALSE);
}

/**************************************************************/
/*	 main loop, argv pars, fname pars					*/
/* chattest(InFname) test the first TESTLIM turns of the input data file to
 make sure that the file is in the CHAT format. If the first part is
 correct it is assumed that the rest of the data is in a good form.

*/

char chattest(FNType *InFname, char isJustCheck, char *isUTF8SymFound) {
					/* chatmode =1 - y switch & errmargin =0;	*/
					/*	   =2 - no y switch & errmargin =0; */
					/*	   =3 - y opt & errmargin = TESTLIM */
					/*	   =4 - no y switch & no testing	*/
	char isFTime, fdata = 0, data, ID_Code_MatchFound = 0;
	char *x;
	long ln, len;
	int i, err = 0, count = 0;

	isFTime = TRUE;
	*isUTF8SymFound = 0;
	ln = 1;
	len = 0;
	templineC4[0] = EOS;
	while ((x=fgets_cr(templineC3, UTTLINELEN, fpin)) != NULL) {
		if (uS.partcmp(templineC3,"@Languages:", FALSE, FALSE)) {
			NewFontInfo finfo;
			char isThaiFound = FALSE;

			for (i=0; templineC3[i] != ':' && templineC3[i] != EOS; i++) ;
			if (templineC3[i] == ':')
				i++;
			for (; isSpace(templineC3[i]); i++) ;
			for (; templineC3[i] != EOS; i++) {
				if (templineC3[i] == 't' && templineC3[i+1] == 'h' &&
					(isSpace(templineC3[i+2]) || templineC3[i+2]== ',' || templineC3[i+2]== EOS || templineC3[i+2]== '\n'))
					isThaiFound = TRUE;
			}
			if (isThaiFound) {
				SetDefaultThaiFinfo(&finfo);
				selectChoosenFont(&finfo, FALSE, FALSE);
			}
		}
		if (isFTime) {
			if (templineC3[0] == (char)0xef && templineC3[1] == (char)0xbb && templineC3[2] == (char)0xbf) {
				*isUTF8SymFound = 1;
				strcpy(templineC3, templineC3+3);
			} else if ((templineC3[0] == (char)0xff && templineC3[1] == (char)0xfe) || 
					   (templineC3[0] == (char)0xfe && templineC3[1] == (char)0xff)) {
				*isUTF8SymFound = 2;
			}
		}
		isFTime = FALSE;
		for (i=0; templineC3[i]; i++) {
			if (templineC3[i] == ATTMARKER)
				strcpy(templineC3+i, templineC3+i+2);
		}
		i = strlen(templineC3) - 1;
		if (templineC3[i] == '\n') {
			i--;
			ln++;
		} else if (!feof(fpin))
			break;
		for (; i > -1 && isSpace(templineC3[i]); i--) ;
		templineC3[i+1] = EOS;
		data = 0;
		i = 0;

		for (; templineC3[i] != ':' && templineC3[i] != EOS; i++) {
			if (!isSpace(templineC3[i]))
				data = 1;
		}
		if (templineC3[i] == ':') {
			if (isSpace(*templineC3)) {
				if (!fdata)
					break;
			} else if (!isSpeaker(*templineC3) && (!isdigit(templineC3[i-1]) || i == 0))
				break;
			count++;
			fdata = 1;
			err = 0;
		} else if ((data && !fdata) || (data && *templineC3 != ' ' && *templineC3 != '\t')) {
//			uS.uppercasestr(templineC3, &dFnt, MBF);
			if (*templineC3 != '@') {
				if (chatmode != 3 || ++err == TESTLIM)
					break;
			} else
				count++;
			fdata = 1;
		}
		if (isSpeaker(*templineC3)) {
			if (IDField != NULL || CODEField != NULL) {
				if (IDField != NULL) {
					if (strncmp(templineC4, "@ID:", 4) == 0) {
						for (i=4; isSpace(templineC4[i]); i++) ;
						if (CheckIDNumber(templineC4+i, templineC))
							ID_Code_MatchFound = (ID_Code_MatchFound | 1);
						else if (isIDSpeakerSpecified(templineC4+i, templineC4+i, FALSE))
							ID_Code_MatchFound = (ID_Code_MatchFound | 1);
					}
				}
				if (CODEField != NULL) {
					if (strncmp(templineC4, "@Keywords:", 10) == 0) {
						for (i=6; isSpace(templineC4[i]); i++) ;
						if (CheckCodeNumber(templineC4+i))
							ID_Code_MatchFound = (ID_Code_MatchFound | 2);
					} 
				}
				strcpy(templineC4, templineC3);
				len = strlen(templineC3);
			}
			if (isMainSpeaker(templineC3[0])) {
				if (count > 0)
					count = TESTLIM;
				break;
			}
		} else {
			if (IDField != NULL || CODEField != NULL) {
				len = len + strlen(templineC3);
				if (len >= UTTLINELEN) {
					count = 0;
					err = TESTLIM;
					break;
				} else
					strcat(templineC4, templineC3);
			}
		}
	}
	if ((count < TESTLIM && x != NULL) || err >= TESTLIM) {
		if (!isJustCheck) {
			fprintf(stderr,"*** File \"%s\": line %ld.\n", InFname, ln);			
			fprintf(stderr, "ERROR: File is NOT in a proper CHAT format.\n%s\n", templineC3);
			if (option_flags[CLAN_PROG_NUM] & Y_OPTION) {
				fprintf(stderr, "Use +y option if the file is NOT supposed to be in CHAT format.\n");
				fprintf(stderr,"Otherwise fix the error!\n");
			}
			cutt_exit(0);
		} else
			return(FALSE);
	}
	if (!isJustCheck) {
		if (IDField != NULL && !(ID_Code_MatchFound & 1)) {
			return(FALSE);
		} else if (CODEField != NULL) {
			if (ID_Code_MatchFound & 2)
				return(tcode);
			else
				return(!tcode);
		}
	}
	return(TRUE);
}

int pathcmp(const FNType *path1, const FNType *path2) {
	for (; toupper(*path1) == toupper(*path2) && *path2 != EOS; path1++, path2++) ;
	if (*path1 == EOS && *path2 == PATHDELIMCHR && *(path2+1) == EOS)
		return(0);
	else if (*path1 == PATHDELIMCHR && *(path1+1) == EOS && *path2 == EOS)
		return(0);
	else if (*path1 == EOS && *path2 == EOS)
		return(0);
	else if (toupper(*path1) > toupper(*path2))
		return(1);
	else
		return(-1);	
}

void addFilename2Path(FNType *path, const FNType *filename) {
	int len;

	len = strlen(path);
#if defined(_DOS)
	if (len > 0 && path[len-1] != PATHDELIMCHR)
#else
	if (path[len-1] != PATHDELIMCHR)
#endif
		path[len++] = PATHDELIMCHR;
	if (filename[0] == PATHDELIMCHR)
		strcpy(path+len, filename+1);
	else
		strcpy(path+len, filename);
}

char extractPath(FNType *mDirPathName, FNType *file_path) {
	FNType *p;

	strcpy(mDirPathName, file_path);
	p = strrchr(mDirPathName, PATHDELIMCHR);
	if (p != NULL) {
		*(p+1) = EOS;
		return(TRUE);
	} else {
//		mDirPathName[0] = EOS;
		return(FALSE);
	}
}

void extractFileName(FNType *FileName, FNType *file_path) {
	FNType *p;

	p = strrchr(file_path, PATHDELIMCHR);
	if (p != NULL) {
		strcpy(FileName, p+1);
	} else
		strcpy(FileName, file_path);
}

/* parsfname(old,fname,fext) take the file name in "old" removes, if it
   exists, an extention from the end of it. Then it removes the full path, if
   it exists. Then it copies the remainder to the "fname" and concatanate the
   extention "ext" to the end of string in "fname".
*/
void parsfname(FNType *oldO, FNType *fname, const char *fext) {
	long i, ti, j;
	FNType *old, oldS[FNSize];

	if (strlen(oldO) < FNSize-1) {
		strcpy(oldS, oldO);
		old = oldS;
	} else
		old = oldO;
	i = strlen(old) - 1;
	while (i >= 0L && (old[i] == ' ' || old[i] == '\t'))
		i--;
	old[++i] = EOS;
	while (i > 0L && old[i] != '.' && old[i] != PATHDELIMCHR)
		i--;
	ti = 0L;
	if (old[i] == PATHDELIMCHR) {
		if (!Preserve_dir)
			ti = i+1;
		i = strlen(old);
	} else if (old[i] == '.') {
		if (old[i-1] != PATHDELIMCHR) {
			if (!Preserve_dir) {
				ti = i;
				while (ti > 0L && old[ti] != PATHDELIMCHR) ti--;
				if (old[ti] == PATHDELIMCHR) ti++;
			}
		} else {
			if (!Preserve_dir)
				ti = i;
			i = strlen(old);
		}
	} else {
		ti = 0L;
		i = strlen(old);
	}
	j = 0L;
	while (ti < i)
		fname[j++] = old[ti++];
	fname[j] = EOS;
	uS.str2FNType(fname, j, fext);
}

/* openwfile(oldfname,fname,fp) opens file "fname" for writing while trying to
   maintain version numbers of output files. The limit is 10 versions as of
   now.
*/
FILE *openwfile(const FNType *oldfname, FNType *fname, FILE *fp) {
#ifndef _COCOA_APP
	register char res;
#endif
	register int i = 0;
	register int len;

	if (!FirstTime && combinput)
		return(fp);
	len = strlen(fname);
	if (AddCEXExtension[0] != EOS)
		uS.str2FNType(fname, strlen(fname), AddCEXExtension);
	if (!OverWriteFile) {
		for (i=0; i < 100 && !access(fname,0); i++) {
			sprintf(fname+len, "%d", i);
			if (AddCEXExtension[0] != EOS)
				uS.str2FNType(fname, strlen(fname), AddCEXExtension);
		}
		if (i >= 100) {
			strcpy(fname+len, "?: Too many versions.");
			return(NULL);
		}
	}
	if (!uS.mStricmp(oldfname, fname))
		uS.str2FNType(fname, strlen(fname), ".1");
	fp = fopen(fname,"w");
	return(fp);
}

int IsOutTTY(void) {
	int i ;

//fprintf(stderr, "IsOutTTY\n");

//	return(1);
#if defined(UNX)
	#ifdef USE_TERMIO
		struct termio otty;
		i = ioctl(fileno(stdout), TCGETA, &otty);
//fprintf(stderr, "IsOutTTY=%d\n", i);
		if (i == -1) {
			if (errno == ENOTTY)
				return(0);
		}
	#elif defined(APPLEUNX)
		struct sgttyb otty;
		i = ioctl(fileno(stdout), TIOCGETP, &otty);
//fprintf(stderr, "IsOutTTY=%d\n", i);
		return(i == 0);
	#else
		struct sgttyb otty;
		i = gtty(fileno(stdout), &otty);
//fprintf(stderr, "IsOutTTY=%d\n", i);
		return(i == 0);
	#endif
#endif
	return(1);
}

/* help function to CurTierSearch() function.
*/
char listtcs(char todo, char code) {
	struct tier *p;
	char f = TRUE;

	if (todo == '\0' || todo == '\001') {
		for (p=defheadtier; p != NULL; p=p->nexttier) {
			if ((p->include || todo) && *p->tcode == code && !p->tcode[1])
				return('\004');
			else if (*p->tcode == code)
				f = FALSE;
		}
		for (p=headtier; p != NULL; p=p->nexttier) {
			if ((p->include || todo) && *p->tcode == code && !p->tcode[1])
				return('\004');
			else if (*p->tcode == code)
				f = FALSE;
		}
		if (SPRole != NULL) {
			if (code == '*')
				f = FALSE;
		}
		if (todo == '\001' && f)
			return('\003');
	} else
	if (todo == '\004') {
		for (p=defheadtier; p != NULL; p=p->nexttier) {
			if (p->include == 0 && *p->tcode == code) {
				if (f) {
					f = FALSE;
					if (FirstTime)
						fputs(" EXCEPT the ones matching:",stderr);
					if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
						fputs(" EXCEPT the ones matching:", fpout);
				}
				if (FirstTime)
					fprintf(stderr," %s;",p->tcode);
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
					fprintf(fpout," %s;",p->tcode);
			}
		}
		for (p=headtier; p != NULL; p=p->nexttier) {
			if (p->include == 0 && *p->tcode == code) {
				if (f) {
					f = FALSE;
					if (FirstTime)
						fputs(" EXCEPT the ones matching:",stderr);
					if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
						fputs(" EXCEPT the ones matching:",fpout);
				}
				if (FirstTime)
					fprintf(stderr," %s;",p->tcode);
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
					fprintf(fpout," %s;",p->tcode);
			}
		}
	} else {
		for (p=defheadtier; p != NULL; p=p->nexttier) {
			if ((p->include || todo == '\003') && *p->tcode == code) {
				if (FirstTime)
					fprintf(stderr," %s;",p->tcode);
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
					fprintf(fpout," %s;",p->tcode);
			}
		}
		for (p=headtier; p != NULL; p=p->nexttier) {
			if ((p->include || todo == '\003') && *p->tcode == code) {
				if (FirstTime)
					fprintf(stderr," %s;",p->tcode);
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
					fprintf(fpout," %s;",p->tcode);
			}
		}
	}
	return('\002');
}

/* CurTierSearch(pn) displays the list of all tiers that would be
   included/excluded from the analyses. "pn" is a current program name.
*/
void CurTierSearch(char *pn) {
	int i;
	char t;
	time_t timer;
	struct IDtype *tID;

	if (onlydata >= 10)
		return;
	time(&timer);
	if (FirstTime) {
		fprintf(stderr,"%s", ctime(&timer));
	}
	if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData) {
		fprintf(fpout,"%s", ctime(&timer));
	}
	for (i=strlen(pn)-1; i >= 0 && pn[i] != ':' && pn[i] != PATHDELIMCHR; i--) ;
	if (FirstTime) {
		fprintf(stderr,"%s", pn+i+1);
		VersionNumber(TRUE, stderr);
		if (chatmode)
			fprintf(stderr," is conducting analyses on:\n");
		else
			fprintf(stderr,"\n");
	}
	if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData) {
		fprintf(fpout,"%s", pn+i+1);
		VersionNumber(TRUE, fpout);
		if (chatmode)
			fprintf(fpout," is conducting analyses on:\n");
		else
			fprintf(fpout,"\n");
	}
	if (!chatmode) {
		if (FirstTime)
			fputs("****************************************\n",stderr);
		if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
			fputs("****************************************\n",fpout);
		return;
	}
	i = FALSE;

	if (!nomain) {
		if (FirstTime)
			fputs("  ",stderr);
		if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
			fputs("  ", fpout);
		if (tcs) {
			if ((t=listtcs('\0','*')) == 4) {
				if (FirstTime) {
					if (IDField != NULL)
						fputs("ALL speaker main tiers whose IDs",stderr);
					else if (SPRole != NULL)
						fputs("ALL speaker main tiers whose role(s):",stderr);
					else
						fputs("ALL speaker main tiers",stderr);
				}
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData) {
					if (IDField != NULL)
						fputs("ALL speaker main tiers whose IDs",fpout);
					else if (SPRole != NULL)
						fputs("ALL speaker main tiers whose role(s):",fpout);
					else
						fputs("ALL speaker main tiers",fpout);
				}
			} else {
				if (FirstTime) {
					if (IDField != NULL)
						fputs("ONLY speaker main tiers with IDs matching:",stderr);
					else if (SPRole != NULL)
						fputs("ONLY speaker main tiers with role(s):",stderr);
					else
						fputs("ONLY speaker main tiers matching:",stderr);
				}
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData) {
					if (IDField != NULL)
						fputs("ONLY speaker main tiers with IDs matching:",fpout);
					else if (SPRole != NULL)
						fputs("ONLY speaker main tiers with role(s):",fpout);
					else
						fputs("ONLY speaker main tiers matching:",fpout);
				}
			}
			for (tID=SPRole; tID != NULL; tID=tID->next_ID) {
				if (FirstTime)
					fprintf(stderr," %s;", tID->ID);
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
					fprintf(fpout," %s;", tID->ID);
			}
			for (tID=IDField; tID != NULL; tID=tID->next_ID) {
				if (FirstTime)
					fprintf(stderr," %s;", tID->ID);
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
					fprintf(fpout," %s;", tID->ID);
			}
			listtcs(t,'*');
			i = TRUE;
		} else
		if ((t=listtcs('\001','*')) != 4) {
			if (t == 3) {
				if (FirstTime) {
					if (IDField != NULL)
						fputs("ALL speaker tiers with IDs",stderr);
					else if (SPRole != NULL)
						fputs("ALL speaker tiers with role(s):",stderr);
					else
						fputs("ALL speaker tiers",stderr);
				}
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData) {
					if (IDField != NULL)
						fputs("ALL speaker tiers with IDs",fpout);
					else if (SPRole != NULL)
						fputs("ALL speaker tiers with role(s):",fpout);
					else
						fputs("ALL speaker tiers",fpout);
				}
			} else {
				if (FirstTime) {
					if (IDField != NULL)
						fputs("ALL speaker main tiers EXCEPT the ones with IDs matching:",stderr);
					else if (SPRole != NULL)
						fputs("ALL speaker main tiers EXCEPT the ones with role(s):",stderr);
					else
						fputs("ALL speaker main tiers EXCEPT the ones matching:",stderr);
				}
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData) {
					if (IDField != NULL)
						fputs("ALL speaker main tiers EXCEPT the ones with IDs matching:",fpout);
					else if (SPRole != NULL)
						fputs("ALL speaker main tiers EXCEPT the ones with role(s):",fpout);
					else
						fputs("ALL speaker main tiers EXCEPT the ones matching:",fpout);
				}
				listtcs('\003','*');
			}
			for (tID=SPRole; tID != NULL; tID=tID->next_ID) {
				if (FirstTime)
					fprintf(stderr," %s;", tID->ID);
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
					fprintf(fpout," %s;", tID->ID);
			}
			for (tID=IDField; tID != NULL; tID=tID->next_ID) {
				if (FirstTime)
					fprintf(stderr," %s;", tID->ID);
				if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
					fprintf(fpout," %s;", tID->ID);
			}
			i = TRUE;
		}
	}

	if (tct) {
		if (nomain) {
			if (FirstTime) fputs("  ",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("  ",fpout);
		} else {
			if (FirstTime)
				fprintf(stderr,"\n	and those speakers' ");
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fprintf(fpout,"\n    and those speakers' ");
		}
		if ((t=listtcs('\0','%')) == 4) {
			if (FirstTime)
				fputs("ALL dependent tiers",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("ALL dependent tiers",fpout);
		} else {
			if (FirstTime)
				fputs("ONLY dependent tiers matching:",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("ONLY dependent tiers matching:",fpout);
		}
		listtcs(t,'%');
		i = TRUE;
	} else
	if ((t=listtcs('\001','%')) != 4) {
		if (nomain) {
			if (FirstTime)
				fputs("  ",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("  ",fpout);
		} else {
			if (FirstTime)
				fprintf(stderr,"\n    and those speakers' ");
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fprintf(fpout,"\n    and those speakers' ");
		}
		if (t == 3) {
			if (FirstTime)
				fputs("ALL dependent tiers",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("ALL dependent tiers",fpout);
		} else {
			if (FirstTime)
				fputs("ALL dependent tiers EXCEPT the ones matching:",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("ALL dependent tiers EXCEPT the ones matching:",fpout);
			listtcs('\003','%');
		}
		i = TRUE;
	}

	if (tch) {
		if (i) {
			if (FirstTime)
				fputs("\n  and ",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("\n  and ",fpout);
		}
		if ((t=listtcs('\0','@')) == 4) {
			if (FirstTime)
				fputs("ALL header tiers",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("ALL header tiers",fpout);
		} else {
			if (FirstTime)
				fputs("ONLY header tiers matching:",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("ONLY header tiers matching:",fpout);
		}
		listtcs(t,'@');
		i = TRUE;
	} else
	if ((t=listtcs('\001','@')) != 4) {
		if (i) {
			if (FirstTime)
				fputs("\n  and ",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				 fputs("\n  and ",fpout);
		}
		if (t == 3) {
			if (FirstTime)
				fputs("ALL header tiers",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("ALL header tiers",fpout);
		} else {
			if (FirstTime)
				fputs("ALL header tiers EXCEPT the ones matching:",stderr);
			if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
				fputs("ALL header tiers EXCEPT the ones matching:",fpout);
			listtcs('\003','@');
		}
		i = TRUE;
	}

	if (!i)
		fputs("NO DATA!!!",stderr);
	if (FirstTime)
		fputs("\n****************************************\n",stderr);
	if ((!stout || !IsOutTTY()) && (!onlydata || puredata < 2) && !outputOnlyData)
		fputs("\n****************************************\n",fpout);
}

void printArg(char *argv[], int argc, FILE *fp, char specialCase, FNType *fname) {
	int  i;
	char *st, isSpf, *tst, isDQf;

	for (i=strlen(argv[0])-1; i >= 0 && argv[0][i] != ':' && argv[0][i] != PATHDELIMCHR; i--) ;
	fprintf(fp,"%s", argv[0]+i+1);
	for (i=1; i < argc; i++) {
		isDQf = FALSE;
		isSpf = FALSE;
		st = argv[i];
		if (!specialCase || argv[i][0] == '+' || argv[i][0] == '-' || argv[i][0] == '"' || combinput || !uS.FNTypecmp(fname, argv[i], 0L)) {
			for (tst=st; *tst != EOS; tst++) {
				if (*tst == ' ' || *tst == '\t' || *tst == '|' || *tst == '<' || *tst == '>') {
					isSpf = TRUE;
				} else if ((*tst == '"' || *tst == '\'') && (tst == st || *(tst-1) != '\\'))
					isSpf = TRUE;
				if (*tst == '"')
					isDQf = TRUE;
			}
			if (isSpf) {
				fputc(' ', fp);
				if (*st == '+' || *st == '-') {
					fputc(*st, fp);
					st++;
					fputc(*st, fp);
					st++;
					if (*st == '+') {
						fputc(*st, fp);
						st++;
					}
					if (*st == '@') {
						fputc(*st, fp);
						st++;
					}
				}
				if (isDQf)
					fputc('\'', fp);
				else
					fputc('"', fp);
				for (; *st != EOS; st++)
					fputc(*st, fp);
				if (isDQf)
					fputc('\'', fp);
				else
					fputc('"', fp);
			} else
				fprintf(fp," %s", st);
		}
	}
	putc('\n', fp);
}

static void checkSearchTiers(void) {
	char whatDepTiers;
	struct tier *p;
	IEWORDS *twd;

	whatDepTiers = 0;
	for (p=headtier; p != NULL; p=p->nexttier) {
		if (uS.partcmp("%gra:",p->tcode,p->pat_match,FALSE) || uS.partcmp("%grt:",p->tcode,p->pat_match,FALSE)) {
			whatDepTiers = whatDepTiers | 2;
		} else if (uS.partcmp("%mor:",p->tcode,p->pat_match,FALSE) || uS.partcmp("%xmor:",p->tcode,p->pat_match,FALSE) ||
				   uS.partcmp("%cnl:",p->tcode,p->pat_match,FALSE) || uS.partcmp("%xcnl:",p->tcode,p->pat_match,FALSE) ||
				   uS.partcmp("%trn:",p->tcode,p->pat_match,FALSE)) {
			whatDepTiers = whatDepTiers | 1;
		}
	}
	if (morfptr != NULL) {
		if (whatDepTiers == 0) {
			if (morfptr->rootFeat != NULL && morfptr->rootFeat->pat != NULL) {
				fprintf(stderr,"\n  %%mor tier item specified with %csm%c%c%s option can not be found on Speaker tier.\n",
						(morfptr->type == 'i' ? '+' : '-'), morfptr->rootFeat->featType, morfptr->rootFeat->flag, morfptr->rootFeat->pat);
			} else {
				fprintf(stderr,"\n  The %%mor word specified with +/-sm option can not be found on Speaker tier.\n");
			}
			fprintf(stderr,"  Please either select %%mor tier with +t%%mor option or change the word you are searching for.\n");
			cutt_exit(0);
		} else if (whatDepTiers == 2) {
			if (morfptr->rootFeat != NULL && morfptr->rootFeat->pat != NULL) {
				fprintf(stderr,"\n  %%mor tier item specified with %csm%c%c%s option can not be found on either Speaker or %%gra tier.\n",
						(morfptr->type == 'i' ? '+' : '-'), morfptr->rootFeat->featType, morfptr->rootFeat->flag, morfptr->rootFeat->pat);
			} else {
				fprintf(stderr,"\n  The %%mor word specified with +/-sm option can not be found on either Speaker or %%gra tier.\n");
			}
			fprintf(stderr,"  Please either select %%mor tier with +t%%mor option or change the word you are searching for.\n");
			cutt_exit(0);
		}
	}
	if (grafptr != NULL) {
		if (whatDepTiers == 0) {
			if (grafptr->word != NULL) {
				fprintf(stderr,"\n  %%gra tier item specified with %csg|%s option can not be found on Speaker tier.\n",
						(grafptr->type == 'i' ? '+' : '-'), grafptr->word);
			} else {
				fprintf(stderr,"\n  The %%gra word specified with +/-sg option can not be found on Speaker or %%mor tier.\n");
			}
			fprintf(stderr,"  Please either select %%gra tier with +t%%gra option or change the word you are searching for.\n");
			cutt_exit(0);
		} else if (whatDepTiers == 1) {
			if (grafptr->word != NULL) {
				fprintf(stderr,"\n  %%gra tier item specified with %csg|%s option can not be found on either Speaker or %%mor tier.\n",
						(grafptr->type == 'i' ? '+' : '-'), grafptr->word);
			} else {
				fprintf(stderr,"\n  The %%gra word specified with +/-sg option can not be found on either Speaker or %%mor tier.\n");
			}
			fprintf(stderr,"  Please either select %%gra tier with +t%%gra option or change the word you are searching for.\n");
			cutt_exit(0);
		}
	}
	for (twd=wdptr; twd != NULL; twd=twd->nextword) {
		if (whatDepTiers == 3) {
			fprintf(stderr,"\n  The search target word \"%s\" can not be found on either %%mor or %%gra tier.\n", twd->word);
			fprintf(stderr,"  Please either select appropriate tier with +t option or change the word you are searching for.\n");
			fprintf(stderr,"  To search on %%gra: tier please use this notation +s@g|, +sg|SUBJ to find \"SUBJ\" elements.\n");
			fprintf(stderr,"  To search on %%mor: tier please use this notation +sm, +sm|n to find \"noun\" words.\n");
			cutt_exit(0);
		}
	}
}

/* work(pn,st) opens apropriate input/output streams, figures out output
   file name, if any, and calls the program local "call()" function. It also
   calls functions to test the data format and to display list of user
   specified tier. It returns 2 if input file can not be opened and 1
   otherwise.
*/
static int work(char *argv[], int argc, FNType *fname) {
	FNType *showFName;
	char isUTF8SymFound;
	UInt32 dateValue = 0L;
	FNType tempfName[FNSize], *f;

	foundUttContinuation = FALSE;
	cMediaFileName[0] = EOS;
	oldfname = fname;
	if (utterance != NULL) {
		UTTER *ttt = utterance;

		do {
			ttt->speaker[0]	= EOS;
			ttt->attSp[0]	= EOS;
			ttt->line[0]	= EOS;
			ttt->attLine[0]	= EOS;
			ttt = ttt->nextutt;
		} while (ttt != utterance) ;
	}
	getc_cr_lc = '\0';
	fgets_cr_lc = '\0';
	contSpeaker[0] = EOS;

	cutt_isMultiFound = FALSE;
	cutt_isCAFound = FALSE;
	cutt_isBlobFound = FALSE;
	init('\0');
	if (linkDep2Other) {
		checkSearchTiers();
	}
	if (TSoldsp != NULL)
		*TSoldsp = EOS;
	if (!combinput)
		WUCounter = 0L;
	else if (CLAN_PROG_NUM == QUOTES || CLAN_PROG_NUM == VOCD || CLAN_PROG_NUM == EVAL || CLAN_PROG_NUM == EVALD ||
			 CLAN_PROG_NUM == MORTABLE || CLAN_PROG_NUM == CODES_P ||
			 CLAN_PROG_NUM == COMBO || CLAN_PROG_NUM == SCRIPT_P || CLAN_PROG_NUM == KIDEVAL ||
			 CLAN_PROG_NUM == TIMEDUR || CLAN_PROG_NUM == CORELEX || CLAN_PROG_NUM == FLUCALC || CLAN_PROG_NUM == C_NNLA ||
			 CLAN_PROG_NUM == C_QPA || CLAN_PROG_NUM == SUGAR ||
			 (CLAN_PROG_NUM == MLT && onlydata == 1) || (CLAN_PROG_NUM == MLU && onlydata == 1))
		WUCounter = 0L;
	if (lineno > -1L) {
		tlineno = 1L;
		lineno = deflineno;
	}
	if (!stin) {
		if (access(fname,0)) {
			fprintf(stderr, "\nCan't locate file \"%s\" in a specified directory.\n", fname);
			fprintf(stderr,"Check spelling of file name.\n");
			if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
				CleanUpTempIDSpeakers();
			}
			return(2);
		}
		if ((fpin=fopen(fname, "r")) == NULL) {
			fprintf(stderr,"\nCan't open file %s.\n",fname);
			if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
				CleanUpTempIDSpeakers();
			}
			return(2);
		}
	} else {
		fpin = stdin;
	}

	if (stout) {
		fpout = stdout;

		if (FirstTime) {
			printArg(argv, argc, stderr, FALSE, fname);
			if (!IsOutTTY() && (!onlydata || puredata < 2) && !outputOnlyData) {
				printArg(argv, argc, fpout, FALSE, fname);
			}
		}
		if (!stin) {
			if (chatmode != 0 && chatmode != 4) {
				if (!chattest(fname, FALSE, &isUTF8SymFound)) {
					if (fpin != NULL && fpin != stdin) {
						fclose(fpin);
						fpin = NULL;
					}
					FirstTime = FALSE;
					if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
						CleanUpTempIDSpeakers();
					}
					return(0);
				}
				rewind(fpin);
				if (isUTF8SymFound == 1) {
					getc(fpin); getc(fpin); getc(fpin);
				} else if (isUTF8SymFound == 2) {
					fprintf(stderr, "\n\nCLAN can't read UTF-16 encoded files: %s.\n\n", fname);
					if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
						CleanUpTempIDSpeakers();
					}
					return(2);
				} else {
				}
			} else if (!stin && CLAN_PROG_NUM != CP2UTF) {
				int c;
				isUTF8SymFound = FALSE;
				c = getc(fpin);
				if (c == (int)0xef) {
					c = getc(fpin);
					if (c == (int)0xbb) {
						c = getc(fpin);
						if (c == (int)0xbf) {
							isUTF8SymFound = TRUE;
						}
					}
				} else if (c == (int)0xff) {
					c = getc(fpin);
					if (c == (int)0xfe) {
						fprintf(stderr, "\n\nCLAN can't read UTF-16 encoded files: %s.\n\n", fname);
						if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
							CleanUpTempIDSpeakers();
						}
						return(2);
					}
				} else if (c == (int)0xfe) {
					c = getc(fpin);
					if (c == (int)0xff) {
						fprintf(stderr, "\n\nCLAN can't read UTF-16 encoded files: %s.\n\n", fname);
						if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
							CleanUpTempIDSpeakers();
						}
						return(2);
					}
				}
				if (!isUTF8SymFound)
					rewind(fpin);
			}
		}
		if (FirstTime)
			CurTierSearch(argv[0]);

		if (stin) {
			if (FirstTime) {
				if (CLAN_PROG_NUM != CHECK_P) { // 2017-04-11
					fprintf(stderr,"From pipe input\n");
					if (!IsOutTTY() && (!onlydata || !puredata) && !outputOnlyData)
						fprintf(stdout,"From pipe input\n");
				}
			}
			do {
				call();
				if (ByTurn < 2)
					break;
				ByTurn = 3;
				init('\0');
				if (CLAN_PROG_NUM != CHECK_P) // 2017-04-11
					fprintf(fpout, "############### TURN BREAK ###############\n");
			} while (1) ;
		} else {
			fprintf(stderr,"From file <%s>\n",fname);
			if (!IsOutTTY() && (!onlydata || !puredata) && !outputOnlyData) {
				fprintf(stdout,"From file <%s>\n",fname);
			}
			do {
				call();
				if (ByTurn < 2)
					break;
				ByTurn = 3;
				init('\0');
				if (CLAN_PROG_NUM != CHECK_P) // 2017-04-11
					fprintf(fpout, "############### TURN BREAK ###############\n");
			} while (1) ;
			if (fpin != NULL && fpin != stdin) {
				fclose(fpin);
				fpin = NULL;
			}
		}
		FirstTime = FALSE;
	} else { /* else !stout */
		if (FirstTime || !combinput)
			parsfname(fname,newfname,GExt);
		if ((fpout=openwfile(fname,newfname,fpout)) == NULL) {
			fprintf(stderr,"\nCan't create file \"%s\", perhaps it is opened by another application\n",newfname);
			if (fpin != NULL && fpin != stdin) {
				fclose(fpin);
				fpin = NULL;
			}
			if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
				CleanUpTempIDSpeakers();
			}
			return(2);
		} else {
//			if (FirstTime || !combinput)
//				getpunct(punctFile);
			if (!combinput && replaceFile) {
				if (WD_Not_Eq_OD) {
					f = strrchr(fname, PATHDELIMCHR);
					if (f == NULL) {
						strcpy(tempfName, fname);
					} else {
						f++;
						strcpy(tempfName, od_dir);
						addFilename2Path(tempfName, f);
					}
					showFName = tempfName;
				} else
					showFName = fname;
			} else {
				showFName = newfname;
			}
			if (FirstTime) {
				printArg(argv, argc, stderr, FALSE, fname);
			}
			if (FirstTime || !combinput) {
				if ((!onlydata || puredata < 2) && !outputOnlyData) {
					printArg(argv, argc, fpout, TRUE, fname);
				}
				if (!stin) {
					if (chatmode != 0 && chatmode != 4) {
						if (!chattest(fname, FALSE, &isUTF8SymFound)) {
							if (fpin != NULL && fpin != stdin) {
								fclose(fpin);
								fpin = NULL;
							}
							if (!combinput) {
								fclose(fpout);
								if (unlink(newfname))
									fprintf(stderr, "\nCan't delete output file \"%s\".", newfname);
								fpout = NULL;
							}
							if (delSkipedFile != NULL && !replaceFile && fpout != stdout && !combinput) {
								fprintf(stderr, "%s    \"%s\"", delSkipedFile, oldfname);
								if (unlink(newfname))
									fprintf(stderr, "Can't delete output file \"%s\".", newfname);
							}
							isFileSkipped = TRUE;
							FirstTime = FALSE;
							if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
								CleanUpTempIDSpeakers();
							}
							return(0);
						}
						rewind(fpin);
						if (isUTF8SymFound == 1) {
							getc(fpin); getc(fpin); getc(fpin);
						} else if (isUTF8SymFound == 2) {
							fprintf(stderr, "\n\nCLAN can't read UTF-16 encoded files: %s.\n\n", fname);
							if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
								CleanUpTempIDSpeakers();
							}
							return(2);
						} else {
						}
					} else if (CLAN_PROG_NUM != CP2UTF) {
						int c;

						isUTF8SymFound = FALSE;
						c = getc(fpin);
						if (c == (int)0xef) {
							c = getc(fpin);
							if (c == (int)0xbb) {
								c = getc(fpin);
								if (c == (int)0xbf) {
									isUTF8SymFound = TRUE;
								}
							}
						} else if (c == (int)0xff) {
							c = getc(fpin);
							if (c == (int)0xfe) {
								fprintf(stderr, "\n\nCLAN can't read UTF-16 encoded files: %s.\n\n", fname);
								if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
									CleanUpTempIDSpeakers();
								}
								return(2);
							}
						} else if (c == (int)0xfe) {
							c = getc(fpin);
							if (c == (int)0xff) {
								fprintf(stderr, "\n\nCLAN can't read UTF-16 encoded files: %s.\n\n", fname);
								if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
									CleanUpTempIDSpeakers();
								}
								return(2);
							}
						}
						if (!isUTF8SymFound)
							rewind(fpin);
					}
				}
				CurTierSearch(argv[0]);
			} else {
				if (!stin) {
					if (chatmode != 0 && chatmode != 4) {
						if (!chattest(fname, FALSE, &isUTF8SymFound)) {
							if (fpin != NULL && fpin != stdin) {
								fclose(fpin);
								fpin = NULL;
							}
							if (!combinput)
								fclose(fpout);
							FirstTime = FALSE;
							if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
								CleanUpTempIDSpeakers();
							}
							return(0);
						}
						rewind(fpin);
						if (isUTF8SymFound == 1) {
							getc(fpin); getc(fpin); getc(fpin);
						} else if (isUTF8SymFound == 2) {
							fprintf(stderr, "\n\nCLAN can't read UTF-16 encoded files: %s.\n\n", fname);
							if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
								CleanUpTempIDSpeakers();
							}
							return(2);
						}
					}
				}
			}

			if (stin) {
				if (CLAN_PROG_NUM != CHECK_P) { // 2017-04-11
					if ((!onlydata || !puredata) && !outputOnlyData) {
						fprintf(fpout,"From file <%s>\n",fname);
					}
					fprintf(stderr,"From file <%s>\n",fname);
				}
				do {
					call();
					if (ByTurn < 2)
						break;
					ByTurn = 3;
					init('\0');
					if (CLAN_PROG_NUM != CHECK_P) // 2017-04-11
						fprintf(fpout, "############### TURN BREAK ###############\n");
				} while (1) ;
				if (CLAN_PROG_NUM != CHECK_P) { // 2017-04-11
					if (!combinput)
						fprintf(stderr,"Output file <%s>\n",showFName);
				}
			} else {
				if ((!onlydata || !puredata) && !outputOnlyData) {
					fprintf(fpout,"From file <%s>\n",fname);
				}
				fprintf(stderr,"From file <%s>\n",fname);
				do {
					call();
					if (ByTurn < 2)
						break;
					ByTurn = 3;
					init('\0');
					if (CLAN_PROG_NUM != CHECK_P) // 2017-04-11
						fprintf(fpout, "############### TURN BREAK ###############\n");
				} while (1) ;
				if (!combinput) { // 2017-04-11
					fprintf(stderr,"Output file <%s>\n",showFName);
				}
				if (fpin != NULL && fpin != stdin) {
					fclose(fpin);
					fpin = NULL;
				}
			}
			FirstTime = FALSE;
			if (!combinput) {
				if (fpout)
					fclose(fpout);
				fpout = NULL;
				if (((option_flags[CLAN_PROG_NUM] & FR_OPTION) || CLAN_PROG_NUM == MOR_P) && replaceFile) {
					if (!WD_Not_Eq_OD) {
						if (unlink(fname)) {
							fprintf(stderr, "\nCan't delete original file \"%s\". Perhaps it is opened by some application.", fname);
							if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
								CleanUpTempIDSpeakers();
							}
							return(2);
						}
						rename(newfname, fname);
					} else { //	if (!WD_Not_Eq_OD)
						if (!access(tempfName, 0)) {
							if (unlink(tempfName)) {
								fprintf(stderr, "\nCan't delete original file \"%s\". Perhaps it is opened by some application.", tempfName);
								if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
									CleanUpTempIDSpeakers();
								}
								return(2);
							}
						}
						rename(tempfName, newfname);
					}
				}
			}
		}
	}
	if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
		CleanUpTempIDSpeakers();
	}
	return(0);
}

int Get_Dir(FNType *dirname, int index) {
	struct dirent *dp;
	struct stat sb;

	if (index == 1) {
		if ((cDIR=opendir(".")) == NULL) {
			return(0);
		}
	}
	while ((dp=readdir(cDIR)) != NULL) {
		if (stat(dp->d_name, &sb) == 0) {
			if (!S_ISDIR(sb.st_mode)) {
				continue;
			}
		} else
			continue;
		strcpy(dirname, dp->d_name);
		if (!strcmp(dirname, ".") || !strcmp(dirname, ".."))
			continue;
		return(2);
	}
	closedir(cDIR);
	return(0);
}

int Get_File(FNType *filename, int index) {
	struct dirent *dp;
	struct stat sb;

	if (index == 1) {
		if ((cDIR=opendir(".")) == NULL) {
			return(0);
		}
	}
	while ((dp=readdir(cDIR)) != NULL) {
		if (stat(dp->d_name, &sb) == 0) {
			if (S_ISDIR(sb.st_mode)) {
				continue;
			}
		}
		if (dp->d_name[0] == '.')
			continue;
		strcpy(filename, dp->d_name);
		return(2);
	}
	closedir(cDIR);
	return(0);
}

/* find files recursively or otherwise */
struct cutt_FileList {
	FNType fname[FNSize];
	struct cutt_FileList *next_file;
} ;
typedef struct cutt_FileList cutt_FileList;

struct cutt_opts {
	FNType *path;
	FNType *targ;
	char IsPathDotDot;
	char **argv;
	int  argc;
	cutt_FileList *root_file;
} ;
typedef struct cutt_opts cutt_opts;

static cutt_FileList *free_FileList(cutt_FileList *p) {
	cutt_FileList *t;
///
	while (p != NULL) {
		t = p;
		p = p->next_file;
		free(t);
	}
	return(NULL);
}

static char cutt_addToFileList(cutt_opts *args, FNType *fname) {
	cutt_FileList *tF, *t, *tt;
///
	tF = NEW(cutt_FileList);
	if (tF == NULL) {
		fprintf(stderr, "\nOut of memory\n");
		return(FALSE);
	}
	if (args->root_file == NULL) {
		args->root_file = tF;
		tF->next_file = NULL;
	} else if (strcmp(args->root_file->fname, fname) > 0) {
		tF->next_file = args->root_file;
		args->root_file = tF;
	} else {
		t = args->root_file;
		tt = args->root_file->next_file;
		while (tt != NULL) {
			if (strcmp(tt->fname, fname) > 0)
				break;
			t = tt;
			tt = tt->next_file;
		}
		if (tt == NULL) {
			t->next_file = tF;
			tF->next_file = NULL;
		} else {
			tF->next_file = tt;
			t->next_file = tF;
		}
	}
	strcpy(tF->fname, fname);
	return(TRUE);
}

///
struct cutt_FolderList {
	FNType *fname;
	struct cutt_FolderList *next_folder;
} ;
typedef struct cutt_FolderList cutt_FolderList;

static cutt_FolderList *free_FolderList(cutt_FolderList *p) {
	cutt_FolderList *t;
///
	while (p != NULL) {
		t = p;
		p = p->next_folder;
		if (t->fname != NULL)
			free(t->fname);
		free(t);
	}
	return(NULL);
}

static cutt_FolderList *addToFolderList(cutt_FolderList *root_folder, FNType *fname) {
	cutt_FolderList *tF, *t, *tt;
///
	tF = NEW(cutt_FolderList);
	if (tF == NULL) {
		fprintf(stderr, "\nOut of memory\n");
		root_folder = free_FolderList(root_folder);
		return(NULL);
	}
	if (root_folder == NULL) {
		root_folder = tF;
		tF->next_folder = NULL;
	} else if (uS.mStricmp(root_folder->fname, fname) > 0) {
		tF->next_folder = root_folder;
		root_folder = tF;
	} else {
		t = root_folder;
		tt = root_folder->next_folder;
		while (tt != NULL) {
			if (uS.mStricmp(tt->fname, fname) > 0)
				break;
			t = tt;
			tt = tt->next_folder;
		}
		if (tt == NULL) {
			t->next_folder = tF;
			tF->next_folder = NULL;
		} else {
			tF->next_folder = tt;
			t->next_folder = tF;
		}
	}
	tF->fname = (char *)malloc(strlen(fname)+1);
	if (tF->fname == NULL) {
		fprintf(stderr, "\nOut of memory\n");
		root_folder = free_FolderList(root_folder);
		return(NULL);
	}
	strcpy(tF->fname, fname);
	return(root_folder);
}

static int PathEqCwd(char *st1, char *st2) {
	for (; *st1 == *st2 && *st2 != EOS; st1++, st2++) ;
	if ((*st1 == '/' && *(st1+1) == EOS && *st2 == EOS) ||
		(*st2 == '/' && *(st2+1) == EOS && *st1 == EOS) ||
		*st1 == EOS)
		return(1);
	else
		return(0);
}

static char cutt_dir(char *argvS, cutt_opts *args, char *isFF, int *worked) {
	int i, dPos, offset;
	FNType fname[FILENAME_MAX];
	cutt_FileList *tFile;
	FNType tfname[FNSize];
	cutt_FolderList *root_folder, *tFolder;
	struct dirent *dp;
	struct stat sb;
	DIR *cDIR;

	dPos = strlen(args->path);
 	if (SetNewVol(args->path)) {
		fprintf(stderr,"\nCan't change to folder: %s.\n", args->path);
		if (strchr(argvS, PATHDELIMCHR) != NULL)
			fprintf(stderr,"    From original option: %s.\n", argvS);
		return(FALSE);
	}
	args->root_file = free_FileList(args->root_file);
	i = 1;
	while ((i=Get_File(fname, i)) != 0) {
		if (uS.fIpatmat(fname, args->targ)) {
			addFilename2Path(args->path, fname);
			if (isRecursive || WD_Not_Eq_OD || args->IsPathDotDot) {
				offset = 0;
			} else if (args->path[dPos] == PATHDELIMCHR) {
				offset = dPos + 1;
			} else {
				offset = dPos;
			}
			if (!cutt_addToFileList(args, args->path+offset)) {
				return(FALSE);
			}
			args->path[dPos] = EOS;
		}
	}
	for (tFile=args->root_file; tFile != NULL; tFile=tFile->next_file) {
		if (WD_Not_Eq_OD)
			SetNewVol(od_dir);
		if (PathEqCwd(args->path, wd_dir) || tFile->fname[0] == '/' || uS.mStrnicmp(tFile->fname, "C:/", 3) == 0) {
			strcpy(tfname, tFile->fname);
		} else {
			strcpy(tfname, args->path);
			addFilename2Path(tfname, tFile->fname);
		}
		*worked = work(args->argv,args->argc,tfname);
		if (WD_Not_Eq_OD)
			SetNewVol(args->path);
		*isFF = TRUE;
		if (*worked == 2)
			break;
	}

	if (!isRecursive)
		return(TRUE);

 	SetNewVol(args->path);
	root_folder = NULL;
	if ((cDIR=opendir(".")) != NULL) {
		while ((dp=readdir(cDIR)) != NULL) {
			if (stat(dp->d_name, &sb) == 0) {
				if (!S_ISDIR(sb.st_mode)) {
					continue;
				}
			} else
				continue;
			if (dp->d_name[0] == '.')
				continue;

			root_folder = addToFolderList(root_folder, dp->d_name);
			if (root_folder == NULL) {
				return(FALSE);
			}
		}
		closedir(cDIR);
	}
	for (tFolder=root_folder; tFolder != NULL; tFolder=tFolder->next_folder) {
		addFilename2Path(args->path, tFolder->fname);
		uS.str2FNType(args->path, strlen(args->path), PATHDELIMSTR);
		if (!cutt_dir(argvS, args, isFF, worked)) {
			args->path[dPos] = EOS;
			root_folder = free_FolderList(root_folder);
			return(FALSE);
		}
		args->path[dPos] = EOS;
		SetNewVol(args->path);
	}
	root_folder = free_FolderList(root_folder);
	return(TRUE);
}

static char isDotDotInPath(char *path) {
	int i;

	for (i=0; path[i] != EOS; i++) {
		if (path[i] == '/' && path[i+1] == '.' && path[i+2] == '.' && (path[i+3] == '/' || path[i+3] == EOS))
			return(TRUE);
	}
	return(FALSE);
}

/* bmain(argc,argv,pr_result) calls getflag to pars given options and call
   work to perform work on a given input data. It returns 0 if no work was
   performed on any input data files, 2 if last input file couldn't have been
   opened, 1 otherwise. The options setup could be: -sword, or -s word.
   "argc" and "argv" are the same as in main(). "pr_result" says which
   function is used to print result. It is set to NULL if combined result
   need NOT be printed.
*/
char bmain(int argc, char *argv[], void (*pr_result)(void)) {
	int i;
	int worked = 1;
	char isCRPrinted, *ts, res;
	struct tier *tt;
	FNType path[FNSize];
	int  j;
	char st[FNSize];
	FNType targ[FNSize];
	char isFileFound, isApplyRelativePath;
	cutt_opts args;
	FILE *fp;

#if defined(UNX)
  #ifdef USE_TERMIO
	struct termio otty;
  #else
	struct sgttyb  otty;
  #endif
#endif
	res = TRUE;
	getcwd(wd_dir, FNSize);
	strcpy(od_dir, wd_dir);
	strcpy(lib_dir, DEPDIR);
	strcpy(mor_lib_dir, DEPDIR);
	targs = (char *) malloc(argc);
	if (targs == NULL)
		out_of_mem();
	mmaininit();
	if (argc > 1) {
		i = 1;
//		for (i=1; i < argc; i++) {
			if (argv[i][0] == '-' || argv[i][0] == '+') {
				if (argv[i][1] == 'v' && argv[i][2] == 'e' && argv[i][3] == 'r') {
					VersionNumber(FALSE, stdout);
					fprintf(stdout, "\n");
					res = FALSE;
					cutt_exit(0);
				}
			}
//		}
	}
	InitOptions();
// 07/28/2023 Removes Janky stdin pipe test, as it is not needed and interfers with Conda operations - hjl
/*#if defined(UNX)*/
  /*#ifdef USE_TERMIO*/
	/*if (ioctl(fileno(stdin), TCGETA, &otty) == -1) {*/
		/*if (errno == ENOTTY) {*/
/*//fprintf(stderr, "stin = TRUE;\n");*/
			/*stin = TRUE;*/
		/*}*/
	/*}*/
  /*#elif defined(APPLEUNX)*/
	/*if (ioctl(fileno(stdin), TIOCGETP, &otty) != 0) {*/
/*//fprintf(stderr, "stin = TRUE;\n");*/
		/*stin = TRUE;*/
	/*}*/
  /*#else*/
	/*if (gtty(fileno(stdin), &otty) != 0) {*/
/*//fprintf(stderr, "stin = TRUE;\n");*/
		/*stin = TRUE;*/
	/*}*/
  /*#endif*/
/*#endif*/
//fprintf(stderr, "stin = %d;\n", stin);
	if (argc < 2 && !stin) {
		usage();
	}
/* this MUST follow the mmaininit function call but preceed user args check */
	init('\001');

	strcpy(GExt,options_ext[CLAN_PROG_NUM]);
	for (i=1; i < argc; i++) {
		targs[i] = 1;
		if (*argv[i] == '+'  || *argv[i] == '-') {
			if (CLAN_PROG_NUM == FREQ) {
				if (argv[i][1] == 'c') {
					if (argv[i][2] == '3')
						anyMultiOrder = TRUE;
					else if (argv[i][2] == '4')
						onlySpecWsFound = TRUE;
				}
			}
			if (argv[i][1] == 'k') {
				if (!nomap)
					nomap = TRUE;
				else
					nomap = FALSE;
			} else if (argv[i][1] == 's') {
				if (argv[i][2] == 'm') {
					if (argv[i][3] == EOS || (argv[i][3] == ':' && argv[i][4] == EOS)) {
						morSearchSytaxUsage(argv[i]);
					}
				} else if (argv[i][2] == 'g') {
					if (argv[i][3] == EOS || (argv[i][3] == ':' && argv[i][4] == EOS)) {
						graSearchSytaxUsage(argv[i]);
					}
				}
			}

			if (CLAN_PROG_NUM == FREQ || CLAN_PROG_NUM == KWAL) {
				if (argv[i][1] == 'd') {
					if (argv[i][2] == '7' && argv[i][3] == EOS) {
						linkDep2Other = TRUE;
					}
				}
/*
				else if (argv[i][1] == 't') {
					if (uS.mStrnicmp(argv[i]+2, "%gra", 4) == 0 || uS.mStrnicmp(argv[i]+2, "%grt", 4) == 0) {
					} else if (uS.mStrnicmp(argv[i]+2, "%mor", 4) == 0 || uS.mStrnicmp(argv[i]+2, "%trn", 4) == 0) {
					}
				}
*/
			}

		}
	}
	for (; i < argc; i++)
		targs[i] = 1;
	for (i=1; i < argc; i++) {
		if (*argv[i] == '-' || *argv[i] == '+') {
			if (i+1 < argc) {
				getflag(argv[i],argv[i+1],&i);
			} else {
				getflag(argv[i],NULL,&i);
			}
		} else if (*argv[i] != EOS)
			targs[i] = 0;
	}
	for (i=1; i < argc; i++) {
		if (targs[i])
			continue;

		isFileFound = FALSE;
		isApplyRelativePath = TRUE;
		strcpy(path, wd_dir);
		uS.strn2FNType(targ, 0L, argv[i], FILENAME_MAX-1);
		targ[FILENAME_MAX-1] = EOS;
#if defined(_DOS)
		if (isApplyRelativePath) {
			for (j=0; targ[j] != EOS; j++) {
				if (targ[j] == '\\')
					targ[j] = '/';
			}
			for (j = 0; path[j] != EOS; j++) {
				if (path[j] == '\\')
					path[j] = '/';
			}
		}
#endif
		if (isApplyRelativePath && (ts=strrchr(targ, '/')) != NULL) {
			*ts = EOS;
#if defined(_DOS)
			if (strnicmp(targ, "C:/", 3) == 0)
#else
			if (targ[0] == '/')
#endif
			{
				DirPathName[0] = EOS;
				path[0] = EOS;
			} else
				strcpy(DirPathName, wd_dir);
#if defined(_DOS)
			if (isApplyRelativePath) {
				for (j = 0; DirPathName[j] != EOS; j++) {
					if (DirPathName[j] == '\\')
						DirPathName[j] = '/';
				}
			}
#endif
			addFilename2Path(DirPathName, targ);
			addFilename2Path(DirPathName, "");
			addFilename2Path(path, targ);
			addFilename2Path(path, "");
			strcpy(targ, ts+1);
			SetNewVol(DirPathName);
			args.IsPathDotDot = isDotDotInPath(path);
		} else {
			SetNewVol(wd_dir);
			args.IsPathDotDot = FALSE;
		}
		totalFilesNum = 0; // 2019-04-18 TotalNumFiles
		args.root_file = NULL;
		args.path = path;
		args.argv = argv;
		args.argc = argc;
		args.targ = targ;
		if (!cutt_dir(argv[i], &args, &isFileFound, &worked))
			break;
		args.root_file = free_FileList(args.root_file);
		if (!isFileFound) {
			res = FALSE;
			fprintf(stderr,"\n**** WARNING: No file matching \"%s\" was found.\n\n", argv[i]);
			worked = 2;
		}
	}

	if (worked == 1) {
		res = FALSE;
		fprintf(stderr,"\nNo input file given.\n");
	}
	if (combinput && worked == 0 && !FirstTime) {
		if (!stout) {
			if (pr_result != NULL)
				(*pr_result)();
			fprintf(stderr,"Output file <%s>\n",newfname);
			fclose(fpout);
			fpout = NULL;
		} else
			if (pr_result != NULL)
				(*pr_result)();
	}
	if (worked == 0 && chatmode) {
		if (mor_link.error_found) {
			fprintf(stderr,"\n*** File \"%s\": line %ld.\n", mor_link.fname, mor_link.lineno);
			if (linkDep2Other)
				fprintf(stderr, "WARNING: SELECTED DEPENDENT TIER DOES NOT LINK IN SIZE TO SECOND SELECTED TIER.\n");
			else
				fprintf(stderr, "WARNING: %%MOR: TIER DOES NOT LINK IN SIZE TO ITS SPEAKER TIER.\n");
			fprintf(stderr, "THIS MAY EFFECT RESULTS OF THE ANALYSES.\n\n");
			res = FALSE;
		}
		isCRPrinted = FALSE;
		if (IDField != NULL || CODEField != NULL || SPRole != NULL) {
			if (CLAN_PROG_NUM != POST && CLAN_PROG_NUM != CMDI_P && CLAN_PROG_NUM != RELY) {
				struct IDtype *tID;
				for (tID=IDField; tID != NULL; tID=tID->next_ID) {
					if (tID->used == FALSE) {
						if (!isCRPrinted) {
							isCRPrinted = TRUE;
							fprintf(stderr, "\n\n");
						}
						fprintf(stderr,"    CAN'T FIND ID \"%s\" IN ANY FILE.\n",tID->ID);
						res = FALSE;
					}
				}
				for (tID=CODEField; tID != NULL; tID=tID->next_ID) {
					if (tID->used == FALSE) {
						if (!isCRPrinted) {
							isCRPrinted = TRUE;
							fprintf(stderr, "\n\n");
						}
						fprintf(stderr,"    CAN'T FIND KEYWORD \"%s\" IN ANY FILE.\n",tID->ID);
						res = FALSE;
					}
				}
				for (tID=SPRole; tID != NULL; tID=tID->next_ID) {
					if (tID->used == FALSE) {
						if (!isCRPrinted) {
							isCRPrinted = TRUE;
							fprintf(stderr, "\n\n");
						}
						fprintf(stderr,"    CAN'T FIND SPEAKER WITH ROLE \"%s\" IN ANY FILE.\n",tID->ID);
						res = FALSE;
					}
				}
				if (isCRPrinted)
					fprintf(stderr, "\n");
			}
		}
		for (tt=headtier; tt != NULL; tt=tt->nexttier) {
			if (tt->used == FALSE && tt->include) {
				if (!isCRPrinted) {
					isCRPrinted = TRUE;
					fprintf(stderr, "\n\n");
				}
				if (IDField != NULL && uS.partcmp(tt->tcode,"@ID:",FALSE,FALSE)) {
					res = FALSE;
					fprintf(stderr, "    TIER \"%s\" HASN'T BEEN FOUND IN THE INPUT DATA!\n", tt->tcode);
				} else {
					res = FALSE;
					fprintf(stderr, "    TIER \"%s\", ASSOCIATED WITH A SELECTED SPEAKER,\n         HASN'T BEEN FOUND IN THE INPUT DATA!\n", tt->tcode);
					if ((CLAN_PROG_NUM == MLU || CLAN_PROG_NUM == CHIP || CLAN_PROG_NUM == WDLEN) && uS.mStricmp(tt->tcode, "%mor") == 0) {
						fprintf(stderr, "ADD -t%%mor TO YOUR COMMAND TO ANALYZE JUST THE MAIN LINE\n");
					}
				}
			}
		}
	}
/*
	for (tt=HeadOutTier; tt != NULL; tt=tt->nexttier) {
		if (tt->used == FALSE) {
			fprintf(stderr, "\n\nTIER \"%s\" HASN'T BEEN FOUND IN THE INPUT DATA!\n", tt->tcode);
		}
	}
*/
	main_cleanup();
	return(res);
}

/* output tiers handling
 */
int CheckOutTier(char *s) {
	struct tier *p;

	if (!chatmode)
		return(TRUE);
	for (p=HeadOutTier; p != NULL; p=p->nexttier) {
		if (uS.partcmp(s,p->tcode,p->pat_match,FALSE)) {
/*			if (p->used != '\002') p->used = '\001'; */
			break;
		}
	}
	if (p != NULL) {
		if (!p->include) {
			return(FALSE);
		}
	} else {
		if ((*s == '*' && otcs) || (*s == '@' && otch) || (*s == '%' && otct)) {
			return(FALSE);
		}
	}
	return(TRUE);
}

/* MakeOutTierChoice(ts,inc) creats a list tiers which should be included or
 excluded from KWAL and COMBO output. "ts" is a tier string, inc is either "+" or "-". 
 Procedure will remove apropriate tiers from the default list based on the tiers
 specified by the user.
 */
void MakeOutTierChoice(char *ts, char inc) {
	struct tier *nt, *tnt;
	int l1, l2;

	if (!*ts) {
		fprintf(stderr,"String expected after %co option.\n", '+');
		cutt_exit(0);
	}
	uS.uppercasestr(ts, &dFnt, C_MBF);
	if (HeadOutTier == NULL) {
		HeadOutTier = NEW(struct tier);
		nt = HeadOutTier;
		nt->nexttier = NULL;
	} else {
		l1 = strlen(ts);
		tnt= HeadOutTier;
		nt = HeadOutTier;
		while (1) {
			if (nt == NULL) break;
			else if (uS.partcmp(ts,nt->tcode,FALSE,TRUE)) {
				l2 = strlen(nt->tcode);
				if (l1 > l2) break;
				else return;
			}
			tnt = nt;
			nt = nt->nexttier;
		}
		if (nt == NULL) {
			tnt->nexttier = NEW(struct tier);
			nt = tnt->nexttier;
			nt->nexttier = NULL;
		} else if (nt == HeadOutTier) {
			HeadOutTier = NEW(struct tier);
			HeadOutTier->nexttier = nt;
			nt = HeadOutTier;
		} else {
			nt = NEW(struct tier);
			nt->nexttier = tnt->nexttier;
			tnt->nexttier = nt;
		}
	}
/*
	if (*ts == '*' && *(ts+1) == ':')
		nt->used = '\001';
	else
		nt->used = FALSE;
	nt->include = TRUE;
*/
	if (*ts != '*' && *ts != '@' && *ts != '%')
		strcpy(nt->tcode,"*");
	else
		nt->tcode[0] = EOS;
	nt->include = (char)((inc == '-') ? FALSE : 1);
	nt->pat_match = FALSE;
	for (l1=0; ts[l1] != EOS; l1++) {
		if (ts[l1] == '#') {
			nt->pat_match = TRUE;
			ts[l1] = '*';
		}
	}
	if (ts[0] == '%')
		otcdt = TRUE;
	strcat(nt->tcode,ts);
	if (!nt->include) {
		if (*ts == '@') otch = FALSE;
		else if (*ts == '%') otct = FALSE;
		else otcs = FALSE;
	}
}

/**************************************************************/
/*	 tiers options, find the right tier and get tier line.  */

/* checktier(s) determines if the "s" tier should be selected or not.
*/
int checktier(char *s) {
	struct tier *p;

	if (!chatmode)
		return(TRUE);
	for (p=defheadtier; p != NULL; p=p->nexttier) {
		if (uS.partcmp(s,p->tcode,p->pat_match,FALSE))
			break;
	}
	if (p == NULL) {
		for (p=headtier; p != NULL; p=p->nexttier) {
			if (uS.partcmp(s,p->tcode,p->pat_match,FALSE)) {
				if (p->used != '\002')
					p->used = '\001';
				break;
			}
		}
	}
	if (p != NULL) {
		if (p->include) {
			if (p->include == 2) {
				register int i, j;
				i = strlen(p->tcode);
				for (j=i; s[j] != ':' && s[j]; j++) ;
				if (j > i)
					strcpy(s+i, s+j);
			}
		} else {
			if (ByTurn) {
				if (*s == '*')
					*TSoldsp = EOS;
			}
			return(FALSE);
		}
	} else {
		if ((*s == '*' && tcs) || (*s == '@' && tch) || (*s == '%' && tct)) {
			if (ByTurn) {
				if (*s == '*')
					*TSoldsp = EOS;
			}
			return(FALSE);
		}
	}
	return(TRUE);
}

/* maketierchoice(ts,inc) creats a list tiers which should be included or
 excluded. "ts" is a tier string, inc is either "+" or "-". Procedure will
 remove apropriate tiers from the default list based on the tiers
 specified by the user.
 */
void maketierchoice(const char *SpId, char inc, char istemp) {
	struct tier *nt, *tnt;
	register int l1;
	register int l2;
	int inc_type = 1;
	char ts[SPEAKERLEN];

	if (!*SpId) {
		fprintf(stderr,"Tier name is expected after %ct option.\n", inc);
		cutt_exit(0);
	}
	if (inc == '-' && !strcmp(SpId,"*")) {
		if (chatmode)
			nomain = TRUE;
		else {
			fprintf(stderr, "\"-t*\" option is not allowed with non-CHAT format\n");
			cutt_exit(0);
		}
		return;
	}
	if (*SpId != '*' && *SpId != '@' && *SpId != '%')
		strcpy(ts, "*");
	else
		*ts = EOS;
	strcat(ts, SpId);
	uS.uppercasestr(ts, &dFnt, C_MBF);
	nt = defheadtier;
	tnt = nt;
	while (nt != NULL) {
		if (*ts == *nt->tcode) {
			if (nt == tnt)
				defheadtier = nt->nexttier;
			else
				tnt->nexttier = nt->nexttier;
			free(nt);
			if (*ts == '@') tch = FALSE;
			else if (*ts == '%') tct = FALSE;
			else if (*ts == '*') tcs = FALSE;
			break;
		} else if (*ts != '@' && *ts != '%' && *nt->tcode == '*') {
			if (nt == tnt)
				defheadtier = nt->nexttier;
			else
				tnt->nexttier = nt->nexttier;
			free(nt);
			tcs = FALSE;
			break;
		}
		tnt = nt;
		nt = nt->nexttier;
	}
	l1 = strlen(ts);
	if (ts[l1-1] == '%' && l1 > 1) {
		ts[--l1] = EOS;
		inc_type = 2;
	}
	if (headtier == NULL) {
		headtier = NEW(struct tier);
		nt = headtier;
		nt->nexttier = NULL;
	} else {
		tnt= headtier;
		nt = headtier;
		while (1) {
			if (nt == NULL)
				break;
			if (uS.partcmp(ts,nt->tcode,FALSE,FALSE)) {
				l2 = strlen(nt->tcode);
				if (l1 > l2) {
					if (nt->include != FALSE && inc == '-')
						nt->used = 1;
					break;
				} else {
					if (l1 == l2-1 && nt->tcode[l2-1] == ':' && l2 > 0)
						nt->tcode[l2-1] = EOS;
					if (istemp != '\002' || istemp == nt->used)
						return;
				}
			}
			tnt = nt;
			nt = nt->nexttier;
		}
		if (nt == NULL) {
			tnt->nexttier = NEW(struct tier);
			nt = tnt->nexttier;
			nt->nexttier = NULL;
		} else if (nt == headtier) {
			headtier = NEW(struct tier);
			headtier->nexttier = nt;
			nt = headtier;
		} else {
			nt = NEW(struct tier);
			nt->nexttier = tnt->nexttier;
			tnt->nexttier = nt;
		}
	}
	nt->used = istemp;
	nt->include = (char)((inc == '-') ? FALSE : inc_type);
	nt->pat_match = FALSE;
	for (l1=1; ts[l1] != EOS; l1++) {
		if (ts[l1] == '#' || ts[l1] == '*') {
			nt->pat_match = TRUE;
			ts[l1] = '*';
		}
	}
	strcpy(nt->tcode,ts);
	if (nt->include) {
		if (*ts == '@')
			tch = TRUE;
		else if (*ts == '%')
			tct = TRUE;
		else
			tcs = TRUE;
	}
}

static char SpecialDelim(char *ch, int index) {
	if (ch[index] == ';' && !uS.isskip(ch, index, &dFnt, MBF)) {
		return(TRUE);
	} else if (CLAN_PROG_NUM != MOR_P && ch[index] == ',' && !uS.isskip(ch, index, &dFnt, MBF)) {
		return(TRUE);
	}
	return(FALSE);
}

static void removeAngleBracketsFromLastElem(int lastElem, char *st) {
	int i, agb;

	agb = 0;
	for (i=strlen(st)-1; i >= lastElem && st[i] != ']'; i--) {
		if (st[i] == '<') {
			agb--;
			st[i] = ' ';
		} else if (st[i] == '>') {
			agb++;
			st[i] = ' ';
		}
	}	
	for (i=lastElem; st[i] != EOS && agb > 0; i--) {
		if (st[i] == '<') {
			agb--;
			st[i] = ' ';
		} else if (st[i] == '>') {
			break;
		}
	}	
}

static void removeExtraSpaceFromExpandX(char *st, AttTYPE *att) {
	int i;

	for (i=0; st[i] != EOS; ) {
		if (st[i]=='\n')
			st[i] = ' ';
		if (st[i]==' ' || st[i]=='\t' || (st[i]=='<' && (i==0 || st[i-1]==' ' || st[i-1]=='\t'))) {
			i++;
			while (st[i] == ' ' || st[i] == '\t' || st[i] == '\n')
				att_cp(0,st+i,st+i+1,att+i,att+i+1);
		} else
			i++;
	}
}

int expandX(char *ch, AttTYPE *att, int x, int index) {
	int i, b, e, sqb, agb, n, cnt, lastElem, isRemoveAgb;

	isRemoveAgb = TRUE;
	for (i=index; uS.isskip(ch,i,&dFnt,MBF) && ch[i] != '['  && ch[i] != '>' && ch[i] != EOS; i++) ;
	if (ch[i] == '[')
		isRemoveAgb = FALSE;
	for (e=x-2; e >= 0 && (isSpace(ch[e]) || ch[e] == '\n'); e--) ;
	if (e < 0)
		return(index);
	sqb = 0;
	for (b=e; b >= 0; b--) {
		if (ch[b] == '[')
			sqb--;
		else if (ch[b] == ']')
			sqb++;
		else if (sqb == 0 && (ch[b] == '>' || !uS.isskip(ch,b,&dFnt,MBF)))
			break;
	}
	if (b < 0)
		return(index);
	if (ch[b] == '>') {
		agb = 0;
		sqb = 0;
		for (; b >= 0; b--) {
			if (ch[b] == '[')
				sqb--;
			else if (ch[b] == ']')
				sqb++;
			else if (sqb == 0) {
				if (ch[b] == '<') {
					agb--;
					if (agb <= 0)
						break;
				} else if (ch[b] == '>')
					agb++;
			}
		}
		if (b < 0)
			return(index);
	} else {
		for (; b >= 0; b--) {
			if (uS.isskip(ch,b,&dFnt,MBF) || ch[b] == '<' || ch[b] == '>' || ch[b] == '[' || ch[b] == ']') {
				break;
			}
		}
		b++;
	}
	for (n=x+1; isalpha(ch[n]) || isSpace(ch[n]); n++) ;
	cnt = atoi(ch+n);
	if (cnt < 2)
		return(index);
	att_cp(0,ch+x-1,ch+index,att+x-1,att+index);
	index = x - 1;
	i = 0;
	lastElem = i;
	while (cnt > 1) {
		cnt--;
		tempTier[i] = '[';
		tempAtt[i] = 0;
		i++;
		tempTier[i] = '/';
		tempAtt[i] = 0;
		i++;
		tempTier[i] = ']';
		tempAtt[i] = 0;
		i++;
		tempTier[i] = ' ';
		tempAtt[i] = 0;
		i++;
		lastElem = i;
		for (n=b; n <= e; n++) {
			tempTier[i] = ch[n];
			tempAtt[i] = att[n];
			i++;
		}
		tempTier[i] = ' ';
		tempAtt[i] = 0;
		i++;
	}
	tempTier[i] = EOS;
	if (isRemoveAgb)
		removeAngleBracketsFromLastElem(lastElem, tempTier);
	removeExtraSpaceFromExpandX(tempTier, tempAtt);
	n = strlen(tempTier);
	att_shiftright(ch+index, att+index, n);
	for (i=0; tempTier[i] != EOS; i++) {
		ch[index] = tempTier[i];
		att[index] = tempAtt[i];
		index++;
	}
	return(index);
}

static void rearangePOSLang(char *ch, int tIndex, int index) {
	int  i, j;
	char lang[8], POS[1024];

	i = 0;
	lang[i++] = ch[index];
	for (j=index+1; ch[j] != EOS && isalpha(ch[j]) && i < 8; j++, i++)
		lang[i] = ch[j];
	lang[i] = EOS;
	if (i == 0)
		i = NUMLANGUAGESINTABLE;
	else {
		for (i=0; LanguagesTable[i] != EOS && i < NUMLANGUAGESINTABLE; i++) {
			if (!uS.mStricmp(lang+1, LanguagesTable[i]))
				break;
		}
	}
	if (i < NUMLANGUAGESINTABLE) {
		for (i=0,j=tIndex; j < index; j++, i++)
			POS[i] = ch[j];
		POS[i]  = EOS;
		for (i=0, j=tIndex; lang[i] != EOS; i++, j++) {
			ch[j] = lang[i];
		}
		for (i=0; POS[i] != EOS; i++, j++) {
			ch[j] = POS[i];
		}
	}
}

static char DoubleQuote(char *ch) {
	if (*ch == (char)0xE2 && *(ch+1) == (char)0x80 && *(ch+2) == (char)0x9D)
		return(TRUE);
	return(FALSE);

}

void cutt_cleanUpLine(const char *sp, char *ch, AttTYPE *att, int oIndex) {
	int x, index;
	int matchType;

	if (uS.partcmp(sp,"%mor:",FALSE, FALSE)) {
		index = oIndex;
		while (ch[index] != EOS) {
			if (ch[index] == '.') {
				if (!uS.isPlusMinusWord(ch, index) && !isSpace(ch[index-1]) && 
					(!isdigit(ch[index-1]) || !isdigit(ch[index+1])) && index > 0) {
					att_shiftright(ch+index, att+index, 1);
					ch[index] = ' ';
					index += 2;
				} else
					index++;
				while (ch[index] == '.' || ch[index] == '?' || ch[index] == '!')
					index++;
			} else if (ch[index] == '?' || ch[index] == '!' || SpecialDelim(ch, index)) {
				if (!uS.isPlusMinusWord(ch, index) && !isSpace(ch[index-1]) &&
					(!isdigit(ch[index-1]) || *sp == '*') && index > 0) {
					att_shiftright(ch+index, att+index, 1);
					ch[index] = ' ';
					index += 2;
				} else
					index++;
				while (ch[index] == '.' || ch[index] == '?' || ch[index] == '!')
					index++;
			} else
				index++;
		}
		return;
	}

	index = oIndex;
	while (ch[index] != EOS) {
		if (uS.isRightChar(ch, index, '[', &dFnt, MBF)) {
			x = index + 1;
			for (index++; ch[index] != EOS && !uS.isRightChar(ch, index, ']', &dFnt, MBF); index++) ;
			if (ch[index] != EOS)
				index++;
			if ((ch[x] == 'x' || ch[x] == 'X') && isSpace(ch[x+1]) && isExpendX && CLAN_PROG_NUM < MEGRASP) {
				if (ch[index] != EOS)
					index = expandX(ch, att, x, index);
			}
		} else if (ch[index] == HIDEN_C) {
			for (index++; ch[index] != EOS && ch[index] != HIDEN_C; index++) ;
			if (ch[index] != EOS)
				index++;
		} else if (uS.isRightChar(ch, index, '&', &dFnt, MBF)) {
			for (index++; ch[index] != EOS; index++) {
				if (uS.isskip(ch, index, &dFnt, MBF) || ch[index] == '>' || ch[index] == '[' || ch[index] == ',' || 
					ch[index] == '+' || ch[index] == '.' || ch[index] == '?' || ch[index] == '!')
					break;
			}
		} else if (uS.isRightChar(ch, index, ',', &dFnt, MBF) && uS.isRightChar(ch, index+1, ',', &dFnt, MBF)) {
			if (!uS.isPlusMinusWord(ch, index) && !uS.atUFound(ch, index, &dFnt, MBF) && 
				  !isSpace(ch[index-1]) && (!isdigit(ch[index-1]) || *sp != '@') && index > 0) {
				att_shiftright(ch+index, att+index, 1);
				ch[index] = ' ';
				index += 3;
			} else
				index += 2;
		} else if (ch[index] == '.') {
			if (!uS.isPlusMinusWord(ch, index) && !uS.atUFound(ch, index, &dFnt, MBF) && !uS.isPause(ch,index,NULL,NULL) &&
				!isSpace(ch[index-1]) && (!isdigit(ch[index-1]) || (*sp == '*' && !isdigit(ch[index+1]))) && index > 0) {
				att_shiftright(ch+index, att+index, 1);
				ch[index] = ' ';
				index += 2;
			} else
				index++;
			while (ch[index] == '.' || ch[index] == '?' || ch[index] == '!')
				index++;
		} else if (ch[index] == '?' || ch[index] == '!' || SpecialDelim(ch, index)) {
			if (!uS.isPlusMinusWord(ch, index) && !uS.atUFound(ch, index, &dFnt, MBF) &&
				!isSpace(ch[index-1]) && (!isdigit(ch[index-1]) || *sp == '*') && index > 0) {
				att_shiftright(ch+index, att+index, 1);
				ch[index] = ' ';
				index += 2;
			} else
				index++;
			while (ch[index] == '.' || ch[index] == '?' || ch[index] == '!')
				index++;
		} else
			index++;
	}
}

static char isSpeakerMarker(char ch) {
	if (CLAN_PROG_NUM != CHSTRING) {
		if (ch != '\n' && ch != '\r' && !isSpace(ch))
			return(TRUE);
	} else {
		if (isSpeaker(ch))
			return(TRUE);
	}
	return(FALSE);
}

/* cutt_getline(ch, ccnt) reads in character by character until either end of file
   or new tier has been reached. The characters are read into string pointed
   to by "ch". At the and "currentchar" is set to the first character of a
   new tier or to EOF marker. The character count is kept to make sure that
   the function stays within allocated memory limits.
*/
void cutt_getline(const char *sp, char *ch, AttTYPE *att, register int index) {
	register int  lc;
	register int  s;

	if (!chatmode) {
		if (lineno > -1L)
			lineno = lineno + tlineno;
		tlineno = 0L;
	}
	if (getrestline) {
		s = index;
		lc = index;
		ch[index] = currentchar;
		att[index] = currentatt;
		index++;
		att[index] = att[index-1];
		while (1) {
			if (ch[lc] == '\n')
				tlineno++;
			ch[index] = (char)getc_cr(fpin, &att[index]);
			if (!chatmode) {
				if (feof(fpin))
					break;
				else if (y_option == 1) {
					if (uS.IsUtteranceDel(ch, index)) {
						do {
							index++;
							att[index] = att[index-1];
							ch[index] = (char)getc_cr(fpin, &att[index]);
						} while (uS.IsUtteranceDel(ch, index)) ;
						do {
							if (ch[index] == '\n') tlineno++;
							index++;
							att[index] = att[index-1];
							ch[index] = (char)getc_cr(fpin, &att[index]);
						} while (uS.isskip(ch, index, &dFnt, MBF) && !feof(fpin)) ;
						break;
					}
				} else {
					if (ch[lc] == '\n')
						break;
				}
			} else if (feof(fpin))
				break;
			else if (isSpeakerMarker(ch[index])) {
				if (ch[lc] == '\n')
					break;
			}
			lc = index;

			if (index >= UTTLINELEN) {
				int i;
				ch[index] = EOS;
				fprintf(stderr,"ERROR. Speaker turn is longer than ");
				fprintf(stderr,"%ld characters.\n",UTTLINELEN);
				fprintf(stderr,"On line: %ld.\n", lineno);
				for (i=0; ch[i]; i++) putc(ch[i],stderr);
				putc('\n',stderr);
				cutt_exit(-1);
				index--;
			} else {
				index++;
				att[index] = att[index-1];
			}
		}
		currentchar = ch[index];
		currentatt = att[index];
		ch[index] = EOS;
		if (IsModUttDel && chatmode)
			cutt_cleanUpLine(sp, ch, att, s);
	} else if (skipgetline == FALSE) {
		ch[index] = '\n';
		att[index] = 0;
		index++;
		ch[index] = EOS;
	} else
		skipgetline = FALSE;
	getrestline = 1;
}

/* killline(char *line, char *atts) skip all data in the input stream starting from the current
   character until either end of file or new tier has been reached.  At the
   and "currentchar" is set to the first character of a new tier or to EOF
   marker.
*/
void killline(char *line, AttTYPE *atts) {
	register char lc = 0;

	if (getrestline) {
		if (line == NULL) {
			char *chrs;
			AttTYPE att;

			att = currentatt;
			chrs = templineC3;
			*chrs = currentchar;
			while (1) {
				*++chrs = (char)getc_cr(fpin, &att);
				if (*chrs == '\n')
					tlineno++;
				if (feof(fpin))
					break;
				else if (isSpeakerMarker(*chrs)) {
					if (lc == '\n')
						break;
				}
				lc = *chrs;
			}
			currentchar = *chrs;
			currentatt = att;
			*chrs = EOS;
			if (uS.partwcmp(templineC3, FONTMARKER))
				cutt_SetNewFont(templineC3,']');
		} else {
			int pos;

			line[0] = currentchar;
			atts[0] = currentatt;
			pos = 1;
			atts[pos] = atts[pos-1];
			while (1) {
				line[pos] = (char)getc_cr(fpin, &atts[pos]);
				if (line[pos] == '\n')
					tlineno++;
				if (feof(fpin))
					break;
				else if (isSpeakerMarker(line[pos])) {
					if (lc == '\n')
						break;
				}
				lc = line[pos];
				if (pos >= UTTLINELEN) {
					int i;
					line[pos] = EOS;
					fprintf(stderr,"ERROR. Speaker turn is longer than ");
					fprintf(stderr,"%ld characters.\n",UTTLINELEN);
					fprintf(stderr,"On line: %ld.\n", lineno);
					for (i=0; line[i]; i++)
						putc(line[i],stderr);
					putc('\n',stderr);
					cutt_exit(-1);
					pos--;
				} else {
					pos++;
					atts[pos] = atts[pos-1];
				}
			}
			currentchar = line[pos];
			currentatt = atts[pos];
			line[pos] = EOS;
			if (uS.partwcmp(line, FONTMARKER))
				cutt_SetNewFont(line,']');
		}
	} else
		skipgetline = FALSE;
	getrestline = 1;
}

/**************************************************************/
/*	 Tier print routines for programs with windows   */
/* befprintout(char isChangeBullets) displays specified number of tiers before the current tier.
   -w option is used to specify number of tiers to be displayed before the
   target one.
*/
void befprintout(char isChangeBullets) {
	UTTER *temp;
	int w;

	w = 0;
	if (lutter != utterance) {
		temp = utterance;
		do {
			temp = temp->nextutt;
			w++;
		} while (temp != lutter) ;
	}
	for (temp=lutter->nextutt; w < aftwin; w++)
		temp = temp->nextutt;
	for (; temp != utterance; temp=temp->nextutt) {
		if (chatmode && *temp->speaker) {
			if (nomain)
				remove_main_tier_print(temp->speaker, temp->line, temp->attLine);
			else {
				if (cMediaFileName[0] != EOS)
					changeBullet(temp->line, temp->attLine);
				printout(temp->speaker,temp->line,temp->attSp,temp->attLine,FALSE);
			}
		} else if (!chatmode && *temp->line) {
			if (cMediaFileName[0] != EOS && isChangeBullets)
				changeBullet(temp->line, temp->attLine);
			printout(NULL,temp->line,NULL,temp->attLine,FALSE);
		}
	}
}

/* aftprintout() displays specified number of tiers after the current tier.
  +w option is used to specify number of tiers to be displayed after the
   target one.
*/
void aftprintout(char isChangeBullets) {
	UTTER *temp, *oldlutter;
	int w;

	w = 0;
	if (lutter != utterance) {
		temp = utterance;
		do {
			temp = temp->nextutt;
			w++;
			if (*temp->speaker) {
				if (nomain)
					remove_main_tier_print(temp->speaker, temp->line, temp->attLine);
				else {
					if (cMediaFileName[0] != EOS && isChangeBullets)
						changeBullet(temp->line, temp->attLine);
					printout(temp->speaker,temp->line,temp->attSp,temp->attLine,FALSE);
				}
			}
		} while (temp != lutter) ;
	}
	for (; w < aftwin; w++) {
		oldlutter = lutter;
		lutter = lutter->nextutt;
		if (chatmode) {
			postcodeRes = 0;
			if (!getmaincode()) {
				lutter = oldlutter;
				return;
			}
		} else {
			postcodeRes = 0;
			if (!gettextspeaker()) {
				lutter = oldlutter;
				return;
			}
			IsModUttDel = chatmode < 3;
			cutt_getline("*", lutter->line, lutter->attLine, 0);
			strcpy(lutter->tuttline,lutter->line);
			if (uS.isskip("[", 0, &dFnt, FALSE) || uS.isskip("]", 0, &dFnt, FALSE))
				filterData("*",lutter->tuttline);
			else
				filterwords("*",lutter->tuttline,excludedef);
//			if (WordMode != 'e' && MorWordMode != 'e')
			if (WordMode != 'e')
				filterwords("*",lutter->tuttline,exclude);
		}
		lutter->tlineno = lineno;
		lutter->uttLen = 0L;
		if (nomain)
			remove_main_tier_print(lutter->speaker, lutter->line, lutter->attLine);
		else {
			if (cMediaFileName[0] != EOS && isChangeBullets)
				changeBullet(lutter->line, lutter->attLine);
			if (lutter->speaker[0] == '@') {
				if (checktier(lutter->speaker) || CheckOutTier(lutter->speaker))
					printout(lutter->speaker,lutter->line,lutter->attSp,lutter->attLine,FALSE);
			} else
				printout(lutter->speaker,lutter->line,lutter->attSp,lutter->attLine,FALSE);
		}
	}
}

/**************************************************************/
/*	 printint in chat format								  */
long DealWithAtts_cutt(char *line, long i, AttTYPE att, AttTYPE oldAtt) {
	if (att != oldAtt) {
		if (is_underline(att) != is_underline(oldAtt)) {
			if (is_underline(att))
				sprintf(line+i, "%c%c", ATTMARKER, underline_start);
			else
				sprintf(line+i, "%c%c", ATTMARKER, underline_end);
			i += 2;
		}
		if (is_italic(att) != is_italic(oldAtt)) {
			if (is_italic(att))
				sprintf(line+i, "%c%c", ATTMARKER, italic_start);
			else
				sprintf(line+i, "%c%c", ATTMARKER, italic_end);
			i += 2;
		}
		if (is_bold(att) != is_bold(oldAtt)) {
			if (is_bold(att))
				sprintf(line+i, "%c%c", ATTMARKER, bold_start);
			else
				sprintf(line+i, "%c%c", ATTMARKER, bold_end);
			i += 2;
		}
		if (is_error(att) != is_error(oldAtt)) {
			if (is_error(att))
				sprintf(line+i, "%c%c", ATTMARKER, error_start);
			else
				sprintf(line+i, "%c%c", ATTMARKER, error_end);
			i += 2;
		}
		if (is_word_color(att) != is_word_color(oldAtt)) {
			char color;

			color = get_color_num(att);
			if (color) {
				if (color == blue_color)
					sprintf(line+i, "%c%c", ATTMARKER, blue_start);
				else if (color == red_color)
					sprintf(line+i, "%c%c", ATTMARKER, red_start);
				else if (color == green_color)
					sprintf(line+i, "%c%c", ATTMARKER, green_start);
				else // if (color == magenta_color)
					sprintf(line+i, "%c%c", ATTMARKER, magenta_start);
			} else
				sprintf(line+i, "%c%c", ATTMARKER, color_end);
			i += 2;
		}
	}
	return(i);
}

void printAtts(AttTYPE att, AttTYPE oldAtt, FILE *fp) {
	if (att != oldAtt) {
		if (is_underline(att) != is_underline(oldAtt)) {
			if (is_underline(att))
				fprintf(fp, "%c%c", ATTMARKER, underline_start);
			else
				fprintf(fp, "%c%c", ATTMARKER, underline_end);
		}
		if (is_italic(att) != is_italic(oldAtt)) {
			if (is_italic(att))
				fprintf(fp, "%c%c", ATTMARKER, italic_start);
			else
				fprintf(fp, "%c%c", ATTMARKER, italic_end);
		}
		if (is_bold(att) != is_bold(oldAtt)) {
			if (is_bold(att))
				fprintf(fp, "%c%c", ATTMARKER, bold_start);
			else
				fprintf(fp, "%c%c", ATTMARKER, bold_end);
		}
		if (is_error(att) != is_error(oldAtt)) {
			if (is_error(att))
				fprintf(fp, "%c%c", ATTMARKER, error_start);
			else
				fprintf(fp, "%c%c", ATTMARKER, error_end);
		}

		if (is_word_color(att) != is_word_color(oldAtt)) {
			char color;

			color = get_color_num(att);
			if (color) {
				if (color == blue_color)
					fprintf(fp, "%c%c", ATTMARKER, blue_start);
				else if (color == red_color)
					fprintf(fp, "%c%c", ATTMARKER, red_start);
				else if (color == green_color)
					fprintf(fp, "%c%c", ATTMARKER, green_start);
				else // if (color == magenta_color)
					fprintf(fp, "%c%c", ATTMARKER, magenta_start);
			} else
				fprintf(fp, "%c%c", ATTMARKER, color_end);
		}
	}
}

static void checkPrintFont(const char *st, char isChangeScreenOuput) {
	for (; *st == '\n'; st++) ;
	for (; *st != EOS && *st != '\n'; st++) {
		if (uS.partwcmp(st, FONTMARKER)) {
			cutt_SetNewFont(st,']');
			break;
		}
	}
}

void ActualPrint(const char *st, AttTYPE *att, AttTYPE *oldAtt, char justFirstAtt, char needCR, FILE *fp) {
	char lastCR = FALSE;

	while (*st) {
		if (att != NULL)
			printAtts(*att, *oldAtt, fp);
		else
			printAtts(0, *oldAtt, fp);
		if (*st == '\n') {
			checkPrintFont(st, FALSE);
			lastCR = TRUE;
		} else
			lastCR = FALSE;
		putc(*st,fp);
		st++;
		if (att != NULL) {
			*oldAtt = *att;
			if (!justFirstAtt)
				att++;
		} else
			*oldAtt = 0;
	}
	if (!lastCR && needCR) {
		printAtts(0, *oldAtt, fp);
		putc('\n',fp);
	}
}

/* printout(sp,line) displays tier properly indented according to the CHAT
   format. "sp" points to the code identification part, "line" points to
   the text line.
*/
void printout(const char *sp, char *line, AttTYPE *attSp, AttTYPE *attLine, char format) {
	register long colnum;
	register long splen = 0;
	register AttTYPE oldAtt;
	AttTYPE *posAtt, *tposAtt;
	char *pos, *tpos, *s, first = TRUE, sb = FALSE, isLineSpace, isBulletFound, BulletFound;

	if (sp != NULL) {
		if (uS.mStrnicmp(sp, "@ID:", 4) == 0 || uS.mStrnicmp(sp, PIDHEADER, strlen(PIDHEADER)) == 0 ||
			uS.mStrnicmp(sp, CKEYWORDHEADER, strlen(CKEYWORDHEADER)) == 0)
			format = FALSE;
	}
	oldAtt = 0;
	if (line != NULL)
		checkPrintFont(line, fpout == stdout);
	if (!format) {
		if (sp == NULL) {
			if (line != NULL)
				ActualPrint(line, attLine, &oldAtt, FALSE, TRUE, fpout);
		} else {
			if (*sp != EOS) {
				ActualPrint(sp, attSp, &oldAtt, FALSE, FALSE, fpout);
				splen = strlen(sp) - 1;
			} else
				splen = 0;

			if (sp[splen] == '\n' && *line == EOS) ;
			else if (line != NULL) {
				if (sp[0] == '@' && *line == EOS) ;
				else if (!isSpace(sp[splen]) && sp[0] != EOS && line[0] != '\n')
					putc('\t',fpout);
				ActualPrint(line, attLine, &oldAtt, FALSE, TRUE, fpout);
			}
		}
		return;
	}
	if (sp != NULL) {
		for (colnum=0; sp[colnum] != EOS; colnum++) {
			if (sp[colnum] == '\t')
				splen = ((splen / TabSize) + 1) * TabSize;
			else
				splen++;
		}
		ActualPrint(sp, attSp, &oldAtt, FALSE, FALSE, fpout);
		if (line != NULL) {
			if (colnum != 0 && !isSpace(sp[colnum-1]) && line[0] != EOS && line[0] != '\n') {
				putc('\t',fpout);
				splen = ((splen / TabSize) + 1) * TabSize;
			}
		}
	}
	if (line == NULL) {
		printAtts(0, oldAtt, fpout);
		putc('\n',fpout);
		return;
	}
	if (isSpace(*line))
		*line = ' ';
	colnum = splen;
	tpos = line;
	tposAtt = attLine;
	isLineSpace = *line;
	BulletFound = FALSE;
	while (*line != EOS) {
		pos = line;
		posAtt = attLine;
		colnum++;
		if (*line == '[')
			sb = TRUE;
		if (BulletFound) {
			BulletFound = FALSE;
			if (!cutt_isMultiFound)
				colnum = MAXOUTCOL + 1;
		}
		isBulletFound = FALSE;
		while (((*line != ' ' && *line != '\t' && *line != '\n') || sb || isBulletFound) && *line != EOS) {
			if (*line == HIDEN_C) {
				isBulletFound = !isBulletFound;
				if (isBulletFound && isdigit(line[1]))
					BulletFound = TRUE;
			}
			line++;
			if (attLine != NULL)
				attLine++;
			if ((*line == '\n' && sb) || *line == '\t') *line = ' ';
			else if (*line == '[') sb = TRUE;
			else if (*line == ']') sb = FALSE;
			if (!isBulletFound && (UTF8_IS_SINGLE((unsigned char)*line) || UTF8_IS_LEAD((unsigned char)*line)))
				colnum++;
		}
		isLineSpace = *line;
		if (*line != EOS) {
			*line++ = EOS;
			if (attLine != NULL)
				attLine++;
		}
		if (colnum > MAXOUTCOL) {
			if (first)
				first = FALSE;
			else {
				if (tposAtt != NULL) {
					while (isSpace(*tpos)) {
						tpos++;
						printAtts(*tposAtt, oldAtt, fpout);
						oldAtt = *tposAtt;
						tposAtt++;
					}
					if (*tpos == '\n') {
						tpos++;
						tposAtt++;
					}
				}
				printAtts(0, oldAtt, fpout);
				oldAtt = 0;
				putc('\n',fpout);
				if (tposAtt != NULL) {
					while (isSpace(*tpos)) {
						tpos++;
						printAtts(*tposAtt, oldAtt, fpout);
						oldAtt = *tposAtt;
						tposAtt++;
					}
				}
				putc('\t',fpout);
				colnum = splen;
				isBulletFound = FALSE;
				for (s=pos; *s != EOS; s++) {
					if (*s == HIDEN_C)
						isBulletFound = !isBulletFound;
					if (!isBulletFound && (UTF8_IS_SINGLE((unsigned char)*s) || UTF8_IS_LEAD((unsigned char)*s)))
						colnum++;
				}
			}
		} else if (!first) {
			if (tposAtt != NULL) {
				while (isSpace(*tpos) || *tpos == '\n') {
					tpos++;
					printAtts(*tposAtt, oldAtt, fpout);
					oldAtt = *tposAtt;
					tposAtt++;
				}
			}
			putc(' ',fpout);
		} else
			first = FALSE;
		if (pos != line)
			ActualPrint(pos, posAtt, &oldAtt, FALSE, FALSE, fpout);
		if (isLineSpace != EOS) {
			line--;
			*line = isLineSpace;
			if (attLine != NULL)
				attLine--;
		}
		tpos = line;
		tposAtt = attLine;
		while (isSpace(*line) || *line == '\n') {
			line++;
			if (attLine != NULL)
				attLine++;
		}
	}
	printAtts(0, oldAtt, fpout);
	putc('\n',fpout);
}

void printoutline(FILE *fp, char *line) {
	register long colnum;
	char *pos, *tpos, *s, first = TRUE, sb = FALSE, isLineSpace;

	colnum = 0;
	tpos = line;
	isLineSpace = *line;
	while (*line != EOS) {
		pos = line;
		colnum++;
		if (*line == '[')
			sb = TRUE;
		while (((*line != ' ' && *line != '\t' && *line != '\n') || sb) && *line != EOS) {
			line++;
			if ((*line == '\n' && sb) || *line == '\t')
				*line = ' ';
			else if (*line == '[')
				sb = TRUE;
			else if (*line == ']')
				sb = FALSE;
			if ((UTF8_IS_SINGLE((unsigned char)*line) || UTF8_IS_LEAD((unsigned char)*line)))
				colnum++;
		}
		isLineSpace = *line;
		if (*line != EOS) {
			*line++ = EOS;
		}
		if (colnum > MAXOUTCOL) {
			if (first)
				first = FALSE;
			else {
				putc('\n',fp);
				putc('\t',fp);
				colnum = TabSize;
				colnum = 0;
				for (s=pos; *s != EOS; s++) {
					if ((UTF8_IS_SINGLE((unsigned char)*s) || UTF8_IS_LEAD((unsigned char)*s)))
						colnum++;
				}
			}
		} else if (!first) {
			putc(' ',fp);
		} else
			first = FALSE;
		if (pos != line) {
			for (s=pos; *s; s++) {
				putc(*s,fp);
			}
		}
		if (isLineSpace != EOS) {
			line--;
			*line = isLineSpace;
		}
		tpos = line;
		while (isSpace(*line) || *line == '\n') {
			line++;
		}
	}
	putc('\n',fp);
}

void changeBullet(char *line, AttTYPE *att) {
	long i, j;

	for (i=0; line[i] != EOS; i++) {
		if (line[i] == HIDEN_C && isdigit(line[i+1])) {
			i++;
			if (att == NULL)
				uS.shiftright(line+i, strlen(cMediaFileName)+strlen(SOUNDTIER)+3);
			else
				att_shiftright(line+i, att+i, strlen(cMediaFileName)+strlen(SOUNDTIER)+3);
			for (j=0L; SOUNDTIER[j] != EOS; j++)
				line[i++] = SOUNDTIER[j];
			line[i++] = '"';
			for (j=0L; cMediaFileName[j] != EOS; j++)
				line[i++] = cMediaFileName[j];
			line[i++] = '"';
			line[i++] = '_';
		}
	}
}

void remove_CRs_Tabs(char *line) {
	long i;

	for (i=0L; line[i] != EOS; ) {
		if (line[i] == '\n') {
			if (line[i+1] != '%')
				strcpy(line+i, line+i+1);
			else
				i++;
		} else if (line[i] == '\t') {
			line[i] = ' ';
		} else {
			i++;
		}
	}
}

void remove_main_tier_print(const char *sp, char *line, AttTYPE *att) {
	char isBulletFound;
	long i, j;

	i = 0L;
	j = 0L;
	isBulletFound = FALSE;
	if (*sp == '*' && (!isTierLabel(line+i, TRUE) || line[i] != '%')) {
		while (!isTierLabel(line+i, FALSE) && line[i] != EOS)
			i++;
	}

	for (; line[i] != EOS; i++) {
		if (line[i] == '*' && isTierLabel(line+i, FALSE)) {
			do {
				i++;
			} while (!isTierLabel(line+i, FALSE) && line[i] != EOS) ;
			if (line[i] == EOS)
				break;
		}

		if ((line[i-1] =='\n' || line[i-1] == '\r') && !isTierLabel(line+i, FALSE)) {
			if (line[i] == ' ')
				line[i] = '\t';
			else if (!isSpace(line[i])) {
				tempTier[j] = '\t';
				if (att != NULL)
					tempAtt[j] = att[i];
				else
					tempAtt[j] = 0;
				j++;
			}
		}
		if (line[i] == HIDEN_C)
			isBulletFound = TRUE;
		tempTier[j] = line[i];
		if (att != NULL)
			tempAtt[j] = att[i];
		else
			tempAtt[j] = 0;
		j++;
	}
	tempTier[j] = EOS;
	if (isBulletFound && cMediaFileName[0] != EOS)
		changeBullet(tempTier, tempAtt);
	if (CLAN_PROG_NUM == KWAL && onlydata == 5) {
		remove_CRs_Tabs(tempTier);
		fprintf(fpout, "%s", tempTier);
	} else
		printout(NULL,tempTier,NULL,tempAtt,FALSE);
}
/**************************************************************/
/*	 get whole tier and filterData it					  */
void checkOptions(char *st) {
	int t;
	NewFontInfo finfo;

	for (t=0; st[t] != EOS; t++) {
		if (!strncmp(st+t, "multi", 5)) {
			cutt_isMultiFound = TRUE;
		} else if (!strncmp(st+t, "CA-Unicode", 10)) {
			cutt_isCAFound = TRUE;
		} else if (!strncmp(st+t, "CA", 2)) {
			cutt_isCAFound = TRUE;
			finfo.fontName[0] = EOS;
			SetDefaultCAFinfo(&finfo);
			selectChoosenFont(&finfo, FALSE, FALSE);
		} else if (!strncmp(st+t, "heritage", 8)) {
			cutt_isBlobFound = TRUE;
		}
	}
}

static char extractRightTier(const char *sp, char *line, char *tierName, char *outTier) {
	int  i, j;
	char isFound;

	i = 0;
	j = 0;
	isFound = FALSE;
	if (!nomain) {
		strcpy(tierName, sp);
		uS.remblanks(tierName);
		for (j=0; line[i] != EOS; i++) {
			outTier[j++] = line[i];
			if (line[i] == '\n' && line[i+1] == '%')
				break;
		}
		isFound = TRUE;
	} else {
		do {
			for (; line[i] != EOS; i++) {
				if (line[i] == '\n' && line[i+1] == '%')
					break;
			}
			if (line[i] != EOS)
				i++;
			for (j=0; line[i] != EOS; i++) {
				tierName[j++] = line[i];
				if (line[i] == ':')
					break;
			}
			tierName[j] = EOS;
			j = 0;
			if (line[i] == EOS)
				break;
			if (checktier(tierName)) {
				isFound = TRUE;
				if (line[i] == ':')
					i++;
				while (isSpace(line[i]))
					i++;
				for (j=0; line[i] != EOS; i++) {
					outTier[j++] = line[i];
					if (line[i] == '\n' && line[i+1] == '%')
						break;
				}
				break;
			}
		} while (line[i] != EOS) ;
	}
	outTier[j] = EOS;
	return(isFound);
}

static void correctForXXXYYYWWW(char *line, char *outTier) {
	int i;

	if (!nomain) {
		for (i=0; outTier[i] != EOS; i++) {
			if (restoreXXX) {
				if (line[i] == 'x' && line[i+1] == 'x' && line[i+2] == 'x') {
					outTier[i] = line[i];
					outTier[i+1] = line[i+1];
					outTier[i+2] = line[i+2];
				}
				i += 2;
			} else {
				if (outTier[i] == 'x' && outTier[i+1] == 'x' && outTier[i+2] == 'x') {
					outTier[i] = ' ';
					outTier[i+1] = ' ';
					outTier[i+2] = ' ';
				}
				i += 2;
			}
			if (restoreYYY) {
				if (line[i] == 'y' && line[i+1] == 'y' && line[i+2] == 'y') {
					outTier[i] = line[i];
					outTier[i+1] = line[i+1];
					outTier[i+2] = line[i+2];
				}
				i += 2;
			} else {
				if (outTier[i] == 'y' && outTier[i+1] == 'y' && outTier[i+2] == 'y') {
					outTier[i] = ' ';
					outTier[i+1] = ' ';
					outTier[i+2] = ' ';
				}
				i += 2;
			}
			if (restoreWWW) {
				if (line[i] == 'w' && line[i+1] == 'w' && line[i+2] == 'w') {
					outTier[i] = line[i];
					outTier[i+1] = line[i+1];
					outTier[i+2] = line[i+2];
				}
				i += 2;
			} else {
				if (outTier[i] == 'w' && outTier[i+1] == 'w' && outTier[i+2] == 'w') {
					outTier[i] = ' ';
					outTier[i+1] = ' ';
					outTier[i+2] = ' ';
				}
				i += 2;
			}
		}
	}
}

/* gettextspeaker() is used in a chatmode = 0. It is a dummy function.
*/
int gettextspeaker() {
	if (feof(fpin)) return(FALSE);
	return(TRUE);
}

static void SpeakerNameError(char *name, int i, int lim) {
	register int j;

	fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
	fprintf(stderr,"Tier name is longer than %d characters.\n",lim);
	if (name[4] == ';')
		fprintf(stderr,"Possibly ';' character should be changed to ':'.\n");
	fprintf(stderr,"Maybe ':' character is missing or '-' is used.\n");
	j = 0;
	for (; j < i; j++)
		putc(name[j],stderr);
	putc('\n',stderr);
	cutt_exit(-1);
}

/* getspeaker(chrs) gets the code identifier part of the tier and stores it
   into memory pointed by "chrs". It returns 0 if end of file has been
   reached, 1 otherwise. If the tier does not have a line part, i.e. @begin,
   or the end of file has been reached then the "getrestline" is set to 0.
   "currentchar" is set to the last character read.
*/
int getspeaker(char *sp, AttTYPE *att, register int index) {
	register int orgIndex;
	register int i;
	register int j;

	if (feof(fpin)) {
		if (ByTurn)
			ByTurn = 1;
		return(FALSE);
	}

	if (chatmode && !isSpeaker(currentchar)) {
		fprintf(stderr,"\n\n*** File \"%s\": line %ld.\n", oldfname, lineno);
		if (currentchar <= 32) {
			fprintf(stderr,"Illegal speaker character found: 0x%x.\n", currentchar);
			if (currentchar == '\r' || currentchar == '\n')
				fprintf(stderr,"It looks like a blank line. Blank lines are not allowed in CHAT\n");
		} else
			fprintf(stderr,"Illegal speaker character found: %c.\n", currentchar);
		cutt_exit(-1);
	}

	sp[index] = currentchar;
	att[index] = currentatt;
	orgIndex = index;

	for (i=0; sp[index] != ':' && sp[index] != '\n' && !feof(fpin); i++) {
		if (i >= SPEAKERLEN) {
			SpeakerNameError(sp+orgIndex, i, SPEAKERLEN);
			index--;
		}
		index++;
		att[index] = att[index-1];
		sp[index] = (char)getc_cr(fpin, &att[index]);
/*
		if (sp[index] == '-')
			break;
*/
	}

	j = 0;
	while (sp[index] != ':' && sp[index] != '\n' && !feof(fpin)) {
		j++;
		if (i+j >= SPEAKERLEN) {
			i += j;
			SpeakerNameError(sp+orgIndex, i, SPEAKERLEN);
			index--;
		}
		index++;
		att[index] = att[index-1];
		sp[index] = (char)getc_cr(fpin, &att[index]);
	}

	if (sp[orgIndex] == '*' || sp[orgIndex] == '%') {
		if (i > 8+1 && CLAN_PROG_NUM != CHSTRING) {
			if (sp[index] == ':') {
				index++;
				att[index] = att[index-1];
				sp[index] = EOS;
			} else
				sp[index] = EOS;
			SpeakerNameError(sp+orgIndex, strlen(sp+orgIndex), 8);
		}
	}

	if (ByTurn) {
		sp[index+1] = EOS;
		if (sp[orgIndex] == '*' && strcmp(TSoldsp,sp+orgIndex) != 0) {
			if (*TSoldsp != EOS)
				ByTurn = 2;
			strcpy(TSoldsp, sp+orgIndex);
		}
	}

	if (sp[index] == ':') {
		i += j+1;
		if (i >= SPEAKERLEN) {
			SpeakerNameError(sp+orgIndex, i, SPEAKERLEN);
			index--;
		}
		index++;
		att[index] = att[index-1];
		sp[index] = (char)getc_cr(fpin, &att[index]);
		if (sp[index] == '\t'/* && CLAN_PROG_NUM == CHSTRING*/ && cutt_isCAFound) {
			i++;
			if (i >= SPEAKERLEN) {
				SpeakerNameError(sp+orgIndex, i, SPEAKERLEN);
				index--;
			}
			index++;
			att[index] = att[index-1];
			sp[index] = (char)getc_cr(fpin, &att[index]);
		} else {
			while (sp[index] == ' ' || sp[index] == '\t') {
				i++;
				if (i >= SPEAKERLEN) {
					SpeakerNameError(sp+orgIndex, i, SPEAKERLEN);
					index--;
				}
				index++;
				att[index] = att[index-1];
				sp[index] = (char)getc_cr(fpin, &att[index]);
			}
		}
	} else {
		for (i=index; (isSpace(sp[i]) || sp[i] == '\n') && i >= 0; i--) ;
		if (i < index) {
			index = i + 1;
			sp[index] = '\n';
		}
	}

	if (sp[index] == '\n') {
		if (CLAN_PROG_NUM != CHECK_P && CLAN_PROG_NUM != CHSTRING) {
			 do {
				tlineno++;
				sp[index] = (char)getc_cr(fpin, &att[index]);
			} while (sp[index] == '\n') ;
			if (isSpeaker(sp[index]) || uS.isUTF8(sp))
				getrestline = 0;
		}
	}
	if (feof(fpin))
		getrestline = 0;
	currentchar = sp[index];
	currentatt = att[index];
	sp[index] = EOS;
	return(TRUE);
}

static void changeClitic2Space(char *line) {
	int i;

	for (i=0; line[i] != EOS; ) {
		if (line[i] == '~' || line[i] == '$')
			line[i] = ' ';
		else
			i++;
	}
}

static void changeEngShort(char *w) {
	char *s;

	s = strchr(w, '\'');
	if (s != NULL) {
		if (s-2 >= w && *(s-1) == 'n' && *(s+1) == 't' && (*(s+2) == EOS || *(s+2) == '@')) {
			strcpy(w, s-1);
		} else if (s-1 >= w) {
			if (*(s+1) == 's' && (*(s+2) == EOS || *(s+2) == '@')) {
				strcpy(w, s+1);
			} else if (*(s+1) == 'm' && (*(s+2) == EOS || *(s+2) == '@')) {
				strcpy(w, s+1);
			} else if (*(s+1) == 'd' && (*(s+2) == EOS || *(s+2) == '@')) {
				strcpy(w, s+1);
			} else if (*(s+1) == 'r' && *(s+2) == 'e' && (*(s+3) == EOS || *(s+3) == '@')) {
				strcpy(w, s+1);
			} else if (*(s+1) == 'l' && *(s+2) == 'l' && (*(s+3) == EOS || *(s+3) == '@')) {
				strcpy(w, s+1);
			} else if (*(s+1) == 'v' && *(s+2) == 'e' && (*(s+3) == EOS || *(s+3) == '@')) {
				strcpy(w, s+1);
			}
		}
	}
}

void processSPTier(char *spTier, char *line) {
	int i, j, cnt;
	char morItem[BUFSIZ], spWord[BUFSIZ];

	spTier[0] = EOS;
	strcpy(spWord, " '");
	i = 0;
	while ((i=getNextDepTierPair(line, morItem, spWord+2, NULL, i)) != 0) {
		if (morItem[0] != EOS && spWord[2] != EOS) {
			cnt = 0;
			for (j=0; morItem[j] != EOS; j++) {
				if (morItem[j] == '$')
					cnt++;
			}
			spWord[0] = '$';
			while (cnt > 0) {
				strcat(spTier, spWord);
				strcat(spTier, " ");
				cnt--;
			}
			strcat(spTier, spWord+2);
			strcat(spTier, " ");
			cnt = 0;
			for (j=0; morItem[j] != EOS; j++) {
				if (morItem[j] == '~')
					cnt++;
			}
			spWord[0] = '-';
			changeEngShort(spWord+2);
			while (cnt > 0) {
				strcat(spTier, spWord);
				strcat(spTier, " ");
				cnt--;
			}
		}
	}
	uS.remblanks(spTier);
}

static void cleanupAtts(int pos, int len,  AttTYPE *attLine) {
	for (; len > 0 && pos < UTTLINELEN; pos++, len--) {
		attLine[pos] = 0;
	}
}

/* getmaincode() gets all the tiers that should be clastered together and
   stores them into "->line" field. It discards the tiers that were not
   selected by the user, and then it filters the selected data. It returns 0
   if getspeaker returned 0, and 1 otherwise.
*/
int getmaincode() {
	int posL, posLN, posT, posTN, i, j;
	char tc, *tl, RightTier, RightSpeaker, isTierSelected, isLinkDep2Other, isSkipTier;

	do {
		isSkipTier = FALSE;
		org_depTier[0] = EOS;
		isLinkDep2Other = TRUE;
		if (lineno > -1L)
			lineno = lineno + tlineno;
		tlineno = 0L;
		if (!getspeaker(lutter->speaker, lutter->attSp, 0))
			return(FALSE);
		lutter->tlineno = lineno;
		lutter->uttLen = 0L;
		if (*lutter->speaker == '@') {
			strcpy(templineC,lutter->speaker);
			uS.uppercasestr(templineC, &dFnt, MBF);
			if (uS.partcmp(templineC,"@ID:",FALSE, TRUE)) {
				punctuation = PUNCT_ID_SET;
			} else {
				punctuation = GlobalPunctuation;
			}
			IsModUttDel = FALSE;
		} else if (*lutter->speaker == '*') {
			if (uS.partcmp(templineC,"@ID:",FALSE, TRUE))
				templineC[0] = EOS;
			IsModUttDel = chatmode < 3;
		} else {
			strcpy(templineC,lutter->speaker);
			uS.uppercasestr(templineC, &dFnt, MBF);
			if (uS.partcmp(templineC,"%PHO:",FALSE, TRUE) || uS.partcmp(templineC,"%MOD:",FALSE, TRUE)) {
				IsModUttDel = FALSE;
				punctuation = PUNCT_PHO_MOD_SET;
			} else {
				IsModUttDel = (chatmode < 3);
				punctuation = GlobalPunctuation;
			}
		}
		cutt_getline(lutter->speaker, lutter->line, lutter->attLine, 0);
		strcpy(org_spName, lutter->speaker);
		strcpy(org_spTier, lutter->line);
		if (*lutter->speaker == '@' && currentchar == '%') {
			fprintf(stderr, "Header tier should not be followed by the Coder Dependent tier.\n");
			fprintf(stderr, "Error found in file \"%s\" on line %ld\n", oldfname, lineno);
			cutt_exit(0);
		}
		if (*lutter->speaker == '*' && nomain) {
			postcodeRes = isPostCodeFound(lutter->speaker, lutter->line);
			for (i=0; lutter->line[i] != EOS; i++)
				lutter->tuttline[i] = ' ';
			if (i > 0)
				lutter->tuttline[i-1] = '\n';
			lutter->tuttline[i] = EOS;
			posL = i;
			posT = posL;
		} else {
			if (*lutter->speaker == '*' && linkDep2Other) {
				for (i=0; lutter->line[i] != EOS; i++)
					lutter->tuttline[i] = ' ';
				if (i > 0)
					lutter->tuttline[i-1] = '\n';
				lutter->tuttline[i] = EOS;
				posL = i;
				posT = posL;
			} else {
				strcpy(lutter->tuttline,lutter->line);
				if (lutter->speaker[0] != '@' || !CheckOutTier(lutter->speaker)) {
					if (FilterTier > 0)
						filterData(lutter->speaker,lutter->tuttline);
					if (FilterTier > 1 && WordMode != 'e')
						filterwords(lutter->speaker,lutter->tuttline,exclude);
				}
				posL = strlen(lutter->line);
				posT = posL;
			}
		}
		if (uS.partcmp(lutter->speaker,"%PHO:",FALSE, FALSE) || uS.partcmp(lutter->speaker,"%MOD:",FALSE, FALSE)) {
			HandleSpCAs(lutter->tuttline, 0, strlen(lutter->tuttline), FALSE);
		}
		RightSpeaker = (char)checktier(lutter->speaker);
		while (!isMainSpeaker(currentchar) && currentchar != '@' && !feof(fpin)) {
			if (CLAN_PROG_NUM != COMBO)
				posTN = posT;
			if (!getspeaker(lutter->line, lutter->attLine, posL))
				return(FALSE);
			if (lineno > -1L)
				utterance->tlineno = lineno;
			utterance->uttLen = 0L;
			isTierSelected = (char)checktier(lutter->line+posL);
			RightTier = (RightSpeaker && isTierSelected);
			if (otcdt && !RightTier && wdptr == NULL && isMORSearch() == FALSE && isGRASearch() == FALSE && tct == FALSE && tcs)
				RightTier = CheckOutTier(lutter->line+posL);
//				RightTier = (!RightSpeaker && CheckOutTier(lutter->line+posL));
			else if (otcdt && RightTier && RightSpeaker && wdptr == NULL && isMORSearch() == FALSE && isGRASearch() == FALSE && tct == FALSE)
				RightTier = CheckOutTier(lutter->line+posL);
			else
				RightTier = (RightTier || CheckOutTier(lutter->line+posL));
			if (CLAN_PROG_NUM == KWAL)
				isTierSelected = RightTier;
			if (RightTier) {
				if (posT > 0 && lutter->tuttline[posT-1] != '\n' && lutter->tuttline[posT-1] != EOS) {
					if (posT > 2 && lutter->tuttline[posT-3] == (char)0xE2 && lutter->tuttline[posT-2] == (char)0x87 && lutter->tuttline[posT-1] == (char)0x94) {
					} else
						lutter->tuttline[posT-1] = '\n';
				}
				tl = lutter->line + posL;
				strcat(lutter->tuttline+posT,lutter->line+posL);
				if (CLAN_PROG_NUM == COMBO) {
					posLN = strlen(lutter->line);
					if (posLN > 0 && isSpace(lutter->line[posLN-1]))
						posLN--;
					if (posLN > 0 && lutter->line[posLN-1] == ':')
						posLN--;
					posTN = strlen(lutter->tuttline);
					if (posTN > 0 && isSpace(lutter->tuttline[posTN-1]))
						posTN--;
					if (posTN > 0 && lutter->tuttline[posTN-1] == ':')
						posTN--;
				}
				templineC[0] = EOS;
				if (lutter->line[posL] == '@') {
					strcpy(templineC,lutter->line+posL);
					uS.uppercasestr(templineC, &dFnt, MBF);
					if (uS.partcmp(templineC,"@ID:",FALSE, TRUE)) {
						punctuation = PUNCT_ID_SET;
					} else {
						punctuation = GlobalPunctuation;
					}
					IsModUttDel = FALSE;
				} else if (lutter->line[posL] == '*')
					IsModUttDel = chatmode < 3;
				else {
					strcpy(templineC,lutter->line+posL);
					uS.uppercasestr(templineC, &dFnt, MBF);
					if (uS.partcmp(templineC,"%PHO:",FALSE, TRUE) || uS.partcmp(templineC,"%MOD:",FALSE, TRUE)) {
						IsModUttDel = FALSE;
						punctuation = PUNCT_PHO_MOD_SET;
					} else {
						IsModUttDel = (chatmode < 3);
						punctuation = GlobalPunctuation;
					}
				}
				posL += strlen(lutter->line+posL);
				posT += strlen(lutter->tuttline+posT);
				cutt_getline("%", lutter->line, lutter->attLine, posL);
				if (linkDep2Other) {
					if (sDepTierName[0] == EOS) {
						if (uS.partcmp(templineC,"%MOR",FALSE,FALSE) ||
							uS.partcmp(templineC,"%TRN",FALSE,FALSE) ||
							uS.partcmp(templineC,"%XMOR",FALSE,FALSE)) {
							strcpy(org_depTier, lutter->line+posL);
						}
					}
				}
				tc = *utterance->speaker;
				*utterance->speaker = *tl;
				if (isTierSelected) {
					isSkipTier = FALSE;
					strcat(lutter->tuttline+posT,lutter->line+posL);
					if (uS.partcmp(templineC,"%PHO:",FALSE, TRUE) || uS.partcmp(templineC,"%MOD:",FALSE, TRUE)) {
						HandleSpCAs(lutter->tuttline+posT, 0, strlen(lutter->tuttline+posT), FALSE);
					}
					if (linkDep2Other) {
						isLinkDep2Other = FALSE;
						if (sDepTierName[0] == EOS) {
							if (fDepTierName[0] != EOS && uS.partcmp(templineC,fDepTierName,FALSE,FALSE)) {
								if (uS.partcmp(fDepTierName,"%GRA",FALSE,FALSE) ||
									uS.partcmp(fDepTierName,"%GRT",FALSE,FALSE) ||
									uS.partcmp(fDepTierName,"%CNL",FALSE,FALSE) ||
									uS.partcmp(fDepTierName,"%XCNL",FALSE,FALSE)) {
									if (org_depTier[0] == EOS) {
										fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
										fprintf(stderr, "ERROR: CAN'T LOCATE %%MOR: TIER BEFORE FINDING \"%s\" TARGET TIER.\n", fDepTierName);
										fprintf(stderr, "IN TOP TO BOTTOM ORDER THE %%MOR: TIER HAS TO BE LOCATED BEFORE \"%s\"\n\nQUITTING.\n\n", fDepTierName);
										cutt_exit(0);

									}
									createMorUttline(tempTier1,org_spTier,"%mor:",org_depTier,'\002',linkDep2Other);
									if (mor_link.error_found) {
										fprintf(stderr,"\n*** File \"%s\": line %ld.\n", mor_link.fname, mor_link.lineno);
										fprintf(stderr, "ERROR: %%MOR: TIER DOES NOT LINK IN SIZE TO ITS SPEAKER TIER.\n");
										fprintf(stderr, "QUITTING.\n\n");
										cutt_exit(0);
									} else {
										processSPTier(org_spTier, tempTier1);
									}
									if (linkDep2Other && CLAN_PROG_NUM == COMBO && strchr(org_spTier, '&') != NULL) {
										copyFillersToDepTier(org_spTier, lutter->tuttline+posT);
									}
									createMorUttline(tempTier1,org_spTier,templineC,lutter->tuttline+posT,FALSE,linkDep2Other);
									if (mor_link.error_found) {
										fprintf(stderr,"\n*** File \"%s\": line %ld.\n", mor_link.fname, mor_link.lineno);
										fprintf(stderr, "ERROR: %%MOR: TIER DOES NOT LINK IN SIZE TO ITS SPEAKER TIER.\n");
										fprintf(stderr, "QUITTING.\n\n");
										cutt_exit(0);
									}
									if (CLAN_PROG_NUM == COMBO) {
										if (strchr(tempTier1, dMarkChr) != NULL) {
											removeMainTierWords(tempTier1);
										}
										removeExtraSpace(tempTier1);
										uS.remFrontAndBackBlanks(tempTier1);
										rewrapComboTier(tempTier1);
									}
								} else
									createMorUttline(tempTier1,org_spTier,templineC,lutter->tuttline+posT,TRUE,linkDep2Other);
								strcpy(lutter->tuttline+posT, tempTier1);
								filterMorTier(lutter->tuttline+posT, tempTier1, 1, linkDep2Other);
								cleanupAtts(posT, strlen(tempTier1), lutter->attLine);
								if (CLAN_PROG_NUM == COMBO && onlydata == 0) {
									strcpy(lutter->line+posL, tempTier1);
									filterMorTier(lutter->line+posL, tempTier1, 1, linkDep2Other);
								}
								isLinkDep2Other = TRUE;
								isSkipTier = FALSE;
							}
						} else {
							if (org_depTier[0] != EOS) {
								if (fDepTierName[0] != EOS && uS.partcmp(templineC,fDepTierName,FALSE,FALSE)) {
									if ((uS.partcmp(templineC,"%GRA:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%GRT:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%CNL:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%XCNL:",FALSE,FALSE)) &&
										(uS.partcmp(sDepTierName,"%MOR",FALSE,FALSE) ||
										 uS.partcmp(sDepTierName,"%TRN",FALSE,FALSE) ||
										 uS.partcmp(sDepTierName,"%XMOR",FALSE,FALSE))) {
											changeClitic2Space(org_depTier);
											createMorUttline(tempTier1,lutter->tuttline+posT,sDepTierName,org_depTier,FALSE,linkDep2Other);
									} else if ((uS.partcmp(sDepTierName,"%GRA:",FALSE,FALSE) ||
												uS.partcmp(sDepTierName,"%GRT:",FALSE,FALSE) ||
												uS.partcmp(sDepTierName,"%CNL:",FALSE,FALSE) ||
												uS.partcmp(sDepTierName,"%XCNL:",FALSE,FALSE)) &&
											   (uS.partcmp(templineC,"%MOR",FALSE,FALSE) ||
												uS.partcmp(templineC,"%TRN",FALSE,FALSE) ||
												uS.partcmp(templineC,"%XMOR",FALSE,FALSE))) {
											changeClitic2Space(lutter->tuttline+posT);
											createMorUttline(tempTier1,org_depTier,templineC,lutter->tuttline+posT,FALSE,linkDep2Other);
									}
									strcpy(lutter->tuttline+posT, tempTier1);
									filterMorTier(lutter->tuttline+posT, tempTier1, 1, linkDep2Other);
									cleanupAtts(posT, strlen(tempTier1), lutter->attLine);
									if (CLAN_PROG_NUM == COMBO && onlydata == 0) {
										strcpy(lutter->line+posL, tempTier1);
										filterMorTier(lutter->line+posL, tempTier1, 1, linkDep2Other);
									}
									isLinkDep2Other = TRUE;
									isSkipTier = FALSE;
								} else if (uS.partcmp(templineC,sDepTierName,FALSE, FALSE)) {
									if ((uS.partcmp(templineC,"%GRA:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%GRT:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%CNL:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%XCNL:",FALSE,FALSE)) &&
										(uS.partcmp(fDepTierName,"%MOR",FALSE,FALSE) ||
										 uS.partcmp(fDepTierName,"%TRN",FALSE,FALSE) ||
										 uS.partcmp(fDepTierName,"%XMOR",FALSE,FALSE))) {
											changeClitic2Space(org_depTier);
											createMorUttline(tempTier1,lutter->tuttline+posT,fDepTierName,org_depTier,FALSE,linkDep2Other);
									} else if ((uS.partcmp(fDepTierName,"%GRA:",FALSE,FALSE) ||
												uS.partcmp(fDepTierName,"%GRT:",FALSE,FALSE) ||
												uS.partcmp(fDepTierName,"%CNL:",FALSE,FALSE) ||
												uS.partcmp(fDepTierName,"%XCNL:",FALSE,FALSE)) &&
											   (uS.partcmp(templineC,"%MOR",FALSE,FALSE) ||
												uS.partcmp(templineC,"%TRN",FALSE,FALSE) ||
												uS.partcmp(templineC,"%XMOR",FALSE,FALSE))) {
											changeClitic2Space(lutter->tuttline+posT);
											createMorUttline(tempTier1,org_depTier,templineC,lutter->tuttline+posT,FALSE,linkDep2Other);
									}
									strcpy(lutter->tuttline+posT, tempTier1);
									filterMorTier(lutter->tuttline+posT, tempTier1, 1, linkDep2Other);
									cleanupAtts(posT, strlen(tempTier1), lutter->attLine);
									if (CLAN_PROG_NUM == COMBO && onlydata == 0) {
										strcpy(lutter->line+posL, tempTier1);
										filterMorTier(lutter->line+posL, tempTier1, 1, linkDep2Other);
									}
									isLinkDep2Other = TRUE;
									isSkipTier = FALSE;
								}
							}
							if (!isLinkDep2Other && sDepTierName[0] != EOS) {
								if (fDepTierName[0] != EOS && uS.partcmp(templineC,fDepTierName,FALSE,FALSE)) {
									strcpy(org_depTier, lutter->line+posL);
									isSkipTier = TRUE;
								} else if (uS.partcmp(templineC,sDepTierName,FALSE, FALSE)) {
									isSkipTier = TRUE;
									strcpy(org_depTier, lutter->line+posL);
								}
							}
						}
					} else if (uS.partcmp(templineC,"%mor:",FALSE, FALSE)) {
						if (isMORSearch() || linkMain2Mor) {
							createMorUttline(tempTier1, org_spTier, "%mor:", lutter->tuttline+posT, TRUE, linkMain2Mor);
							strcpy(lutter->tuttline+posT, tempTier1);
							filterMorTier(lutter->tuttline+posT, tempTier1, 1, linkMain2Mor);
							cleanupAtts(posT, strlen(tempTier1), lutter->attLine);
						} else
							filterMorTier(lutter->tuttline+posT, NULL, 0, linkMain2Mor);
					}
					if (isSkipTier) {
						posT = posTN;
						if (CLAN_PROG_NUM == COMBO) {
							lutter->tuttline[posT++] = (char)0xE2;
							lutter->tuttline[posT++] = (char)0x87;
							lutter->tuttline[posT++] = (char)0x94;
							lutter->tuttline[posT] = EOS;
							if (onlydata == 0) {
								posL = posLN;
								lutter->line[posL++] = (char)0xE2;
								lutter->line[posL++] = (char)0x87;
								lutter->line[posL++] = (char)0x94;
								lutter->line[posL] = EOS;
							}
						} else
							lutter->tuttline[posT] = EOS;
					} else {
						if (isKWAL2013) {
							for (i=posT; lutter->line[i]; i++) {
								if (lutter->tuttline[i] != '\n')
									lutter->tuttline[i] = ' ';
							}
						} else if (FilterTier > 0)
							filterData(tl,lutter->tuttline+posT);
						if (FilterTier > 1 && WordMode != 'e')
							filterwords(tl,lutter->tuttline+posT,exclude);
					}
				} else { // if (isTierSelected)
					for (i=posL, j=posT; lutter->line[i]; i++) {
						if (lutter->line[i] == '\n')
							lutter->tuttline[j] = '\n';
						else
							lutter->tuttline[j] = ' ';
						if (CLAN_PROG_NUM != KWAL)
							j++;
					}
					lutter->tuttline[j] = EOS;
				}
				*utterance->speaker = tc;
				posL += strlen(lutter->line+posL);
				posT += strlen(lutter->tuttline+posT);
			} else {
				if (linkDep2Other && sDepTierName[0] == EOS) {
					strcpy(templineC,lutter->line+posL);
					uS.uppercasestr(templineC, &dFnt, MBF);
					if (uS.partcmp(templineC,"%MOR",FALSE,FALSE) ||
						uS.partcmp(templineC,"%TRN",FALSE,FALSE) ||
						uS.partcmp(templineC,"%XMOR",FALSE,FALSE)) {
						cutt_getline("%", tempTier1, tempAtt, 0);
						strcpy(org_depTier, tempTier1);
						lutter->line[posL] = EOS;
						lutter->tuttline[posT] = EOS;
					} else {
						lutter->line[posL] = EOS;
						lutter->tuttline[posT] = EOS;
						killline(NULL, NULL);
					}
				} else {
					lutter->line[posL] = EOS;
					lutter->tuttline[posT] = EOS;
					killline(NULL, NULL);
				}
			}
		}
	} while (linkDep2Other && !isLinkDep2Other) ;
	punctuation = GlobalPunctuation;
	return(TRUE);
}

void blankoSelectedSpeakers(char *line) {
	int pos;

	pos = 0;
	while (line[pos] != EOS) {
		if (line[pos] == '\n' && line[pos+1] == '%') {
			pos++;
			while (!checktier(line+pos)) {
				for (; line[pos] && line[pos] != ':'; pos++) ;
				if (line[pos] == ':') {
					pos++;
					if (line[pos] == '\t')
						pos++;
				}
				for (; line[pos] && (line[pos] != '\n' || line[pos+1] != '%'); pos++) {
					line[pos] = ' ';
				}
				if (line[pos] == '\n')
					pos++;
				if (line[pos] == EOS)
					break;
			}
			for (; line[pos] && line[pos] != ':'; pos++) ;
			if (line[pos] == ':') {
				pos++;
				if (line[pos] == '\t')
					pos++;
			}
			if (line[pos] == EOS)
				break;
		}
		pos++;
	}
}

int getwholeutter() {
	if (isWinMode) {
/* if isWinMode getwholeutter() call getmaincode() to get cluster tiers make sure that
   the tiers are the selected ones and that the data in the cluster tiers is
   in the right range. It returns 0 if getmaincode() or gettextspeaker
   returned 0, and 1 otherwise.
*/
		char *chrs;

		do {
			if (chatmode) {
				do {
					if (lutter != utterance) {
						utterance = utterance->nextutt;
						chrs = utterance->speaker;
					} else {
						utterance = utterance->nextutt;
						lutter = lutter->nextutt;
						postcodeRes = 0;
						if (!getmaincode())
							return(FALSE);
						chrs = lutter->speaker;
					}
					if (*utterance->speaker == '@') {
						if (SPRole != NULL) {
							if (uS.partcmp(utterance->speaker,"@ID:",FALSE, FALSE)) {
								SetSPRoleIDs(utterance->line);
							} else if (uS.partcmp(utterance->speaker,PARTICIPANTS,FALSE,FALSE)) {
								SetSPRoleParticipants(utterance->line);
							}
						}
						if (uS.partcmp(utterance->speaker,"@ID:",FALSE, FALSE)) {
							cleanUpIDTier(utterance->line);
							if (IDField != NULL) {
								SetIDTier(utterance->line);
							}
						} else if (uS.partcmp(utterance->speaker,"@Languages:",FALSE,FALSE) || uS.partcmp(utterance->speaker,"@New Language:",FALSE,FALSE)) {
						} else if (uS.partcmp(utterance->speaker,"@Options:",FALSE,FALSE)) {
							checkOptions(utterance->line);
						} else if (uS.partcmp(utterance->speaker,MEDIAHEADER,FALSE,FALSE)) {
							getMediaName(utterance->line, cMediaFileName, FILENAME_MAX);
						} else if (uS.isUTF8(utterance->speaker)) {
							dFnt.isUTF = 1;
							if (isOrgUnicodeFont(dFnt.orgFType)) {
								NewFontInfo finfo;
								SetDefaultUnicodeFinfo(&finfo);
								selectChoosenFont(&finfo, FALSE, FALSE);
							}
							tlineno--;
						} else if (uS.partcmp(utterance->speaker,FONTHEADER,FALSE,FALSE)) {
							cutt_SetNewFont(utterance->line, EOS);
							tlineno--;
						} else if (uS.isInvisibleHeader(utterance->speaker)) {
							tlineno--;
						} else {
							if (uS.partwcmp(utterance->line, FONTMARKER))
								cutt_SetNewFont(utterance->line,']');
						}
						if (checktier(chrs) || CheckOutTier(chrs)) break;
					} else if (rightspeaker || *chrs == '*') {
						if (checktier(chrs)) {
							rightspeaker = TRUE;
							if (uS.partwcmp(utterance->line, FONTMARKER))
								cutt_SetNewFont(utterance->line,']');
							break;
						} else if (CheckOutTier(chrs)) {
							if (*utterance->speaker == '*')
								rightspeaker = FALSE;
							if (uS.partwcmp(utterance->line, FONTMARKER))
								cutt_SetNewFont(utterance->line,']');
							break;
						} else if (*utterance->speaker == '*') {
							rightspeaker = FALSE;
							if (uS.partwcmp(utterance->line, FONTMARKER))
								cutt_SetNewFont(utterance->line,']');
						} else {
							if (uS.partwcmp(utterance->line, FONTMARKER))
								cutt_SetNewFont(utterance->line,']');
						}
					} else {
						if (uS.partwcmp(utterance->line, FONTMARKER))
							cutt_SetNewFont(utterance->line,']');
					}
				} while (1) ;
				if (uS.partcmp(templineC,"@ID:",FALSE,FALSE)) {
					punctuation = PUNCT_ID_SET;
				} else {
					punctuation = GlobalPunctuation;
				}
				uttline = utterance->tuttline;
				break;
			} else {
				if (lutter != utterance) {
					utterance = utterance->nextutt;
				} else {
					utterance = utterance->nextutt;
					lutter = lutter->nextutt;
					if (!gettextspeaker())
						return(FALSE);
					postcodeRes = 0;
					IsModUttDel = chatmode < 3;
					cutt_getline("*", lutter->line, lutter->attLine, 0);
					lutter->tlineno = lineno;
					lutter->uttLen = 0L;
					strcpy(lutter->tuttline,lutter->line);
					if (uS.isskip("[", 0, &dFnt, FALSE) || uS.isskip("]", 0, &dFnt, FALSE))
						filterData("*",lutter->tuttline);
					else
						filterwords("*",lutter->tuttline,excludedef);
//					if (WordMode != 'e' && MorWordMode != 'e')
					if (WordMode != 'e')
						filterwords("*",lutter->tuttline,exclude);
				}
				uttline = utterance->tuttline;
				if (uS.isUTF8(utterance->line)) {
					tlineno--;
				} else if (uS.partcmp(utterance->line,FONTHEADER,FALSE,FALSE)) {
					int i = strlen(FONTHEADER);
					while (isSpace(utterance->line[i]))
						i++;
					cutt_SetNewFont(utterance->line+i, EOS);
					tlineno--;
				} else if (uS.isInvisibleHeader(utterance->line)) {
					tlineno--;
				}
				break;
			}
		} while (1) ;
		return(TRUE);
	} else {
/* if !isWinMode getwholeutter()  gets next tier into utterance and determines if the tier
   has been selected by the user and if it is in the right range, if yes
   then it is filtered, other wise it gets the next tier from the input
   stream. It returns 0 if the getspeaker() or gettextspeaker() returns 0,
   and 1 otherwise.
*/
		char isLinkDep2Other;
		if (ByTurn >= 2) {
			ByTurn = 1;
			return(TRUE);
		}
		isLinkDep2Other = FALSE;
		do {
			if (chatmode) {
				if (LocalTierSelect) {
					do {
						if (lineno > -1L)
							lineno = lineno + tlineno;
						tlineno = 0L;
						if (!getspeaker(utterance->speaker, utterance->attSp, 0))
							return(FALSE);
						if (SPRole != NULL) {
							if (uS.partcmp(utterance->speaker,"@ID:",FALSE, FALSE)) {
								IsModUttDel = FALSE;
								cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
								skipgetline = TRUE;
								getrestline = 0;
								SetSPRoleIDs(utterance->line);
							} else if (uS.partcmp(utterance->speaker,PARTICIPANTS,FALSE,FALSE)) {
								IsModUttDel = FALSE;
								cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
								skipgetline = TRUE;
								getrestline = 0;
								SetSPRoleParticipants(utterance->line);
							}
						}
						if (uS.partcmp(utterance->speaker,"@ID:",FALSE,FALSE)) {
							IsModUttDel = FALSE;
							cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
							skipgetline = TRUE;
							getrestline = 0;
							cleanUpIDTier(utterance->line);
							if (IDField != NULL) {
								SetIDTier(utterance->line);
							}
						} else if (uS.partcmp(utterance->speaker,"@Languages:",FALSE,FALSE) || uS.partcmp(utterance->speaker,"@New Language:",FALSE,FALSE)) {
						} else if (uS.partcmp(utterance->speaker,"@Options:",FALSE,FALSE)) {
							IsModUttDel = FALSE;
							cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
							skipgetline = TRUE;
							getrestline = 0;
							checkOptions(utterance->line);
						} else if (uS.partcmp(utterance->speaker,MEDIAHEADER,FALSE,FALSE)) {
							IsModUttDel = FALSE;
							cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
							skipgetline = TRUE;
							getrestline = 0;
							getMediaName(utterance->line, cMediaFileName, FILENAME_MAX);
						} else if (uS.isUTF8(utterance->speaker)) {
							dFnt.isUTF = 1;
							if (isOrgUnicodeFont(dFnt.orgFType)) {
								NewFontInfo finfo;
								SetDefaultUnicodeFinfo(&finfo);
								selectChoosenFont(&finfo, FALSE, FALSE);
							}
							tlineno--;
						} else if (uS.partcmp(utterance->speaker,FONTHEADER,FALSE,FALSE)) {
							IsModUttDel = FALSE;
							cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
							skipgetline = TRUE;
							getrestline = 0;
							cutt_SetNewFont(utterance->line, EOS);
							tlineno--;
						} else if (uS.isInvisibleHeader(utterance->speaker)) {
							tlineno--;
						}
						break;
					} while (1) ;
					if (*utterance->speaker == '*')
						postcodeRes = 0;
				} else {
					do {
						if (lineno > -1L)
							lineno = lineno + tlineno;
						tlineno = 0L;
						if (!getspeaker(utterance->speaker, utterance->attSp, 0))
							return(FALSE);
						if (*utterance->speaker == '*')
							postcodeRes = 0;
						if (*utterance->speaker == '@') {
							if (SPRole != NULL) {
								if (uS.partcmp(utterance->speaker,"@ID:",FALSE, FALSE)) {
									IsModUttDel = FALSE;
									cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
									skipgetline = TRUE;
									getrestline = 0;
									SetSPRoleIDs(utterance->line);
								} else if (uS.partcmp(utterance->speaker,PARTICIPANTS,FALSE,FALSE)) {
									IsModUttDel = FALSE;
									cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
									skipgetline = TRUE;
									getrestline = 0;
									SetSPRoleParticipants(utterance->line);
								}
							}

							if (uS.partcmp(utterance->speaker,"@ID:",FALSE,FALSE)) {
								IsModUttDel = FALSE;
								cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
								skipgetline = TRUE;
								getrestline = 0;
								cleanUpIDTier(utterance->line);
								if (IDField != NULL) {
									SetIDTier(utterance->line);
								}
							} else if (uS.partcmp(utterance->speaker,"@Languages:",FALSE,FALSE) || uS.partcmp(utterance->speaker,"@New Language:",FALSE,FALSE)) {
							} else if (uS.partcmp(utterance->speaker,"@Options:",FALSE,FALSE)) {
								IsModUttDel = FALSE;
								cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
								skipgetline = TRUE;
								getrestline = 0;
								checkOptions(utterance->line);
							} else if (uS.partcmp(utterance->speaker,MEDIAHEADER,FALSE,FALSE)) {
								IsModUttDel = FALSE;
								cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
								skipgetline = TRUE;
								getrestline = 0;
								getMediaName(utterance->line, cMediaFileName, FILENAME_MAX);
							} else if (uS.partcmp(utterance->speaker,PIDHEADER,FALSE,FALSE)) {
								tlineno--;
							} else if (uS.partcmp(utterance->speaker,CKEYWORDHEADER,FALSE,FALSE)) {
								tlineno--;
							} else if (uS.isUTF8(utterance->speaker)) {
								dFnt.isUTF = 1;
								if (isOrgUnicodeFont(dFnt.orgFType)) {
									NewFontInfo finfo;
									SetDefaultUnicodeFinfo(&finfo);
									selectChoosenFont(&finfo, FALSE, FALSE);
								}
								tlineno--;
							} else if (uS.partcmp(utterance->speaker,FONTHEADER,FALSE,FALSE)) {
								IsModUttDel = FALSE;
								cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
								skipgetline = TRUE;
								getrestline = 0;
								cutt_SetNewFont(utterance->line, EOS);
								tlineno--;
							} else if (uS.isInvisibleHeader(utterance->speaker)) {
								tlineno--;
							}
							if (checktier(utterance->speaker) || CheckOutTier(utterance->speaker))
								break;
							killline(utterance->line, utterance->attLine);
						} else if (rightspeaker || *utterance->speaker == '*') {
							if (checktier(utterance->speaker)) {
								rightspeaker = TRUE;
								if (*utterance->speaker != '*' || !nomain)
									break;
								killline(utterance->line, utterance->attLine);
								if (*utterance->speaker == '*')
									postcodeRes = isPostCodeFound(utterance->speaker, utterance->line);
							} else if (CheckOutTier(utterance->speaker)) {
								rightspeaker = TRUE;
								if (*utterance->speaker != '*' || !nomain)
									break;
								killline(utterance->line, utterance->attLine);
								if (*utterance->speaker == '*')
									postcodeRes = isPostCodeFound(utterance->speaker, utterance->line);
							} else if (*utterance->speaker == '*') {
								rightspeaker = FALSE;
								killline(utterance->line, utterance->attLine);
							} else
								killline(utterance->line, utterance->attLine);
							if (*utterance->speaker == '*') {
								if (linkMain2Mor)
									cutt_cleanUpLine(utterance->speaker, utterance->line, utterance->attLine, 0);
								strcpy(org_spName, utterance->speaker);
								strcpy(org_spTier, utterance->line);
							}
						} else
							killline(utterance->line, utterance->attLine);
					} while (1) ;
				}

				isLinkDep2Other = TRUE;
				strcpy(templineC,utterance->speaker);
				uS.uppercasestr(templineC, &dFnt, MBF);
				if (uS.partcmp(templineC,"@ID:",FALSE,FALSE)) {
					IsModUttDel = FALSE;
					punctuation = PUNCT_ID_SET;
					cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
					if (uttline != utterance->line)
						strcpy(uttline,utterance->line);
				} else if (uS.partcmp(templineC,"%PHO:",FALSE,FALSE) || uS.partcmp(templineC,"%MOD:",FALSE,FALSE)) {
					IsModUttDel = FALSE;
					punctuation = PUNCT_PHO_MOD_SET;
					cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
					if (uS.partwcmp(utterance->line, FONTMARKER))
						cutt_SetNewFont(utterance->line,']');
					if (uttline != utterance->line)
						strcpy(uttline,utterance->line);
					if (FilterTier > 0) {
						HandleSpCAs(uttline, 0, strlen(uttline), FALSE);
						filterscop("%", uttline, TRUE);
					}
				} else {
					IsModUttDel = ((*templineC != '@') && (chatmode < 3));
					punctuation = GlobalPunctuation;
					cutt_getline(utterance->speaker, utterance->line, utterance->attLine, 0);
					if (*utterance->speaker == '*') {
						strcpy(org_spName, utterance->speaker);
						strcpy(org_spTier, utterance->line);
					}
					if (uS.partwcmp(utterance->line, FONTMARKER))
						cutt_SetNewFont(utterance->line,']');
					if (uttline != utterance->line)
						strcpy(uttline,utterance->line);
					if (linkDep2Other) {
						if (*utterance->speaker == '*' || *utterance->speaker == '@') {
							org_depTier[0] = EOS;
							isLinkDep2Other = TRUE;
						} else {
							if (sDepTierName[0] == EOS) {
								if (uS.partcmp(templineC,"%MOR",FALSE,FALSE) ||
									uS.partcmp(templineC,"%TRN",FALSE,FALSE) ||
									uS.partcmp(templineC,"%XMOR",FALSE,FALSE)) {
									strcpy(org_depTier, utterance->line);
								}
							}
							isLinkDep2Other = FALSE;
						}
						if (sDepTierName[0] == EOS) {
							if (fDepTierName[0] != EOS && uS.partcmp(templineC,fDepTierName,FALSE,FALSE)) {
								if (uS.partcmp(fDepTierName,"%GRA",FALSE,FALSE) ||
									uS.partcmp(fDepTierName,"%GRT",FALSE,FALSE) ||
									uS.partcmp(fDepTierName,"%CNL",FALSE,FALSE) ||
									uS.partcmp(fDepTierName,"%XCNL",FALSE,FALSE)) {
									if (org_depTier[0] == EOS) {
										fprintf(stderr,"\n*** File \"%s\": line %ld.\n", oldfname, lineno);
										fprintf(stderr, "ERROR: CAN'T LOCATE %%MOR: TIER BEFORE FINDING \"%s\" TARGET TIER.\n", fDepTierName);
										fprintf(stderr, "IN TOP TO BOTTOM ORDER THE %%MOR: TIER HAS TO BE LOCATED BEFORE \"%s\"\n\nQUITTING.\n\n", fDepTierName);
										cutt_exit(0);

									}
									createMorUttline(utterance->line,org_spTier,"%mor:",org_depTier,TRUE,linkDep2Other);
									if (mor_link.error_found) {
										fprintf(stderr,"\n*** File \"%s\": line %ld.\n", mor_link.fname, mor_link.lineno);
										fprintf(stderr, "ERROR: %%MOR: TIER DOES NOT LINK IN SIZE TO ITS SPEAKER TIER.\n");
										fprintf(stderr, "QUITTING.\n\n");
										cutt_exit(0);
									} else {
										processSPTier(org_spTier, utterance->line);
									}
									createMorUttline(utterance->line,org_spTier,templineC,utterance->tuttline,FALSE,linkDep2Other);
									if (mor_link.error_found) {
										fprintf(stderr,"\n*** File \"%s\": line %ld.\n", mor_link.fname, mor_link.lineno);
										fprintf(stderr, "ERROR: %%MOR: TIER DOES NOT LINK IN SIZE TO ITS SPEAKER TIER.\n");
										fprintf(stderr, "QUITTING.\n\n");
										cutt_exit(0);
									}
								} else
									createMorUttline(utterance->line,org_spTier,templineC,utterance->tuttline,TRUE,linkDep2Other);
								strcpy(utterance->tuttline, utterance->line);
								filterMorTier(uttline, utterance->line, 2, linkDep2Other);
								isLinkDep2Other = TRUE;
							}
						} else {
							if (org_depTier[0] != EOS) {
								if (fDepTierName[0] != EOS && uS.partcmp(templineC,fDepTierName,FALSE,FALSE)) {
									if ((uS.partcmp(templineC,"%GRA:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%GRT:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%CNL:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%XCNL:",FALSE,FALSE)) &&
										(uS.partcmp(sDepTierName,"%MOR",FALSE,FALSE) ||
										 uS.partcmp(sDepTierName,"%TRN",FALSE,FALSE) ||
										 uS.partcmp(sDepTierName,"%XMOR",FALSE,FALSE))) {
											changeClitic2Space(org_depTier);
											createMorUttline(utterance->line,utterance->tuttline,sDepTierName,org_depTier,FALSE,linkDep2Other);
									} else if ((uS.partcmp(sDepTierName,"%GRA:",FALSE,FALSE) ||
												uS.partcmp(sDepTierName,"%GRT:",FALSE,FALSE) ||
												uS.partcmp(sDepTierName,"%CNL:",FALSE,FALSE) ||
												uS.partcmp(sDepTierName,"%XCNL:",FALSE,FALSE)) &&
											   (uS.partcmp(templineC,"%MOR",FALSE,FALSE) ||
												uS.partcmp(templineC,"%TRN",FALSE,FALSE) ||
												uS.partcmp(templineC,"%XMOR",FALSE,FALSE))) {
											changeClitic2Space(utterance->tuttline);
											createMorUttline(utterance->line,org_depTier,templineC,utterance->tuttline,FALSE,linkDep2Other);
									}
									strcpy(utterance->tuttline, utterance->line);
									filterMorTier(uttline, utterance->line, 2, linkDep2Other);
									isLinkDep2Other = TRUE;
								} else if (uS.partcmp(templineC,sDepTierName,FALSE, FALSE)) {
									if ((uS.partcmp(templineC,"%GRA:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%GRT:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%CNL:",FALSE,FALSE) ||
										 uS.partcmp(templineC,"%XCNL:",FALSE,FALSE)) &&
										(uS.partcmp(fDepTierName,"%MOR",FALSE,FALSE) ||
										 uS.partcmp(fDepTierName,"%TRN",FALSE,FALSE) ||
										 uS.partcmp(fDepTierName,"%XMOR",FALSE,FALSE))) {
											changeClitic2Space(org_depTier);
											createMorUttline(utterance->line,utterance->tuttline,fDepTierName,org_depTier,FALSE,linkDep2Other);
									} else if ((uS.partcmp(fDepTierName,"%GRA:",FALSE,FALSE) ||
												uS.partcmp(fDepTierName,"%GRT:",FALSE,FALSE) ||
											    uS.partcmp(fDepTierName,"%CNL:",FALSE,FALSE) ||
											    uS.partcmp(fDepTierName,"%XCNL:",FALSE,FALSE)) &&
											   (uS.partcmp(templineC,"%MOR",FALSE,FALSE) ||
												uS.partcmp(templineC,"%TRN",FALSE,FALSE) ||
												uS.partcmp(templineC,"%XMOR",FALSE,FALSE))) {
											changeClitic2Space(utterance->tuttline);
											createMorUttline(utterance->line,org_depTier,templineC,utterance->tuttline,FALSE,linkDep2Other);
									}
									strcpy(utterance->tuttline, utterance->line);
									filterMorTier(uttline, utterance->line, 2, linkDep2Other);
									isLinkDep2Other = TRUE;
								}
							}
							if (!isLinkDep2Other && sDepTierName[0] != EOS) {
								if (fDepTierName[0] != EOS && uS.partcmp(templineC,fDepTierName,FALSE,FALSE)) {
									checktier(utterance->speaker);
									strcpy(org_depTier, utterance->line);
								} else if (uS.partcmp(templineC,sDepTierName,FALSE, FALSE)) {
									checktier(utterance->speaker);
									strcpy(org_depTier, utterance->line);
								}
							}
						}
					} else if (uS.partcmp(templineC,"%mor:",FALSE,FALSE)) {
						if (isMORSearch() || linkMain2Mor || isRightR8Command()) {
							createMorUttline(utterance->line, org_spTier, "%mor:", utterance->tuttline, TRUE, linkMain2Mor);
							strcpy(utterance->tuttline, utterance->line);
							filterMorTier(uttline, utterance->line, 2, linkMain2Mor);
						} else
							filterMorTier(uttline, NULL, 0, linkMain2Mor);
					}
					if (FilterTier > 0)
						filterData(utterance->speaker,uttline);
				}
				if (linkDep2Other && !isLinkDep2Other) {
				} else {
					if (FilterTier > 1)
						filterwords(utterance->speaker,uttline,exclude);
					break;
				}
			} else { /* if (chatmode) */
				postcodeRes = 0;
				if (!gettextspeaker())
					return(FALSE);
				IsModUttDel = chatmode < 3;
				cutt_getline("*", utterance->line, utterance->attLine, 0);
				if (uS.isUTF8(utterance->line)) {
					tlineno--;
				} else if (uS.partcmp(utterance->line,FONTHEADER,FALSE,FALSE)) {
					int i = strlen(FONTHEADER);
					while (isSpace(utterance->line[i]))
						i++;
					cutt_SetNewFont(utterance->line+i, EOS);
					tlineno--;
				} else if (uS.isInvisibleHeader(utterance->line)) {
					tlineno--;
				}
				if (uttline != utterance->line)
					strcpy(uttline,utterance->line);
				if (FilterTier > 0)
					filterData("*",uttline);
				if (FilterTier > 1)
					filterwords("*",uttline,exclude);
				break;
			}
		} while (1) ;
		if (ByTurn == 2)
			return(FALSE);
		else
			return(TRUE);
	}
}
