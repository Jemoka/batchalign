/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#define CHAT_MODE 0

#include "cu.h"
#include "cutt-xml.h"

#define IS_WIN_MODE FALSE
#include "mul.h" 

extern struct tier *defheadtier;
extern char OverWriteFile;
extern char cutt_isMultiFound;

#define IDSTRLEN 128
#define ELANCHATTIERS struct ElanChatTiers
struct ElanChatTiers {
	char isWrap;
	char ID[IDSTRLEN+1];
	int  IDn;
	char *refSp;
	char *sp;
	char *line;
	long beg, end;
	ELANCHATTIERS *depTiers;
	ELANCHATTIERS *nextTier;
} ;

static char mediaFName[FILENAME_MAX+2];
static char isMultiBullets, isMFA;
static ELANCHATTIERS *e2c_RootTiers, *RootUnmatchedTiers;
static long *Times_table, Times_index;
static Element *Elan_stack[30];

void usage() {
	printf("convert Elan XML files to CHAT files\n");
	printf("Usage: elan2chat [b %s] filename(s)\n",mainflgs());
	puts("+b: Specify that multiple bullets per line (default only one bullet per line).");
	puts("+c: The input .eaf file was created by MFA aligner.");
	mainusage(TRUE);
}

void init(char s) {
	if (s) {
		OverWriteFile = TRUE;
		AddCEXExtension = ".cha";
		stout = FALSE;
		onlydata = 1;
		e2c_RootTiers = NULL;
		RootUnmatchedTiers = NULL;
		CurrentElem = NULL;
		Times_table = NULL;
		Times_index = 0L;
		isMultiBullets = FALSE;
		isMFA = FALSE;
		if (defheadtier) {
			if (defheadtier->nexttier != NULL)
				free(defheadtier->nexttier);
			free(defheadtier);
			defheadtier = NULL;
		}
	}
}

CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = ELAN2CHAT;
	OnlydataLimit = 0;
	UttlineEqUtterance = FALSE;
	bmain(argc,argv,NULL);
}

void getflag(char *f, char *f1, int *i) {

	f++;
	switch(*f++) {
		case 'b':
			isMultiBullets = TRUE;
			no_arg_option(f);
			break;
		case 'c':
			isMultiBullets = TRUE;
			isMFA = TRUE;
			no_arg_option(f);
			break;
		default:
			maingetflag(f-2,f1,i);
			break;
	}
}

static ELANCHATTIERS *freeTiers(ELANCHATTIERS *p) {
	ELANCHATTIERS *t;

	while (p != NULL) {
		t = p;
		p = p->nextTier;
		if (t->depTiers != NULL)
			freeTiers(t->depTiers);
		if (t->refSp != NULL)
			free(t->refSp);
		if (t->sp != NULL)
			free(t->sp);
		if (t->line != NULL)
			free(t->line);
		free(t);
	}
	return(NULL);
}

static void Elan_fillNT(ELANCHATTIERS *nt, const char *ID, long beg, long end, const char *refSp, const char *sp, const char *line, char isAddBullet) {
	if (beg == end)
		isAddBullet = FALSE;
	if (isAddBullet && (beg != 0L || end != 0L))
		sprintf(templineC3, " %c%ld_%ld%c", HIDEN_C, beg, end, HIDEN_C);
	else
		templineC3[0] = EOS;

	nt->depTiers = NULL;
	strcpy(nt->ID, ID);
	nt->IDn = atoi(ID+1);
	nt->isWrap = TRUE;
	if (refSp == NULL) {
		nt->refSp = (char *)malloc(strlen("")+1);
		if (nt->refSp == NULL)
			out_of_mem();
		strcpy(nt->refSp, "");
	} else {
		nt->refSp = (char *)malloc(strlen(refSp)+1);
		if (nt->refSp == NULL)
			out_of_mem();
		strcpy(nt->refSp, refSp);
	}
	nt->sp = (char *)malloc(strlen(sp)+1);
	if (nt->sp == NULL) out_of_mem();
	strcpy(nt->sp, sp);
	nt->line = (char *)malloc(strlen(line)+strlen(templineC3)+1);
	if (nt->line == NULL) out_of_mem();
	strcpy(nt->line, line);
	if (templineC3[0] != EOS)
		strcat(nt->line, templineC3);
	nt->beg = beg;
	nt->end = end;
}

static void Elan_refillNTline(ELANCHATTIERS *nt, char *ID, long beg, long end, char *line) {
	char *oline;

	oline = nt->line;

	if (beg != end && (beg != 0L || end != 0L))
		sprintf(templineC3, " %c%ld_%ld%c", HIDEN_C, beg, end, HIDEN_C);
	else
		templineC3[0] = EOS;

	if (isMultiBullets) {
		nt->isWrap = FALSE;
	} else {
		nt->isWrap = TRUE;
	}
	if (strlen(nt->ID)+strlen(ID) < IDSTRLEN)
		strcat(nt->ID, ID);
	nt->end = end;
	nt->line = (char *)malloc(strlen(oline)+strlen(line)+strlen(templineC3)+3);
	if (nt->line == NULL) out_of_mem();
	strcpy(nt->line, oline);
	strcat(nt->line, "\n\t");
	strcat(nt->line, line);
	if (templineC3[0] != EOS)
		strcat(nt->line, templineC3);
	free(oline);
}

static void Elan_replaceNT(ELANCHATTIERS *nt, char *ID, long beg, long end, char *line) {
	if (beg != end && (beg != 0L || end != 0L))
		sprintf(templineC3, " %c%ld_%ld%c", HIDEN_C, beg, end, HIDEN_C);
	else
		templineC3[0] = EOS;

	strcpy(nt->ID, ID);
	nt->IDn = atoi(ID+1);
	nt->isWrap = TRUE;
	if (nt->line != NULL)
		free(nt->line);
	nt->line = (char *)malloc(strlen(line)+strlen(templineC3)+1);
	if (nt->line == NULL) out_of_mem();
	strcpy(nt->line, line);
	if (templineC3[0] != EOS)
		strcat(nt->line, templineC3);
	nt->beg = beg;
	nt->end = end;
}

static ELANCHATTIERS *Elan_addDepTiers(ELANCHATTIERS *depTiers, char *ID, long beg, long end, char *sp, char *line) {
	ELANCHATTIERS *nt, *tnt;

	if (depTiers == NULL) {
		if ((depTiers=NEW(ELANCHATTIERS)) == NULL)
			out_of_mem();
		nt = depTiers;
		nt->nextTier = NULL;
	} else {
		tnt= depTiers;
		nt = depTiers;
		while (nt != NULL) {
			if (!strcmp(nt->sp, sp)) {
				Elan_refillNTline(nt, ID, beg, end, line);
				return(depTiers);
			}
			if (beg < nt->beg)
				break;
			tnt = nt;
			nt = nt->nextTier;
		}
		if (isMFA == true) {
			if (uS.partcmp(sp, "%wor", FALSE, TRUE) || uS.partcmp(sp, "%xwor", FALSE, TRUE))
				nt = depTiers;
		}
		if (nt == NULL) {
			tnt->nextTier = NEW(ELANCHATTIERS);
			if (tnt->nextTier == NULL)
				out_of_mem();
			nt = tnt->nextTier;
			nt->nextTier = NULL;
		} else if (nt == depTiers) {
			depTiers = NEW(ELANCHATTIERS);
			if (depTiers == NULL)
				out_of_mem();
			depTiers->nextTier = nt;
			nt = depTiers;
		} else {
			nt = NEW(ELANCHATTIERS);
			if (nt == NULL)
				out_of_mem();
			nt->nextTier = tnt->nextTier;
			tnt->nextTier = nt;
		}
	}

	Elan_fillNT(nt, ID, beg, end, NULL, sp, line, TRUE);
	return(depTiers);
}

static ELANCHATTIERS *Elan_insertNT(ELANCHATTIERS *nt, ELANCHATTIERS *tnt) {
	if (nt == e2c_RootTiers) {
		e2c_RootTiers = NEW(ELANCHATTIERS);
		if (e2c_RootTiers == NULL)
			out_of_mem();
		e2c_RootTiers->nextTier = nt;
		nt = e2c_RootTiers;
	} else {
		nt = NEW(ELANCHATTIERS);
		if (nt == NULL)
			out_of_mem();
		nt->nextTier = tnt->nextTier;
		tnt->nextTier = nt;
	}
	return(nt);
}

static ELANCHATTIERS *Elan_add2Unmatched(void) {
	ELANCHATTIERS *nt;

	if (RootUnmatchedTiers == NULL) {
		RootUnmatchedTiers = NEW(ELANCHATTIERS);
		nt = RootUnmatchedTiers;
	} else {
		for (nt=RootUnmatchedTiers; nt->nextTier != NULL; nt=nt->nextTier) ;
		nt->nextTier = NEW(ELANCHATTIERS);
		nt = nt->nextTier;
	}
	if (nt == NULL)
		out_of_mem();
	nt->nextTier = NULL;
	nt->depTiers = NULL;
	return(nt);
}

static char Elan_isUttDel(char *line) {
	char bullet;
	long i;

/* 2010-01-20
	if (isMultiBullets)
		return(FALSE);
*/
	bullet = FALSE;
	i = strlen(line);
	for (; i >= 0L; i--) {
		if ((uS.IsUtteranceDel(line, i) || uS.IsCAUtteranceDel(line, i)) && !bullet)
			return(TRUE);
		if (line[i] == HIDEN_C)
			bullet = !bullet;
	}
	return(FALSE);
}

static char Elan_isPostCodes(char *line) {
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

static char isFoundBegOverlap(char *line) {
	for (; *line != EOS; line++) {
		if (UTF8_IS_LEAD((unsigned char)*line) && *line == (char)0xe2 && *(line+1) == (char)0x8c) {
			if (*(line+2) == (char)0x88) // raised [
				return(TRUE);
			if (*(line+2) == (char)0x8a) // lowered [
				break;
		}
	}

	return(FALSE);
}

static char Elan_isLineJustPause(char *line) {
	int i;

	for (i=0; isSpace(line[i]); i++) ;
	if (uS.isRightChar(line,i,'(',&dFnt,MBF) && uS.isPause(line,i,NULL,&i)) {
		if (uS.isRightChar(line,i,')',&dFnt,MBF)) {
			for (i++; isSpace(line[i]); i++) ;
			if (line[i] == EOS || line[i] == '\n')
				return(TRUE);
		}
	}
	return(FALSE);
}

static ELANCHATTIERS *isIDMatch(ELANCHATTIERS *nt, const char *refID, char isCheckNext) {
	char *ID;
	ELANCHATTIERS *res;

	while (nt != NULL) {
		for (ID=nt->ID; *ID != EOS; ID++) {
			if (uS.partwcmp(ID, refID))
				return(nt);
		}
		if (nt->depTiers != NULL) {
			res = isIDMatch(nt->depTiers, refID, TRUE);
			if (res != NULL)
				return(res);
		}
		if (isCheckNext)
			nt = nt->nextTier;
		else
			break;
	}
	return(NULL);
}

static char insertIntoMatchedRef(ELANCHATTIERS *nt, const char *refID, char *sp, char *line) {
	int  i, j, len;
	char *ID;

	if (nt->line == NULL)
		templineC[0] = EOS;
	else
		strcpy(templineC, nt->line);
	i = 0;
	for (ID=nt->ID; *ID != EOS; ID++) {
		if (uS.partwcmp(ID, refID)) {
			for (; templineC[i] != EOS && templineC[i] != '\n' && templineC[i] != HIDEN_C; i++) ;
			len = 0;
			for (j=0; sp[j] != EOS && sp[j] != ':' && sp[j] != '-'; j++)
				len++;
			len = len + strlen(line) + 7;
			uS.shiftright(templineC+i, len);
			templineC[i++] = ' ';
			templineC[i++] = '[';
			templineC[i++] = '%';
			for (j=0; sp[j] != EOS && sp[j] != ':' && sp[j] != '-'; j++)
				templineC[i++] = sp[j];
			templineC[i++] = ':';
			templineC[i++] = ' ';
			for (j=0; line[j] != EOS; j++)
				templineC[i++] = line[j];
			templineC[i++] = ']';
			templineC[i++] = ' ';
			if (nt->line != NULL)
				free(nt->line);
			nt->line = (char *)malloc(strlen(templineC)+3);
			if (nt->line == NULL)
				out_of_mem();
			strcpy(nt->line, templineC);
			return(TRUE);
		} else if (*ID == '|') {
			for (; templineC[i] != EOS && templineC[i] != '\n'; i++) ;
			if (templineC[i] == EOS)
				return(FALSE);
			else
				i++;
		}
	}
	return(FALSE);
}

static void Elan_addToTiersID(char *ID, const char *refID, long beg, long end, char *sp, char *line, char isCreateEmpty) {
	int  IDn;
	char isPostCodeFound, isJustPause;
	ELANCHATTIERS *nt, *tnt, *resNT;

	if (sp[0] == '%' && refID[0] == EOS) {
		fprintf(stderr, "\n*** ERROR: ID=%s: refID is empty\n", ID);
		fprintf(stderr, "%s\t%s\n\n", sp, line);
		cutt_exit(0);
	} 
	if (e2c_RootTiers == NULL) {
		if ((e2c_RootTiers=NEW(ELANCHATTIERS)) == NULL)
			out_of_mem();
		nt = e2c_RootTiers;
		nt->nextTier = NULL;
	} else {
		IDn = atoi(ID+1);
		isJustPause = Elan_isLineJustPause(line);
		isPostCodeFound = Elan_isPostCodes(line);
		tnt= e2c_RootTiers;
		nt = e2c_RootTiers;
		while (nt != NULL) {
			if (sp[0] == '*' || sp[0] == '@') {
				if (IDn < nt->IDn) {
					break;
				}
			} else if (sp[0] == '%' && refID[0] != EOS && (resNT=isIDMatch(nt, refID, FALSE)) != NULL) {
				if (resNT->sp[0] == '*')
					resNT->depTiers = Elan_addDepTiers(resNT->depTiers, ID, beg, end, sp, line);
				else if (insertIntoMatchedRef(resNT, refID, sp, line) == FALSE)
					nt->depTiers = Elan_addDepTiers(nt->depTiers, ID, beg, end, sp, line);
				return;
			}
			tnt = nt;
			nt = nt->nextTier;
		}
		if (nt == NULL) {
			if (sp[0] == '%') {
				fprintf(stderr, "\n*** ERROR: ID=%s: can't find refID (%s) speaker this tier belongs to\n", ID, refID);
				fprintf(stderr, "%s\t%s\n\n", sp, line);
				cutt_exit(0);
			} else {
				tnt->nextTier = NEW(ELANCHATTIERS);
				if (tnt->nextTier == NULL)
					out_of_mem();
				nt = tnt->nextTier;
				nt->nextTier = NULL;
			}
		} else
			nt = Elan_insertNT(nt, tnt);
	}
	Elan_fillNT(nt, ID, beg, end, NULL, sp, line, TRUE);
}

static void Elan_addToTiers(char *ID, const char *refID, long beg, long end, const char *refSp, char *sp, char *line, char isCreateEmpty) {
	char isRefIDMatch, isPostCodeFound, isJustPause;
	ELANCHATTIERS *nt, *tnt, *resNT;

	if (e2c_RootTiers == NULL) {
		if ((e2c_RootTiers=NEW(ELANCHATTIERS)) == NULL)
			out_of_mem();
		nt = e2c_RootTiers;
		nt->nextTier = NULL;
	} else {
		isRefIDMatch = FALSE;
		isJustPause = Elan_isLineJustPause(line);
		isPostCodeFound = Elan_isPostCodes(line);
		tnt= e2c_RootTiers;
		nt = e2c_RootTiers;
		while (nt != NULL) {
			if (sp[0] == '%' && refID[0] != EOS && (resNT=isIDMatch(nt, refID, FALSE)) != NULL) {
				isRefIDMatch = TRUE;
				if (beg == 0L && end == 0L) {
					if (resNT->sp[0] == '*') {
//						beg = resNT->beg;
//						end = resNT->end;
						nt->depTiers = Elan_addDepTiers(resNT->depTiers, ID, beg, end, sp, line);
					} else if (insertIntoMatchedRef(resNT, refID, sp, line) == FALSE) {
						nt->depTiers = Elan_addDepTiers(nt->depTiers, ID, beg, end, sp, line);
					}
				} else {
					nt->depTiers = Elan_addDepTiers(nt->depTiers, ID, beg, end, sp, line);
				}
				return;
			} else if (sp[0] == '%' && refID[0] == EOS) {
				if (beg >= nt->beg && beg < nt->end && uS.partcmp(refSp, nt->sp, FALSE, TRUE)) {
					nt->depTiers = Elan_addDepTiers(nt->depTiers, ID, beg, end, sp, line);
					return;
				} else if (end-beg < 3 && beg >= nt->beg && beg <= nt->end && uS.partcmp(refSp, nt->sp, FALSE, TRUE)) {
					nt->depTiers = Elan_addDepTiers(nt->depTiers, ID, beg, end, sp, line);
					return;
				} else if (beg < nt->beg) {
					if (isCreateEmpty) {
						nt = Elan_insertNT(nt, tnt);
						Elan_fillNT(nt, "0", beg, end, NULL, refSp, "0.", TRUE);
						nt->depTiers = Elan_addDepTiers(nt->depTiers, ID, beg, end, sp, line);
					} else {
						nt = Elan_add2Unmatched();
						Elan_fillNT(nt, ID, beg, end, refSp, sp, line, FALSE);
					}
					return;
				}
			} else if (sp[0] == '*') {
				if (beg>=nt->end && beg<=nt->end+50 && !strcmp(sp,nt->sp) && (!Elan_isUttDel(nt->line) || isJustPause)) {
					Elan_refillNTline(nt, ID, beg, end, line);
					return;
				} else if (beg == nt->end && !strcmp(sp, nt->sp) && isPostCodeFound) {
					Elan_refillNTline(nt, ID, beg, end, line);
					return;
				} else if (beg >= nt->beg-50 && beg <= nt->beg+50 && end >= nt->end-50 && end <= nt->end+50 &&
						   !strcmp(sp, nt->sp) && !strcmp(nt->ID, "0")) {
					Elan_replaceNT(nt, ID, beg, end, line);
					return;
				} else if (beg == nt->beg && isFoundBegOverlap(line)) {
					break;
				} else if (beg < nt->beg)
					break;
			}
			tnt = nt;
			nt = nt->nextTier;
		}
		if (nt == NULL) {
			if (!isRefIDMatch && sp[0] == '%' && refID[0] != EOS) {

			}
			if (sp[0] == '%' && refID[0] == EOS) {
				if (isCreateEmpty) {
					tnt->nextTier = NEW(ELANCHATTIERS);
					if (tnt->nextTier == NULL)
						out_of_mem();
					nt = tnt->nextTier;
					nt->nextTier = NULL;
					Elan_fillNT(nt, "0", beg, end, NULL, refSp, "0.", TRUE);
					nt->depTiers = Elan_addDepTiers(nt->depTiers, ID, beg, end, sp, line);
				} else {
					nt = Elan_add2Unmatched();
					Elan_fillNT(nt, ID, beg, end, refSp, sp, line, FALSE);
				}
				return;
			} else {
				tnt->nextTier = NEW(ELANCHATTIERS);
				if (tnt->nextTier == NULL)
					out_of_mem();
				nt = tnt->nextTier;
				nt->nextTier = NULL;
			}
		} else
			nt = Elan_insertNT(nt, tnt);
	}

	Elan_fillNT(nt, ID, beg, end, refSp, sp, line, TRUE);
}

static void Elan_addHeadersTiers(char *ID, const char *refID, char *sp, char *line) {
	ELANCHATTIERS *nt, *resNT, *headNt;
	nt = e2c_RootTiers;
	while (nt != NULL) {
		if (refID[0] != EOS && (resNT=isIDMatch(nt, refID, FALSE)) != NULL) {
			headNt = NEW(ELANCHATTIERS);
			if (headNt == NULL)
				out_of_mem();
			headNt->depTiers = NULL;
			headNt->nextTier = resNT->nextTier;
			resNT->nextTier = headNt;
			Elan_fillNT(headNt, ID, 0L, 0L, NULL, sp, line, FALSE);
			break;
		}
		nt = nt->nextTier;
	}
}

static char Elan_isOverlapFound(char *line, char ovChar) {
	long i;

	for (i=0L; line[i] != EOS; i++) {
		if (line[i] == (char)0xe2 && line[i+1] == (char)0x8c && line[i+2] == (char)ovChar)
			return(TRUE);
	}
	return(FALSE);
}

static void Elan_finalTimeSort(void) {
	char isWrongOverlap;
	ELANCHATTIERS *nt, *prev_nt, *prev_prev_nt;

	nt = e2c_RootTiers;
	prev_nt = nt;
	prev_prev_nt = nt;
	while (nt != NULL) {
		if (prev_nt != nt) {
			if (nt->beg == prev_nt->beg && nt->end == prev_nt->end) {
				prev_nt->nextTier = nt->nextTier;
				nt->nextTier = prev_nt;
				if (prev_prev_nt == e2c_RootTiers) {
					e2c_RootTiers = nt;
					prev_prev_nt = e2c_RootTiers;
				} else
					prev_prev_nt->nextTier = nt;
				prev_nt = prev_prev_nt;
				nt = prev_nt->nextTier;
				while (nt->nextTier != NULL) {
					if (nt->nextTier->beg != nt->beg || nt->nextTier->end != nt->end)
						break;
					prev_prev_nt = prev_nt;
					prev_nt = nt;
					nt = nt->nextTier;
				}
			} else if (nt->beg == prev_nt->beg && nt->end < prev_nt->end) {
				prev_nt->nextTier = nt->nextTier;
				nt->nextTier = prev_nt;
				if (prev_prev_nt == e2c_RootTiers) {
					e2c_RootTiers = nt;
					prev_prev_nt = e2c_RootTiers;
				} else
					prev_prev_nt->nextTier = nt;
				prev_nt = prev_prev_nt;
				nt = prev_nt->nextTier;
			} else if (nt->beg == prev_nt->beg && nt->end == prev_nt->end) {
				isWrongOverlap = 0;
				if (Elan_isOverlapFound(nt->line, (char)0x88) && !Elan_isOverlapFound(nt->line, (char)0x8a))
					isWrongOverlap++;
				if (isWrongOverlap == 1) {
					if (Elan_isOverlapFound(prev_nt->line, (char)0x8a) && !Elan_isOverlapFound(prev_nt->line, (char)0x88))
						isWrongOverlap++;
				}
				if (isWrongOverlap == 2) {
					prev_nt->nextTier = nt->nextTier;
					nt->nextTier = prev_nt;
					if (prev_prev_nt == e2c_RootTiers) {
						e2c_RootTiers = nt;
						prev_prev_nt = e2c_RootTiers;
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
}

static char Elan_isSpecialCode(char isWrap, char *line) {
	int i, bulletCnt;

	bulletCnt = 0;
	for (i=0; line[i] != EOS; i++) {
		if (strncmp(line+i, "[%%", 3) == 0)
			return(FALSE);
		if (line[i] == HIDEN_C && isdigit(line[i+1]))
			bulletCnt++;
		if (bulletCnt > 1)
			return(FALSE);
	}
	return(isWrap);
}

static void Elan_printOutTiers(ELANCHATTIERS *p) {
	while (p != NULL) {
//int i;
//if (strcmp(p->ID, "a31|") == 0)
//	i = 0;
		if (p->sp[0] == '*' || p->line[0] != EOS)
			printout(p->sp, p->line, NULL, NULL, Elan_isSpecialCode(p->isWrap, p->line));
		if (p->depTiers != NULL)
			Elan_printOutTiers(p->depTiers);
		p = p->nextTier;
	}
}

static void Elan_makeText(char *line) {
	char *e;
	unsigned int c;

	while (*line != EOS) {
		if (!strncmp(line, "&amp;", 5)) {
			*line = '&';
			line++;
			strcpy(line, line+4);
		} else if (!strncmp(line, "&quot;", 6)) {
			*line = '"';
			line++;
			strcpy(line, line+5);
		} else if (!strncmp(line, "&apos;", 6)) {
			*line = '\'';
			line++;
			strcpy(line, line+5);
		} else if (!strncmp(line, "&lt;", 4)) {
			*line = '<';
			line++;
			strcpy(line, line+3);
		} else if (!strncmp(line, "&gt;", 4)) {
			*line = '>';
			line++;
			strcpy(line, line+3);
		} else if (*line == '{' && *(line+1) == '0' && *(line+2) == 'x') {
			for (e=line; *e != EOS && *e != '}'; e++) ;
			if (*e == '}') {
				sscanf(line+1, "%x", &c);
				*line = c;
				line++;
				strcpy(line, e+1);
			} else
				line++;
		} else
			line++;
	}
}

static void Elan_addUnmacthed(void) {
	ELANCHATTIERS *unmt;

	for (unmt=RootUnmatchedTiers; unmt != NULL; unmt=unmt->nextTier) {
		Elan_addToTiers(unmt->ID, "", unmt->beg, unmt->end, unmt->refSp, unmt->sp, unmt->line, TRUE);
	}
}

/* Elan-EAF Begin **************************************************************** */
static void sortElanSpAndDepTiers(void) {
	char firstDepTierFound;
	int  lStackIndex;
	Attributes *att;
	Element *nt, *tnt, *firstTier;
	
	if (CurrentElem == NULL)
		return;
	
	firstTier = NULL;
	firstDepTierFound = FALSE;
	lStackIndex = -1;
	nt = CurrentElem;
	tnt = nt;
	do {
		if (nt != NULL && !strcmp(nt->name, "ANNOTATION_DOCUMENT") && nt->next == NULL && lStackIndex == -1) {
			nt = nt->data;
			lStackIndex++;
			if (nt == NULL)
				return;
		}
		if (!strcmp(nt->name, "TIER") && lStackIndex == 0) {
			if (firstTier == NULL)
				firstTier = tnt;
			for (att=nt->atts; att != NULL; att=att->next) {
				if (!strcmp(att->name, "TIER_ID")) {
					if (att->value[0] == '%')
						firstDepTierFound = TRUE;
					else if (firstDepTierFound && att->value[0] == '*' && tnt != NULL) {
						tnt->next = nt->next;
						nt->next = firstTier->next;
						firstTier->next = nt;
						nt = tnt;
					}
					break;
				}
			}
		}
		tnt = nt;
		nt = nt->next;
	} while (nt != NULL);
}

static void setElanTimeOrderElem(void) {
	int  lStackIndex;
	char isTimeValueFound;
	long index;
	long timeValue = 0L;
	Attributes *att;
	Element *TimeSlots, *Time_Order = NULL;
	
	if (CurrentElem == NULL)
		return;
	lStackIndex = -1;
	Time_Order = CurrentElem;
	do {
		if (Time_Order != NULL && !strcmp(Time_Order->name, "ANNOTATION_DOCUMENT") && Time_Order->next == NULL && lStackIndex == -1) {
			Time_Order = Time_Order->data;
			lStackIndex++;
		} else if (Time_Order != NULL)
			Time_Order = Time_Order->next;
		
		if (Time_Order == NULL) {
			return;
		}
		if (!strcmp(Time_Order->name, "TIME_ORDER") && lStackIndex == 0) {
			Times_index = 0L;
			TimeSlots = Time_Order->data;
			while (TimeSlots != NULL) {
				for (att=TimeSlots->atts; att != NULL; att=att->next) {
					if (!strcmp(att->name, "TIME_SLOT_ID")) {
						index = atol(att->value+2);
						if (index > Times_index)
							Times_index = index;
						break;
					}
				}
				TimeSlots = TimeSlots->next;
			}
			
			if (Times_index > 0) {
				Times_index++;
				Times_table = (long *)malloc((size_t)Times_index * (size_t)sizeof(long));
				for (index=0; index < Times_index; index++)
					Times_table[index] = -1L;
			}
			
			if (Times_table == NULL)
				return;
			
			TimeSlots = Time_Order->data;
			while (TimeSlots != NULL) {
				index = 0L;
				timeValue = 0L;
				isTimeValueFound = FALSE;
				for (att=TimeSlots->atts; att != NULL; att=att->next) {
					if (!strcmp(att->name, "TIME_SLOT_ID")) {
						index = atol(att->value+2);
						if (isMFA == TRUE) {
							timeValue = 0L;
							isTimeValueFound = TRUE;
						}
					} else if (!strcmp(att->name, "TIME_VALUE")) {
						isTimeValueFound = TRUE;
						timeValue = atol(att->value);
					} 
				}
				if (isTimeValueFound)
					Times_table[index] = timeValue;
				TimeSlots = TimeSlots->next;
			}
			
			freeElements(Time_Order->data);
			Time_Order->data = NULL;
			return;
		}
	} while (1);
}

static long getElanTimeValue(char *TimeSlotIDSt) {
	long index;
	
	if (Times_table == NULL)
		return(0L);
	
	index = atol(TimeSlotIDSt+2);
	if (index >= 0L && index < Times_index) {
		if (isMFA == TRUE && Times_table[index] == -1) {
			fprintf(stderr, "\nCan't find TIME_SLOT_ID \"%s\" in <TIME_ORDER> table\n", TimeSlotIDSt);
			return(0);
		} else
			return(Times_table[index]);
	} else
		return(0L);
}

static void getCurrentElanData(Element *cElem, char *line, int max) {
	Element *data;
	
	if (cElem->cData != NULL) {
		strncpy(line, cElem->cData, max);
		line[max-1] = EOS;
	} else if (cElem->data != NULL) {
		line[0] = EOS;
		for (data=cElem->data; data != NULL; data=data->next) {
			if (!strcmp(data->name, "CONST")) {
				strncpy(line, data->cData, max);
				line[max-1] = EOS;
			}
		}
	} else
		line[0] = EOS;
}

static char getElanAnnotation(Element *cElem, char *line, int max, long *beg, long *end, char *ID, char *refID) {
	Attributes *att;
	
	if (cElem == NULL) {
		if (*end == -1L)
			*end = 0L;
		return(FALSE);
	}
	
	ID[0]    = EOS;
	refID[0] = EOS;
	*beg = 0L;
	*end = 0L;
	
	for (att=cElem->atts; att != NULL; att=att->next) {
		if (!strcmp(att->name, "ANNOTATION_ID")) {
			strcpy(ID, att->value);
			strcat(ID, "|");
		} else if (!strcmp(att->name, "ANNOTATION_REF")) {
			strcpy(refID, att->value);
			strcat(refID, "|");
		} else if (!strcmp(att->name, "TIME_SLOT_REF1")) {
			*beg = getElanTimeValue(att->value);
		} else if (!strcmp(att->name, "TIME_SLOT_REF2")) {
			*end = getElanTimeValue(att->value);
		} 
	}
	
	cElem = cElem->data;
	line[0] = EOS;
	if (cElem == NULL || strcmp(cElem->name, "ANNOTATION_VALUE")) {
		if (*end == -1L)
			*end = 0L;
		return(FALSE);
	}
	getCurrentElanData(cElem, line, max);
	return(TRUE);
}

static char getNextElanTier(UTTER *utterance, long *beg, long *end, char *ID, char *refID) {
	Attributes *att;
	int  uttMax, lineOffset;
	long orgBeg;
	char *p;
	char tier_id[SPEAKERLEN+2];
	char parent_ref[SPEAKERLEN+2];
	
	if (CurrentElem == NULL)
		return(FALSE);
	
	do {
		if (CurrentElem != NULL && !strcmp(CurrentElem->name, "ANNOTATION_DOCUMENT") && CurrentElem->next == NULL)
			CurrentElem = CurrentElem->data;
		else if (CurrentElem != NULL)
			CurrentElem = CurrentElem->next;
		
		if (CurrentElem == NULL) {
			if (stackIndex >= 0) {
				CurrentElem = Elan_stack[stackIndex];
				stackIndex--;
				freeElements(CurrentElem->data);
				CurrentElem->data = NULL;
				CurrentElem = CurrentElem->next;
				if (CurrentElem == NULL)
					return(FALSE);
			} else
				return(FALSE);
		}
		
		if (!strcmp(CurrentElem->name, "TIER") && CurrentElem->data != NULL) {
			if (stackIndex < 29) {
				tier_id[0] = EOS;
				parent_ref[0] = EOS;
				for (att=CurrentElem->atts; att != NULL; att=att->next) {
					if (!strcmp(att->name, "TIER_ID")) {
						strncpy(tier_id, att->value, SPEAKERLEN);
						tier_id[SPEAKERLEN-1] = EOS;
					} else if (!strcmp(att->name, "PARENT_REF")) {
						strncpy(parent_ref, att->value, SPEAKERLEN);
						parent_ref[SPEAKERLEN-1] = EOS;
					}
				}
				if (parent_ref[0] == EOS && (isMFA == FALSE || tier_id[0] != '@')) {
					p = strchr(tier_id, '@');
					if (p != NULL)
						strcpy(parent_ref, p+1);
				}
				utterance->speaker[0] = EOS;
				if (parent_ref[0] == EOS) {
					if (tier_id[0] != '*' && (isMFA == FALSE || tier_id[0] != '@'))
						strcat(utterance->speaker, "*");
					strcat(utterance->speaker, tier_id);
				} else {
					if (tier_id[0] != '%')
						strcat(utterance->speaker, "%");
					p = strchr(tier_id, '@');
					if (p != NULL) {
						if (parent_ref[0] == '*' && !uS.mStricmp(p+1, parent_ref+1))
							parent_ref[0] = EOS;
						else if (!uS.mStricmp(p+1, parent_ref))
							parent_ref[0] = EOS;
						else
							*p = '-';
					}
					strcat(utterance->speaker, tier_id);
					if (parent_ref[0] != EOS) {
						strcat(utterance->speaker, "@");
						if (parent_ref[0] == '*')
							strcat(utterance->speaker, parent_ref+1);
						else
							strcat(utterance->speaker, parent_ref);
					}
				}
				stackIndex++;
				Elan_stack[stackIndex] = CurrentElem;
				CurrentElem = CurrentElem->data;
			} else {
				fprintf(stderr, "Internal error exceeded stack size of %d\n", stackIndex);
				freeXML_Elements();
				if (Times_table != NULL)
					free(Times_table);
				Times_table = NULL;
				Times_index = 0L;
				cutt_exit(0);
			}
		}
		if (!strcmp(CurrentElem->name, "ANNOTATION") && stackIndex == 0) {
			uttMax = UTTLINELEN;
			lineOffset = 0;
			orgBeg = -1L;
			do {
				if (getElanAnnotation(CurrentElem->data, utterance->line+lineOffset, uttMax-lineOffset, beg, end, ID, refID)) {
					if (*end > -1L) {
						if (orgBeg > -1L)
							*beg = orgBeg;
						uS.remblanks(utterance->line);
						return(TRUE);
					} else if (*beg > -1L)
						orgBeg = *beg;
				}
				if (*end == -1L) {
					if (CurrentElem->next != NULL && !strcmp(CurrentElem->next->name, "ANNOTATION")) {
						CurrentElem = CurrentElem->next;
						strcat(utterance->line, " ");
						lineOffset = strlen(utterance->line);
					} else {
						if (*end == -1L)
							*end = 0L;
						if (orgBeg > -1L)
							*beg = orgBeg;
						break;
					}
				} else {
					if (orgBeg > -1L)
						*beg = orgBeg;
					break;
				}
			} while (1) ;
		}
	} while (1);
	return(FALSE);
}

static char getNextElanHeader(UTTER *utterance, char *mediaFName, char *spList[]) {
	int i;
	Attributes *att;
	char tier_id[SPEAKERLEN+2];

	if (CurrentElem == NULL)
		return(FALSE);
	
	do {
		if (CurrentElem != NULL && !strcmp(CurrentElem->name, "ANNOTATION_DOCUMENT") && CurrentElem->next == NULL)
			CurrentElem = CurrentElem->data;
		else if (CurrentElem != NULL)
			CurrentElem = CurrentElem->next;
		
		if (CurrentElem == NULL) {
			if (stackIndex >= 0) {
				CurrentElem = Elan_stack[stackIndex];
				stackIndex--;
				freeElements(CurrentElem->data);
				CurrentElem->data = NULL;
				CurrentElem = CurrentElem->next;
				if (CurrentElem == NULL)
					return(FALSE);
			} else
				return(FALSE);
		}
		
		if (!strcmp(CurrentElem->name, "HEADER") && CurrentElem->data != NULL) {
			if (stackIndex < 29) {
				stackIndex++;
				Elan_stack[stackIndex] = CurrentElem;
				CurrentElem = CurrentElem->data;
			} else {
				fprintf(stderr, "Internal error exceeded stack size of %d\n", stackIndex);
				freeXML_Elements();
				if (Times_table != NULL)
					free(Times_table);
				Times_table = NULL;
				Times_index = 0L;
				cutt_exit(0);
			}
		}
		if (!strcmp(CurrentElem->name, "PROPERTY") && stackIndex == 0) {
			for (att=CurrentElem->atts; att != NULL; att=att->next) {
				if (!strcmp(att->name, "NAME")) {
					strncpy(utterance->speaker, att->value, SPEAKERLEN);
					utterance->speaker[SPEAKERLEN-1] = EOS;
					if (*utterance->speaker == '@') {
						getCurrentElanData(CurrentElem, utterance->line, UTTLINELEN);
						return(TRUE);
					}
				}
			}
		}
		if (!strcmp(CurrentElem->name, "MEDIA_DESCRIPTOR") && stackIndex == 0) {
			for (att=CurrentElem->atts; att != NULL; att=att->next) {
				if (!strcmp(att->name, "MEDIA_URL")) {
					strncpy(mediaFName, att->value, FILENAME_MAX);
					mediaFName[FILENAME_MAX] = EOS;
				}
			}
		}
		if (!strcmp(CurrentElem->name, "TIER") && CurrentElem->atts != NULL) {
			tier_id[0] = EOS;
			for (att=CurrentElem->atts; att != NULL; att=att->next) {
				if (!strcmp(att->name, "TIER_ID")) {
					strncpy(tier_id, att->value, SPEAKERLEN);
					tier_id[SPEAKERLEN-1] = EOS;
				}
			}
			if (tier_id[0] != EOS && strchr(tier_id, '@') == NULL) {
				for (i=0; i < 32; i++) {
					if (spList[i] == NULL) {
						spList[i] = (char *)malloc(strlen(tier_id)+1);
						if (spList[i] == NULL)
							out_of_mem();
						uS.uppercasestr(tier_id, NULL, 0);
						strcpy(spList[i], tier_id);
						break;
					}
				}
			}
		}
	} while (1);
	return(FALSE);
}
/* Elan-EAF End ****************************************************************** */

void call() {		/* this function is self-explanatory */
	int  i;
	const char *mediaType;
	char ID[IDSTRLEN+1], refID[IDSTRLEN+1], refSp[SPEAKERLEN], *p;
	char isUTFPrinted, isBeginPrinted;
	char isLanguagesFound, isParticipantsFound, isIDFound, isOptionFound, isFirstHeaderFound, isMediaFound;
	char *spList[32];
	long len;
	long beg, end;
	long lineno = 0L, tlineno = 0L;

	mediaFName[0] = EOS;
	BuildXMLTree(fpin);

	for (i=0; i < 32; i++) {
		spList[i] = NULL;
	}
	isUTFPrinted = FALSE;
	isBeginPrinted = FALSE;
	isOptionFound = FALSE;
	isFirstHeaderFound = FALSE;
	isLanguagesFound = FALSE;
	isParticipantsFound = FALSE;
	isIDFound = FALSE;
	isMediaFound = FALSE;
	while (getNextElanHeader(utterance, mediaFName,spList)) {
		uS.remblanks(utterance->speaker);
		len = strlen(utterance->speaker) - 1;
		if (utterance->line[0] != EOS) {
			if (utterance->speaker[len] != ':')
				strcat(utterance->speaker, ":");
		} else {
			if (utterance->speaker[len] == ':')
				utterance->speaker[len] = EOS;
		}
		if (uS.partcmp(utterance->speaker, MEDIAHEADER, FALSE, FALSE)) {
		} else if (uS.partcmp(utterance->speaker, "@PID:", FALSE, FALSE)) {
			if (!isUTFPrinted) {
				fprintf(fpout, "%s\n", UTF8HEADER);
				isUTFPrinted = TRUE;
			}
			printout(utterance->speaker,utterance->line,NULL,NULL,TRUE);
		} else if (uS.partcmp(utterance->speaker, "@Window:", FALSE, FALSE)) {
			if (!isUTFPrinted) {
				fprintf(fpout, "%s\n", UTF8HEADER);
				isUTFPrinted = TRUE;
			}
			printout(utterance->speaker,utterance->line,NULL,NULL,TRUE);
		} else if (uS.partcmp(utterance->speaker, "@Languages:", FALSE, FALSE)) {
			if (!isUTFPrinted) {
				fprintf(fpout, "%s\n", UTF8HEADER);
				isUTFPrinted = TRUE;
			}
			if (!isBeginPrinted) {
				fprintf(fpout, "@Begin\n");
				isBeginPrinted = TRUE;
			}
			isLanguagesFound = TRUE;
			printout(utterance->speaker,utterance->line,NULL,NULL,TRUE);
		} else if (uS.partcmp(utterance->speaker, "@Participants:", FALSE, FALSE)) {
			if (!isUTFPrinted) {
				fprintf(fpout, "%s\n", UTF8HEADER);
				isUTFPrinted = TRUE;
			}
			if (!isBeginPrinted) {
				fprintf(fpout, "@Begin\n");
				isBeginPrinted = TRUE;
			}
			isParticipantsFound = TRUE;
			printout(utterance->speaker,utterance->line,NULL,NULL,TRUE);
		} else if (uS.partcmp(utterance->speaker, "@ID:", FALSE, FALSE)) {
			if (!isUTFPrinted) {
				fprintf(fpout, "%s\n", UTF8HEADER);
				isUTFPrinted = TRUE;
			}
			if (!isBeginPrinted) {
				fprintf(fpout, "@Begin\n");
				isBeginPrinted = TRUE;
			}
			if (isParticipantsFound && !isOptionFound) {
				if (isMultiBullets) {
					if (isMFA == TRUE)
						fprintf(fpout, "@Options:\tmulti\n");
					else
						fprintf(fpout, "@Options:\tmulti, heritage\n");
				} else if (isMFA == FALSE)
					fprintf(fpout, "@Options:\theritage\n");
				isOptionFound = TRUE;
			}
			isIDFound = TRUE;
			printout(utterance->speaker,utterance->line,NULL,NULL,TRUE);
		} else if (uS.partcmp(utterance->speaker, "@Options:", FALSE, FALSE)) {
			if (!isUTFPrinted) {
				fprintf(fpout, "%s\n", UTF8HEADER);
				isUTFPrinted = TRUE;
			}
			if (!isBeginPrinted) {
				fprintf(fpout, "@Begin\n");
				isBeginPrinted = TRUE;
			}
			uS.remblanks(utterance->line);
			if (isMultiBullets) {
				for (beg=0; utterance->line[beg] != EOS; beg++) {
					if (!strncmp(utterance->line+beg, "multi", 5))
						break;
				}
				if (utterance->line[beg] == EOS)
					strcat(utterance->line, ", multi");
			}
			printout(utterance->speaker,utterance->line,NULL,NULL,TRUE);
			isOptionFound = TRUE;
		} else if (!isFirstHeaderFound && uS.partcmp(utterance->speaker, FONTHEADER, FALSE, FALSE)) {
			printout(utterance->speaker,utterance->line,NULL,NULL,TRUE);
			if (!isUTFPrinted) {
				fprintf(fpout, "%s\n", UTF8HEADER);
				isUTFPrinted = TRUE;
			}
			if (!isBeginPrinted) {
				fprintf(fpout, "@Begin\n");
				isBeginPrinted = TRUE;
			}
		} else if (uS.partcmp(utterance->speaker, "@Birth of ", FALSE, FALSE)) {
			Elan_makeText(utterance->line);
			printout(utterance->speaker,utterance->line,NULL,NULL,TRUE);
			if (!isUTFPrinted) {
				fprintf(fpout, "%s\n", UTF8HEADER);
				isUTFPrinted = TRUE;
			}
			if (!isBeginPrinted) {
				fprintf(fpout, "@Begin\n");
				isBeginPrinted = TRUE;
			}
		} else {
			if (!isUTFPrinted) {
				fprintf(fpout, "%s\n", UTF8HEADER);
				isUTFPrinted = TRUE;
			}
			if (!isBeginPrinted) {
				fprintf(fpout, "@Begin\n");
				isBeginPrinted = TRUE;
			}
			if (!isMediaFound && mediaFName[0] != EOS) {
				p = strrchr(mediaFName, '/');
				if (p != NULL)
					strcpy(mediaFName, p+1);
				p = strrchr(mediaFName, '.');
				if (p != NULL) {
					if (uS.mStricmp(p, ".wav") == 0 || uS.mStricmp(p, ".aif") == 0 || uS.mStricmp(p, ".aiff") == 0)
						mediaType = "audio";
					else
						mediaType = "video";
					*p = EOS;
				} else
					mediaType = "video";
				fprintf(fpout, "%s\t%s, %s\n", MEDIAHEADER, mediaFName, mediaType);
				isMediaFound = TRUE;
			}
			Elan_makeText(utterance->line);
			printout(utterance->speaker,utterance->line,NULL,NULL,TRUE);
			isFirstHeaderFound = TRUE;
		}
	}
	if (!isUTFPrinted) {
		fprintf(fpout, "%s\n", UTF8HEADER);
		isUTFPrinted = TRUE;
	}
	if (!isBeginPrinted) {
		fprintf(fpout, "@Begin\n");
		isBeginPrinted = TRUE;
	}
	if (!isLanguagesFound)
		fprintf(fpout, "@Languages:\teng\n");
	if (!isParticipantsFound) {
		templineC[0] = EOS;
		for (i=0; i < 32; i++) {
			if (spList[i] != NULL) {
				if (templineC[0] != EOS)
					strcat(templineC, ", ");
				strcat(templineC, spList[i]);
				strcat(templineC, " ");
				if (!strcmp(spList[i], "CHI"))
					strcat(templineC, "Child");
				else
					strcat(templineC, "Participant");
			}
		}
		if (templineC[0] != EOS)
			printout("@Participants:",templineC,NULL,NULL,TRUE);
		else
			isIDFound = TRUE;
	}
	if (!isOptionFound) {
		if (isMultiBullets) {
			if (isMFA == TRUE)
				fprintf(fpout, "@Options:\tmulti\n");
			else
				fprintf(fpout, "@Options:\tmulti, heritage\n");
		} else if (isMFA == FALSE)
			fprintf(fpout, "@Options:\theritage\n");
	}
	if (!isIDFound) {
		for (i=0; i < 32; i++) {
			if (spList[i] != NULL) {
				strcpy(templineC, "eng|change_me_later|");
				strcat(templineC, spList[i]);
				strcat(templineC, "|||||");
				if (!strcmp(spList[i], "CHI"))
					strcat(templineC, "Child");
				else
					strcat(templineC, "Participant");
				strcat(templineC, "|||");
				printout("@ID:",templineC,NULL,NULL,TRUE);
			}
		}
	}
	if (!isMediaFound) {
		p = strrchr(mediaFName, '/');
		if (p != NULL)
			strcpy(mediaFName, p+1);
		p = strrchr(mediaFName, '.');
		if (p != NULL) {
			if (uS.mStricmp(p, ".wav") == 0 || uS.mStricmp(p, ".aif") == 0 || uS.mStricmp(p, ".aiff") == 0)
				mediaType = "audio";
			else
				mediaType = "video";
			*p = EOS;
		} else
			mediaType = "video";
		fprintf(fpout, "%s\t%s, %s\n", MEDIAHEADER, mediaFName, mediaType);
		isMediaFound = TRUE;
	}

	ResetXMLTree();
	sortElanSpAndDepTiers();
	setElanTimeOrderElem();

	refSp[0] = EOS;
	while (getNextElanTier(utterance, &beg, &end, ID, refID)) {
//		if (strcmp(ID, "a34|") == 0)
//			p = NULL;
		if (lineno > tlineno) {
			tlineno = lineno + 200;
			fprintf(stderr,"\r%ld ",lineno);
		}
		lineno++;
		uS.remblanks(utterance->speaker);
		len = strlen(utterance->speaker);
		if (utterance->speaker[0] == '@' && beg == 0L && end == 0L) {
			uS.remblanks(utterance->line);
			if (utterance->speaker[len-1] != ':' && utterance->line[0] != EOS)
				strcat(utterance->speaker, ":");
		} else {
			if (utterance->speaker[0] == '%') {
				p = strchr(utterance->speaker, '@');
				if (p != NULL) {
					strcpy(refSp, "*");
					strcat(refSp, p+1);
					strcat(refSp, ":");
					*p = EOS;
				}
			} else
				refSp[0] = EOS;
			if (utterance->speaker[len-1] != ':')
				strcat(utterance->speaker, ":");
		}
		Elan_makeText(utterance->line);
		if (isMFA == TRUE)
			Elan_addToTiersID(ID, refID, beg, end, utterance->speaker, utterance->line, FALSE);
		else
			Elan_addToTiers(ID, refID, beg, end, refSp, utterance->speaker, utterance->line, FALSE);
//		fprintf(fpout, "ID=%s, IDRef=%s; (%ld-%ld)\n", ID, refID, beg, end);
//		fprintf(fpout, "%s:\t%s\n", utterance->speaker, utterance->line);
	}
	fprintf(stderr, "\r	  \r");
	cutt_isMultiFound = isMultiBullets;
	if (isMFA == FALSE) {
		Elan_addUnmacthed();
		Elan_finalTimeSort();
	}
	Elan_printOutTiers(e2c_RootTiers);
	fprintf(fpout, "@End\n");
	freeXML_Elements();
	if (Times_table != NULL)
		free(Times_table);
	Times_table = NULL;
	Times_index = 0L;
	e2c_RootTiers = freeTiers(e2c_RootTiers);
	RootUnmatchedTiers = freeTiers(RootUnmatchedTiers);
}
