/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/

#define CHAT_MODE 0

#include "cu.h"

#define IS_WIN_MODE FALSE
#include "mul.h" 

extern struct tier *defheadtier;
extern char OverWriteFile;
extern char cutt_isMultiFound;

#define UTF8_code  1
#define UTF16_code 2
#define ANSI_code  0

#define UMAC 1
#define UPC  2

#define TAGSLEN  128

struct TiersRec {
    char *sp;
    char *line;
	long beg;
	long end;
	struct TiersRec *sameSP;
	struct TiersRec *nextPraatTier;
} ;
typedef struct TiersRec P2C_TIERS;

struct PraatAtribsRec {
	char tag[TAGSLEN];
	char chatName[TAGSLEN];
	char depOn[TAGSLEN];
	struct PraatAtribsRec *nextTag;
} ;
typedef struct PraatAtribsRec ATTRIBS;

#define ALL_BGs struct BG_tiers
struct BG_tiers {
	char *GEMname;
	char *GEMtext;
	long beg, end;
	char isBGPrinted;
	char isEGPrinted;
	ALL_BGs *nextBG;
} ;

#define P2C_LINESEG struct P2C_LineBulletSegment
struct P2C_LineBulletSegment {
	char *text;
//	unsigned short wordPos;
	unsigned short fWordPos, lWordPos;
	long beg, end;
	P2C_LINESEG *nextSeg;
} ;

#define P2C_CHATTIERS struct ChatTiers
struct ChatTiers {
	char isWrap;
	char isOverlapOrig;
	long beg, end;
	unsigned short wordCnt;
	char *sp;
	P2C_LINESEG *lineSeg;
	P2C_LINESEG *lastLineSeg;
	P2C_CHATTIERS *depTiers;
	P2C_CHATTIERS *nextTier;
} ;

enum {
	SP_MODE = 1,
	TIER_MODE,
} ;

static char lang[TAGSLEN+1];
static char isMultiBullets;
static char mediaFName[FILENAME_MAX+2];
static P2C_CHATTIERS *RootTiers;
static ALL_BGs *RootBGs;
static ATTRIBS *attsRoot;
static unsigned short lEncode;

void usage() {
	printf("convert Praat TextGrid files to CHAT files\n");
	printf("Usage: praat2chat [b dF oS %s] filename(s)\n",mainflgs());
	puts("+b:  Specify that multiple bullets per line (default only one bullet per line).");
    puts("+dF: specify attribs/tags dependencies file F.");
	puts("+oS: Specify code page. Please type \"+o?\" for full listing of codes");
	puts("     utf8  - Unicode UTF-8");
	puts("     macl  - Mac Latin (German, Spanish ...)");
	puts("     pcl   - PC  Latin (German, Spanish ...)");
	mainusage(FALSE);
	puts("\nExample: praat2chat +b -opcl bysoc1ny-06-TEL.TextGrid");
	puts("\tpraat2chat +b -opcl +dpraat_attribs.cut odderny-05-HFU.TextGrid");
	cutt_exit(0);
}

void init(char s) {
	if (s) {
		OverWriteFile = TRUE;
		AddCEXExtension = ".cha";
		stout = FALSE;
		onlydata = 1;
		RootTiers = NULL;
		RootBGs = NULL;
		attsRoot = NULL;
		isMultiBullets = FALSE;
		strcpy(lang, "UNK");
		lEncode = 0;
		if (defheadtier) {
			if (defheadtier->nexttier != NULL)
				free(defheadtier->nexttier);
			free(defheadtier);
			defheadtier = NULL;
		}
	}
}

static ATTRIBS *freeAttribs(ATTRIBS *p) {
	ATTRIBS *t;
	
	while (p != NULL) {
		t = p;
		p = p->nextTag;
		free(t);
	}
	return(NULL);
}

static ALL_BGs *freeBGs(ALL_BGs *p) {
	ALL_BGs *t;
	
	while (p != NULL) {
		t = p;
		p = p->nextBG;
		if (t->GEMname != NULL)
			free(t->GEMname);
		if (t->GEMtext != NULL)
			free(t->GEMtext);
		free(t);
	}
	return(NULL);
}

static void freeLineSeg(P2C_LINESEG *p) {
	P2C_LINESEG *t;
	
	while (p != NULL) {
		t = p;
		p = p->nextSeg;
		if (t->text != NULL)
			free(t->text);
		free(t);
	}
}

static P2C_CHATTIERS *freeTiers(P2C_CHATTIERS *p) {
	P2C_CHATTIERS *t;
	
	while (p != NULL) {
		t = p;
		p = p->nextTier;
		if (t->depTiers != NULL)
			freeTiers(t->depTiers);
		if (t->sp != NULL)
			free(t->sp);
		if (t->lineSeg != NULL)
			freeLineSeg(t->lineSeg);
		free(t);
	}
	return(NULL);
}

static void freePraat2ChatMem(void) {
	attsRoot = freeAttribs(attsRoot);
	RootBGs = freeBGs(RootBGs);
	RootTiers = freeTiers(RootTiers);
}

static ATTRIBS *add_each_praatTag(ATTRIBS *root, const char *fname, long ln, const char *tag, const char *chatName, const char *depOn) {
	long len;
	ATTRIBS *p, *tp;
	
	if (tag[0] == EOS)
		return(root);
	
	if (root == NULL) {
		if ((p=NEW(ATTRIBS)) == NULL)
			out_of_mem();
		root = p;
	} else {
		for (p=root; p->nextTag != NULL && uS.mStricmp(p->tag, tag); p=p->nextTag) ;
		if (uS.mStricmp(p->tag, tag) == 0)
			return(root);
		
		if ((p->nextTag=NEW(ATTRIBS)) == NULL)
			out_of_mem();
		p = p->nextTag;
	}
	
	p->nextTag = NULL;
	if ((strlen(tag) >= TAGSLEN) || (strlen(depOn) >= TAGSLEN) || (strlen(chatName) >= TAGSLEN)) {
		freePraat2ChatMem();
		fprintf(stderr,"*** File \"%s\"", fname);
		if (ln != 0)
			fprintf(stderr,": line %ld.\n", ln);
		fprintf(stderr, "Tag(s) too long. Longer than %d characters\n", TAGSLEN);
		cutt_exit(0);
	}
	strcpy(p->tag, tag);
	strcpy(p->chatName, chatName);
	if (p->chatName[0] == '@' || p->chatName[0] == '*' || p->chatName[0] == '%') {
		len = strlen(p->chatName) - 1;
		if (p->chatName[len] != ':')
			strcat(p->chatName, ":");
	}
	if (depOn[0] != EOS && chatName[0] != '*') {
		for (tp=root; tp != NULL; tp=tp->nextTag) {
			if (uS.mStricmp(tp->tag, depOn) == 0 || uS.partcmp(tp->chatName, depOn, FALSE, FALSE))
				break;
		}
		if (tp == NULL) {
			freePraat2ChatMem();
			fprintf(stderr,"*** File \"%s\"", fname);
			if (ln != 0)
				fprintf(stderr,": line %ld.\n", ln);
			fprintf(stderr, " ** Can't match praat tag \"%s\" to any speaker declaration in attributes file.\n", depOn);
			cutt_exit(0);
		}
		strcpy(p->depOn, tp->chatName);
	} else
		strcpy(p->depOn, depOn);
	
	return(root);
}

static void rd_PraatAtts_f(const char *fname, char isFatalError) {
	int  cnt;
	const char *tag, *depOn, *chatName;
	char isQF;
	long i, j, ln;
	FNType mFileName[FNSize];
	FILE *fp;
	
	if (*fname == EOS) {
		fprintf(stderr,	"No dep. tags file specified.\n");
		cutt_exit(0);
	}
	if ((fp=OpenGenLib(fname,"r",TRUE,FALSE,mFileName)) == NULL) {
		fprintf(stderr, "\n    Warning: Not using any attribs file: \"%s\", \"%s\"\n\n", fname, mFileName);
		if (isFatalError)
			cutt_exit(0);
		else {
//			fprintf(stderr, "    Please specify attribs file with +d option.\n\n");
			return;
		}
	}
	ln = 0L;
	while (fgets_cr(templineC, 255, fp)) {
		if (uS.isUTF8(templineC) || uS.isInvisibleHeader(templineC))
			continue;
		ln++;
		if (templineC[0] == '#' || templineC[0] == ';')
			continue;
		uS.remblanks(templineC);
		if (templineC[0] == EOS)
			continue;
		i = 0;
		cnt = 0;
		tag = "";
		chatName = "";
		depOn = "";
		while (1) {
			isQF = 0;
			for (; isSpace(templineC[i]); i++) ;
			if (templineC[i] == '"' || templineC[i] == '\'') {
				isQF = templineC[i];
				i++;
			}
			for (j=i; (!isSpace(templineC[j]) || isQF) && templineC[j] != EOS; j++) {
				if (templineC[j] == isQF) {
					isQF = 0;
					strcpy(templineC+j, templineC+j+1);
					j--;
				}
			}
			if (cnt == 0)
				tag = templineC+i;
			else if (cnt == 1)
				chatName = templineC+i;
			else if (cnt == 2)
				depOn = templineC+i;
			if (templineC[j] == EOS)
				break;
			templineC[j] = EOS;
			i = j + 1;
			cnt++;
		}
		if (tag[0] == EOS || chatName[0] == EOS) {
			freePraat2ChatMem();
			fprintf(stderr,"*** File \"%s\": line %ld.\n", fname, ln);
			if (tag[0] == EOS)
				fprintf(stderr, "Missing Praat tag\n");
			else if (chatName[0] == EOS)
				fprintf(stderr, "Missing Chat tier name for \"%s\"\n", tag);
			cutt_exit(0);
		}
		if (depOn[0] == EOS && chatName[0] == '%') {
			freePraat2ChatMem();
			fprintf(stderr,"*** File \"%s\": line %ld.\n", fname, ln);
			fprintf(stderr, "Missing \"Associated with tag\" for \"%s\"\n", tag);
			cutt_exit(0);
		}
		attsRoot = add_each_praatTag(attsRoot, fname, ln, tag, chatName, depOn);
	}
	fclose(fp);
}

CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = PRAAT2CHAT;
	OnlydataLimit = 0;
	UttlineEqUtterance = FALSE;
	bmain(argc,argv,NULL);
	attsRoot = freeAttribs(attsRoot);
}

static void cleanup_lang(char *lang) {
	while (*lang) {
		if (isSpace(*lang) || *lang < (unCH)' ')
			strcpy(lang, lang+1);
		else
			lang++;
	}
	if (*(lang-1) == ',')
		strcpy(lang-1, lang);
}

void getflag(char *f, char *f1, int *i) {
	f++;
	switch(*f++) {
		case 'b':
			isMultiBullets = TRUE;
			no_arg_option(f);
			break;
		case 'd':
			if (*f) {
				rd_PraatAtts_f(getfarg(f,f1,i), TRUE);
			} else {
				fprintf(stderr,"Missing argument to option: %s\n", f-2);
				cutt_exit(0);
			}
			break;
		case 'l':
			if (*f) {
				strncpy(lang, getfarg(f,f1,i), TAGSLEN);
				lang[TAGSLEN] = EOS;
				cleanup_lang(lang);
			} else {
				fprintf(stderr,"Missing argument to option: %s\n", f-2);
				cutt_exit(0);
			}
			break;
		case 'o':
			fprintf(stderr,"Illegal option: %s\n", f-2);
			cutt_exit(0);
			break;
		default:
			maingetflag(f-2,f1,i);
			break;
	}
}

static ALL_BGs *addBGs(ALL_BGs *lastBgTier, long beg, long end, char *tag, char *line) {
	char isFirstTry;
	ALL_BGs *nt, *tnt;

	if (RootBGs == NULL) {
		if ((RootBGs=NEW(ALL_BGs)) == NULL)
			out_of_mem();
		nt = RootBGs;
		nt->nextBG = NULL;
		tnt= RootBGs;
	} else {
repeat_add_bg:
		if (lastBgTier != RootBGs)
			isFirstTry = TRUE;
		else
			isFirstTry = FALSE;
		tnt= lastBgTier;
		nt = lastBgTier;
		while (nt != NULL) {
			if (beg == nt->beg && end < nt->end)
				break;
			else if (beg < nt->beg)
				break;
			tnt = nt;
			nt = nt->nextBG;
		}

		if (isFirstTry && (nt == tnt || beg < tnt->beg)) {
			lastBgTier = RootBGs;
			goto repeat_add_bg;
		}

		if (nt == NULL) {
			tnt->nextBG = NEW(ALL_BGs);
			if (tnt->nextBG == NULL)
				out_of_mem();
			nt = tnt->nextBG;
			nt->nextBG = NULL;
		} else if (nt == RootBGs) {
			RootBGs = NEW(ALL_BGs);
			if (RootBGs == NULL)
				out_of_mem();
			RootBGs->nextBG = nt;
			nt = RootBGs;
			tnt= RootBGs;
		} else {
			nt = NEW(ALL_BGs);
			if (nt == NULL)
				out_of_mem();
			nt->nextBG = tnt->nextBG;
			tnt->nextBG = nt;
		}
	}

	nt->isBGPrinted = FALSE;
	nt->isEGPrinted = FALSE;
	nt->beg = beg;
	nt->end = end;
	nt->GEMname = (char *)malloc(strlen(tag)+1);
	if (nt->GEMname == NULL)
		out_of_mem();
	strcpy(nt->GEMname, tag);
	nt->GEMtext = (char *)malloc(strlen(line)+1);
	if (nt->GEMtext == NULL)
		out_of_mem();
	strcpy(nt->GEMtext, line);
	return(tnt);
}

static void Praat_addLineSeg(P2C_CHATTIERS *nt, long beg, long end, const char *line, char isAddBullet) {
	P2C_LINESEG *seg;

	if (nt->lineSeg == NULL) {
		if ((nt->lineSeg=NEW(P2C_LINESEG)) == NULL)
			out_of_mem();
		seg = nt->lineSeg;
		nt->lastLineSeg = seg;
	} else {
//		for (seg=nt->lineSeg; seg->nextSeg != NULL; seg=seg->nextSeg) ;
		seg = nt->lastLineSeg;
		if ((seg->nextSeg=NEW(P2C_LINESEG)) == NULL)
			out_of_mem();
		seg = seg->nextSeg;
		nt->lastLineSeg = seg;
	}
	seg->nextSeg = NULL;
//	seg->wordPos = 0;
	seg->fWordPos = 0;
	seg->lWordPos = 0;
	seg->beg = beg;
	seg->end = end;

	if (isAddBullet && (beg != 0L || end != 0L))
		sprintf(templineC3, " %c%ld_%ld%c", HIDEN_C, beg, end, HIDEN_C);
	else
		templineC3[0] = EOS;

	seg->text = (char *)malloc(strlen(line)+strlen(templineC3)+1);
	if (seg->text == NULL)
		out_of_mem();
	strcpy(seg->text, line);
	if (templineC3[0] != EOS)
		strcat(seg->text, templineC3);
}

static void Praat_fillNT(P2C_CHATTIERS *nt, long beg, long end, const char *sp, const char *line, char isAddBullet) {
	nt->depTiers = NULL;
	nt->isWrap = TRUE;
	nt->sp = (char *)malloc(strlen(sp)+1);
	if (nt->sp == NULL) out_of_mem();
	strcpy(nt->sp, sp);
	nt->lineSeg = NULL;
	Praat_addLineSeg(nt, beg, end, line, isAddBullet);
	nt->beg = beg;
	nt->end = end;
}

static P2C_CHATTIERS *Praat_addDepTiers(P2C_CHATTIERS *depTiers, long beg, long end, char *sp, char *line, char isAddBullet) {
	P2C_CHATTIERS *nt, *tnt;

	if (depTiers == NULL) {
		if ((depTiers=NEW(P2C_CHATTIERS)) == NULL)
			out_of_mem();
		nt = depTiers;
		nt->nextTier = NULL;
	} else {
		tnt= depTiers;
		nt = depTiers;
		while (nt != NULL) {
			if (!strcmp(nt->sp, sp)) {
				if (end > nt->end)
					nt->end = end;
				if (beg < nt->beg)
					nt->beg = beg;
				Praat_addLineSeg(nt, beg, end, line, isAddBullet);
				return(depTiers);
			}
			if (beg < nt->beg)
				break;
			tnt = nt;
			nt = nt->nextTier;
		}
		if (nt == NULL) {
			tnt->nextTier = NEW(P2C_CHATTIERS);
			if (tnt->nextTier == NULL)
				out_of_mem();
			nt = tnt->nextTier;
			nt->nextTier = NULL;
		} else if (nt == depTiers) {
			depTiers = NEW(P2C_CHATTIERS);
			if (depTiers == NULL)
				out_of_mem();
			depTiers->nextTier = nt;
			nt = depTiers;
		} else {
			nt = NEW(P2C_CHATTIERS);
			if (nt == NULL)
				out_of_mem();
			nt->nextTier = tnt->nextTier;
			tnt->nextTier = nt;
		}
	}

	Praat_fillNT(nt, beg, end, sp, line, isAddBullet);
	return(depTiers);
}

static P2C_CHATTIERS *Praat_insertNT(P2C_CHATTIERS *nt, P2C_CHATTIERS *tnt) {
	if (nt == RootTiers) {
		RootTiers = NEW(P2C_CHATTIERS);
		if (RootTiers == NULL)
			out_of_mem();
		RootTiers->nextTier = nt;
		nt = RootTiers;
	} else {
		nt = NEW(P2C_CHATTIERS);
		if (nt == NULL)
			out_of_mem();
		nt->nextTier = tnt->nextTier;
		tnt->nextTier = nt;
	}
	return(nt);
}

static char Praat_foundUttDel(P2C_LINESEG *seg) {
	char bullet;
	long i;

//	if (isMultiBullets)
//		return(FALSE);

	for (; seg != NULL; seg=seg->nextSeg) {
		i = 0L;
		if (seg->text[i] == HIDEN_C)
			bullet = TRUE;
		else
			bullet = FALSE;
		for (; seg->text[i] != EOS; i++) {
			if (uS.IsUtteranceDel(seg->text, i) && !bullet)
				return(TRUE);

			if (seg->text[i] == HIDEN_C)
				bullet = !bullet;
		}
	}
	return(FALSE);
}

static char Praat_isPostCodes(char *line) {
	char bullet;
	int  sq;
	long i;

	sq = 0;
	bullet = FALSE;
	i = strlen(line) - 1;
	for (; i >= 0L && (isSpace(line[i]) || isdigit(line[i]) || line[i] == '#' || line[i] == '_' || bullet || sq); i--) {
		if (line[i] == HIDEN_C)
			bullet = !bullet;
		else if (line[i] == ']')
			sq++;
		else if (line[i] == '[') {
			sq--;
			if (line[i+1] != '+')
				return(FALSE);
		}
	}
	if (i < 0L)
		return(TRUE);
	else
		return(FALSE);
}

static long findEndTime(P2C_CHATTIERS *nt, long beg, long end, char *sp) {
	while (nt != NULL && end > nt->beg) {
		if (!strcmp(sp, nt->sp))
			return(nt->beg);
		nt = nt->nextTier;
	}
	return(end);
}

static P2C_CHATTIERS *Praat_addToTiers(P2C_CHATTIERS *lastSpTier, long beg, long end, char *sp, char *refSp, char *line, char isAddBullet) {
	char isPostCodeFound, isFirstTry;
	long tTime;
	P2C_CHATTIERS *nt, *tnt;

	if (RootTiers == NULL) {
		if ((RootTiers=NEW(P2C_CHATTIERS)) == NULL)
			out_of_mem();
		nt = RootTiers;
		nt->nextTier = NULL;
		tnt= RootTiers;
	} else {
		isPostCodeFound = Praat_isPostCodes(line);
repeat_add_tier:
		if (lastSpTier != RootTiers)
			isFirstTry = TRUE;
		else
			isFirstTry = FALSE;
		tnt= lastSpTier;
		nt = lastSpTier;
		while (nt != NULL) {
			if (sp[0] == '%') {
				if (beg >= nt->beg && beg < nt->end && uS.partcmp(refSp, nt->sp, FALSE, TRUE)) {
					nt->depTiers = Praat_addDepTiers(nt->depTiers, beg, end, sp, line,isAddBullet);
					return(tnt);
				} else if (end-beg < 3 && beg >= nt->beg && beg <= nt->end && uS.partcmp(refSp, nt->sp, FALSE, TRUE)) {
					nt->depTiers = Praat_addDepTiers(nt->depTiers, beg, end, sp, line, isAddBullet);
					return(tnt);
				} else if (beg < nt->beg) {
					tTime = findEndTime(nt, beg, end, refSp);
					nt = Praat_insertNT(nt, tnt);
					Praat_fillNT(nt, beg, tTime, refSp, "0.", isAddBullet);
					nt->depTiers = Praat_addDepTiers(nt->depTiers, beg, end, sp, line, isAddBullet);
					return(tnt);
				}
			} else if (sp[0] == '*' && beg >= nt->end && beg <= nt->end+50 && !strcmp(sp, nt->sp) && !Praat_foundUttDel(nt->lineSeg)) {
				nt->end = end;
				Praat_addLineSeg(nt, beg, end, line, isAddBullet);
				return(tnt);
			} else if (sp[0] == '*' && beg == nt->end && !strcmp(sp, nt->sp) && isPostCodeFound) {
				nt->end = end;
				Praat_addLineSeg(nt, beg, end, line, isAddBullet);
				return(tnt);
			} else if (sp[0] == '*' && beg < nt->beg)
				break;
			tnt = nt;
			nt = nt->nextTier;
		}
		if (isFirstTry && (nt == tnt || beg < tnt->beg)) {
			lastSpTier = RootTiers;
			goto repeat_add_tier;
		}
		if (nt == NULL) {
			tnt->nextTier = NEW(P2C_CHATTIERS);
			if (tnt->nextTier == NULL)
				out_of_mem();
			nt = tnt->nextTier;
			nt->nextTier = NULL;
			if (sp[0] == '%') {
				Praat_fillNT(nt, beg, end, refSp, "0.", isAddBullet);
				nt->depTiers = Praat_addDepTiers(nt->depTiers, beg, end, sp, line, isAddBullet);
				return(tnt);
			}
		} else 
			nt = Praat_insertNT(nt, tnt);
	}

	Praat_fillNT(nt, beg, end, sp, line, isAddBullet);
	return(tnt);
}

static char Praat_isOverlapFound(P2C_LINESEG *seg, char ovChar) {
	long i;

	for (; seg != NULL; seg=seg->nextSeg) {
		for (i=0L; seg->text[i] != EOS; i++) {
			if (seg->text[i] == (char)0xe2 && seg->text[i+1] == (char)0x8c && seg->text[i+2] == (char)ovChar)
				return(TRUE);
		}
	}
	return(FALSE);
}

static void addBegOverlap(P2C_LINESEG *seg_s, P2C_LINESEG *seg_nt, P2C_CHATTIERS *s_nt, P2C_CHATTIERS *nt) {
	char *s;

	for (s=seg_s->text; isSpace(*s) && *s != EOS; s++)
		;
//	if ((unsigned char)*s == 0xe2 && (unsigned char)*(s+1) == 0x8c && (unsigned char)*(s+2) == 0x88)
//		;
//	else {
		s = seg_s->text;
		seg_s->text = (char *)malloc(strlen(s)+5);
		if (seg_s->text == NULL) {
			seg_s->text = s;
		} else {
			s_nt->isOverlapOrig = FALSE;
			seg_s->text[0] = (char)0xe2;
			seg_s->text[1] = (char)0x8c;
			seg_s->text[2] = (char)0x88;
			seg_s->text[3] = ' ';
			seg_s->text[4] = EOS;
			strcat(seg_s->text, s);
			free(s);
		}
//	}

	for (s=seg_nt->text; isSpace(*s) && *s != EOS; s++)
		;
//	if ((unsigned char)*s == 0xe2 && (unsigned char)*(s+1) == 0x8c && (unsigned char)*(s+2) == 0x8a)
//		;
//	else {
		s = seg_nt->text;
		seg_nt->text = (char *)malloc(strlen(s)+5);
		if (seg_nt->text == NULL) {
			seg_nt->text = s;
		} else {
			nt->isOverlapOrig = FALSE;
			seg_nt->text[0] = (char)0xe2;
			seg_nt->text[1] = (char)0x8c;
			seg_nt->text[2] = (char)0x8a;
			seg_nt->text[3] = ' ';
			seg_nt->text[4] = EOS;
			strcat(seg_nt->text, s);
			free(s);
		}
//	}
}

static void addEndOverlap(P2C_LINESEG *seg_s, P2C_LINESEG *seg_nt, P2C_CHATTIERS *s_nt, P2C_CHATTIERS *nt) {
	int  i;
	char *s, bf;

	s = seg_s->text;
	bf = FALSE;
	for (i=strlen(s); (isSpace(s[i]) || s[i] == EOS || s[i] == HIDEN_C || bf) && i >= 0; i--) {
		if (s[i] == HIDEN_C)
			bf = !bf;
	}
//	if (i >= 2 && (unsigned char)s[i-2] == 0xe2 && (unsigned char)s[i-1] == 0x8c && (unsigned char)s[i] == 0x89)
//		;
//	else {
		i++;
		seg_s->text = (char *)malloc(strlen(s)+5);
		if (seg_s->text == NULL) {
			seg_s->text = s;
		} else {
			s_nt->isOverlapOrig = FALSE;
			strcpy(seg_s->text, s);
			free(s);
			uS.shiftright(seg_s->text+i, 4);
			seg_s->text[i++] = ' ';
			seg_s->text[i++] = (char)0xe2;
			seg_s->text[i++] = (char)0x8c;
			seg_s->text[i++] = (char)0x89;
		}
//	}

	s = seg_nt->text;
	bf = FALSE;
	for (i=strlen(s); (isSpace(s[i]) || s[i] == EOS || s[i] == HIDEN_C || bf) && i >= 0; i--) {
		if (s[i] == HIDEN_C)
			bf = !bf;
	}
//	if (i >= 2 && (unsigned char)s[i-2] == 0xe2 && (unsigned char)s[i-1] == 0x8c && (unsigned char)s[i] == 0x8b)
//		;
//	else {
		i++;
		seg_nt->text = (char *)malloc(strlen(s)+5);
		if (seg_nt->text == NULL) {
			seg_nt->text = s;
		} else {
			nt->isOverlapOrig = FALSE;
			strcpy(seg_nt->text, s);
			free(s);
			uS.shiftright(seg_nt->text+i, 4);
			seg_nt->text[i++] = ' ';
			seg_nt->text[i++] = (char)0xe2;
			seg_nt->text[i++] = (char)0x8c;
			seg_nt->text[i++] = (char)0x8b;
		}
//	}
}

static void addOverlaps(P2C_CHATTIERS *s_nt, P2C_CHATTIERS *e_nt) {
	P2C_CHATTIERS *nt;
	P2C_LINESEG *seg_s, *seg_nt, *lseg_s, *lseg_nt;

	for (nt=s_nt->nextTier; nt != e_nt; nt=nt->nextTier) {
		seg_s  = s_nt->lineSeg;
		seg_nt = nt->lineSeg;
		while (seg_s != NULL && seg_nt != NULL) {
			if (!isMultiBullets && seg_s->beg == seg_nt->beg) {
				addBegOverlap(seg_s, seg_nt, s_nt, nt);
				break;
			} else if (isMultiBullets &&
						((seg_s->beg >= seg_nt->beg && seg_s->beg < seg_nt->end) || 
						 (seg_nt->beg >= seg_s->beg && seg_nt->beg < seg_s->end))) {
				addBegOverlap(seg_s, seg_nt, s_nt, nt);
				break;
			} else if (seg_s->beg < seg_nt->beg) {
				seg_s = seg_s->nextSeg;
			} else if (seg_s->beg > seg_nt->beg) {
				seg_nt = seg_nt->nextSeg;
			}
		}

		lseg_s = lseg_nt = NULL;
		seg_s  = s_nt->lineSeg;
		seg_nt = nt->lineSeg;
		while (seg_s != NULL && seg_nt != NULL) {
			if (!isMultiBullets && seg_s->end == seg_nt->end) {
				lseg_s  = seg_s;
				lseg_nt = seg_nt;
				seg_s = seg_s->nextSeg;
				seg_nt = seg_nt->nextSeg;
			} else if (isMultiBullets &&
						((seg_s->end > seg_nt->beg && seg_s->end <= seg_nt->end) || 
						 (seg_nt->end > seg_s->beg && seg_nt->end <= seg_s->end))) {
				lseg_s  = seg_s;
				lseg_nt = seg_nt;
				if (seg_s->end == seg_nt->end) {
					seg_s = seg_s->nextSeg;
					seg_nt = seg_nt->nextSeg;
				} else {
					if (/*seg_s->nextSeg != NULL  && */seg_s->end < seg_nt->end)
						seg_s = seg_s->nextSeg;
					else if (/*seg_nt->nextSeg != NULL && */seg_s->end > seg_nt->end)
						seg_nt = seg_nt->nextSeg;
				}
			} else if (seg_s->end < seg_nt->end) {
				seg_s = seg_s->nextSeg;
			} else if (seg_s->end > seg_nt->end) {
/*
				if (seg_nt->nextSeg == NULL) {
					if (lseg_nt != NULL && lseg_s != NULL && lseg_s->end < seg_nt->end && seg_nt->end != seg_s->beg) {
						lseg_s  = seg_s;
						lseg_nt = seg_nt;
					}
				}
*/
				seg_nt = seg_nt->nextSeg;
			}
		}
		if (lseg_s != NULL && lseg_nt != NULL)
			addEndOverlap(lseg_s, lseg_nt, s_nt, nt);
	}
}

#define timeMatch(sp,dep) (sp->beg >= dep->beg-1 && sp->beg <= dep->beg+1 && sp->end >= dep->end-1 && sp->end <= dep->end+1)

static char findSpMatch(P2C_LINESEG *mwO, P2C_LINESEG *dw) {
	char isOneToOne;
	unsigned short wordCnt;
	P2C_LINESEG *mw;
	
	isOneToOne = FALSE; // TRUE;
	for (; dw != NULL; dw=dw->nextSeg) {
		wordCnt = 0;
		for (mw=mwO; mw != NULL; mw=mw->nextSeg) {
			wordCnt++;
			if (timeMatch(mw, dw)) {
/*
				if (isOneToOne)
					dw->wordPos = wordCnt;
				else {
*/
					dw->fWordPos = wordCnt;
					dw->lWordPos = wordCnt;
//				}
			} else if (mw->beg >= dw->beg-1 && mw->beg <= dw->beg+1) {
//				isOneToOne = FALSE;
				dw->fWordPos = wordCnt;
			} else if (mw->end >= dw->end-1 && mw->end <= dw->end+1) {
//				isOneToOne = FALSE;
				dw->lWordPos = wordCnt;
			} else if (mw->nextSeg == NULL && mw->end <= dw->end)
				dw->lWordPos = wordCnt;
		}
		if (dw->fWordPos != 0 && dw->lWordPos == 0)
			dw->fWordPos = 0;
		else if (dw->fWordPos == 0 && dw->lWordPos != 0)
			dw->lWordPos = 0;
	}
	return(isOneToOne);
}

static void matchDepTierToSpeakerTier(P2C_CHATTIERS *nt) {
	char isOneToOne;
	unsigned short wordCnt;
	P2C_CHATTIERS *depT;
	P2C_LINESEG *dw;

	wordCnt = 0;
	for (dw=nt->lineSeg; dw != NULL; dw=dw->nextSeg)
		wordCnt++;

	nt->wordCnt = wordCnt;
	for (depT=nt->depTiers; depT != NULL; depT = depT->nextTier) {
		depT->wordCnt = wordCnt;
		if (depT->lineSeg != NULL && timeMatch(nt, depT->lineSeg) && depT->lineSeg->nextSeg == NULL)
			;
		else {
			isOneToOne = findSpMatch(nt->lineSeg, depT->lineSeg);
/*
			if (!isOneToOne) {
				for (dw=depT->lineSeg; dw != NULL; dw=dw->nextSeg) {
					if (dw->fWordPos == 0 && dw->lWordPos == 0 && dw->wordPos != 0) {
						dw->fWordPos = dw->wordPos;
						dw->lWordPos = dw->wordPos;
						dw->wordPos = 0;
					}
				}
			}
*/
		}
	}
}

static void PropagateCode(P2C_CHATTIERS *cur_nt, char *tag, P2C_LINESEG *lineSeg, P2C_CHATTIERS *nt) {
	char ftime = TRUE, *s;
	long end, tempEnd;

	s = lineSeg->text;
	end = lineSeg->end;
	while (nt != NULL && end > nt->beg) {
		if (!strcmp(cur_nt->sp, nt->sp)) {
			if (ftime) {
				ftime = FALSE;
				lineSeg->end = cur_nt->end;
				strcpy(templineC4, "$b ");
				strcat(templineC4, s);
				lineSeg->text = (char *)malloc(strlen(templineC4)+1);
				if (lineSeg->text == NULL)
					out_of_mem();
				strcpy(lineSeg->text, templineC4);
			}

			if (end > nt->end) {
				tempEnd = nt->end;
				strcpy(templineC4, "$c ");
				strcat(templineC4, s);
			} else {
				tempEnd = end;
				strcpy(templineC4, "$e ");
				strcat(templineC4, s);
			}
			nt->depTiers = Praat_addDepTiers(nt->depTiers, nt->beg, tempEnd, tag, templineC4, FALSE);
		}
		nt = nt->nextTier;
	}

	if (!ftime && s != NULL)
		free(s);
}

static void joinConsecutive(void) {
	char doJoin, isBgEgBreak;
	ALL_BGs *bg, *eg;
	P2C_CHATTIERS *nt, *prev_nt;
	P2C_LINESEG *lineSeg;
	P2C_CHATTIERS *depTier;

	for (bg=RootBGs; bg != NULL; bg=bg->nextBG) {
		bg->isBGPrinted = FALSE;
		bg->isEGPrinted = FALSE;
	}

	bg = RootBGs;
	nt = RootTiers;
	prev_nt = nt;
	while (nt != NULL) {
		isBgEgBreak = FALSE;
		if (!isBgEgBreak) {
			for (eg=RootBGs; eg != NULL; eg=eg->nextBG) {
				if (!eg->isEGPrinted && eg->end <= nt->beg) {
					eg->isEGPrinted = TRUE;
					isBgEgBreak = TRUE;
				}
			}
		}
		if (nt->nextTier != NULL && bg != NULL && bg->end <= nt->nextTier->beg && bg->beg > nt->beg) {
			for (; bg != NULL && bg->beg < nt->end; bg=bg->nextBG) {
				bg->isBGPrinted = TRUE;
				isBgEgBreak = TRUE;
			}
		} else {
			for (; bg != NULL && bg->beg <= nt->beg; bg=bg->nextBG) {
				bg->isBGPrinted = TRUE;
				isBgEgBreak = TRUE;
			}
		}
		if (prev_nt != nt) {
			if (!isBgEgBreak && !strcmp(prev_nt->sp, nt->sp)) {
				doJoin = FALSE;
				for (depTier=prev_nt->depTiers; depTier != NULL; depTier=depTier->nextTier) {
					if (depTier->end > nt->beg) {
						doJoin = TRUE;
						break;
					}
				}
				if (doJoin) {
					prev_nt->end = nt->end;
					if (prev_nt->depTiers == NULL)
						prev_nt->depTiers = nt->depTiers;
					else {
						for (depTier=prev_nt->depTiers; depTier->nextTier != NULL; depTier=depTier->nextTier) ;
						depTier->nextTier = nt->depTiers;
					}
					if (prev_nt->lineSeg == NULL)
						prev_nt->lineSeg = nt->lineSeg;
					else {
						for (lineSeg=prev_nt->lineSeg; lineSeg->nextSeg != NULL; lineSeg=lineSeg->nextSeg) {
							if (!strncmp(lineSeg->text, "0.", 2) && lineSeg->nextSeg != NULL) {
								lineSeg->text[0] = '(';
								lineSeg->text[1] = '.';
								lineSeg->text[2] = ')';
								lineSeg->text[3] = ' ';
							}
						}
						lineSeg->nextSeg = nt->lineSeg;
						for (; lineSeg != NULL; lineSeg=lineSeg->nextSeg) {
							if (!strncmp(lineSeg->text, "0.", 2)) {
								if (lineSeg == prev_nt->lineSeg && prev_nt->beg == nt->beg && prev_nt->end == nt->end) {
									lineSeg->text[0] = EOS;
								} else {
									lineSeg->text[0] = '(';
									lineSeg->text[1] = '.';
									lineSeg->text[2] = ')';
									lineSeg->text[3] = ' ';
								}
							}
						}
					}
					prev_nt->nextTier = nt->nextTier;
					if (nt->sp != NULL)
						free(nt->sp);
					free(nt);
					nt = prev_nt;
				}
			} else {
				for (depTier=prev_nt->depTiers; depTier != NULL; depTier=depTier->nextTier) {
					if (depTier->end > nt->beg) {
						for (lineSeg=depTier->lineSeg; lineSeg != NULL; lineSeg=lineSeg->nextSeg) {
							if (lineSeg->end > nt->beg)
								PropagateCode(prev_nt, depTier->sp, lineSeg, nt);
						}
					}
				}
			}
		}
		prev_nt = nt;
		nt = nt->nextTier;
	}
}

static void Praat_finalTimeSort(void) {
	char isWrongOverlap;
	P2C_CHATTIERS *nt, *prev_nt, *prev_prev_nt;

	prev_nt = RootTiers;
	if (prev_nt != NULL) {
		nt = RootTiers->nextTier;
		if (nt != NULL && nt->sp[0] == '*' && prev_nt->sp[0] == '%') {
			if (nt->beg <= prev_nt->beg && nt->end >= prev_nt->beg) {
				prev_nt->nextTier = NULL;
				nt->depTiers = prev_nt;
				RootTiers = nt;
			}
		}
	}
	nt = RootTiers;
	prev_nt = nt;
	prev_prev_nt = nt;
	while (nt != NULL) {
		if (prev_nt != nt) {
			if (nt->beg == prev_nt->beg && nt->end < prev_nt->end) {
				prev_nt->nextTier = nt->nextTier;
				nt->nextTier = prev_nt;
				if (prev_prev_nt == RootTiers) {
					RootTiers = nt;
					prev_prev_nt = RootTiers;
				} else
					prev_prev_nt->nextTier = nt;
				prev_nt = prev_prev_nt;
				nt = prev_nt->nextTier;
			} else if (nt->beg == prev_nt->beg && nt->end == prev_nt->end) {
				isWrongOverlap = 0;
				if (Praat_isOverlapFound(nt->lineSeg, (char)0x88) && !Praat_isOverlapFound(nt->lineSeg, (char)0x8a))
					isWrongOverlap++;
				if (isWrongOverlap == 1) {
					if (Praat_isOverlapFound(prev_nt->lineSeg, (char)0x8a) && !Praat_isOverlapFound(prev_nt->lineSeg, (char)0x88))
						isWrongOverlap++;
				}
				if (isWrongOverlap == 2) {
					prev_nt->nextTier = nt->nextTier;
					nt->nextTier = prev_nt;
					if (prev_prev_nt == RootTiers) {
						RootTiers = nt;
						prev_prev_nt = RootTiers;
					} else
						prev_prev_nt->nextTier = nt;
					prev_nt = prev_prev_nt;
					nt = prev_nt->nextTier;
					if (nt == NULL)
						break;
				}
			}
		}
		prev_prev_nt = prev_nt;
		prev_nt = nt;
		nt = nt->nextTier;
	}
	joinConsecutive();
	nt = RootTiers;
	while (nt != NULL) {
		nt->isOverlapOrig = TRUE;
		matchDepTierToSpeakerTier(nt);
		nt = nt->nextTier;
	}
	prev_nt = RootTiers;
	while (prev_nt != NULL) {
		isWrongOverlap = 0;
		if (Praat_isOverlapFound(prev_nt->lineSeg, (char)0x8a) || Praat_isOverlapFound(prev_nt->lineSeg, (char)0x88)) {
			if (prev_nt->isOverlapOrig)
				isWrongOverlap = 1;
		}
		for (nt=prev_nt->nextTier; nt != NULL; nt = nt->nextTier) {
			if (nt->beg >= prev_nt->end)
				break;
		}
		if (isWrongOverlap) {
			prev_nt = nt;
		} else {
			if (prev_nt != nt && prev_nt->nextTier != NULL)
				addOverlaps(prev_nt, nt);
			prev_nt = prev_nt->nextTier;
		}
	}
}

static void Praat_createLineFromSegs(char *line, P2C_CHATTIERS *nt, char whichLevel) {
	char s[512];
//	char isOneToOne;
	long i, lastEnd;
	P2C_LINESEG *seg;
//	P2C_CHATTIERS *depT;
//	unsigned short wordPos;

	if (isMultiBullets) {
		nt->isWrap = TRUE;
	} else {
		nt->isWrap = FALSE;
	}
//	isOneToOne = FALSE;
//	wordPos = 1;
	seg = nt->lineSeg;
	if (seg != NULL)
		lastEnd = seg->end;
	else
		lastEnd = 0L;
	line[0] = EOS;
	for (; seg != NULL; seg=seg->nextSeg) {
		if (seg->fWordPos != 0 || seg->lWordPos != 0) {
			if (seg->fWordPos == seg->lWordPos)
				sprintf(s, "<%dw> ", seg->fWordPos);
			else
				sprintf(s, "<%dw-%dw> ", seg->fWordPos, seg->lWordPos);
			strcat(line, s);
		}
/*		 else if (seg->wordPos != 0) {
			isOneToOne = TRUE;
			for (; wordPos < seg->wordPos; wordPos++)
				strcat(line, "0 ");
			wordPos++;
		}
*/
		if (whichLevel == 0) {
			if (lastEnd <= seg->beg-50)
				strcat(line, "# ");
			strcat(line, seg->text);
		} else
			strcat(line, seg->text);

		if (seg->text[0] != EOS) {
			if (whichLevel != 0)
				strcat(line, ", ");
			else if (isMultiBullets)
				strcat(line, " ");
			else
				strcat(line, "\n\t");
		}
		lastEnd = seg->end;
		if (strlen(line) > UTTLINELEN-50) {
			fprintf(stderr, "Out of memory!!! Tier is too long.\n");
			fprintf(stderr, "Line:\n");
			for (i=0L; line[i] != EOS; i++) {
				putc(line[i], stderr);
			}
			putc('\n', stderr);
			freePraat2ChatMem();
			cutt_exit(0);
		}
	}
/*
	if (isOneToOne) {
		for (; wordPos <= nt->wordCnt; wordPos++)
			strcat(line, "0 ");
	}
*/
/*
	if (whichLevel == 0) {
		for (depT=nt->depTiers; depT != NULL; depT = depT->nextTier) {
			if (depT->end > nt->end) {
				strcat(line, "#");
				break;
			}
		}
	}
*/

	i = ::strlen(line) - 1;
	while (i >= 0 && (isSpace(line[i]) || line[i] == '\n' || line[i] == ',')) i--;
	line[i+1] = EOS;
}

static void Praat_printOutTiers(P2C_CHATTIERS *nt, char whichLevel) {
	ALL_BGs *bg = NULL, *tBg, *eg;

	if (whichLevel == 0) {
		for (bg=RootBGs; bg != NULL; bg=bg->nextBG) {
			bg->isBGPrinted = FALSE;
			bg->isEGPrinted = FALSE;
		}
		bg = RootBGs;
	}

	while (nt != NULL) {
		if (whichLevel == 0) {
			for (eg=RootBGs; eg != NULL; eg=eg->nextBG) {
				if (!eg->isEGPrinted && eg->end <= nt->beg) {
					eg->isEGPrinted = TRUE;
					strcpy(templineC3, eg->GEMname);
					strcat(templineC3, " ");
					strcat(templineC3, eg->GEMtext);
					printout("@Eg:", templineC3, NULL, NULL, TRUE);
				}
			}
			if (nt->nextTier != NULL && bg != NULL && bg->end <= nt->nextTier->beg && bg->beg > nt->beg) {
				tBg = bg;
				for (; bg != NULL && bg->beg < nt->end; bg=bg->nextBG) {
					if (bg->beg > nt->beg) {
						for (eg=RootBGs; eg != NULL; eg=eg->nextBG) {
							if (!strcmp(bg->GEMname, eg->GEMname) && !eg->isEGPrinted && eg->isBGPrinted && eg->end <= bg->beg) {
								eg->isEGPrinted = TRUE;
								strcpy(templineC3, eg->GEMname);
								strcat(templineC3, " ");
								strcat(templineC3, eg->GEMtext);
								printout("@Eg:", templineC3, NULL, NULL, TRUE);
							}
						}
					}
				}
				for (bg=tBg; bg != NULL && bg->beg < nt->end; bg=bg->nextBG) {
					bg->isBGPrinted = TRUE;
					strcpy(templineC3, bg->GEMname);
					strcat(templineC3, " ");
					strcat(templineC3, bg->GEMtext);
					printout("@Bg:", templineC3, NULL, NULL, TRUE);
				}
			} else {
				for (; bg != NULL && bg->beg <= nt->beg; bg=bg->nextBG) {
					bg->isBGPrinted = TRUE;
					strcpy(templineC3, bg->GEMname);
					strcat(templineC3, " ");
					strcat(templineC3, bg->GEMtext);
					printout("@Bg:", templineC3, NULL, NULL, TRUE);
				}
			}
		}
		Praat_createLineFromSegs(templineC3, nt, whichLevel);
		printout(nt->sp, templineC3, NULL, NULL, nt->isWrap);
		if (nt->depTiers != NULL)
			Praat_printOutTiers(nt->depTiers, 1);
		nt = nt->nextTier;
	}
	if (whichLevel == 0) {
		for (; bg != NULL; bg=bg->nextBG) {
			strcpy(templineC3, bg->GEMname);
			strcat(templineC3, " ");
			strcat(templineC3, bg->GEMtext);
			printout("@Bg:", templineC3, NULL, NULL, TRUE);
		}
		for (eg=RootBGs; eg != NULL; eg=eg->nextBG) {
			if (!eg->isEGPrinted) {
				eg->isEGPrinted = TRUE;
				strcpy(templineC3, eg->GEMname);
				strcat(templineC3, " ");
				strcat(templineC3, eg->GEMtext);
				printout("@Eg:", templineC3, NULL, NULL, TRUE);
			}
		}
	}
}

static void Praat_makeText(char *line) {
	long  e, i, j;
	AttTYPE att, oldAtt;
	unsigned int hex;

	i = 0L;
	j = 0L;
	oldAtt = 0;
	while (line[i] != EOS) {
		if (!strncmp(line+i, "&quot;", 6)) {
			templineC[j++] = '"';
			i += 6;
		} else if (line[i] == '{' && line[i+1] == '0' && (line[i+2] == 'x' || line[i+2] == 't')) {
			for (e=i; line[e] != EOS && line[e] != '}'; e++) ;
			if (line[e] == '}') {
				if (line[i+2] == 't') {
					sscanf(line+i+3, "%x", &hex);
					att = hex;
					j = DealWithAtts_cutt(templineC, j, att, oldAtt);
					oldAtt = att;
				} else {
					sscanf(line+i+3, "%x", &hex);
					templineC[j++] = hex;
				}
				i = e + 1;
			} else
				templineC[j++] = line[i++];
		} else
			templineC[j++] = line[i++];
	}
	templineC[j] = EOS;
	strcpy(line, templineC);
}

static void extractSpeakerName(char *text, char *sp) {
	int i;
	char *p1, *p2;
	ATTRIBS *p;

	for (i=0; isSpace(text[i]) || text[i] == '"' || text[i] == '\n'; i++) ;
	if (i > 0)
		strcpy(text, text+i);
	for (i=strlen(text)-1; i >= 0 && (isSpace(text[i]) || text[i] == '"' || text[i] == '\n'); i--) ;
	i++;
	text[i] = EOS;
	uS.remFrontAndBackBlanks(text);
	for (p=attsRoot; p != NULL; p=p->nextTag) {
		if (uS.mStricmp(p->tag, text) == 0)
			break;
	}	
	sp[0] = EOS;

	if (p == NULL) {
		p1 = strchr(text, '[');
		if (p1 != NULL) {
			if (uS.partwcmp(p1, "[main]")) {
				*p1 = EOS;
				uS.remblanks(text);
				if (text[0] != '*' && text[0] != '%' && text[0] != '@')
					strcpy(sp, "*");
				else
					sp[0] = EOS;
				strcat(sp, text);
			} else if (uS.partwcmp(p1, "[MAIN-CHAT-HEADERS]")) {
				uS.remblanks(p1);
				strcpy(sp, p1);
			} else if (p1[0] == '[' && p1[1] == '%') {
				p2 = strrchr(text, ']');
				if (p2 != NULL)
					*p2 = EOS;
				uS.remblanks(text);
				strcpy(sp, p1+1);
				*p1 = EOS;
				uS.remblanks(text);
				strcat(sp, "@");
				strcat(sp, text);
				if (text[0] == EOS)
					sp[0] = EOS;
			} else if (attsRoot != NULL) {
				fprintf(stderr, "\r ** Can't match praat tag \"%s\" to CHAT declaration in attributes file.\n", text);
			} else {
				p2 = strrchr(text, ']');
				if (p2 != NULL)
					*p2 = EOS;
				uS.remblanks(text);
				if (*(p1+1) != '*' && *(p1+1) != '%' && *(p1+1) != '@')
					strcpy(sp, "%");
				else
					sp[0] = EOS;
				strcat(sp, p1+1);
				*p1 = EOS;
				uS.remblanks(text);
				strcat(sp, "@");
				strcat(sp, text);
				if (text[0] == EOS)
					sp[0] = EOS;
			}
		} else {
			p1 = strchr(text, '(');
			if (p1 != NULL) {
				p2 = strrchr(text, ')');
				if (p2 != NULL)
					*p2 = EOS;
				*p1 = EOS;
				p1++;
				if (uS.partwcmp(text, "ortografi") || uS.partwcmp(text, "Ortografi") || uS.partwcmp(text, "ORTOGRAFI")) {
					if (p1[0] != '*' && p1[0] != '%' && p1[0] != '@')
						strcpy(sp, "*");
					else
						sp[0] = EOS;
					strcat(sp, p1);
				} else if (attsRoot != NULL) {
					fprintf(stderr, "\r ** Can't match praat tag \"%s\" to CHAT declaration in attributes file.\n", text);
				} else {
					uS.remblanks(text);
					if (text[0] != '*' && text[0] != '%' && text[0] != '@')
						strcpy(sp, "%");
					else
						sp[0] = EOS;
					strcat(sp, text);
					strcat(sp, "@");
					strcat(sp, p1);
					if (text[0] == EOS)
						sp[0] = EOS;
				}
			} else if (uS.mStricmp(text, "ortho") == 0) {
				strcpy(sp, "*SPK");
			} else if (uS.mStricmp(text, "phono") == 0) {
				strcpy(sp, "%pho@SPK");
			} else if (*text == '*') {
				strcpy(sp, text);
				uS.uppercasestr(sp, &dFnt, FALSE);
			} else if (*text == '%') {
				strcpy(sp, text);
				uS.lowercasestr(sp, &dFnt, FALSE);
			} else if (attsRoot != NULL) {
				fprintf(stderr, "\r ** Can't match praat tag \"%s\" to CHAT declaration in attributes file.\n", text);
			} else {
				uS.remblanks(text);
				strcat(sp, "@bg@");
				strcat(sp, text);
			}
		}
	} else {
		strcpy(sp, p->chatName);
		if (p->chatName[0] != '*') {
			strcat(sp, "@");
			if (p->depOn[0] == '*')
				strcat(sp, p->depOn+1);
			else
				strcat(sp, p->depOn);
		}
	}
}

static void extractTierText(char *text, char *line, char isUnicode) {
	int  i;

	for (i=0; isSpace(text[i]) || text[i] == '"' || text[i] == '\n'; i++) ;
	if (i > 0)
		strcpy(text, text+i);
	for (i=strlen(text)-1; i >= 0 && (isSpace(text[i]) || text[i] == '"' || text[i] == '\n'); i--) ;
	i++;
	text[i] = EOS;

	strcpy(line, text);
}

static char getNextLine(char *st, int size, FILE *fp, char isUnicode, char *isNLFound) {
	long  i;

	if (isUnicode == UTF16_code) {
		return(fgets_cr(st, UTTLINELEN, fpin) != NULL);
	} else {
		return(fgets_cr(st, UTTLINELEN, fpin) != NULL);
	}
}

static char Praat_getNextTier(char *whichMode, UTTER *utterance, long *beg, long *end, char isUnicode) {
	int i;
	float xmin, xmax;
	double xmind, xmaxd;
	char isNLFound = FALSE;

	xmin = 0.0;
	xmax = 0.0;
	utterance->line[0] = EOS;
	while (getNextLine(templineC, UTTLINELEN, fpin, isUnicode, &isNLFound)) {
		for (i=0; isSpace(templineC[i]); i++) ;
		if (i > 0)
			strcpy(templineC, templineC+i);
		if (uS.partwcmp(templineC, "item ["))
			*whichMode = SP_MODE;
		else if (uS.partwcmp(templineC, "intervals ["))
			*whichMode = TIER_MODE;
		else if (*whichMode == SP_MODE) {
			if (uS.partwcmp(templineC, "name = "))
				extractSpeakerName(templineC+7, utterance->speaker);
		} else if (*whichMode == TIER_MODE) {
			if (uS.partwcmp(templineC, "xmin = "))
				xmin = atof(templineC+7);
			else if (uS.partwcmp(templineC, "xmax = "))
				xmax = atof(templineC+7);
			else if (uS.partwcmp(templineC, "text = ")) {
				xmind = (double)xmin * 1000.0000;
				*beg = (long)xmind;
				xmaxd = (double)xmax * 1000.0000;
				*end = (long)xmaxd;
				extractTierText(templineC+7, utterance->line, isUnicode);
				if (xmax != 0 && utterance->speaker[0] != EOS && utterance->line[0] != EOS)
					return(TRUE);
				xmin = 0.0;
				xmax = 0.0;
				utterance->line[0] = EOS;
			}
		}
	}
	return(FALSE);
}

static char UTFTestPass(void) {
	int  i;

	if (stin)
		return(TRUE);
	while (fgets_cr(templineC, UTTLINELEN, fpin)) {
		for (i=0; isSpace(templineC[i]); i++) ;
		if (uS.partwcmp(templineC+i, "name = \"[MAIN-CHAT-HEADERS]\"")) {
			rewind(fpin);
			return(TRUE);
		}
	}
	rewind(fpin);
	return(FALSE);
}

static void printHeaders(void) {
	int i;
	char sp[SPEAKERLEN];
	ATTRIBS *p;
	
	templineC1[0] = EOS;
	for (p=attsRoot; p != NULL; p=p->nextTag) {
		if (p->chatName[0] == '*') {
			strcat(templineC1, ", ");
			strcpy(sp, p->chatName+1);
			for (i=strlen(sp)-1; i >= 0 && (isSpace(sp[i]) || sp[i] == ':'); i--) ;
			sp[++i] = EOS;
			strcat(templineC1, sp);
			strcat(templineC1, " ");
			strcat(templineC1, p->depOn);
		}
	}
	for (i=0; templineC1[i] == ',' || isSpace(templineC1[i]); i++) ;
	if (templineC1[i] != EOS)
		printout("@Participants:", templineC1+i, NULL, NULL, TRUE);
	if (isMultiBullets)
		fprintf(fpout, "@Options:\tmulti\n");
	for (p=attsRoot; p != NULL; p=p->nextTag) {
		if (p->chatName[0] == '*') {
			strcpy(sp, p->chatName+1);
			for (i=strlen(sp)-1; i >= 0 && (isSpace(sp[i]) || sp[i] == ':'); i--) ;
			sp[++i] = EOS;
			fprintf(fpout, "@ID:\t%s|change_me_later|%s|||||%s|||\n", lang, sp, p->depOn);
		}
	}
}

static char extractChatHeaders(char *beg) {
	int  t;
	char *col, *end, isBeginDone, isMultiFound;
	char  spC[SPEAKERLEN];

	isBeginDone = FALSE;
	while (*beg != EOS) {
		for (end=beg; *end != EOS; end++) {
			if (end[0] == '>' && end[1] == '@') {
				end[0] = EOS;
				end++;
				break;
			}
		} 
					
		col = strchr(beg, ':');
		if (col == NULL) {
			strcpy(spC, beg);
			beg[0] = EOS;
		} else {
			*col = EOS;
			strcpy(spC, beg);
			strcat(spC, ":");
			strcpy(beg, col+1);
		}

		strcpy(templineC1, beg);
		if (uS.partcmp(spC, "@Languages:", FALSE, TRUE) || uS.partcmp(spC, "@Options:", FALSE, TRUE)) {
			isMultiFound = FALSE;
			for (t=0; templineC1[t] != EOS; t++) {
				if (!strncmp(templineC1+t, "multi", 5)) {
					 if (!isMultiBullets) {
						strcpy(templineC1+t, templineC1+t+5);
						if (templineC1[t] == ',')
							strcpy(templineC1+t, templineC1+t+1);
						else {
							for (t=strlen(templineC1); (templineC1[t] == EOS || isSpace(templineC1[t]) || templineC1[t] == ',') && t >= 0; t--) ;
							templineC1[t+1] = EOS;
						}
					}
					isMultiFound = TRUE;
					break;
				}
			}
			if (!isMultiFound && isMultiBullets) {
				uS.remblanks(templineC1);
				if (templineC1[0] == EOS)
					strcat(templineC1, "multi");
				else
					strcat(templineC1, ", multi");
			}
		}
		if (spC[0] == '@' && spC[1] == 'I' && spC[2] == 'D' && spC[3] == ':')
			printout(spC, templineC1, NULL, NULL, FALSE);
		else
			printout(spC, templineC1, NULL, NULL, TRUE);
			
		if (uS.partcmp(spC, "@Begin", FALSE, FALSE))
			isBeginDone = TRUE;

		beg = end;
	}
	return(isBeginDone);
}

static P2C_TIERS *freeRootTiers(P2C_TIERS *p) {
	P2C_TIERS *t;

	while (p != NULL) {
		if (p->nextPraatTier != NULL)
			p->nextPraatTier = freeRootTiers(p->nextPraatTier);
		t = p;
		p = p->sameSP;
		if (t->sp != NULL)
			free(t->sp);
		if (t->line != NULL)
			free(t->line);
		free(t);
	}
	return(NULL);
}

static P2C_TIERS *add2SameSp(P2C_TIERS *sameSP, UTTER *utterance, long beg, long end) {
	P2C_TIERS *p;

	if (sameSP == NULL) {
		p = NEW(P2C_TIERS);
		if (p == NULL)
			out_of_mem();
		sameSP = p;
	} else {
		for (p=sameSP; p->sameSP != NULL; p=p->sameSP) ;
		p->sameSP = NEW(P2C_TIERS);
		p = p->sameSP;
		if (p == NULL)
			out_of_mem();
	}
	p->sp = NULL;
	p->line = (char *)malloc(strlen(utterance->line)+1);
	if (p->line == NULL)
		out_of_mem();
	strcpy(p->line, utterance->line);
	p->beg = beg;
	p->end = end;
	p->sameSP = NULL;
	p->nextPraatTier = NULL;
	return(sameSP);
}

static P2C_TIERS *add2Tiers(P2C_TIERS *rootPraatTiers, UTTER *utterance, long beg, long end) {
	P2C_TIERS *p, *t;

	p = NEW(P2C_TIERS);
	if (p == NULL)
		out_of_mem();
	p->nextPraatTier = NULL;
	if (rootPraatTiers == NULL) {
		rootPraatTiers = p;
	} else {
		for (t=rootPraatTiers; t->nextPraatTier != NULL; t=t->nextPraatTier) {
			if (uS.mStricmp(t->sp, utterance->speaker) == 0) {
				t->sameSP = add2SameSp(t->sameSP, utterance, beg, end);
				return(rootPraatTiers);
			}
		}
		if (uS.mStricmp(t->sp, utterance->speaker) == 0) {
			t->sameSP = add2SameSp(t->sameSP, utterance, beg, end);
			return(rootPraatTiers);
		}
		t->nextPraatTier = p;
	}
	p->sp = (char *)malloc(strlen(utterance->speaker)+1);
	if (p->sp == NULL)
		out_of_mem();
	strcpy(p->sp, utterance->speaker);
	p->line = (char *)malloc(strlen(utterance->line)+1);
	if (p->line == NULL)
		out_of_mem();
	strcpy(p->line, utterance->line);
	p->beg = beg;
	p->end = end;
	p->sameSP = NULL;
	return(rootPraatTiers);
}

void call() {		/* this function is self-explanatory */
	const char *mediaType;
	char refSp[SPEAKERLEN], lastSp[SPEAKERLEN], *p, whichMode, isUnicode, isBeginDone;
	long len;
	long beg, end;
	long lineno, tlineno;
	P2C_TIERS *rootPraatTiers, *PraatTier, *SPTier;
	ALL_BGs *lastBgTier;
	P2C_CHATTIERS *lastSpTier;

	if (attsRoot == NULL) {
		rd_PraatAtts_f("attribs.cut", FALSE);
	}
	isUnicode = ANSI_code;
	if (!feof(fpin)) {
		int  c;

		c = getc(fpin);
		if (c == (int)0xFF || c == (int)0xFE) {
			if (c == (int)0xFF)
				lEncode = UPC;
			else
				lEncode = UMAC;
			isUnicode = UTF16_code;
		}
		if (!stin)
			rewind(fpin);
	}
	if (isUnicode == UTF16_code) {
		fclose(fpin);
		if ((fpin=fopen(oldfname, "rb")) == NULL) {
			fprintf(stderr,"Can't open file %s.\n",oldfname);
			return;
		}
		getc(fpin);
		if (!feof(fpin))
			getc(fpin);

	} else if (lEncode == 0xFFFF)
		isUnicode = UTF8_code;
	else if (UTFTestPass())
		isUnicode = UTF8_code;
	else if (lEncode == 0) {
		fprintf(stderr, "Please specify text encoding with +o option.\n");
		freePraat2ChatMem();
		cutt_exit(0);
	}
	lineno = 0L;
	tlineno = 0L;
	whichMode = 0;
	utterance->speaker[0] = EOS;
	SPTier = NULL;
	rootPraatTiers = NULL;
	while (Praat_getNextTier(&whichMode, utterance, &beg, &end, isUnicode)) {
		if (lineno > tlineno) {
			tlineno = lineno + 500;
			fprintf(stderr,"\r%ld ",lineno);
		}
		lineno++;
		if (utterance->speaker[0] == '*')
			SPTier = add2Tiers(SPTier, utterance, beg, end);
		else
			rootPraatTiers = add2Tiers(rootPraatTiers, utterance, beg, end);
	}
	if (SPTier != NULL) {
		for (PraatTier=SPTier; PraatTier->nextPraatTier != NULL; PraatTier=PraatTier->nextPraatTier) ;
		PraatTier->nextPraatTier = rootPraatTiers;
		rootPraatTiers = SPTier;
	}
  	strcpy(mediaFName, "dummy.mov");
  	isBeginDone = FALSE;
	refSp[0] = EOS;
	lastSp[0] = EOS;
	lastBgTier = RootBGs;
	lastSpTier = RootTiers;
	PraatTier = rootPraatTiers;
	SPTier = PraatTier;
	while (PraatTier != NULL) {
		while (SPTier == NULL) {
			PraatTier = PraatTier->nextPraatTier;
			if (PraatTier == NULL)
				break;
			SPTier = PraatTier;
		}
		if (PraatTier == NULL)
			break;
		if (SPTier->sp != NULL)
			strcpy(utterance->speaker, SPTier->sp);
		strcpy(utterance->line, SPTier->line);
		beg = SPTier->beg;
		end = SPTier->end;
		SPTier = SPTier->sameSP;
		if (strcmp(lastSp, utterance->speaker)) {
			lastBgTier = RootBGs;
			lastSpTier = RootTiers;
		}
		strcpy(lastSp, utterance->speaker);
		uS.remblanks(utterance->speaker);
/*
 if (strcmp(utterance->speaker, "%gpx@BET") == 0)
	len = 0;
*/
		if (strcmp(utterance->speaker, "[MAIN-CHAT-HEADERS]") == 0) {
			isBeginDone = extractChatHeaders(utterance->line);
		} else if (strncmp(utterance->speaker, "@bg@", 4) == 0) {
			uS.remblanks(utterance->line);
			Praat_makeText(utterance->line);
			lastBgTier = addBGs(lastBgTier, beg, end, utterance->speaker+4, utterance->line);
		} else {
			if (utterance->speaker[0] == '%') {
				p = strchr(utterance->speaker, '@');
				if (p != NULL) {
					strcpy(refSp, "*");
					strcat(refSp, p+1);
					len = strlen(refSp);
					if (refSp[len-1] != ':')
						strcat(refSp, ":");
					*p = EOS;
				}
			} else
				refSp[0] = EOS;
			len = strlen(utterance->speaker);
			if (utterance->speaker[len-1] != ':')
				strcat(utterance->speaker, ":");
			Praat_makeText(utterance->line);
			lastSpTier = Praat_addToTiers(lastSpTier, beg, end, utterance->speaker, refSp, utterance->line, TRUE);
		}
	}
	rootPraatTiers = freeRootTiers(rootPraatTiers);
	fprintf(stderr, "\r	                      \r");
	if (!isBeginDone) {
		fprintf(fpout, "%s\n", UTF8HEADER);
		fprintf(fpout, "@Begin\n");
		fprintf(fpout, "@Languages:\t%s\n", lang);
		printHeaders();
		p = strrchr(mediaFName, '.');
		if (p != NULL) {
			if (uS.mStricmp(p, ".wav") == 0 || uS.mStricmp(p, ".aif") == 0 || uS.mStricmp(p, ".aiff") == 0)
				mediaType = "audio";
			else
				mediaType = "video";
			*p = EOS;
		} else
			mediaType = "audio";
		fprintf(fpout, "%s\t%s, %s\n", MEDIAHEADER, mediaFName, mediaType);
	}
	Praat_finalTimeSort();
	cutt_isMultiFound = isMultiBullets;
	Praat_printOutTiers(RootTiers, 0);
	fprintf(fpout, "@End\n");
	RootBGs = freeBGs(RootBGs);
	RootTiers = freeTiers(RootTiers);
}
