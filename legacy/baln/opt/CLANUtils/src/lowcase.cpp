/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#define CHAT_MODE 4

#include "cu.h"
#include "c_curses.h"

#define IS_WIN_MODE FALSE
#include "mul.h" 

#define DICNAME "caps.cut"
#define PERIOD 50

extern struct tier *defheadtier;
extern char OverWriteFile;

struct lowcase_tnode		/* binary lowcase_tree for word and count */
{
	char *word;
	int count;
	struct lowcase_tnode *left;
	struct lowcase_tnode *right;
};

static int  arrCnt;
static char **lowcase_array;
static char lowcase_ftime, isFWord, isMakeBold, isMorEnabled, isCapsSpecified, isChangeToUpper;
static FNType lc_dicname[FNSize];
static struct lowcase_tnode *lowcase_root;

void usage() {
	puts("LOWCASE converts all words to lower case except those specified as pronouns.");
	printf("Usage: lowcase [b c d iF %s] filename(s)\n", mainflgs());
	puts("+b : mark upper case letters as bold, then convert them to lower case");
	puts("+c : convert only the first word on a tier to lower case");
	fprintf(stdout, "+d : do NOT change words from \"%s\" file, lower case the rest\n", DICNAME);
	fprintf(stdout, "+d1: capitalize words from \"%s\" file, do NOT change the rest\n", DICNAME);
	fprintf(stdout, "+iF: file F with capitalize words (default: %s)\n", DICNAME);
	puts("+LF: specify full path F of the lib folder");
	mainusage(TRUE);
}

static void free_lowcase_array(void) {
	int i;

	if (lowcase_array == NULL)
		return;
	for (i=0; i < arrCnt; i++) {
		free(lowcase_array[i]);
	}
	free(lowcase_array);
}

static void free_lowcase_root(struct lowcase_tnode *p) {
	struct lowcase_tnode *t;

	if (p != NULL) {
		free_lowcase_root(p->left);
		do {
			if (p->right == NULL)
				break;
			if (p->right->left != NULL) {
				free_lowcase_root(p->right);
				break;
			}
			t = p;
			p = p->right;
			free(t);
		} while (1);
		free(p);
	}
}

static void lowcase_overflow(char isMem) {
	if (isMem)
		fprintf(stderr,"Uniq: no more core available.\n");
	free_lowcase_root(lowcase_root);
	free_lowcase_array();
	cutt_exit(0);
}

static void initArray(void) {
	int i;

	if (arrCnt > 0) {
		lowcase_array = (char **) malloc (arrCnt * sizeof(char *));
		if (lowcase_array == NULL)
			lowcase_overflow(TRUE);
		for (i=0; i < arrCnt; i++)
			lowcase_array[i] = NULL;
	}
}

static void addWordToArray(char *w) {
	int i;

	for (i=0; i < arrCnt; i++) {
		if (lowcase_array[i] == NULL) {
			lowcase_array[i] = w;
			return;
		}
	}
	fprintf(stderr, "\n\n\t Internal Error\n\n");
	lowcase_overflow(FALSE);
}

static void createSortedArray(struct lowcase_tnode *p) {
	struct lowcase_tnode *t;

	if (p != NULL) {
		createSortedArray(p->left);
		do {
			addWordToArray(p->word);
			if (p->right == NULL)
				break;
			if (p->right->left != NULL) {
				createSortedArray(p->right);
				break;
			}
			t = p;
			p = p->right;
		} while (1);
	}
}

static struct lowcase_tnode *lowcase_talloc(void) {
	struct lowcase_tnode *p;

	if ((p=NEW(struct lowcase_tnode)) == NULL)
		lowcase_overflow(TRUE);
	return(p);
}

static char *lowcase_strsave(char *s) {
	char *p;

	if ((p=(char *)malloc(strlen(s)+1)) == NULL)
		lowcase_overflow(TRUE);
	strcpy(p, s);
	return(p);
}

static struct lowcase_tnode *lowcase_tree(struct lowcase_tnode *p, char *w) {
	int cond;
	struct lowcase_tnode *t = p;

	if (p == NULL) {
		arrCnt++;
		p = lowcase_talloc();
		p->word = lowcase_strsave(w);
		p->count = 1;
		p->left = p->right = NULL;
	} else if ((cond=strcmp(w, p->word)) == 0) {
		p->count++;
	} else if (cond < 0) {
		p->left = lowcase_tree(p->left, w);
	} else {
		for (; (cond=strcmp(w, p->word)) > 0 && p->right != NULL; p=p->right) ;
		if (cond == 0)
			p->count++;
		else if (cond < 0)
			p->left = lowcase_tree(p->left, w);
		else
			p->right = lowcase_tree(p->right, w);
		return(t);
	}
	return(p);
}

static void read_file(FILE *fdic) {
	int  i;

	while (fgets_cr(templineC4, UTTLINELEN, fdic)) {
		if (uS.isUTF8(templineC4) || uS.isInvisibleHeader(templineC4))
			continue;
		for (i=0; isSpace(templineC4[i]) || templineC4[i] == '\n' || templineC4[i] == '\r'; i++) ;
		if (i > 0)
			strcpy(templineC4, templineC4+i);
		for (i=0; templineC4[i] != EOS && templineC4[i] != '\t' && templineC4[i] != '\n'; i++);
		templineC4[i] = EOS;
		uS.remblanks(templineC4);
		if (templineC4[0] != EOS) {
			uS.uppercasestr(templineC4, &dFnt, C_MBF);
			lowcase_root = lowcase_tree(lowcase_root, templineC4);
		}
	}
}

static void OpenBriCaps(FNType *mFileName) {
	int  index, t, len;
	FNType tFName[FNSize];
	FILE *bricaps;

	index = 1;
	t = strlen(mFileName);
	while ((index=Get_File(tFName, index)) != 0) {
		len = strlen(tFName) - 4;
		if (tFName[0] != '.' && len >= 0 && uS.FNTypeicmp(tFName+len, ".cut", 0) == 0) {
			addFilename2Path(mFileName, tFName);
			if ((bricaps=fopen(mFileName, "r"))) {
				fprintf(stderr,"\rUsing BriCaps: %s.\n", mFileName);
				read_file(bricaps);
				fclose(bricaps);
				bricaps = NULL;
			}
			mFileName[t] = EOS;
		}
	}
}


static void lowcase_readdict() {
	FILE *fdic;
	FNType mFileName[FNSize];

	if (*lc_dicname == EOS) {
		fprintf(stderr,"Warning: No dep-file was specified.\n");
		cutt_exit(0);
		fdic = NULL;
	} else if ((fdic=OpenGenLib(lc_dicname,"r",TRUE,TRUE,mFileName)) == NULL) {
		if (isCapsSpecified) {
			fprintf(stderr, "ERROR: Can't open either one of the dep-files:\n\t\"%s\"\n\t\"%s\"\n", lc_dicname, mFileName);
			lowcase_overflow(FALSE);
		} else
			fprintf(stderr, "Warning: Can't open either one of the dep-files:\n\t\"%s\"\n\t\"%s\"\n", lc_dicname, mFileName);
	}
	arrCnt = 0;
	if (fdic != NULL) {
		fprintf(stderr, "Using dep-files \"%s\"\n", mFileName);
		read_file(fdic);
		fclose(fdic);
	}
	strcpy(mFileName,lib_dir);
	addFilename2Path(mFileName, "Bricaps");
	if (!SetNewVol(mFileName)) {
		OpenBriCaps(mFileName);
	}
	SetNewVol(wd_dir);
	initArray();
	createSortedArray(lowcase_root);
	free_lowcase_root(lowcase_root);
}

void init(char f) {
	if (f) {
		lowcase_ftime = TRUE;
		isMorEnabled = FALSE;
		isChangeToUpper = 10;
		isFWord = FALSE;
		isCapsSpecified = FALSE;
		isMakeBold = FALSE;
		stout = FALSE;
		lowcase_array = NULL;
		lowcase_root = NULL;
		uS.str2FNType(lc_dicname, 0L, DICNAME);
		onlydata = 1;
		OverWriteFile = TRUE;
		FilterTier = 0;
		LocalTierSelect = TRUE;
// defined in "mmaininit" and "globinit" - nomap = TRUE;
/*
		if (defheadtier) {
			if (defheadtier->nexttier != NULL)
				free(defheadtier->nexttier);
			free(defheadtier);
			defheadtier = NULL;
		}
*/
	} else if (lowcase_ftime) {
		lowcase_readdict();
		lowcase_ftime = FALSE;
		
	}
}

CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = LOWCASE;
	OnlydataLimit = 0;
	UttlineEqUtterance = FALSE;
	bmain(argc,argv,NULL);
	free_lowcase_array();
}

static char gotmatch(char *word) {
	int i, cond, e;
	long low, high, mid;

	strcpy(templineC4,word);
	i = 0;
	while (templineC4[i] != EOS) {
		if ((e=uS.HandleCAChars(templineC4+i, NULL)) != 0) {
			strcpy(templineC4+i, templineC4+i+e);
		} else
			i++;
	}
	uS.uppercasestr(templineC4, &dFnt, MBF);
	e = strlen(templineC4) - 1;
	if (e < 0)
		return(FALSE);
	if (templineC4[e-1] == 'S' && templineC4[e] == '\'')
		templineC4[e] = EOS;
	else if (templineC4[e-1] == '\'' && templineC4[e] == 'S')
		templineC4[e-1] = EOS;
	low = 0;
	high = arrCnt - 1;
	while (low <= high) {
		mid = (low+high) / 2;
		if ((cond=strcmp(templineC4,lowcase_array[mid])) < 0)
			high = mid - 1;
		else if (cond > 0)
			low = mid + 1;
		else {
			return(TRUE);
		}
	}
	return(FALSE);
}

static int findNextWord(char *line, long *pos, int i) {
	register int  temp;
	register char sq;
	register char t;

getnewword:
	if (chatmode && *utterance->speaker == '%') {
		if (line[i] == EOS)
			return(0);
//6-6-03		if (i > 0) i--;
		while ((t=line[i]) != EOS && uS.isskip(line,i,&dFnt,MBF) && !uS.isRightChar(line,i,'[',&dFnt,MBF)) {
			i++;
			if (t == '<') {
				temp = i;
				for (i++; line[i] != '>' && line[i]; i++) {
					if (isdigit(line[i])) ;
					else if (line[i]== ' ' || line[i]== '\t' || line[i]== '\n') ;
					else if ((i-1 == temp+1 || !isalpha(line[i-1])) &&
							 line[i] == '-' && !isalpha(line[i+1])) ;
					else if ((i-1 == temp+1 || !isalpha(line[i-1])) &&
							 (line[i] == 'u' || line[i] == 'U') &&
							 !isalpha(line[i+1])) ;
					else if ((i-1 == temp+1 || !isalpha(line[i-1])) &&
							 (line[i] == 'w' || line[i] == 'W') &&
							 !isalpha(line[i+1])) ;
					else if ((i-1 == temp+1 || !isalpha(line[i-1])) &&
							 (line[i] == 's' || line[i] == 'S') &&
							 !isalpha(line[i+1])) ;
					else
						break;
				}
				if (line[i] == '>')
					i++;
				else
					i = temp;
			}
		}
	} else {
		while ((t=line[i]) != EOS && uS.isskip(line,i,&dFnt,MBF) && !uS.isRightChar(line,i,'[',&dFnt,MBF))
			 i++;
	}
	if (t == EOS)
		return(0);
	if (uS.isRightChar(line, i, '[',&dFnt,MBF)) {
		if (uS.isSqBracketItem(line, i+1, &dFnt, MBF))
			sq = TRUE;
		else
			sq = FALSE;
	} else
		sq = FALSE;
	*pos = i;
getword_rep:

//	if (dFnt.isUTF) {
		while ((temp=uS.HandleCAChars(line+i, NULL)) != 0) {
			i += temp;
			*pos = i;
		}
		if (uS.isRightChar(line,i,'[',&dFnt,MBF)) {
			for (; line[i] != EOS && !uS.isRightChar(line,i,']',&dFnt,MBF); i++) ;
			if (uS.isRightChar(line,i,']',&dFnt,MBF))
				goto getnewword;
		}
		if (uS.isskip(line,i,&dFnt,MBF))
			goto getnewword;
		if (line[i] == EOS)
			return(0);
//	}
	
	if ((t == '+' || t == '-') && !sq) {
		while (line[++i] != EOS && (!uS.isskip(line,i,&dFnt,MBF) ||
				uS.isRightChar(line,i,'/',&dFnt,MBF) || uS.isRightChar(line,i,'<',&dFnt,MBF) ||
				uS.isRightChar(line,i,'.',&dFnt,MBF) || uS.isRightChar(line,i,'!',&dFnt,MBF) ||
				uS.isRightChar(line,i,'?',&dFnt,MBF) || uS.isRightChar(line,i,',',&dFnt,MBF))) {
			if (uS.isRightChar(line, i, ']', &dFnt, MBF)) {
				return(i+1);
			}
		}
	} else if (uS.atUFound(line, i, &dFnt, MBF)) {
		while (line[++i] != EOS && (!uS.isskip(line,i,&dFnt,MBF) ||
				uS.isRightChar(line,i,'.',&dFnt,MBF) || uS.isRightChar(line,i,'!',&dFnt,MBF) ||
				uS.isRightChar(line,i,'?',&dFnt,MBF) || uS.isRightChar(line,i,',',&dFnt,MBF))) {
			if (uS.isRightChar(line, i, ']', &dFnt, MBF)) {
				return(i+1);
			}
		}
	} else {
		while (line[++i] != EOS && (!uS.isskip(line,i,&dFnt,MBF) || sq)) {
			if (uS.isRightChar(line, i, ']', &dFnt, MBF)) {
				return(i+1);
			}
		}
	}
	if (uS.isRightChar(line, i, '[', &dFnt, MBF)) {
		if (!uS.isSqBracketItem(line, i+1, &dFnt, MBF))
			goto getword_rep;
	}
//	if (line[i] != EOS && line[i] != '\n')
//		i++;
	return(i);
}

static int isNextCharUpper(char *line, NewFontInfo *finfo) {
	while (*line) {
		if (finfo->isUTF) {
			if (UTF8_IS_SINGLE((unsigned char)*line)) {
				if (*line == ':' || *line == '/' || *line == '(' || *line == ')' || uS.HandleCAChars(line, NULL))
					;
				else
					return(isupper((unsigned char)*line));
			} else
				return(FALSE);
		} else {
			if (*line == ':' || *line == '/' || *line == '(' || *line == ')')
				;
			else
				return(isupper((unsigned char)*line));
		}
		line++;
	}
	return(FALSE);
}

static long makeBold(char *line, AttTYPE *atts, NewFontInfo *finfo) {
	int  isUpperC;
	long isFound;
	char isHiden, isSq;

	isSq = FALSE;
	isHiden = FALSE;
	isUpperC = FALSE;
	isFound = 0L;
	while (*line) {
		if (*line == '[')
			isSq = TRUE;
		else if (*line == ']')
			isSq = FALSE;
		if (*line == HIDEN_C)
			isHiden = !isHiden;

		if (isSq || isHiden)
			isUpperC = FALSE;
		else if (finfo->isUTF) {
			if (UTF8_IS_SINGLE((unsigned char)*line)) {
				if (*line == ':' || *line == '/' || *line == '(' || *line == ')' || uS.HandleCAChars(line, NULL))
					;
				else
					isUpperC = (isupper((unsigned char)*line) && (isUpperC || isNextCharUpper(line+1, finfo)));
			} else
				isUpperC = FALSE;
		} else {
			if (*line == ':' || *line == '/' || *line == '(' || *line == ')')
				;
			else
				isUpperC = (isupper((unsigned char)*line) && (isUpperC || isNextCharUpper(line+1, finfo)));
		}

		if (isUpperC) {
			if (isupper((unsigned char)*line))
				*line = (char)tolower((unsigned char)*line);
			*atts = set_bold_to_1(*atts);
			isFound++;
		}
		line++;
		atts++;
	}
	return(isFound);
}

void call() {
	char t;
	char RightSpeaker = FALSE;
	long  i, pos;
	long isFound;
	long tlineno = 0L;


	isFound = 0L;
	currentatt = 0;
	currentchar = (char)getc_cr(fpin, &currentatt);
	while (getwholeutter()) {
		if (!stout) {
			tlineno = tlineno + 1L;
			if (tlineno % PERIOD == 0)
				fprintf(stderr,"\r%ld ",tlineno);
		}

		if (!checktier(utterance->speaker) || 
							(!RightSpeaker && *utterance->speaker == '%') ||
							(*utterance->speaker == '*' && nomain)) {
			if (*utterance->speaker == '*') {
				if (nomain)
					RightSpeaker = TRUE;
				else
					RightSpeaker = FALSE;
			}
			printout(utterance->speaker,utterance->line,utterance->attSp,utterance->attLine,FALSE);
		} else {
			if (*utterance->speaker == '*') {
				RightSpeaker = TRUE;
				if (isMakeBold)
					isFound += makeBold(utterance->line, utterance->attLine, &dFnt);
			}
			if (isMakeBold) {
				printout(utterance->speaker,utterance->line,utterance->attSp,utterance->attLine,FALSE);
				continue;
			}

			strcpy(templineC,utterance->speaker);
			uS.uppercasestr(templineC, &dFnt, MBF);
			if (uS.partcmp(templineC,"%TRN:",FALSE,FALSE) || uS.partcmp(templineC,"%MOR:",FALSE,FALSE) ||
				uS.partcmp(templineC,"%SPA:",FALSE,FALSE) || uS.partcmp(templineC,"%SYN:",FALSE,FALSE) ||
				uS.partcmp(templineC,"%SIT:",FALSE,FALSE) || uS.partcmp(templineC,"%PHO:",FALSE,FALSE) ||
				uS.partcmp(templineC,"%MOD:",FALSE,FALSE) || uS.partcmp(templineC,"%ORT:",FALSE,FALSE) ||
				uS.partcmp(templineC,"%LAN:",FALSE,FALSE) || uS.partcmp(templineC,"%COH:",FALSE,FALSE) ||
				uS.partcmp(templineC,"%DEF:",FALSE,FALSE) || uS.partcmp(templineC,"%COD:",FALSE,FALSE) ||
				utterance->speaker[0] == '@') {
				printout(utterance->speaker,utterance->line,utterance->attSp,utterance->attLine,FALSE);
				continue;
			}
			if (uS.partcmp(templineC,"%PHO:",FALSE,FALSE) ||
				uS.partcmp(templineC,"%MOD:",FALSE,FALSE))
				punctuation = ",.;?!";
			i = 0L;
			while ((i=findNextWord(utterance->line, &pos, i))) {
/*
				if (utterance->line[i] == EOS)
					j = i;
				else
					j = i - 1;
*/
				t = utterance->line[i];
				utterance->line[i] = EOS;
				if (gotmatch(utterance->line+pos)) {
					if (isChangeToUpper && islower((unsigned char)utterance->line[pos])) {
						isFound++;
						utterance->line[pos] = (char)toupper((unsigned char)utterance->line[pos]);
					}
				} else if (isalpha(utterance->line[pos]) || utterance->line[pos] == '(') {
					if ((utterance->line[pos] == 'I' || utterance->line[pos] == 'i') &&
						!isalpha(utterance->line[pos+1]) && utterance->line[pos+1] != ':' && 
						utterance->line[pos+1] != '(' && utterance->line[pos+1] != ')') {
						if (islower((unsigned char)utterance->line[pos])) {
							if (utterance->line[pos+1] == '@' && (char)tolower((unsigned char)utterance->line[pos+2]) == 'l')
								;
							else if (utterance->line[pos+1] == '(' && isalpha(utterance->line[pos+2]))
								;
							else {
								isFound++;
								utterance->line[pos] = (char)toupper((unsigned char)utterance->line[pos]);
							}
						}
					} else if (isChangeToUpper != 1) {
						if (uS.lowercasestr(utterance->line+pos, &dFnt, MBF))
							isFound++;
					}
				}
				utterance->line[i] = t;
				if (isFWord && isalpha(utterance->line[pos]))
					break;
			}
			printout(utterance->speaker,utterance->line,utterance->attSp,utterance->attLine,FALSE);
		}
	}
	if (!stout)
		fprintf(stderr,"\n");
	if (isFound > 0L)
		fprintf(stderr,"**+ %ld changes made in this file\n", isFound);
	else
		fprintf(stderr,"**- NO changes made in this file\n");
}


void getflag(char *f, char *f1, int *i) {
	f++;
	switch(*f++) {
		case 'b':
				isMakeBold = TRUE;
				no_arg_option(f);
				break;

		case 'c':
				isFWord = TRUE;
				no_arg_option(f);
				break;

		case 'd':
				isChangeToUpper = (char)atoi(getfarg(f,f1,i));
				if (isChangeToUpper > 1) {
					fputs("The N for +d option must be between 0 - 1\n",stderr);
					cutt_exit(0);
				}
				break;

		case 'i':
				isCapsSpecified = TRUE;
				uS.str2FNType(lc_dicname, 0L, getfarg(f,f1,i));
				break;
		case 'L':
			int len;
			strcpy(lib_dir, f);
			len = strlen(lib_dir);
			if (len > 0 && lib_dir[len-1] != '/')
				strcat(lib_dir, "/");
			break;
		default:
				maingetflag(f-2,f1,i);
				break;
	}
}
