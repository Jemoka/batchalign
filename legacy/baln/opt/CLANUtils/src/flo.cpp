/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#include "common.h"
#define CHAT_MODE 2

#include "cu.h"
#include "check.h"

#define IS_WIN_MODE FALSE
#include "mul.h"

#define FLO_UTT struct flo_utterance /* main utterance structure             */
FLO_UTT {
    char speaker[SPEAKERLEN];		/* code descriptor field of the turn	*/
    AttTYPE attSp[SPEAKERLEN];		/* Attributes assiciated with speaker name	*/
    char line[UTTLINELEN+1];		/* text field of the turn		*/ // found uttlinelen
    AttTYPE attLine[UTTLINELEN+1];	/* Attributes assiciated with text*/
    char tuttline[UTTLINELEN+1];	/* working copy of the text of the turn	*/
    FLO_UTT *nextutt;
} ;

extern char OverWriteFile;
extern struct tier *defheadtier;
extern char GExt[];

static char isFirstTime;
static char isMorFlo, isRFlo, isMFAFlo, isSpeakerSpecified, isAntConc, isUDFlo;
static char leave_AT;
static char substitute_flag;	/* Flo line will be output in */
								/* addition to original main line */

void usage() {
	puts("FLO produces a %flo line for BA");
	printf("Usage: flo [a cm cr d %s] filename(s)\n", mainflgs());
	puts("+a : do not remove @.. suffixes from words (default: change \"word@s\" to \"word\").");
	puts("+cm: filter main tier as \"mor\" does.");
	puts("+cr: filter main tier and remove speaker codes and utterance delimiters.");
	puts("+ca: create output for MFA aligner");
	puts("+cu: create output for UD injection");
	puts("+d:  replaces the main tier with the simplified %flo tier in the output");
	puts("+d1: create FAVE formatted file");
	puts("+d2: create AntConc formatted .txt file with BOM");
	mainusage(TRUE);
}

void init(char first) {
	int i;
    if (isMorFlo) {
        setVocative();
    }

    if (first) {
		isFirstTime = TRUE;
		isMorFlo = FALSE;
		isRFlo = FALSE;
		leave_AT = FALSE;
		isSpeakerSpecified = FALSE;
		substitute_flag = 0;
		stout = FALSE;
		onlydata = TRUE;
// defined in "mmaininit" and "globinit" - nomap = TRUE;
		if (isAntConc) {
			if (defheadtier != NULL) {
				if (defheadtier->nexttier != NULL)
					free(defheadtier->nexttier);
				free(defheadtier);
				defheadtier = NULL;
			}
		} else {
			if (isMFAFlo == FALSE) {
				addword('\0','\0',"+</?>");
				addword('\0','\0',"+</->");
				addword('\0','\0',"+<///>");
				addword('\0','\0',"+<//>");
				addword('\0','\0',"+</>");  // list R6 in mmaininit funtion in cutt.c
				addword('\0','\0',"+&*");
			}
            if (isUDFlo == FALSE) {
                addword('\0','\0',"+xxx");
                addword('\0','\0',"+yyy");
                addword('\0','\0',"+www");
                addword('\0','\0',"+xxx@s*");
                addword('\0','\0',"+yyy@s*");
                addword('\0','\0',"+www@s*");
            }
			addword('\0','\0',"+-*");
			addword('\0','\0',"+#*");
			addword('\0','\0',"+(*.*)");
			mor_initwords();
		}
		OverWriteFile = TRUE;
    } else {
		if (isFirstTime) {
			isFirstTime = FALSE;
			if (isAntConc) {
				strcpy(GExt, ".txt");
				AddCEXExtension = "";
			} else {
				if (isRFlo || isMFAFlo) {
					if (!isSpeakerSpecified) {
						fputs("\nPlease specify at least one speaker tier code with \"+t\" option\n", stderr);
						cutt_exit(0);
					}
					defheadtier->nexttier = NEW(struct tier);
					if (defheadtier->nexttier == NULL) {
						fputs("ERROR: Out of memory.\n",stderr);
						cutt_exit(0);
					}
					strcpy(defheadtier->nexttier->tcode,"%");
					defheadtier->nexttier->include = FALSE;
					defheadtier->nexttier->pat_match = FALSE;
					defheadtier->nexttier->nexttier = NULL;
					substitute_flag = 1;
				}
				if (isMORSearch())
					linkMain2Mor = TRUE;
				if (isMorFlo) {
					addword('\0','\0',"+0");
					addword('\0','\0',"+<e>");
					for (i=0; GlobalPunctuation[i]; ) {
						if (GlobalPunctuation[i] == '!' ||
							GlobalPunctuation[i] == '?' ||
							GlobalPunctuation[i] == '.') 
							strcpy(GlobalPunctuation+i,GlobalPunctuation+i+1);
						else
							i++;
					}
				} else {
					addword('\0','\0',"+0*");
					if (!linkMain2Mor)
						addword('\0','\0',"++*");
					addword('\0','\0',"+<#>");
					addword('\0','\0',"+unk|xxx");
					addword('\0','\0',"+unk|yyy");
					addword('\0','\0',"+*|www");
				}
			}
		}
	}
}

void getflag(char *f, char *f1, int *i) {
    f++;
    switch(*f++) {
		case 'a':
			leave_AT = TRUE;
			no_arg_option(f);
    		break;
		case 'c':
			if (*f == 'm' || *f == 'M')
				isMorFlo = TRUE;
			else if (*f == 'r' || *f == 'R')
				isRFlo = TRUE;
            else if (*f == 'a' || *f == 'A') {
                isMFAFlo = TRUE;
                addword('s','i',"+&+*");
                addword('s','i',"+&-*");
            } else if (*f == 'u' || *f == 'U') {
                isMFAFlo = TRUE;
                isUDFlo = TRUE;
                addword('s','i',"+&+*");
                addword('s','i',"+&-*");
            } else {
                fprintf(stderr,"Invalid argument for option (choose: 'm', 'r', or 'u'): %s\n", f-2);
                cutt_exit(0);
            }
            break;
		case 'd':
			if (*f == '2') {
			} else if (*f == '1')
				substitute_flag = 2; /* Flo line will replace main line */
			else if (*f == EOS || *f == '0')
				substitute_flag = 1; /* Flo line will replace main line */
			else {
				fprintf(stderr,"Invalid argument for option %s\n", f-2);
				cutt_exit(0);
			}
    		break;
		case 't':
			if (*f == '*' || (*f != '%' && *f != '@'))
				isSpeakerSpecified = TRUE;
			if (uS.mStricmp(f, "%mor") == 0) {
				linkMain2Mor = TRUE;
			}
		default:
		maingetflag(f-2,f1,i);
		break;
    }
}

static FLO_UTT *freeUtts(FLO_UTT *p) {
	FLO_UTT *t;
	
	while (p != NULL) {
		t = p;
		p = p->nextutt;
		free(t);
	}
	return(NULL);
}

static FLO_UTT *add2Utts(FLO_UTT *root_utts) {
	FLO_UTT *utt;
	
	if (root_utts == NULL) {
		utt = NEW(FLO_UTT);
		if (utt == NULL) {
			fprintf(stderr,"Error: out of memory\n");
			root_utts = freeUtts(root_utts);
			cutt_exit(0);
		}
		root_utts = utt;
	} else {
		for (utt=root_utts; utt->nextutt != NULL; utt=utt->nextutt) ;
		if ((utt->nextutt=NEW(FLO_UTT)) == NULL) {
			fprintf(stderr,"Error: out of memory\n");
			root_utts = freeUtts(root_utts);
			cutt_exit(0);
		}
		utt = utt->nextutt;
	}
	utt->nextutt = NULL;
	att_cp(0L, utt->speaker, utterance->speaker, utt->attSp, utterance->attSp);
	att_cp(0L, utt->line, utterance->line, utt->attLine, utterance->attLine);
	strcpy(utt->tuttline, utterance->tuttline);
	return(root_utts);
}


/*
 * makeflo: transforms codes in utterance string to make it readable.
 * It does the following transformations:
 *       - converts [...] to ...
 *       - converts [#]   to #
 *       - reinserts quotation marks
 *       - deletes other < > [xxx] constructs, retaining text in the <>
 *       - deletes all other text enclosed in []
 *       - deletes words starting with 0
 *       - deletes word internal - # + ^ ~ characters
 *       - deletes text found after an @ in a word
 *       - converts text found in () to '
*/
/*
static void makeflo(char *line, char *flo_line) {
    register int ui, fi, qi; // indexes into strings
	register int scope_start, scope_end; // indexes to < and > respectively

    ui = 0;
    fi = 0;
    scope_start = -1;	// this is 0 when not parsing a <>[] construct
    scope_end = -1;

    while (line[ui]) {
		switch (line[ui]) {
			case '-':
			case '#':
			case '+':
			case '^':
			case '~':
				ui++;
				break;
			case '<':
				// beginning of scope - record its position in flo_line
				scope_start = fi;
				flo_line[fi++] = line[ui++];
				break;
			case '>':
				// end of scope - record its position in flo_line
				scope_end = fi;
				flo_line[fi++] = line[ui++];
				break;
			case '[':
				// transform  [#] to #
				if(uS.isRightChar(line, ui+1, '#', &dFnt, MBF) && uS.isRightChar(line, ui+2, ']', &dFnt, MBF)) {
					flo_line[fi++] = '#';
					ui+=3;
				} else {
					// if ["], quote previous word or scoped set
					// '\042' is a "
					if(uS.isRightChar(line, ui+1, '\042', &dFnt, MBF) && uS.isRightChar(line, ui+2, ']', &dFnt, MBF)) {
						if(scope_start != -1) { // need to quote multi words
							flo_line[scope_start] = '\042';
							flo_line[scope_end] = '\042';
							ui += 3;
							scope_start = scope_end = -1;
						} else {	// need to quote only preceeding word
							// shift preceeding word right by 2
							for(qi = fi-1; uS.isskip(flo_line,qi,&dFnt,MBF) && flo_line[qi]; qi--)
								flo_line[qi+2] = flo_line[qi];
							flo_line[qi+2] = '\042';
							// shift this part by 1
							for(; !uS.isskip(flo_line,qi,&dFnt,MBF) && flo_line[qi]; qi--)
								flo_line[qi+1] = flo_line[qi];
							flo_line[qi+1] = '\042';
							fi++;
							ui += 3;
						}
					} else {
						// translate <stuff> [XXX] to stuff
						if(scope_start != -1) {
							for (qi=scope_start; qi+1<scope_end; qi++) {
								flo_line[qi] = flo_line[qi+1];
							}
							for (; qi+2 < fi; qi++) {
								flo_line[qi] = flo_line[qi+2];
							}
							fi = qi;
						}
						// delete stuff in []
						while (line[ui] && !uS.isRightChar(line, ui, ']', &dFnt, MBF)) {
							ui++;
						}
						ui++;
					}
				}
				break;
			case '@':
				// Delete chars in words after an @
				while ( !uS.isskip(line,ui++,&dFnt,MBF) && line[ui] )
					;
				// the word is not gone so save trailing white space
				ui--;
				break;
			case '0':
				// Delete words that begin with 0
				if (uS.isskip(line,ui+1,&dFnt,MBF))
					flo_line[fi++] = line[ui++];
				else while ( !uS.isskip(line,ui++,&dFnt,MBF) && line[ui] ) ;
				break;
			case '(':
				// replace (xx)yyy with 'yyy
				// TODO - what if while bumps out cause *uptr == \n
				while( line[ui] && !uS.isRightChar(line, ui, ')', &dFnt, MBF) && !uS.isRightChar(line, ui, '\n', &dFnt, MBF)) {
					ui++;
				}
				if (uS.isRightChar(line, ui, ')', &dFnt, MBF))
					ui++;
				flo_line[fi++] = '\047';	   // ascii for '
				break;
			default:
				// pass through untouched
				flo_line[fi++] = line[ui++];
				break;
		}
    }
    flo_line[fi] = EOS;
}
*/

static void filterAtSym(char *line) {
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

static void removeAngleBrackets(char *line) {
	long i;

	i = 0L;
	while (line[i] != EOS) {
		if ((i == 0 || line[i-1] != '+') && (line[i] == '<' || line[i] == '>')) {
			strcpy(line+i, line+i+1);
		} else
			i++;
	}
}

static void cleanupAnPercentFlo(char *line) {
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

static void outputUtts(FLO_UTT *root_utts) {
	int  i, j;
	char flo_tier_code[SPEAKERLEN+1];
	char isBulletFound;
	FNType Fname[FILENAME_MAX];
	long Beg, End;
	double DBeg, DEnd;
	FLO_UTT *utt;

	strcpy(flo_tier_code,"%flo:\t");
	if (linkMain2Mor) {
		spareTier1[0] = EOS;
		for (utt=root_utts; utt != NULL; utt=utt->nextutt) {
			if (uS.partcmp(utt->speaker, "%mor:", FALSE, TRUE)) {
				strcpy(spareTier1, utt->tuttline);
				if (!leave_AT)
					filterAtSym(spareTier1);
				removeAngleBrackets(spareTier1);
				removeDepTierItems(spareTier1);
				uS.remFrontAndBackBlanks(spareTier1);
				break;
			}
		}
		for (utt=root_utts; utt != NULL; utt=utt->nextutt) {
			if (utt->speaker[0] == '*') {
				if (!substitute_flag) { 
					printout(utt->speaker,utt->line,utt->attSp,utt->attLine,FALSE);
					if (spareTier1[0] != EOS)
						printout(flo_tier_code,spareTier1,NULL,NULL,TRUE);
				} else if (isRFlo || isMFAFlo) {
					for (i=0; spareTier1[i] != EOS; i++) {
						if (uS.IsUtteranceDel(spareTier1, i))
							spareTier1[i] = ' ';
					}
					uS.remFrontAndBackBlanks(spareTier1);
					if (isMFAFlo)
						cleanupAnPercentFlo(spareTier1);
					if (spareTier1[0] != EOS) {
						strcat(spareTier1, ".");
						printout(NULL,spareTier1,NULL,NULL,TRUE);
					}
				} else { 
					printout(utt->speaker,spareTier1,NULL,NULL,TRUE);
				}
			} else {
				if (uS.partcmp(utt->speaker, "%mor:", FALSE, TRUE)) {
					removeMainTierWords(utt->line);
					uS.remFrontAndBackBlanks(utt->line);
					printout(utt->speaker,utt->line,utt->attSp,NULL,TRUE);
				} else
					printout(utt->speaker,utt->line,utt->attSp,utt->attLine,FALSE);
			}
		}
	} else {
		for (utt=root_utts; utt != NULL; utt=utt->nextutt) {
			if (*utt->speaker == '*') {
				if (!leave_AT)
					filterAtSym(utt->tuttline);
				removeAngleBrackets(utt->tuttline);
				if (!substitute_flag) { 
					if (isAntConc) {
						i = 0;
						while (utt->line[i] != EOS) {
							if (utt->line[i] == HIDEN_C) {
								for (j=i+1; utt->line[j] != HIDEN_C && utt->line[j] != EOS; j++) ;
								if (utt->line[j] == HIDEN_C) {
									att_cp(0, utt->line+i, utt->line+j+1, utt->attLine+i, utt->attLine+j+1);
								} else
									i++;
							} else
								i++;
						}
						uS.remblanks(utt->line);
						fprintf(fpout, "%s", utt->speaker);
						fprintf(fpout, "%s\n", utt->line);
					} else {
						printout(utt->speaker,utt->line,utt->attSp,utt->attLine,FALSE);
						uS.remFrontAndBackBlanks(utt->tuttline);
						printout(flo_tier_code,utt->tuttline,NULL,NULL,TRUE);
					}
				} else if (isRFlo || isMFAFlo) {
					for (i=0; utt->tuttline[i] != EOS; i++) {
						if (uS.IsUtteranceDel(utt->tuttline, i))
							utt->tuttline[i] = ' ';
					}
					uS.remFrontAndBackBlanks(utt->tuttline);
					if (isMFAFlo)
						cleanupAnPercentFlo(utt->tuttline);
					if (utt->tuttline[0] != EOS) {
						strcat(utt->tuttline, ".");
						printout(NULL,utt->tuttline,NULL,NULL,TRUE);
					}
				} else if (substitute_flag == 2) {
					for (i=strlen(utt->speaker)-1; (isSpace(utt->speaker[i]) || utt->speaker[i] == ':') && i >= 0; i--) ;
					utt->speaker[i+1] = EOS;
					fprintf(fpout, "%s\t%s\t", utt->speaker+1, utt->speaker+1);
					isBulletFound = FALSE;
					for (i=0; utt->line[i] != EOS; i++) {
						if (utt->line[i] == HIDEN_C) {
							Fname[0] = EOS;
							if (isdigit(utt->line[i+1])) {
								if (cMediaFileName[0] == EOS) {
									fprintf(stderr, "\nMissing %s header tier with media file name.\n\n", MEDIAHEADER);
									cutt_exit(0);
								}
								strcpy(Fname, cMediaFileName);
								if (getMediaTagInfo(utt->line+i, &Beg, &End)) {
									isBulletFound = TRUE;
								}
							} else {
								if (getOLDMediaTagInfo(utt->line+i, SOUNDTIER, Fname, &Beg, &End)) {
									isBulletFound = TRUE;
								} else if (getOLDMediaTagInfo(utt->line+i, REMOVEMOVIETAG, Fname, &Beg, &End)) {
									isBulletFound = TRUE;
								}
							}
						}
					}
					if (isBulletFound) {
						DBeg = (double)Beg;
						DEnd = (double)End;
						DBeg = DBeg / 1000.0000;
						DEnd = DEnd / 1000.0000;
						fprintf(fpout, "%.3lf\t%.3lf\t", DBeg, DEnd);
					} else {
						fprintf(stderr, "\nMissing bullet on tier:\n%s\n", utt->line);
						cutt_exit(0);
					}
					for (i=0; utt->tuttline[i] != EOS; i++) {
						if (utt->tuttline[i] == '\n')
							utt->tuttline[i] = ' ';
					}
					uS.remblanks(utt->tuttline);
					removeExtraSpace(utt->tuttline);
					fprintf(fpout, "%s\n", utt->tuttline);
				} else {
					uS.remFrontAndBackBlanks(utt->tuttline);
					printout(utt->speaker,utt->tuttline,utt->attSp,NULL,TRUE);
				}
			} else {
				if (isAntConc && utt->speaker[0] == '@') {
//					if (utt->line[0] == EOS)
//						uS.remblanks(utt->speaker);
					uS.remblanks(utt->line);
					uS.shiftright(utt->speaker, 1);
					utt->speaker[0] = '<';
					i = strlen(utt->line);
					utt->line[i++] = '>';
					utt->line[i++] = '\n';
					utt->line[i++] = EOS;
					fprintf(fpout, "%s", utt->speaker);
					fprintf(fpout, "%s", utt->line);
				} else
					printout(utt->speaker,utt->line,utt->attSp,utt->attLine,FALSE);
			}
		}
	}
}

void call()
{
	FLO_UTT *root_utts;
	
	if (isAntConc)
		fprintf(fpout, "%c%c%c", 0xef, 0xbb, 0xbf); // BOM UTF8

	/* build %flo: with proper trailing spaces */
	root_utts = NULL;
	currentatt = 0;
	currentchar = (char)getc_cr(fpin, &currentatt);
	while (getwholeutter()) {
		if (uS.isUTF8(utterance->speaker) || uS.isInvisibleHeader(utterance->speaker)) {
			if (isAntConc)
				continue;
		}
		if (utterance->speaker[0] == '*') {
			if (root_utts != NULL) {
				outputUtts(root_utts);
				root_utts = freeUtts(root_utts);
			}
			root_utts = add2Utts(root_utts);
		} else {
			root_utts = add2Utts(root_utts);
		}
	}
	if (root_utts != NULL) {
		outputUtts(root_utts);
		root_utts = freeUtts(root_utts);
	}
}

CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	int i;

    isWinMode = IS_WIN_MODE;
    CLAN_PROG_NUM = FLO;
    chatmode = CHAT_MODE;
    OnlydataLimit = 0;
    UttlineEqUtterance = FALSE;
	isAntConc = FALSE;
	isMFAFlo = FALSE;
    isUDFlo = FALSE;
	for (i=0; i < argc; i++) {
		if (argv[i][0] == '-' || argv[i][0] == '+') {
			if (argv[i][1] == 'd' && argv[i][2] == '2')
				isAntConc = TRUE;
			else if (argv[i][1] == 'c' && argv[i][2] == 'a')
				isMFAFlo = TRUE;
			else if (argv[i][1] == 'c' && argv[i][2] == 'u') {
				isUDFlo = TRUE;
                isMFAFlo = TRUE;
            }
		}
	}
    bmain(argc,argv,NULL);
}
