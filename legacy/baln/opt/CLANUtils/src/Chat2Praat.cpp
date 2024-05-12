/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#include "cu.h"
#ifdef _DOS 
//	#include "stdafx.h"
#endif

#include "mul.h" 

#define IS_WIN_MODE FALSE
#define CHAT_MODE 3

#define ANNOTATIONS struct AllAnnotations
struct AllAnnotations {
	long   begL;
	long   endL;
	double begD;
	double endD;
	char *st;
	ANNOTATIONS *nextAnnotation;
} ;

#define ALLTIERS struct AllTiers
struct AllTiers {
	char name[20];
	char typeRef;
	long num;
//	long xmin, xmax;
	long lastTime;
	char isPrinted;
	ANNOTATIONS *annotation;
	ALLTIERS *nextTier;
} ;

#define C2P_LINESEG struct C2P_LineBulletSegment
struct C2P_LineBulletSegment {
	char *text;
	FNType *mediaFName;
	long beg, end;
	C2P_LINESEG *nextSeg;
} ;

#define C2P_CHATTIERS struct ChatTiers
struct ChatTiers {
	long beg, end;
	char *sp;
	C2P_LINESEG *lineSeg;
	C2P_LINESEG *lastLineSeg;
	C2P_CHATTIERS *nextDepTiers;
} ;


extern struct tier *defheadtier;
extern char OverWriteFile;

static char isMFAC2Praat;
static ALLTIERS *RootTiers;
static ALLTIERS *RootHeaders;
static FNType mediaExt[10];
static long   xminL, xmaxL;
static double xminD, xmaxD;

/* ******************** ab prototypes ********************** */
/* *********************************************************** */

void init(char f) {
	if (f) {
		OverWriteFile = TRUE;
		AddCEXExtension = ".textGrid";
		stout = FALSE;
		onlydata = 1;
		LocalTierSelect = TRUE;
		if (defheadtier->nexttier != NULL)
			free(defheadtier->nexttier);
		free(defheadtier);
		defheadtier = NULL;
		if (isMFAC2Praat == TRUE) {
			addword('\0','\0',"+xxx");
			addword('\0','\0',"+yyy");
			addword('\0','\0',"+www");
			addword('\0','\0',"+xxx@s*");
			addword('\0','\0',"+yyy@s*");
			addword('\0','\0',"+www@s*");
			addword('\0','\0',"+-*");
			addword('\0','\0',"+#*");
			addword('\0','\0',"+(*.*)");
		} else
			FilterTier = 0;
		RootTiers = NULL;
		RootHeaders = NULL;
		mediaExt[0] = EOS;
		xminL = 0L; // 0xFFFFFFFL;
		xmaxL = 0L;
	} else {
		if (isMFAC2Praat == TRUE)
			nomap = TRUE;
		if (mediaExt[0] == EOS) {
			fprintf(stderr,"Please use +e option to specify media file extension.\n    (For example: +e.mov or +e.mpg or +e.wav)\n");
			cutt_exit(0);
		}
	}
}

void usage() {
	printf("convert CHAT files to Praat TextGrid files\n");
	printf("Usage: chat2praat [eS %s] filename(s)\n", mainflgs());
	puts("+eS: Specify media file name extension.");
	mainusage(TRUE);
}


CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	int i;

	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = CHAT2PRAAT;
	OnlydataLimit = 0;
	isMFAC2Praat = FALSE;
	for (i=0; i < argc; i++) {
		if (argv[i][0] == '-' || argv[i][0] == '+') {
			if (argv[i][1] == 'c')
				isMFAC2Praat = TRUE;
		}
	}
	if (isMFAC2Praat == FALSE)
		UttlineEqUtterance = TRUE;
	bmain(argc,argv,NULL);
}
		
void getflag(char *f, char *f1, int *i) {

	f++;
	switch(*f++) {
		case 'c':
			isMFAC2Praat = TRUE;
			addword('s','i',"+&+*");
			addword('s','i',"+&-*");
			no_arg_option(f);
			break;
		case 'e':
			if (f[0] != '.')
				uS.str2FNType(mediaExt, 0L, ".");
			else
				mediaExt[0] = EOS;
			uS.str2FNType(mediaExt, strlen(mediaExt), f);
			break;
		default:
			maingetflag(f-2,f1,i);
			break;
	}
}

static void freeLineSeg(C2P_LINESEG *p) {
	C2P_LINESEG *t;

	while (p != NULL) {
		t = p;
		p = p->nextSeg;
		if (t->mediaFName != NULL)
			free(t->mediaFName);
		if (t->text != NULL)
			free(t->text);
		free(t);
	}
}

static C2P_CHATTIERS *freeChatTiers(C2P_CHATTIERS *t) {
	if (t->sp != NULL)
		free(t->sp);
	if (t->lineSeg != NULL)
		freeLineSeg(t->lineSeg);
	if (t->nextDepTiers != NULL)
		freeChatTiers(t->nextDepTiers);
	free(t);
	return(NULL);
}

static void addLineSeg(C2P_CHATTIERS *nt, long beg, long end, const FNType *mediaFName, char *text) {
	C2P_LINESEG *seg;

	if (nt->lineSeg == NULL) {
		if ((nt->lineSeg=NEW(C2P_LINESEG)) == NULL)
			out_of_mem();
		seg = nt->lineSeg;
		nt->lastLineSeg = seg;
	} else {
//		for (nt=seg; nt->nextSeg != NULL; nt=nt->nextSeg) ;
		seg = nt->lastLineSeg;
		if ((seg->nextSeg=NEW(C2P_LINESEG)) == NULL)
			out_of_mem();
		seg = seg->nextSeg;
		nt->lastLineSeg = seg;
	}
	seg->nextSeg = NULL;
	seg->beg = beg;
	seg->end = end;

	seg->mediaFName = (FNType *)malloc((strlen(mediaFName)+1)*sizeof(FNType));
	if (seg->mediaFName == NULL)
		out_of_mem();
	strcpy(seg->mediaFName, mediaFName);
	seg->text = (char *)malloc(strlen(text)+1);
	if (seg->text == NULL)
		out_of_mem();
	strcpy(seg->text, text);
}

static C2P_CHATTIERS *addToChatTiers(C2P_CHATTIERS *root, const FNType *mediaFName, char *sp, char *text, long beg, long end) {
	C2P_CHATTIERS *nt, *depT;

	if (root == NULL) {
		if ((root=NEW(C2P_CHATTIERS)) == NULL)
			out_of_mem();
		nt = root;
		nt->nextDepTiers = NULL;
		nt->lineSeg = NULL;
	} else
		nt = root;

	if (*sp == '*') {
		nt->sp = (char *)malloc(strlen(sp)+1);
		if (nt->sp == NULL)
			out_of_mem();
		strcpy(nt->sp, sp);
		addLineSeg(nt, beg, end, mediaFName, text);
		nt->beg = beg;
		nt->end = end;
	} else {
		if (nt->nextDepTiers == NULL) {
			if ((nt->nextDepTiers=NEW(C2P_CHATTIERS)) == NULL)
				out_of_mem();
			depT = nt->nextDepTiers;
		} else {
			for (depT=nt->nextDepTiers; depT->nextDepTiers != NULL; depT=depT->nextDepTiers) {
				if (!strcmp(depT->sp, sp)) {
					addLineSeg(depT, beg, end, mediaFName, text);
					depT->beg = beg;
					depT->end = end;
					return(root);
				}
			}
			if (!strcmp(depT->sp, sp)) {
				addLineSeg(depT, beg, end, mediaFName, text);
				depT->beg = beg;
				depT->end = end;
				return(root);
			}

			if ((depT->nextDepTiers=NEW(C2P_CHATTIERS)) == NULL)
				out_of_mem();
			depT = depT->nextDepTiers;
		}
		depT->nextDepTiers = NULL;
		depT->sp = (char *)malloc(strlen(sp)+1);
		if (depT->sp == NULL)
			out_of_mem();
		strcpy(depT->sp, sp);
		depT->lineSeg = NULL;
		addLineSeg(depT, beg, end, mediaFName, text);
		depT->beg = beg;
		depT->end = end;
	}
	return(root);
}

static void freeAnnotations(ANNOTATIONS *p) {
	ANNOTATIONS *t;

	while (p != NULL) {
		t = p;
		p = p->nextAnnotation;
		if (t->st != NULL)
			free(t->st);
		free(t);
	}
}

static ALLTIERS *freeTiers(ALLTIERS *p) {
	ALLTIERS *t;

	while (p != NULL) {
		t = p;
		p = p->nextTier;
		freeAnnotations(t->annotation);
		free(t);
	}
	return(NULL);
}

static ALLTIERS *addLinkedTier(ALLTIERS *root, const char *name, long Beg, long End, const char *st) {
	char *oldSt;
	double praat_time;
	ALLTIERS *t;
	ANNOTATIONS *ta;

	if (st[0] == EOS)
		return(root);

	if (root == NULL) {
		if ((root=NEW(ALLTIERS)) == NULL) out_of_mem();
		t = root;
		t->nextTier = NULL;
		t->annotation = NULL;
		strcpy(t->name, name);
		t->typeRef = 'l';
/*
		if (t->xmin > Beg)
			t->xmin = Beg;
		if (t->xmax < End)
			t->xmax = End;
*/
	} else {
		for (t=root; t->nextTier != NULL; t=t->nextTier) {
			if (strcmp(t->name, name) == 0)
				break;
		}
		if (t->nextTier == NULL && strcmp(t->name, name) != 0) {
			if ((t->nextTier=NEW(ALLTIERS)) == NULL) out_of_mem();
			t = t->nextTier;
			t->nextTier = NULL;
			t->annotation = NULL;
			strcpy(t->name, name);
			t->typeRef = 'l';
/*
			if (t->xmin > Beg)
				t->xmin = Beg;
			if (t->xmax < End)
				t->xmax = End;
*/
		}
	}

	if (t->annotation == NULL) {
		if ((t->annotation=NEW(ANNOTATIONS)) == NULL) out_of_mem();
		ta = t->annotation;
		ta->st = NULL;
	} else if (strcmp(name, "MAIN-CHAT-HEADERS") == 0) {
		ta = t->annotation;
	} else {
		ta = t->annotation;
		while (1) {
			if (uS.mStricmp(ta->st, st) == 0) {
/*
				if (ta->beg > Beg)
					ta->beg = Beg;
				if (ta->end < End)
					ta->end = End;
*/
				return(root);
			}
			if (ta->nextAnnotation == NULL)
				break;
			ta = ta->nextAnnotation;
		}
		if ((ta->nextAnnotation=NEW(ANNOTATIONS)) == NULL) out_of_mem();
		ta = ta->nextAnnotation;
		ta->st = NULL;
	}
	ta->nextAnnotation = NULL;
	ta->begL   = Beg;
	ta->endL   = End;
	praat_time = (double)Beg;
	praat_time = praat_time / 1000.0000000;
	ta->begD   = praat_time;
	praat_time = (double)End;
	praat_time = praat_time / 1000.0000000;
	ta->endD   = praat_time;
	if (ta->st == NULL) {
		ta->st    = (char *)malloc(strlen(st)+1);
		if (ta->st == NULL) out_of_mem();
		strcpy(ta->st, st);
	} else {
		oldSt = ta->st;
		ta->st    = (char *)malloc(strlen(st)+strlen(oldSt)+2);
		strcpy(ta->st, oldSt);
		strcat(ta->st, ">");
		strcat(ta->st, st);
	}

	return(root);
}

static void addAnnotation(ALLTIERS *tier, const FNType *mediaFName, long Beg, long End, char *st) {
	double praat_time;
	ANNOTATIONS *t;
	ANNOTATIONS *root;

	root = tier->annotation;
	if (root == NULL) {
		if ((root=NEW(ANNOTATIONS)) == NULL) out_of_mem();
		t = root;
	} else {
		for (t=root; t->nextAnnotation != NULL; t=t->nextAnnotation) ;
		if ((t->nextAnnotation=NEW(ANNOTATIONS)) == NULL) out_of_mem();
		t = t->nextAnnotation;
	}
	t->nextAnnotation = NULL;
	t->begL   = Beg;
	t->endL   = End;
	praat_time = (double)Beg;
	praat_time = praat_time / 1000.0000000;
	t->begD   = praat_time;
	praat_time = (double)End;
	praat_time = praat_time / 1000.0000000;
	t->endD   = praat_time;
	t->st    = (char *)malloc(strlen(st)+1);
	if (t->st == NULL) out_of_mem();
	strcpy(t->st, st);
	RootHeaders = addLinkedTier(RootHeaders, "aud@MEDIA", Beg, End, mediaFName);
	tier->annotation = root;
/*
	if (tier->xmin > Beg)
		tier->xmin = Beg;
	if (tier->xmax < End)
		tier->xmax = End;
*/
	if (xminL > Beg && Beg >= 0)
		xminL = Beg;
	if (xmaxL < End && End >= 0)
		xmaxL = End;
}

static ALLTIERS *addTierAnnotation(ALLTIERS *root, FNType *mediaFName, const char *name, long *BegO, long *EndO, char *st, char isChangeTime) {
	char typeRef;
	int  len;
	long num;
	long Beg, End;
	ALLTIERS *t;

	if (name[0] == '*')
		typeRef = 't';
	else
		typeRef = 'a';

    len = strlen(st);
	num = 2L;
	Beg = *BegO;
	End = *EndO;
	if (root == NULL) {
		if ((root=NEW(ALLTIERS)) == NULL) out_of_mem();
		t = root;
	} else {
		for (t=root; 1; t=t->nextTier) {
		    if (strcmp(t->name, name) == 0 && t->typeRef == typeRef) {
			    if (Beg < t->lastTime)
			    	Beg = t->lastTime;
			    if (End <= Beg && Beg >= 0) {
			    	if (len < 5)
			    		End = Beg + 5;
			    	else if (len >= 5 && len < 10)
			    		End = Beg + 80;
			    	else if (len >= 10)
			    		End = Beg + 200;
			    }
				t->lastTime = End;
		    	addAnnotation(t, mediaFName, Beg, End, st);
		    	if (name[0] == '*' && isChangeTime) {
		    		*BegO = Beg;
		    		*EndO = End;
		    	}
		    	return(root);
		    }
		    num++;
		    if (t->nextTier == NULL)
		    	break;
		}
		if ((t->nextTier=NEW(ALLTIERS)) == NULL)
			out_of_mem();
		t = t->nextTier;
	}

	if (End <= Beg && Beg >= 0) {
    	if (len < 5)
    		End = Beg + 5;
    	else if (len >= 5 && len < 10)
    		End = Beg + 80;
    	else if (len > 10)
    		End = Beg + 200;
	}
	t->nextTier = NULL;
	t->isPrinted = FALSE;
	strcpy(t->name, name);
	t->typeRef = typeRef;
	t->num = num;
	t->lastTime = End;
//	t->xmin = 0xFFFFFFFL;
//	t->xmax = 0L;
	t->annotation = NULL;
	addAnnotation(t, mediaFName, Beg, End, st);
	if (name[0] == '*' && isChangeTime) {
		*BegO = Beg;
		*EndO = End;
	}
	return(root);
}

static void textToPraatXML(char *an, int bs, int es) {
	long i;
	char *line;
	AttTYPE lastType;

	for (; (isSpace(utterance->line[bs]) || utterance->line[bs] == '\n') && bs < es; bs++) ;
	for (es--; (isSpace(utterance->line[es]) || utterance->line[es] == '\n') && bs <= es; es--) ;
	es++;
	i = 0L;
	lastType = 0;
	if (isMFAC2Praat == FALSE)
		line = utterance->line;
	else
		line = utterance->tuttline;
	for (; bs < es; bs++) {
		if (isMFAC2Praat == FALSE) {
			if (lastType != utterance->attLine[bs]) {
				sprintf(an+i,"{0t%x}", utterance->attLine[bs]);
				i = strlen(an);
			}
			lastType = utterance->attLine[bs];
		}
		if (line[bs] == '"') {
			strcpy(an+i, "&quot;");
			i = strlen(an);
		} else if (line[bs] == '\n')
			an[i++] = ' ';
		else if (line[bs] == '\t')
			an[i++] = ' ';
		else if (line[bs] >= 0 && line[bs] < 32) {
			sprintf(an+i,"{0x%x}", line[bs]);
			i = strlen(an);
		} else
			an[i++] = line[bs];
	}
	if (isMFAC2Praat == FALSE) {
		if (lastType != 0) {
			sprintf(an+i,"{0t%x}", 0);
			i = strlen(an);
		}
	}
	an[i] = EOS;
}

static double AdjustEndTime(double begD, ANNOTATIONS *anns) {
	int cnt;

	cnt = 1;
	for (; anns != NULL; anns=anns->nextAnnotation) {
		if (anns->begD >= 0.0)
			return(begD + ((anns->begD - begD) / cnt));
		cnt++;
	}
	return(begD);
}

static void PrintAnnotations(int num, ALLTIERS *trs, char *s) {
	double lastEnd;
	ANNOTATIONS *anns;

	fprintf(fpout, "    item [%d]:\n", num);
	fprintf(fpout, "        class = \"IntervalTier\"\n");
	if (trs->typeRef == 'l')
		fprintf(fpout, "        name = \"[%s]\"\n", trs->name);
	else if (trs->name[0] == '%')
		fprintf(fpout, "        name = \"%s [%s]\"\n", s+1, trs->name);
	else
		fprintf(fpout, "        name = \"%s [main]\"\n", s+1);
	fprintf(fpout, "        xmin = %.3lf\n", xminD);
	fprintf(fpout, "        xmax = %.3lf\n", xmaxD);
	lastEnd = xminD;
	num = 0;
	for (anns=trs->annotation; anns != NULL; anns=anns->nextAnnotation) {
		if (anns->begD < 0.0)
			anns->begD = lastEnd;
		if (anns->endD < 0.0)
			anns->endD = AdjustEndTime(anns->begD, anns->nextAnnotation);

		if (isMFAC2Praat == FALSE && anns->begD > lastEnd)
			num++;
		num++;
		lastEnd = anns->endD;
//if (strcmp(trs->name, "*CHI") == 0)
//fprintf(stderr, "%d-%s\n", num, anns->st);
	}
	if (isMFAC2Praat == FALSE && xmaxD > lastEnd)
		num++;
	fprintf(fpout, "        intervals: size = %d\n", num);
	
	lastEnd = xminD;
	num = 0;
	for (anns=trs->annotation; anns != NULL; anns=anns->nextAnnotation) {
		if (anns->begD < 0.0)
			anns->begD = lastEnd;
		if (anns->endD < 0.0)
			anns->endD = AdjustEndTime(anns->begD, anns->nextAnnotation);

		if (isMFAC2Praat == TRUE && (anns->endD-anns->begD) <= 0.2 && anns->nextAnnotation)
			anns->endD = anns->nextAnnotation->begD;

		if (isMFAC2Praat == FALSE && anns->begD > lastEnd) {
			num++;
			fprintf(fpout, "        intervals [%d]\n", num);
			fprintf(fpout, "            xmin = %.3lf\n", lastEnd);
			fprintf(fpout, "            xmax = %.3lf\n", anns->begD);
			fprintf(fpout, "            text = \"%s\"\n", "");
		}
		num++;
		fprintf(fpout, "        intervals [%d]\n", num);
		fprintf(fpout, "            xmin = %.3lf\n", anns->begD);
		fprintf(fpout, "            xmax = %.3lf\n", anns->endD);
		fprintf(fpout, "            text = \"%s\"\n", anns->st);
		lastEnd = anns->endD;
//if (strcmp(trs->name, "*CHI") == 0)
//fprintf(stderr, "%d-%s\n", num, anns->st);
	}
	if (isMFAC2Praat == FALSE && xmaxD > lastEnd) {
		num++;
		fprintf(fpout, "        intervals [%d]\n", num);
		fprintf(fpout, "            xmin = %.3lf\n", lastEnd);
		fprintf(fpout, "            xmax = %.3lf\n", xmaxD);
		fprintf(fpout, "            text = \"%s\"\n", "");
	}
	trs->isPrinted = TRUE;
}

static int tiersCount(void) {
	int num;
	ALLTIERS *trs;

	num = 0;
	for (trs=RootTiers; trs != NULL; trs=trs->nextTier)
		num++;
	for (trs=RootHeaders; trs != NULL; trs=trs->nextTier) {
		if (strcmp(trs->name, "aud@MEDIA") != 0)
			num++;
	}
	return(num);
}

static void PrintTiers(void) {
	char *s;
	char spName[20];
	int  num;
	ALLTIERS *maintrs, *trs;

	num = 0;
	for (maintrs=RootTiers; maintrs != NULL; maintrs=maintrs->nextTier) {
		if (maintrs->name[0] != '*')
			continue;

		strcpy(spName, maintrs->name+1);
		num++;
		PrintAnnotations(num, maintrs, maintrs->name);

		for (trs=RootTiers; trs != NULL; trs=trs->nextTier) {
			if (trs->isPrinted == TRUE || trs->name[0] == '*')
				continue;

			if (trs->name[0] == '%') {
				s = strrchr(trs->name, '@');
				if (s == NULL)
					s = trs->name;
				else {
					if (strcmp(spName, s+1) != 0)
						continue;
					*s = EOS;
				}
			} else
				s = trs->name;

			num++;
			PrintAnnotations(num, trs, s);
		}
	}

	for (trs=RootHeaders; trs != NULL; trs=trs->nextTier) {
		if (strcmp(trs->name, "aud@MEDIA") == 0)
			continue;
		s = strrchr(trs->name, '@');
		if (s == NULL)
			s = trs->name;
		else {
			*s = EOS;
		}
		num++;
		PrintAnnotations(num, trs, s);
	}
}

static int getWordPos(char *text, unsigned short *fpos, unsigned short *lpos) {
	int i;
	int j;

	for (i=0; text[i] != EOS; i++) {
		if (text[i] == '<') {
			for (j=i+1; isSpace(text[j]); j++) ;
			if (isdigit((unsigned char)text[j])) {
				*fpos = (unsigned short)atoi(text+j);
				for (j++; isdigit((unsigned char)text[j]); j++) ;
				for (; isSpace(text[j]) || text[j] == 'w' || text[j] == '-'; j++) ;
				if (text[j] == '>') {
					*lpos = *fpos;
					for (j++; text[j] != EOS && isSpace(text[j]); j++) ;
					return(j);
				} else if (isdigit((unsigned char)text[j])) {
					*lpos = (unsigned short)atoi(text+j);
					for (j++; isdigit((unsigned char)text[j]); j++) ;
					for (; isSpace(text[j]) || text[j] == 'w'; j++) ;
					if (text[j] == '>') {
						for (j++; text[j] != EOS && isSpace(text[j]); j++) ;
						return(j);
					}
				}
			}
		}
	}
	return(0);
}

#define timeMatch(sp,dep) (sp->beg >= dep->beg-1 && sp->beg <= dep->beg+1 && sp->end >= dep->end-1 && sp->end <= dep->end+1)

static char findSpMatch(C2P_LINESEG *mwO, C2P_LINESEG *dw, unsigned short fpos, unsigned short lpos) {
	int  i;
	unsigned short wordCntF, wordCntL;
	char isMatchfound;
	C2P_LINESEG *mw;

	if (dw->beg == -1 || dw->end == -1)
		return(0);

	isMatchfound = 0;
	wordCntF = wordCntL = 0;
	for (mw=mwO; mw != NULL; mw=mw->nextSeg) {
		wordCntF = wordCntL;
		wordCntF++;
		for (i=0; mw->text[i] != EOS; ) {
			if (uS.isskip(mw->text, i, &dFnt, MBF)) {
				for (; mw->text[i] != EOS && uS.isskip(mw->text, i, &dFnt, MBF); i++) ;
			} else if (mw->text[i] == '#' || mw->text[i] == '&' || mw->text[i] == '+' || mw->text[i] == '-') {
				for (; mw->text[i] != EOS && !uS.isskip(mw->text, i, &dFnt, MBF); i++) ;
			} else if (mw->text[i] == '[') {
				for (; mw->text[i] != EOS && mw->text[i] != ']'; i++) ;
				if (mw->text[i] != EOS)
					i++;
			} else if (mw->text[i] == HIDEN_C) {
				for (; mw->text[i] != EOS && mw->text[i] != HIDEN_C; i++) ;
				if (mw->text[i] != EOS)
					i++;
			} else if (!uS.isskip(mw->text, i, &dFnt, MBF)) {
				wordCntL++;
				for (; mw->text[i] != EOS && !uS.isskip(mw->text, i, &dFnt, MBF) && mw->text[i] != '['; i++) ;
			}
		}
		if (timeMatch(mw, dw) && wordCntF == fpos && wordCntL == lpos) {
			isMatchfound = 2;
			break;
		} else if (mw->beg >= dw->beg-1 && mw->beg <= dw->beg+1 && wordCntF == fpos && isMatchfound == 0) {
			isMatchfound = 1;
		} else if (mw->end >= dw->end-1 && mw->end <= dw->end+1 && wordCntL == lpos && isMatchfound == 1) {
			isMatchfound = 2;
			break;
		}
	}
	return((char)(isMatchfound == 2));
}

static void addToAnnotations(C2P_CHATTIERS *TierCluster) {
	unsigned short fpos, lpos;
	int		scopePos;
	long	Beg, End, BegSp, EndSp;
	char	name[20], spName[20];
	C2P_CHATTIERS *depT;
	C2P_LINESEG *dw;

	for (depT=TierCluster->nextDepTiers; depT != NULL; depT = depT->nextDepTiers) {
		for (dw=depT->lineSeg; dw != NULL; dw=dw->nextSeg) {
			if (dw->beg != -1 && dw->end != -1) {
				if ((scopePos=getWordPos(dw->text, &fpos, &lpos)) != 0) {
					if (findSpMatch(TierCluster->lineSeg, dw, fpos, lpos)) {
						strcpy(dw->text, dw->text+scopePos);
					}
				}
			}
		}
	}

	Beg = End = 0L;
	BegSp = EndSp = 0L;
	for (dw=TierCluster->lineSeg; dw != NULL; dw=dw->nextSeg) {
		if (dw->beg == -1 && dw->end == -1) {
			strcpy(name, TierCluster->sp);
			strcpy(spName, TierCluster->sp+1);
			if (BegSp == 0L)
				BegSp = -2;
			if (EndSp == 0L)
				EndSp = -2;
			RootTiers = addTierAnnotation(RootTiers, dw->mediaFName, name, &BegSp, &EndSp, dw->text, FALSE);
		} else {
			Beg = dw->beg;
			End = dw->end;
			if (Beg == 1L)
				Beg = 0L;
			strcpy(name, TierCluster->sp);
			strcpy(spName, TierCluster->sp+1);
			RootTiers = addTierAnnotation(RootTiers, dw->mediaFName, name, &Beg, &End, dw->text, TRUE);
			if (BegSp == 0L)
				BegSp = Beg;
			EndSp = End;
		}
	}

	for (depT=TierCluster->nextDepTiers; depT != NULL; depT = depT->nextDepTiers) {
		for (dw=depT->lineSeg; dw != NULL; dw=dw->nextSeg) {
			if (dw->beg == -1 && dw->end == -1) {
				strcpy(name, depT->sp);
				strcat(name, "@");
				strcat(name, spName);
				RootTiers = addTierAnnotation(RootTiers, dw->mediaFName, name, &BegSp, &EndSp, dw->text, FALSE);
			} else {
				Beg = dw->beg;
				End = dw->end;
				if (Beg == 1L)
					Beg = 0L;
				strcpy(name, depT->sp);
				strcat(name, "@");
				strcat(name, spName);
				if (Beg < BegSp)
					Beg = BegSp;
				RootTiers = addTierAnnotation(RootTiers, dw->mediaFName, name, &Beg, &End, dw->text, TRUE);
			}
		}
	}
}

static void cleanupAnPercentC2Praat(char *line) {
	int i, j;
	
	i = 0;
	while (line[i] != EOS) {
		if (line[i] == '&' && (line[i+1] == '+' || line[i+1] == '-'))
			strcpy(line+i, line+i+2);
		else if (line[i] == '$' && isalnum(line[i-1])) {
			for (j=i+1; isalpha((int)line[j]); j++) ;
			if (j > i)
				strcpy(line+i, line+j);
			else
				i++;
		} else
			i++;
	}
}

static void filterAtSymC2Praat(char *line) {
	long i, j;
	
	i = 0L;
	while (line[i] != EOS) {
		if (line[i] == '@') {
			for (j=i; !uS.isskip(line,j,&dFnt,MBF) && line[j] != '+' && line[j] != '-' && line[j] != EOS; j++) ;
			strcpy(line+i, line+j);
		} else
			i++;
	}
}

static void removeAngleBracketsC2Praat(char *line) {
	long i;
	
	i = 0L;
	while (line[i] != EOS) {
		if ((i == 0 || line[i-1] != '+') && (line[i] == '<' || line[i] == '>')) {
			strcpy(line+i, line+i+1);
		} else
			i++;
	}
}

void call() {
	int		i, bs, es;
	long	BegT, EndT;
	long	t, bulletsCnt, numTiers;
	FNType	mediaFName[FNSize], errfile[FNSize];
	char	isBulletFound, ftime, isSpTierFound, isMediaHeaderFound, mediaRes;
	C2P_CHATTIERS *TierCluster;
	FILE	*errFp;

	isMediaHeaderFound = FALSE;
	xminL = 0L; // 0xFFFFFFFL;
	xmaxL = 0L;
	mediaFName[0] = EOS;
	ftime = TRUE;
	errFp = NULL;
	isSpTierFound = FALSE;
	bulletsCnt = 0L;
	numTiers = 0L;
	TierCluster = NULL;
	currentatt = 0;
	currentchar = (char)getc_cr(fpin, &currentatt);
	while (getwholeutter()) {
		if (uS.partcmp(utterance->speaker, MEDIAHEADER, FALSE, FALSE)) {
			getMediaName(utterance->line, mediaFName, FNSize);
			isMediaHeaderFound = TRUE;
		}
		uS.remblanks(utterance->speaker);
		if (!checktier(utterance->speaker))
			continue;
		if (utterance->speaker[0] == '@') {
			if (// uS.isUTF8(utterance->speaker) ||
				//	uS.partcmp(utterance->speaker, FONTHEADER, FALSE, FALSE) ||
				//	uS.partcmp(utterance->speaker, "@Begin", FALSE, FALSE) ||
					uS.partcmp(utterance->speaker, "@End", FALSE, FALSE))
				;
			else if (!isSpTierFound) {
				strcpy(templineC, utterance->speaker);
				strcat(templineC, utterance->line);
				BegT = 0L;
				EndT = 1L;
				uS.cleanUpCodes(templineC, &dFnt, MBF);
				uS.remblanks(templineC);
				RootHeaders = addLinkedTier(RootHeaders, "MAIN-CHAT-HEADERS", BegT, EndT, templineC);
			} else {
				fprintf(stderr,"*** File \"%s\": line %ld.\n", oldfname, lineno);
				fprintf(stderr, "It is illegal to have Header Tiers '@' inside the transcript\n");
				if (errFp != NULL) {
					fprintf(errFp,"*** File \"%s\": line %ld.\n", oldfname, lineno);
					fprintf(errFp, "It is illegal to have Header Tiers '@' inside the transcript\n");
				}
			}
			continue;
		} else
			isSpTierFound = TRUE;
		
		if (utterance->speaker[0] == '*' || utterance->speaker[0] == '@') {
			if (TierCluster != NULL) {
				addToAnnotations(TierCluster);
				TierCluster = freeChatTiers(TierCluster);
			}
		}
		if (utterance->speaker[0] == '*')
			numTiers++;

		isBulletFound = FALSE;
		t = strlen(utterance->speaker) - 1;
		for (; t >= 0L && (isSpace(utterance->speaker[t]) || utterance->speaker[t] == ':'); t--) ;
		utterance->speaker[++t] = EOS;
		bs = 0;
		for (es=bs; utterance->line[es] != EOS;) {
			if (utterance->line[es] == HIDEN_C) {
				if (isMediaHeaderFound)
					mediaRes = getMediaTagInfo(utterance->line+es, &BegT, &EndT);
				else
					mediaRes = getOLDMediaTagInfo(utterance->line+es, NULL, mediaFName, &BegT, &EndT);
				if (mediaRes) {
					if (strrchr(mediaFName, '.') == NULL && mediaFName[0] != EOS)
						strcat(mediaFName, mediaExt);
					textToPraatXML(templineC, bs, es);
					if (templineC[0] != EOS) {
						if (utterance->speaker[0] == '*')
							isBulletFound = TRUE;
						if (isMFAC2Praat == TRUE) {
							filterAtSymC2Praat(templineC);
							removeAngleBracketsC2Praat(templineC);
							for (i=0; templineC[i] != EOS; i++) {
								if (uS.IsUtteranceDel(templineC, i))
									templineC[i] = ' ';
							}
							uS.remFrontAndBackBlanks(templineC);
							cleanupAnPercentC2Praat(templineC);
							if (templineC[0] != EOS) {
								strcat(templineC, " .");
							}
							removeExtraSpace(templineC);
						}
						TierCluster = addToChatTiers(TierCluster, mediaFName, utterance->speaker, templineC, BegT, EndT);
					}
				}

				for (bs=es+1; utterance->line[bs] != HIDEN_C && utterance->line[bs] != EOS; bs++) ;
				if (utterance->line[bs] == HIDEN_C)
					bs++;
				es = bs;
			} else
				 es++;
		}
		if (isBulletFound)
			bulletsCnt++;
		else if (utterance->speaker[0] == '*') {
			if (errFp == NULL && ftime) {
				ftime = FALSE;
				parsfname(oldfname, errfile,".err.cex");
				errFp = fopen(errfile, "w");
				if (errFp == NULL) {
					fprintf(stderr, "Can't create file \"%s\", perhaps it is opened by another application\n", errfile);
				}
			}
			if (errFp != NULL) {
				fprintf(errFp,"*** File \"%s\": line %ld.\n", oldfname, lineno);
				if (isMFAC2Praat == TRUE)
					fprintf(errFp, "Warning: Missing bullet, but utterance is still written out\n");
				else
					fprintf(errFp, "Missing bullet.\n");
			}
		}
		textToPraatXML(templineC, bs, es);
		if (templineC[0] != EOS) {
			if (!isMediaHeaderFound)
				mediaFName[0] = EOS;
			if (isMFAC2Praat == TRUE) {
				filterAtSymC2Praat(templineC);
				removeAngleBracketsC2Praat(templineC);
				for (i=0; templineC[i] != EOS; i++) {
					if (uS.IsUtteranceDel(templineC, i))
						templineC[i] = ' ';
				}
				uS.remFrontAndBackBlanks(templineC);
				cleanupAnPercentC2Praat(templineC);
				if (templineC[0] != EOS) {
					strcat(templineC, " .");
				}
				removeExtraSpace(templineC);
			}
			TierCluster = addToChatTiers(TierCluster, "", utterance->speaker, templineC, -1, -1);
		}
	}

	if (TierCluster != NULL) {
		addToAnnotations(TierCluster);
		TierCluster = freeChatTiers(TierCluster);
	}

	if (bulletsCnt <= 0L) {
		if (isMediaHeaderFound == FALSE) {
			fprintf(stderr, "\n\n   @Media: HEADER WITH MEDIA FILE NAME WAS NOT FOUND\n\n\n");
			if (errFp != NULL)
				fprintf(errFp, "\n\n    @Media: HEADER WITH MEDIA FILE NAME WAS NOT FOUND\n");
		} else {
			fprintf(stderr, "\n\n    NO BULLETS FOUND IN THE FILE\n\n\n");
			if (errFp != NULL)
				fprintf(errFp, "\n\n    NO BULLETS FOUND IN THE FILE\n");
		}
	} else {
		if (bulletsCnt != numTiers)
			fprintf(stderr, "\n\n");
		fprintf(stderr, "    Number of tiers found: %ld\n", numTiers);
		fprintf(stderr, "    Number of missing bullets: %ld\n", numTiers-bulletsCnt);
		if (bulletsCnt != numTiers)
			fprintf(stderr, "\n    Please looks at file \"%s\"\n\n", errfile);
		if (errFp != NULL) {
			fprintf(errFp, "\n\n\n    Number of tiers found: %ld\n", numTiers);
			fprintf(errFp, "    Number of missing bullets: %ld\n", numTiers-bulletsCnt);
		}
	}

	fprintf(fpout, "File type = \"ooTextFile\"\n");
	fprintf(fpout, "Object class = \"TextGrid\"\n\n");

	xminD = (double)xminL;
	xminD = xminD / 1000.0000000;
	fprintf(fpout, "xmin = %.3lf\n",xminD);
	xmaxD = (double)xmaxL;
	xmaxD = xmaxD / 1000.0000000;
	fprintf(fpout, "xmax = %.3lf\n",xmaxD);
	fprintf(fpout, "tiers? <exists>\n");
	fprintf(fpout, "size = %d\n", tiersCount());
	fprintf(fpout, "item []:\n");

	PrintTiers();

	if (errFp != NULL)
		fclose(errFp);
	RootTiers = freeTiers(RootTiers);
	RootHeaders = freeTiers(RootHeaders);
}
