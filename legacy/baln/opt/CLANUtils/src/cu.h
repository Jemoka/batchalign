/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


/* prototype cleanup 3/07/90 Darius Clynes */
#ifndef CUDEF
#define CUDEF

#include "common.h"
#include "c_clan.h"
#include <ctype.h>
#include "progs.h"

extern "C"
{
/* ************************************************************** */
/*  instructions for multiple clan  Darius Clynes March 6, 1990 */
/*

additional files necessary:
   mul.c	-- supermain which decides which clan main to call
   mul.inc	-- sets up all the necessary arrays of functions
		   			(used only by mul.c)   
   mul.h	-- external references to our function arrays 
		  			(used only by cutt.c and mllib.c)
*/
/* ************************************************************** */

#include <unistd.h>

extern const char *clan_name[];
extern char isWinMode;

#ifndef NEW
	#define NEW(type) ((type *)malloc((size_t) sizeof(type)))
#endif

#ifndef EOS			/* End Of String marker			 */
	#define EOS 0 // '\0'
#endif

#define dMarkChr 0x0f
#define sMarkChr 0x0e
#define fSpaceChr 0x10

#define rplChr '\001'
#define errChr '\002'

#define MXWDS 50		/* max number of words or Scopes could be
				   specified to be excluded or included  */

#define IEWORDS struct IEWords
IEWORDS {
	char *word;
	IEWORDS *nextword;
} ;

#define UTTER struct stutterance /* main utterance structure				*/
UTTER {
	char speaker[SPEAKERLEN];		/* code descriptor field of the turn	*/
	AttTYPE attSp[SPEAKERLEN];		/* Attributes assiciated with speaker name	*/
	char line[UTTLINELEN+1];		/* text field of the turn		*/ // found uttlinelen
	AttTYPE attLine[UTTLINELEN+1];	/* Attributes assiciated with text*/
	char tuttline[UTTLINELEN+1];	/* working copy of the text of the turn	*/
	long uttLen;
	long tlineno;				/* line number		*/
	UTTER *nextutt;		/* pointer to the next utterance, if	*/
} ;				/* there is only 1 utterance, i.e. no	*/
				/* windows, then if points to itself	*/
struct tier {			/* specifies which tier should be	*/
	char include;		/* included or exclude from the analyses*/
	char used;			/* if set this tier was found in the data*/
	char pat_match;		/* if =1 then PATMAT else PARTCMP match	*/
	char tcode[SPEAKERLEN];	/* the code descriptor			*/
	struct tier *nexttier;	/* points to the next tier		*/
} ;

struct IDtype {
	char ID[SPEAKERLEN];
	char inc;
	char used;
	struct IDtype *next_ID;
} ;

struct IDparts {
	char *lang;	// @Languages:, @Language of #:
	char *corp;	// such as: MacWhinney, Bates, Sachs, etc...
	char *code;	// @Participants: *CHI
	char *ages;	// @Age of #:
	char *sex;	// @Sex of #:
	char *group;// @Group of #:
	char *SES;	// @Ses of #:
	char *role;	// @Participants: Target_Child
	char *edu;	// @Education of #:
	char *UNQ;	// file name or other unique ID
} ;

#define NUM_PREF 3
#define NUM_SUFF 3
#define NUM_FUSI 4
#define NUM_ERRS 3

#define IXXS struct Ixes
struct Ixes {
	char free_ix_s;
	char *ix_s;
	char isIXMatch;
} ;

#define MORFEATS struct MorFeats
struct MorFeats {
	char *type;		char typeID;
	IXXS prefix[NUM_PREF];
	char *pos;
	char isPosMatch;
	char *stem;
	char isStemMatch;
	IXXS suffix[NUM_SUFF];
	IXXS fusion[NUM_FUSI];
	char free_trans;
	char *trans;
	char isTransMatch;
	char free_repls;
	char *repls;
	char isReplsMatch;
	IXXS error[NUM_ERRS];	//	char mError;
	MORFEATS *clitc;
	MORFEATS *compd;
} ;

#define MORFEATURESLIST struct MorFeaturesList
struct MorFeaturesList {
	char featType;
	char flag;
	char *pat;
	char typeID;
	MORFEATS *featMatched;
	MORFEATURESLIST *nextFeat;
} ;
#define MORWDLST struct MorWordList
struct MorWordList {
	char type;
	char isClitic;
	char numPref;
	char numPos;
	char isWildStem;
	char numStem;
	char numSuf;
	char numFus;
	char numTran;
	char numRepl;
	char numErr;
	MORFEATURESLIST *rootFeat;
	MORWDLST *nextMatch;
} ;

#define GRAWDLST struct GraWordList
GRAWDLST {
	char *word;
	char type;
	GRAWDLST *nextword;
} ;

struct mor_link_struct {
	char error_found;
	FNType fname[FNSize];
	long lineno;
} ;

#define MULTIWORDMAX 25
#define MULTIWORDCONTMAX 1024
#define MULTIWORDMATMAX 80
#define IEMWORDS struct IEMWords
IEMWORDS {
	char *word_arr[MULTIWORDMAX];
	MORWDLST *morwords_arr[MULTIWORDMAX];
	GRAWDLST *grafptr_arr[MULTIWORDMAX];
	char isMatch[MULTIWORDMAX];
	char context_arr[MULTIWORDMAX][MULTIWORDCONTMAX+2];
	char matched_word_arr[MULTIWORDMAX][MULTIWORDMATMAX+2];
	int  i;
	int  cnt;
	int  total;
	IEMWORDS *nextword;
} ;

struct aliasesList {
	char *alias;
	char *argv0;
	char *rest;
	char isPullDownC;
	char isNeedsArgs;
	char loc;
	int  ln;
	struct aliasesList *next_alias;
} ;
typedef struct aliasesList ALIASES_LIST;


extern const char *AddCEXExtension;
extern const char *delSkipedFile;	/* if +t@id="" option used and file didn't match, then delete output file */
extern char isFileSkipped;	/* TRUE - if +t@id="" option used and file didn't match */
extern char f_override;		/* if user specified -f option, then = 1 */
extern char combinput,  /* 1 - merge data from all specified file, dflt 0 */
			nomap,		/* 0 - convert all to lower case, dflt 0		*/
			nomain,		/* 1 - exclude text from main speaker turns,	0 */
			currentchar,/* contains last input character read			*/
			stout,		/* 0 - if the output is NOT stdout, dflt 1		*/
			stin;		/* 1 - if the input is stdin, dflt 0			*/
extern char GlobalPunctuation[];
extern int  postcodeRes;  /* reflects presence of postcodes 0-if non or 4,5,6-if found */
extern char rightspeaker; /* 1 - speaker/code turn is selected by the user*/
extern AttTYPE currentatt;/* contains attribute of the last input character read */
extern char R6;			/* if true then retraces </>,<//>,<///>,</->,</?> are included  */
extern char R6_override;/* includes all </>,<//>,<///>,</->,</?> regardless of other settings */
extern int  onlydata;	/* 1,2,3 - level of amount of data output, dflt 0 */
extern IEWORDS *wdptr;	/* contains words to included/excluded		  */
extern IEMWORDS *mwdptr;/* contains multi-words to included/excluded	*/
extern char WordMode;	/* 'i' - means to include wdptr words, else exclude*/
extern char *uttline;	/* pointer to a working version of the text line  */
extern UTTER *utterance;/* contains turn / turn claster information	  */
extern FILE *fpin,
			*fpout;	/* file pointers to input and output streams	  */
extern FNType *oldfname;
extern FNType newfname[];
extern FNType cMediaFileName[];

extern char org_spName[];
extern char org_spTier[];	/* used by createMorUttline function for compairing speaker to dep tier*/
extern char fDepTierName[];	/* used by createMorUttline function for compairing two dep tiers*/
extern char sDepTierName[];	/* used by createMorUttline function for compairing two dep tiers*/

extern char MBF, C_MBF;
extern char *rootmorf;
extern char FilterTier;
extern char LocalTierSelect;
extern char replaceFile;
extern char linkMain2Mor;
extern char linkMain2Sin;
extern char isCreateFakeMor;
extern char isMultiMorSearch,
			isMultiGraSearch;
extern int  chatmode,
			UttlineEqUtterance,
			OnlydataLimit;
extern long filterUttLen;
extern long WUCounter;	/* current word/utterance count number	   */
extern long lineno,		/* current turn line number (dflt 0)		  */
			tlineno,		/* line number within the current turn		  */
			deflineno;		/* default line number				  */

extern struct mor_link_struct mor_link;

extern MORWDLST *freeMorWords(MORWDLST *p);
extern MORWDLST *makeMorSeachList(MORWDLST *root, char *res, char *wd, char ch);

extern IEWORDS *InsertWord(IEWORDS *,IEWORDS *);
extern IEMWORDS *InsertMulti(IEMWORDS *mroot, char *st, char searchTier);
extern IEWORDS *freeIEWORDS(IEWORDS *ptr);
extern IEMWORDS *freeIEMWORDS(IEMWORDS *ptr);

extern FILE *openwfile(const FNType *, FNType *, FILE *);
extern FILE *OpenGenLib(const FNType *, const char *, char, char, FNType *);
extern FILE *OpenMorLib(const FNType *, const char *, char, char, FNType *);

extern int  exclude(char *);
extern int  excludedef(char *);
extern int  isExcludeScope(char *str);
extern int  isMorExcludePlus(char *w);
extern int  ExcludeScope(char *wline, int pos, char blankit);
extern int  getwholeutter(void);
extern int  checktier(char *);
extern int  CheckOutTier(char *s);
extern int  rightUttLen(const char *sp, char *line, char *tLine, long *cUttLen);
extern int  gettextspeaker(void);
extern int  getspeaker(char *, AttTYPE *att, register int index);
extern int  getrawkey(void);
extern int  checkkey(void);
extern int  getc_cr(FILE *fp, AttTYPE *att);
extern int  getmaincode(void);
extern int  getword(const char *sp, char *cleanLine, char *orgWord, int *wi, int i);
extern int  Get_File(FNType *filename, int index);
extern int  Get_Dir(FNType *dirname, int index);
extern int  isPostCodeFound(const char *sp, char *line);
extern int  isExcludePostcode(char *str);
extern int  getNextDepTierPair(char *cleanLine, char *morItem, char *spWord, int *wi, int i);
extern int  ml_isclause(void);
extern int  ml_UtterClause(char *line, int pos);
extern int  expandX(char *ch, AttTYPE *att, int x, int index);

extern long DealWithAtts_cutt(char *line, long i, AttTYPE att, AttTYPE oldAtt);

extern float getTimeDuration(char *s);
extern float getPauseTimeDuration(char *s);

extern char isUttDel(char *s);
extern char isPostCodeOnUtt(char *line, const char *postcode);
extern char isAge(char *b, int *agef, int *aget);
extern char isMORSearch(void);
extern char isMorPatMatchedWord(MORWDLST *pats, char *word);
extern char isMorPat(char *f);
extern char isMorSearchOption(char *f, char spc, char ch);
extern char isWordFromMORTier(char *word);
extern char isWordFromGRATier(char *word);
extern char isGRASearch(void);
extern char isGraPat(char *f);
extern char isGRASearchOption(char *f, char spc, char ch);
extern char ParseWordMorElems(char *item, MORFEATS *feats);
extern char matchToMorElems(MORWDLST *pat_feat, MORFEATS *word_feats, char skipCompound, char skipClitic, char isClean);
extern char listtcs(char,char);
extern char *getfarg(char *, char *, int *);
extern char isIDSpeakerSpecified(char *IDTier, char *code, char isReportErr);
extern char *getCurrDirName(char *dir);
extern char *fgets_cr(char *beg, int size, FILE *fp);
extern char *mainflgs(void);
extern char cutt_SetNewFont(const char *st, char ec);
extern char chattest(char *, char, char *);
extern char BuildXMLTree(FILE *fpin);
extern char ParseXMLData(char *word, char isAddToWord);
extern char RemPercWildCard;
extern char getMediaTagInfo(char *line, long *Beg, long *End);
extern char getOLDMediaTagInfo(char *line, const char *tag, FNType *fname, long *Beg, long *End);
extern char isEqual(const char *pat, const char *st);
extern char isnEqual(const char *pat, const char *st, int len);
extern char isEqualIxes(const char *pat, IXXS *ixes, int max);
extern char isIxesMatchPat(IXXS *ixes, int max, const char *pat);
extern char isAllv(MORFEATS *feat);
extern char bmain(int argc, char *argv[], void (*pr_result)(void));

extern void IsSearchR7(char *w);
extern void IsSearchCA(char *w);
extern void freeUpFeats(MORFEATS *p);
extern void addIxesToSt(char *item, IXXS *ixes, int max, const char *sym, char isAddFront);
extern void freeUpIxes(IXXS *ixes, int max);
extern void cleanupGRAWord(char *word);
extern void processSPTier(char *spTier, char *line);
extern void filterMorTier(char *morUtt, char *morLine, char isReplace, char linkDepTier2OtherTier);
extern void setVocative(); // 04/30/2023 for Batchalign see cutt.cpp
extern void createMorUttline(char *new_mor_tier, char *spTier, const char *dcode, char *mor_tier, char isFilterSP, char linkTiers);
extern void cutt_cleanUpLine(const char *sp, char *ch, AttTYPE *att, int oIndex);
extern void getMediaName(char *line, FNType *tMediaFileName, long size);
extern void freeXML_Elements(void);
extern void printAtts(AttTYPE att, AttTYPE oldAtt, FILE *fp);
extern void ActualPrint(const char *, AttTYPE *, AttTYPE *, char, char, FILE *);
extern void printout(const char *, char *, AttTYPE *, AttTYPE *, char);
extern void printoutline(FILE *fp, char *line);
extern void printArg(char *argv[], int argc, FILE *fp, char specialCase, FNType *fname);
extern void freedefwdptr(char *);
extern void addword(char,char,const char *);
extern void rdexclf(char,char,const FNType *);
extern void mmaininit(void);
extern void main_cleanup(void);
extern void no_arg_option(char *f);
extern void maininitwords(void);
extern void mor_initwords(void);
extern void gra_initwords(void);
extern void maketierchoice(const char *,char,char);
extern void CleanUpTempIDSpeakers(void);
extern void MakeOutTierChoice(char *ts, char inc);
extern void mainusage(char isQuit);
extern void FOption(char *);
extern void maingetflag(char *, char *, int *);
extern void parsfname(FNType *, FNType *, const char *);
extern void CurTierSearch(char *);
extern void cutt_getline(const char *, char *, AttTYPE *, register int);
extern void killline(char *line, AttTYPE *atts);
extern void filterwords(const char *,char *,int (*)(char *));
extern void filterData(const char *,char *);
extern void remove_main_tier_print(const char *sp, char *line, AttTYPE *att);
extern void out_of_mem(void);
extern void makewind(int);
extern void changeBullet(char *line, AttTYPE *att);
extern void befprintout(char isChangeBullets);
extern void aftprintout(char isChangeBullets);
extern void init_punct(char which);
extern void displayOoption(void);
extern void checkOptions(char *st);
extern void clean_s_option(void);
extern void remove_CRs_Tabs(char *line);
extern void HandleParans(char *s, int beg, int end);
extern void combineMainDepWords(char *line, char isColor);
extern void removeBlankPairs(char *line);
extern void removeMainTierWords(char *line);
extern void removeDepTierItems(char *line);
extern void blankoSelectedSpeakers(char *line);
extern void findWholeWord(char *line, int wi, char *word);
extern void findWholeScope(char *line, int wi, char *word);
extern void cleanupMultiWord(char *st);
extern void Secs2Str(float tm, char *st, char isMsecs);
extern void RoundSecsStr(float tm, char *st);
extern void breakIDsIntoFields(struct IDparts *idTier, char *IDs);
extern void cleanUpIDTier(char *line);
extern void textToXML(char *an, const char *bs, const char *es);

// Excel Output BEGIN
enum {
	ExcelRowStart = 1,
	ExcelRowEnd,
	ExcelRowEmpty,
	ExcelBlkCell,
	ExcelRedCell
} ;
extern void excelTextToXML(char *an, const char *bs, const char *es);
extern void excelHeader(FILE *fp, char *name, int colSize);
extern void excelFooter(FILE *fp);
extern void excelCommasStrCell(FILE *fp, const char *st);
extern void excelStrCell(FILE *fp, const char *st);
extern void excelRedStrCell(FILE *fp, const char *st);
extern void excelNumCell(FILE *fp, const char *format, float num);
extern void excelLongNumCell(FILE *fp, const char *format, long num);
extern void excelOutputID(FILE *fp, char *IDtier);
extern void excelRow(FILE *fp, int type);
extern void excelHeightRowStart(FILE *fp, int height);
extern void excelRowOneStrCell(FILE *fp, int isRed, const char *st);
// Excel Output END
// OLD commas Excel Output BEGIN
extern void outputIDForExcel(FILE *fp, char *IDtier, short isComma);
extern void outputStringForExcel(FILE *fp, const char *st, short isComma);
// OLD commas Excel Output END

}
#endif /* CUDEF */
