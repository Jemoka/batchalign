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

#define TORT struct TimeOrders
struct TimeOrders {
	long num;
	long time;
	struct TimeOrders *nextTR;
} ;

#define ANNOTATIONS struct AllAnnotations
struct AllAnnotations {
	long ID;
	long refID;
	long beg;
	long end;
	char *st;
	ANNOTATIONS *nextAnnotation;
} ;

#define ALLTIERS struct AllTiers
struct AllTiers {
	char name[20];
	char typeRef[16];
	char whatLingType;
	long lastTime;
	char isPrinted;
	ANNOTATIONS *annotation;
	ALLTIERS *nextTier;
} ;

#define HEADERS struct HeaderTiers
struct HeaderTiers {
	char name[20];
	char *line;
	HEADERS *nextTier;
} ;


extern struct tier *defheadtier;
extern char OverWriteFile;

static char isMFA;
static char isSkip;
static TORT *timeOrder;
static ALLTIERS *RootTiers;
static HEADERS *RootHeaders;
static FNType mediaExt[10];

/* ******************** ab prototypes ********************** */
/* *********************************************************** */

void init(char f) {
	if (f) {
		timeOrder = NULL;
		OverWriteFile = TRUE;
		AddCEXExtension = ".eaf";
		stout = FALSE;
		onlydata = 1;
		FilterTier = 0;
		LocalTierSelect = TRUE;
		isMFA = FALSE;
		isSkip = FALSE;
		if (defheadtier->nexttier != NULL)
			free(defheadtier->nexttier);
		free(defheadtier);
		defheadtier = NULL;
		RootTiers = NULL;
		RootHeaders = NULL;
		mediaExt[0] = EOS;
	} else {
		if (mediaExt[0] == EOS) {
			fprintf(stderr,"Please use +e option to specify media file extension.\n    (For example: +e.mov or +e.mpg or +e.wav)\n");
			cutt_exit(0);
		}
	}
}

void usage() {
	printf("convert CHAT files to Elan XML files\n");
	printf("Usage: chat2elan [eS %s] filename(s)\n", mainflgs());
	puts("+c: The output .eaf file is created for MFA aligner.");
	puts("+s: Do not interpolate time codes.");
	puts("+eS: Specify media file name extension.");
	mainusage(TRUE);
}


CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = CHAT2ELAN;
	OnlydataLimit = 0;
	UttlineEqUtterance = TRUE;
	bmain(argc,argv,NULL);
}
		
void getflag(char *f, char *f1, int *i) {

	f++;
	switch(*f++) {
		case 'c':
			isMFA = TRUE;
			no_arg_option(f);
			break;
		case 's':
			isSkip = TRUE;
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

static TORT *freeTR(TORT *p) {
	TORT *t;

	while (p != NULL) {
		t = p;
		p = p->nextTR;
		free(t);
	}
	return(NULL);
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

static HEADERS *freeHeaders(HEADERS *p) {
	HEADERS *t;

	while (p != NULL) {
		t = p;
		p = p->nextTier;
		if (t->line != NULL)
			free(t->line);
		free(t);
	}
	return(NULL);
}

static long add2TR(long time_value) {
	long num;
    TORT *pt, *t, *tt;

	if ((pt=NEW(TORT)) == NULL)
		out_of_mem();

	num = 1L;
	for (t=timeOrder; t != NULL; t=t->nextTR)
		num++;
	if (timeOrder == NULL) {
		timeOrder = pt;
		pt->nextTR = NULL;
	} else if (timeOrder->time > time_value && time_value >= 0L) {
		pt->nextTR = timeOrder;
		timeOrder = pt;
	} else {
		t = timeOrder;
		tt = timeOrder->nextTR;
		while (tt != NULL) {
			if (tt->time > time_value && time_value >= 0L)
				break;
			t = tt;
			tt = tt->nextTR;
		}
		if (tt == NULL) {
			t->nextTR = pt;
			pt->nextTR = NULL;
		} else {
			pt->nextTR = tt;
			t->nextTR = pt;
		}
	}
	pt->num = num;
	pt->time = time_value;
	
	return(pt->num);
}

static ANNOTATIONS *addAnnotation(ANNOTATIONS *root, long antCnt, long spAntCnt, long BegNum, long EndNum, char *st) {
	ANNOTATIONS *t;

	if (root == NULL) {
		if ((root=NEW(ANNOTATIONS)) == NULL) out_of_mem();
		t = root;
	} else {
		for (t=root; t->nextAnnotation != NULL; t=t->nextAnnotation) ;
		if ((t->nextAnnotation=NEW(ANNOTATIONS)) == NULL) out_of_mem();
		t = t->nextAnnotation;
	}
	t->nextAnnotation = NULL;
	t->ID    = antCnt;
	t->refID = spAntCnt;
	t->beg   = BegNum;
	t->end   = EndNum;
	t->st    = (char *)malloc(strlen(st)+1);
	if (t->st == NULL) out_of_mem();
	strcpy(t->st, st);
	return(root);
}

static void addTierAnnotation(const char *name, char whatLingType, long antCnt, long spAntCnt, long *BegO, long *EndO, char *st, char isChangeTime, long BegSp, long EndSp) {
	char typeRef[16];
	int  len;
	long Beg, End;
	long BegNum, EndNum;
	ALLTIERS *t;


	if (name[0] == '*')
		strcpy(typeRef, "orthography");
	else {
		strcpy(typeRef, name);
	}

    len = strlen(st);
	Beg = *BegO;
	End = *EndO;

    // printf("ONE: (%d, %d) \n", Beg, End);

	if (RootTiers == NULL) {
		if ((RootTiers=NEW(ALLTIERS)) == NULL) out_of_mem();
		t = RootTiers;
	} else {
		for (t=RootTiers; 1; t=t->nextTier) {
		    if (strcmp(t->name, name) == 0 && strcmp(t->typeRef, typeRef) == 0) {
                if (isSkip == TRUE && (Beg == -1 || End == -1)) {
                    BegNum = add2TR(0);
                    EndNum = add2TR(0);
                } else {
                    if (Beg < t->lastTime && t->lastTime > -1)
                        Beg = t->lastTime;
                    if (End <= Beg) {
                        if (len < 5)
                            End = Beg + 5;
                        else if (len >= 5 && len < 10)
                            End = Beg + 80;
                        else if (len > 10)
                            End = Beg + 200;
                    }

                    // printf("TWO: (%d, %d) \n", Beg, End);
                    BegNum = add2TR(Beg);
                    EndNum = add2TR(End);
                }

				if (name[0] == '%' && whatLingType) {
					if (Beg >= BegSp && Beg <= EndSp && End >= BegSp && End <= EndSp)
						whatLingType = 1;
					else
						whatLingType = 2;

                    if (isSkip) // I just give up --- dependent tiers should be DEPENDENT - hjl
                        whatLingType = 0;
				}
			    // if (whatLingType && t->whatLingType == 0)
			    // else if (whatLingType == 2 && t->whatLingType == 1)
			    	// t->whatLingType = whatLingType;

                t->whatLingType = whatLingType;
				t->lastTime = End;
		    	t->annotation = addAnnotation(t->annotation, antCnt, spAntCnt, BegNum, EndNum, st);
		    	if (name[0] == '*' && isChangeTime) {
		    		*BegO = Beg;
		    		*EndO = End;
		    	}

                // printf("tier: %s, wlt: %d\n", t->name, whatLingType);

		    	return;
		    }
		    if (t->nextTier == NULL)
		    	break;
		}
		if ((t->nextTier=NEW(ALLTIERS)) == NULL)
			out_of_mem();
		t = t->nextTier;
	}

	if (End <= Beg) {
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
	strcpy(t->typeRef, typeRef);

	if (name[0] == '%' && whatLingType) {
		if (Beg >= BegSp && Beg <= EndSp && End >= BegSp && End <= EndSp)
			whatLingType = 1;
		else
			whatLingType = 2;
	}
	t->whatLingType = whatLingType;

	BegNum = add2TR(Beg);
	EndNum = add2TR(End);
	t->lastTime = End;
	t->annotation = addAnnotation(NULL, antCnt, spAntCnt, BegNum, EndNum, st);
	if (name[0] == '*' && isChangeTime) {
		*BegO = Beg;
		*EndO = End;
	}
}

static void addRefTierAnnotation(const char *name, long antCnt, long spAntCnt, char *st) {
	char typeRef[16];
	ALLTIERS *t;

	if (name[0] == '*')
		strcpy(typeRef, "orthography");
	else {
		strcpy(typeRef, name);
	}

	if (RootTiers == NULL) {
		if ((RootTiers=NEW(ALLTIERS)) == NULL) out_of_mem();
		t = RootTiers;
	} else {
		for (t=RootTiers; 1; t=t->nextTier) {
			if (strcmp(t->name, name) == 0 && strcmp(t->typeRef, typeRef) == 0) {
				t->whatLingType = 0;
				t->lastTime = -1;
				t->annotation = addAnnotation(t->annotation, antCnt, spAntCnt, 0L, 0L, st);
				return;
			}
			if (t->nextTier == NULL)
				break;
		}
		if ((t->nextTier=NEW(ALLTIERS)) == NULL)
			out_of_mem();
		t = t->nextTier;
	}

	t->nextTier = NULL;
	t->isPrinted = FALSE;
	strcpy(t->name, name);
	strcpy(t->typeRef, typeRef);
	t->whatLingType = 0;
	t->lastTime = -1;
	t->annotation = addAnnotation(NULL, antCnt, spAntCnt, 0L, 0L, st);
}

static void addHeaders(char *name, char *line) {
	char	*bs, *es;
	HEADERS *t;

	bs = line;
	es = line + strlen(line);
	textToXML(templineC, bs, es);

	if (RootHeaders == NULL) {
		if ((RootHeaders=NEW(HEADERS)) == NULL) out_of_mem();
		t = RootHeaders;
	} else {
		for (t=RootHeaders; t->nextTier != NULL; t=t->nextTier) ;
		if ((t->nextTier=NEW(HEADERS)) == NULL) out_of_mem();
		t = t->nextTier;
	}

	t->nextTier = NULL;
	strcpy(t->name, name);
	t->line = (char *)malloc(strlen(templineC)+1);
	if (t->line == NULL) out_of_mem();
	strcpy(t->line, templineC);
}

static void PrintAnnotations(ALLTIERS *trs, char *s) {
	ANNOTATIONS *anns;

	if (trs->name[0] == '*')
		fprintf(fpout, "<TIER TIER_ID=\"%s\" PARTICIPANT=\"%s\" LINGUISTIC_TYPE_REF=\"orthography\" DEFAULT_LOCALE=\"us\">\n", trs->name+1, s+1);
	else if (trs->name[0] == '%' && trs->whatLingType == 0)
		fprintf(fpout, "<TIER TIER_ID=\"%s\" PARTICIPANT=\"%s\" LINGUISTIC_TYPE_REF=\"dependency\" DEFAULT_LOCALE=\"us\" PARENT_REF=\"%s\">\n", trs->name+1, s+1, s+1);
	else if (trs->name[0] == '%' && trs->whatLingType == 1)
		fprintf(fpout, "<TIER TIER_ID=\"%s\" PARTICIPANT=\"%s\" LINGUISTIC_TYPE_REF=\"included_in\" DEFAULT_LOCALE=\"us\" PARENT_REF=\"%s\">\n", trs->name+1, s+1, s+1);
	else if (isMFA && trs->name[0] == '@')
		fprintf(fpout, "<TIER TIER_ID=\"%s\" PARTICIPANT=\"%s\" LINGUISTIC_TYPE_REF=\"no_constraint\" DEFAULT_LOCALE=\"us\">\n", trs->name, s+1);
	else
		fprintf(fpout, "<TIER TIER_ID=\"%s\" PARTICIPANT=\"%s\" LINGUISTIC_TYPE_REF=\"no_constraint\" DEFAULT_LOCALE=\"us\">\n", trs->name+1, s+1);

	for (anns=trs->annotation; anns != NULL; anns=anns->nextAnnotation) {
		fprintf(fpout, "    <ANNOTATION>\n");

		if (trs->whatLingType)
			fprintf(fpout, "        <ALIGNABLE_ANNOTATION ANNOTATION_ID=\"a%ld\" TIME_SLOT_REF1=\"ts%ld\" TIME_SLOT_REF2=\"ts%ld\">\n", anns->ID, anns->beg, anns->end);
		else
			fprintf(fpout, "        <REF_ANNOTATION ANNOTATION_ID=\"a%ld\" ANNOTATION_REF=\"a%ld\">\n", anns->ID, anns->refID);

		fprintf(fpout, "            <ANNOTATION_VALUE>%s</ANNOTATION_VALUE>\n", anns->st);

		if (trs->whatLingType)
			fprintf(fpout, "        </ALIGNABLE_ANNOTATION>\n");
		else
			fprintf(fpout, "        </REF_ANNOTATION>\n");
		fprintf(fpout, "    </ANNOTATION>\n");
	}
	fprintf(fpout, "</TIER>\n");
	trs->isPrinted = TRUE;
}

static void PrintTiers(char *fontName) {
	char *s;
	char spName[20];
	FILE *fp;
	ALLTIERS *maintrs, *trs;

	if (fontName[0] != EOS) {
		strcpy(FileName1, newfname);
		s = strrchr(FileName1, '.');
		if (s != NULL)
			*s = EOS;
		strcat(FileName1, ".pfsx");
		fp = fopen(FileName1, "w");
	} else
		fp = NULL;
	if (fp != NULL) {
		fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		fprintf(fp,"<preferences version=\"1.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://www.mpi.nl/tools/elan/Prefs_v1.1.xsd\">\n");
		fprintf(fp,"    <prefGroup key=\"TierFonts\">\n");
	}
	for (maintrs=RootTiers; maintrs != NULL; maintrs=maintrs->nextTier) {
		if (maintrs->name[0] != '*')
			continue;
		if (fp != NULL) {
			fprintf(fp,"        <pref key=\"%s\">\n", maintrs->name+1);
			fprintf(fp,"            <Object class=\"java.awt.Font\">%s,0,1</Object>\n", fontName);
			fprintf(fp,"        </pref>\n");
		}		
		strcpy(spName, maintrs->name+1);
		PrintAnnotations(maintrs, maintrs->name);
		for (trs=RootTiers; trs != NULL; trs=trs->nextTier) {
			if (trs->isPrinted == TRUE)
				continue;
			else if (isMFA && trs->name[0] == '@') {
			} else if (trs->name[0] == '*' || trs->name[0] == '@')
				continue;
			s = strrchr(trs->name, '@');
			if (trs->name[0] == '%') {
				s = strrchr(trs->name, '@');
				if (s == NULL)
					s = trs->name;
				else {
					if (strcmp(spName, s+1) != 0)
						continue;
				}
			} else
				s = trs->name;

			PrintAnnotations(trs, s);
		}
	}
	if (fp != NULL) {
		fprintf(fp, "    </prefGroup>\n");
		fprintf(fp, "</preferences>\n");
		fclose(fp);
	}
}

static char isSpecialCode(char *spName, char *st, long spAntCnt, long *antCnt) {
	int  i, ci, bi, ei;
	char name[512], isCodeFound;

	isCodeFound = FALSE;
	bi = 0;
	while (st[bi] != EOS) {
		if (strncmp(st+bi, "[%%", 3) == 0) {
			for (ei=bi; st[ei] != ']' && st[ei] != EOS; ei++) ;
			if (st[ei] == ']') {
				i = 0;
				for (ci=bi+3; !isSpace(st[ci]) && st[ci] != ':' && st[ci] != EOS; ci++) {
					name[i++] = st[ci];
				}
				name[i] = EOS;
				if (name[0] != EOS) {
					strcat(name, "@");
					strcat(name, spName);
				}
				for (; isSpace(st[ci]) || st[ci] == ':'; ci++) ;
				i = 0;
				for (; st[ci] != ']' && st[ci] != EOS; ci++) {
					templineC1[i++] = st[ci];
				}
				templineC1[i] = EOS;
				isCodeFound = TRUE;
				*antCnt = *antCnt + 1L;
				strcpy(st+bi, st+ei+1);
				addRefTierAnnotation(name, *antCnt, spAntCnt, templineC1);
			} else
				bi = ei;
		} else
			bi++;
	}
	return(isCodeFound);
}

void call() {
	long		Beg, End, BegSp = 0L, EndSp = 0L, t;
	long		bulletsCnt, numTiers;
	long		antCnt, cAntCnt, spAntCnt = 0L;
	FNType		mediaFName[FNSize], errfile[FNSize], *s;
	char		name[20], spName[20], fontName[256];
	char		*bs, *es;
	char		whatLingType, isBulletFound, ftime, isSpTierFound, isMediaHeaderFound, mediaRes;
	FILE		*errFp;
	TORT        *times;
	HEADERS		*hdrs;

	isMediaHeaderFound = FALSE;
	fontName[0] = EOS;
	mediaFName[0] = EOS;
	ftime = TRUE;
	errFp = NULL;
	isSpTierFound = FALSE;
	bulletsCnt = 0L;
	numTiers = 0L;
	antCnt = 0L;
	Beg = End = 0L;
	currentatt = 0;
	currentchar = (char)getc_cr(fpin, &currentatt);
	while (getwholeutter()) {
		if (uS.partcmp(utterance->speaker, MEDIAHEADER, FALSE, FALSE)) {
			getMediaName(utterance->line, mediaFName, FNSize);
			isMediaHeaderFound = TRUE;
		}
		if (utterance->speaker[0] == '@') {
			if (!isSpTierFound) {
				if (!uS.isUTF8(utterance->speaker) &&
						!uS.partcmp(utterance->speaker, "@Begin", FALSE, FALSE) && 
						!uS.partcmp(utterance->speaker, "@End", FALSE, FALSE))
					addHeaders(utterance->speaker, utterance->line);
				if (uS.partcmp(utterance->speaker, FONTHEADER, FALSE, FALSE)) {
					strcpy(fontName, utterance->line);
					bs = strchr(fontName, ':');
					if (bs != NULL)
						*bs = EOS;
					uS.remFrontAndBackBlanks(fontName);
				}
				continue;
			} else {
				if (!isMFA) {
					if (strcmp(utterance->speaker, "@End")) {
						fprintf(stderr,"*** File \"%s\": line %ld.\n", oldfname, lineno);
						fprintf(stderr, "It is illegal to have Header Tiers '@' inside the transcript\n");
						if (errFp != NULL) {
							fprintf(errFp,"*** File \"%s\": line %ld.\n", oldfname, lineno);
							fprintf(errFp, "It is illegal to have Header Tiers '@' inside the transcript\n");
						}
					}
				}
			}
		} else
			isSpTierFound = TRUE;
		if (utterance->speaker[0] == '*')
			numTiers++;
		isBulletFound = FALSE;
		t = strlen(utterance->speaker) - 1;
		for (; t >= 0L && (isSpace(utterance->speaker[t]) || utterance->speaker[t] == ':'); t--) ;
		utterance->speaker[++t] = EOS;
		bs = utterance->line;
		if (utterance->speaker[0] == '*') {
			BegSp = 0L;
			EndSp = 0L;
		}
		for (es=bs; *es != EOS;) {
			if (*es == HIDEN_C) {
				if (isMediaHeaderFound)
					mediaRes = getMediaTagInfo(es, &Beg, &End);
				else
					mediaRes = getOLDMediaTagInfo(es, NULL, mediaFName, &Beg, &End);
				if (mediaRes) {
					if (Beg == 1L)
						Beg = 0L;
					textToXML(templineC, bs, es);
					if (templineC[0] != EOS) {
						antCnt++;
						if (utterance->speaker[0] == '*') {
							whatLingType = 1;
							spAntCnt = antCnt;
							strcpy(name, utterance->speaker);
							strcpy(spName, utterance->speaker+1);
							isBulletFound = TRUE;
						} else {
							strcpy(name, utterance->speaker);
							strcat(name, "@");
							strcat(name, spName);
							if (Beg < BegSp)
								Beg = BegSp;
                            if (!isSkip) {
                                if (Beg >= BegSp && Beg <= EndSp && End >= BegSp && End <= EndSp) 
                                    whatLingType = 1;
                                else
                                    whatLingType = 2;
                            }
						}
						cAntCnt = antCnt;
						if (isSpecialCode(spName, templineC, cAntCnt, &antCnt)) {
							addTierAnnotation(name, whatLingType, cAntCnt, spAntCnt, &Beg, &End, templineC, TRUE, BegSp, EndSp);
						} else {
							addTierAnnotation(name, whatLingType, antCnt, spAntCnt, &Beg, &End, templineC, TRUE, BegSp, EndSp);
						}
						if (utterance->speaker[0] == '*') {
							if (BegSp == 0L)
								BegSp = Beg;
							EndSp = End;
						}
					}
				}

				for (bs=es+1; *bs != HIDEN_C && *bs != EOS; bs++) ;
				if (*bs == HIDEN_C)
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
				fprintf(errFp,"%s:\t%s\n", utterance->speaker, utterance->line);
			}
		}
		textToXML(templineC, bs, es);
		if (templineC[0] != EOS) {
			antCnt++;
			if (isMFA && utterance->speaker[0] == '@') {
				whatLingType = 0;
				strcpy(name, utterance->speaker);
			} else if (utterance->speaker[0] == '*') {
				whatLingType = 1;
				spAntCnt = antCnt;
				strcpy(name, utterance->speaker);
				strcpy(spName, utterance->speaker+1);
				if (BegSp == 0L)
					BegSp = -1;
				if (EndSp == 0L)
					EndSp = -1;
			} else {
				whatLingType = 0;
				strcpy(name, utterance->speaker);
				strcat(name, "@");
				strcat(name, spName);
			}
			cAntCnt = antCnt;
			if (isSpecialCode(spName, templineC, cAntCnt, &antCnt)) {
				addTierAnnotation(name, whatLingType, cAntCnt, spAntCnt, &BegSp, &EndSp, templineC, FALSE, BegSp, EndSp);
			} else {
				addTierAnnotation(name, whatLingType, antCnt, spAntCnt, &BegSp, &EndSp, templineC, FALSE, BegSp, EndSp);
			}
		}
	}

	if (bulletsCnt <= 0L) {
		fprintf(stderr, "\n\n    NO BULLETS FOUND IN THE FILE\n\n\n");
		if (errFp != NULL)
			fprintf(errFp, "\n\n    NO BULLETS FOUND IN THE FILE\n");
	} else {
		if (bulletsCnt != numTiers)
			fprintf(stderr, "\n\n");
		fprintf(stderr, "    Number of tiers found: %ld\n", numTiers);
		fprintf(stderr, "    Number of missing bullets: %ld\n", numTiers-bulletsCnt);
		if (bulletsCnt != numTiers)
			fprintf(stderr, "\n**** PLEASE LOOKS AT FILE \"%s\"\n\n", errfile);
		if (errFp != NULL) {
			fprintf(errFp, "\n\n\n    Number of tiers found: %ld\n", numTiers);
			fprintf(errFp, "    Number of missing bullets: %ld\n", numTiers-bulletsCnt);
		}
	}

	fprintf(fpout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(fpout, "<ANNOTATION_DOCUMENT xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://www.mpi.nl/tools/elan/EAFv2.4.xsd\" DATE=\"2006-03-10T14:56:23-05:00\" AUTHOR=\"unspecified\" VERSION=\"2.4\" FORMAT=\"2.4\">\n");
	fprintf(fpout, "    <HEADER MEDIA_FILE=\"\" TIME_UNITS=\"milliseconds\">\n");
	s = strrchr(mediaFName,'.');
	if (s == NULL)
		strcat(mediaFName, mediaExt);

	s = strrchr(newfname,'.');
	if (s != NULL)
		*s = EOS;
	fprintf(fpout, "        <MEDIA_DESCRIPTOR MEDIA_URL=\"file:///%s\" MIME_TYPE=\"video/quicktime\"/>\n", 	mediaFName);

	fprintf(fpout, "        <PROPERTY NAME=\"lastUsedAnnotationId\">%ld</PROPERTY>\n", antCnt);

	for (hdrs=RootHeaders; hdrs != NULL; hdrs=hdrs->nextTier) {
		fprintf(fpout, "        <PROPERTY NAME=\"%s\">\n", hdrs->name);
		fprintf(fpout, "            %s\n", hdrs->line);
		fprintf(fpout, "        </PROPERTY>\n");
	}

	if (s != NULL)
		*s = '.';
	fprintf(fpout, "    </HEADER>\n");

	fprintf(fpout, "    <TIME_ORDER>\n");
	for (times=timeOrder; times != NULL; times=times->nextTR) {
		if (times->time >= 0L)
			fprintf(fpout, "        <TIME_SLOT TIME_SLOT_ID=\"ts%ld\" TIME_VALUE=\"%ld\"/>\n", times->num, times->time);
		else
			fprintf(fpout, "        <TIME_SLOT TIME_SLOT_ID=\"ts%ld\"/>\n", times->num);
	}
	fprintf(fpout, "    </TIME_ORDER>\n");

	PrintTiers(fontName);

	fprintf(fpout, "    <LINGUISTIC_TYPE LINGUISTIC_TYPE_ID=\"orthography\"/>\n");
	fprintf(fpout, "    <LINGUISTIC_TYPE LINGUISTIC_TYPE_ID=\"no_constraint\"/>\n");
	fprintf(fpout, "    <LINGUISTIC_TYPE LINGUISTIC_TYPE_ID=\"included_in\" CONSTRAINTS=\"Included_In\"/>\n");
	fprintf(fpout, "    <LINGUISTIC_TYPE LINGUISTIC_TYPE_ID=\"dependency\" CONSTRAINTS=\"Symbolic_Association\"/>\n");
	fprintf(fpout, "    <LOCALE LANGUAGE_CODE=\"us\" COUNTRY_CODE=\"EN\"/>\n");
	fprintf(fpout, "    <CONSTRAINT STEREOTYPE=\"Symbolic_Association\"	DESCRIPTION=\"1-1 association with a parent annotation\"/>\n");
	fprintf(fpout, "    <CONSTRAINT STEREOTYPE=\"Included_In\" DESCRIPTION=\"Time alignable annotations within the parent annotation&apos;s time interval, gaps are allowed\"/>\n");
//	fprintf(fpout, "    <CONSTRAINT STEREOTYPE=\"Time_Subdivision\"	DESCRIPTION=\"Time subdivision of parent annotation&apos;s time interval, no time gaps allowed within this interval\"/>\n");
//	fprintf(fpout, "    <CONSTRAINT STEREOTYPE=\"Symbolic_Subdivision\"	DESCRIPTION=\"Symbolic subdivision of a parent annotation. Annotations refering to the same parent are ordered\"/>\n");
	fprintf(fpout, "</ANNOTATION_DOCUMENT>\n");

	if (errFp != NULL)
		fclose(errFp);
	timeOrder = freeTR(timeOrder);
	RootTiers = freeTiers(RootTiers);
	RootHeaders = freeHeaders(RootHeaders);
}
