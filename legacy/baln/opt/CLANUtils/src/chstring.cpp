/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#define CHAT_MODE 3
#include "cu.h"
#include "check.h"

#define IS_WIN_MODE FALSE
#include "mul.h"

#define DICNAME "changes.cut"
#define PERIOD 50

//2015-03-30 extern char cutt_isCAFound;
extern char cutt_isBlobFound;
extern struct tier *defheadtier;
extern char OverWriteFile;

static FNType dicname[256];
static char tab,
	wildcard,
	lineonly,
	NO_CHANGE,
	headeronly,
	DispChanges,
	stringOriented,
	isSearchForBullets;
static char chstring_FirstTime;
static char chstring_isCoreLex;

static struct words {
	char *word;
	char *toword;
	struct words *next;
} *head;


static void pars(char *st) {
	int i, j, len;

	j = 0;
	len = strlen(st);
	for (i=0; i < len; i++) {
		if (st[i] == '\\' && st[i+1] == 't') {
			st[j++] = '\t';
			i++;
		} else if (st[i] == '\\' && st[i+1] == 'n') {
			st[j++] = '\n';
			i++;
		} else
			st[j++] = st[i];
	}
	st[j] = EOS;
}

static void chstring_cleanup(void) {
	struct words *t;

	while (head != NULL) {
		t = head;
		head = head->next;
		free(t->word);
		free(t->toword);
		free(t);
	}
}

static void chstring_overflow() {
	fprintf(stderr,"chstring: no more memory available.\n");
	chstring_cleanup();
	cutt_exit(0);
}

static struct words *makenewsym(char *st) {
	struct words *nextone;

	if (head == NULL) {
		head = NEW(struct words);
		nextone = head;
	} else {
		nextone = head;
		while (nextone->next != NULL) nextone = nextone->next;
		nextone->next = NEW(struct words);
		nextone = nextone->next;
	}
	if (nextone == NULL)
		chstring_overflow();
	nextone->next = NULL;
	nextone->toword = NULL;
	nextone->word = (char *)malloc(strlen(st)+1);
	if (nextone->word == NULL)
		chstring_overflow();
	strcpy(nextone->word,st);
	return(nextone);
}

static void readdict(void) {
	FILE *fdic;
	char chrs, word[BUFSIZ], first = TRUE, term;
	int index = 0, l = 1;
	struct words *nextone = NULL;
	FNType mFileName[FNSize];

	if (dicname[0] == EOS) {
		uS.str2FNType(dicname, 0L, DICNAME);
		if ((tab || chstring_isCoreLex) && access(dicname,0) != 0)
			return;
	}
	if ((fdic=OpenGenLib(dicname,"r",TRUE,TRUE,mFileName)) == NULL) {
		fputs("Can't open either one of the changes files:\n",stderr);
		fprintf(stderr,"\t\"%s\", \"%s\"\n", dicname, mFileName);
		cutt_exit(0);
	}
	while ((chrs=(char)getc_cr(fdic, NULL)) != '"' && chrs!= '\'' && !feof(fdic)) {
		if (chrs == '\n')
			l++;
	}
	term = chrs;
	while (!feof(fdic)) {
		chrs = (char)getc_cr(fdic, NULL);
		if (chrs == term) {
			word[index] = EOS;
			if (first) {
				nextone = makenewsym(word);
				// if (DispChanges)
				// 	fprintf(stderr,"From string \"%s\" to string ",nextone->word);
				first = FALSE;
			} else if (nextone != NULL) {
				nextone->toword = (char *)malloc(strlen(word)+1);
				if (nextone->toword == NULL)
					chstring_overflow();
				strcpy(nextone->toword,word);
				if (DispChanges)
					fprintf(stderr,"\"%s\"\n",nextone->toword);
				pars(nextone->toword);
				first = TRUE;
			}
			word[0] = EOS;
			index = 0;
			while ((chrs=(char)getc_cr(fdic, NULL)) != '"' && chrs != '\'' && !feof(fdic)) {
				if (chrs == '\n')
					l++;
			}
			term = chrs;
		} else if (chrs != '\n') {
			if (chrs == HIDEN_C)
				isSearchForBullets = TRUE;
			word[index++] = chrs;
		} else {
			fflush(stdout);
			fprintf(stderr, "ERROR: Found newline in string on line %d in file %s.\n",l,dicname);
			fclose(fdic);
			chstring_cleanup();
			cutt_exit(0);
		}
	}
	fclose(fdic);
	if (DispChanges)
		fprintf(stderr,"\n");
}

void init(char f) {
	if (f) {
		tab = FALSE;
		stout = FALSE;
		*dicname = EOS;
		wildcard = TRUE;
		lineonly = FALSE;
		NO_CHANGE = FALSE;
		DispChanges = TRUE;
		stringOriented = 0;
		chstring_FirstTime = TRUE;
		chstring_isCoreLex = FALSE;
		headeronly = FALSE;
		OverWriteFile = TRUE;
		FilterTier = 0;
		LocalTierSelect = TRUE;
		isSearchForBullets = FALSE;
/*
		if (defheadtier->nexttier != NULL) 
			free(defheadtier->nexttier);
		free(defheadtier);
		defheadtier = NULL;
*/
		head = NULL;
		onlydata = 1;
	} else if (chstring_FirstTime) {
		readdict();
		chstring_FirstTime = FALSE;
	}
}

void usage() {
	puts("CHSTRING changes a list of strings to other strings.");
	printf("Usage: chstring [b cF d l q sS S x %s] filename(s)\n", mainflgs());
	puts("+b : work ONLY on text to the right of the colon in CHAT format");
	printf("+cF: dictionary file (\"\\t\"-tab, \"\\n\"-newline)(Default %s) \n", DICNAME);
	puts("+d : do not re-wrap tiers");
	puts("+l : work ONLY on codes to the left of the colon in CHAT format");
	puts("+lx: do not show the list of changes specified with +s or +c option");
	puts("+q : clean up tiers. (add tabs after colons and remove blank spaces)");
	puts("+q1: clean up tiers for CORELEX command");
	puts("+LF: specify full path F of the lib folder");
	puts("+sS S: string to change followed by string to change to (\"\\t\"=tab, \"\\n\"=newline)");
	puts("-w : do string oriented search and replacement");
//	puts("-w1: do string oriented search and replacement except within \"quoted\" text");
	puts("+x : interpret *, _ and \\ as literal characters during the search");
	mainusage(FALSE);
	puts("Dictionary file format: \"from string\" \" to string\"");
	puts("\nExample:");
	puts("\tchstring +b +t\"*mot\" +t@ +cFileName.cut *.cha");
	puts("\tchstring +s\"play\" \"work\" *.cha");
	puts("\tchstring +s\"%mor:\" \"%xmor:\" +t% +l *.cha");
	puts("  To remove all bullets:");
	puts("\tchstring +cbullets.cut *.cha");
	cutt_exit(0);
}

CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = CHSTRING;
	OnlydataLimit = 0;
	UttlineEqUtterance = FALSE;
	bmain(argc,argv,NULL);
	chstring_cleanup();
}

static void MkSpeaker(char *line, AttTYPE *att) {
	int i;

	for (i=0; line[i] != ':' && line[i]; i++) {
		utterance->speaker[i] = line[i];
		utterance->attSp[i] = att[i];
	}
	if (line[i] == ':') {
		utterance->speaker[i] = line[i];
		utterance->attSp[i] = att[i];
		i++;
		if (tab || chstring_isCoreLex) {
		} else if ((utterance->speaker[0] == '*' || utterance->speaker[0] == '%') && line[i] == '\t') {
			for (; line[i] == '\t'; i++) {
				utterance->speaker[i] = line[i];
				utterance->attSp[i] = att[i];
			}
		} else {
			for (; isSpace(line[i]); i++) {
				utterance->speaker[i] = line[i];
				utterance->attSp[i] = att[i];
			}
		}
	}
	utterance->speaker[i] = EOS;
	att_cp(0, utterance->line, line+i, utterance->attLine, att+i);
}

static char errorReplace(char *code) {
	int  i;
	char sq;
	
	sq = 0;
	for (i=0; code[i] != EOS; i++) {
		if (code[i] == '[') {
			sq++;
			if (sq == 1 && code[i+1] == '*' && code[i+2] == ' ' && code[i+3] == 's')
				return(TRUE);
		} else if (code[i] == ']') {
			if (sq > 0)
				sq--;
		} else if (!isSpace(code[i]) && code[i] != '\n' && code[i] != '<' && code[i] != '>' && sq == 0) {
			break;
		}
	}
	return(FALSE);
}

static char isRetrace(char *line, AttTYPE *att) {
	int  i, oab, cab;
	char sq, ab;
	
	sq = 0;
	ab = 0;
	cab =0; 
	for (i=0; line[i] != EOS; i++) {
		if (line[i] == '<' && sq == 0) {
			ab++;
			if (ab == 1)
				oab = i;
		} if (line[i] == '>' && sq == 0) {
			if (ab > 0)
				ab--;
			if (ab == 0)
				cab = i;
		} else if (line[i] == '[' && line[i+1] == '/' && line[i+2] == '/' && line[i+3] == ']' && ab == 0 && sq == 0) {
			att_cp(0, line+i, line+i+4, att+i, att+i+4);
			if (cab > 0) {
				att_cp(0, line+cab, line+cab+1, att+cab, att+cab+1);
				att_cp(0, line+oab, line+oab+1, att+oab, att+oab+1);
			}
			return(TRUE);
		} else if (line[i] == '[') {
			sq++;
		} else if (line[i] == ']') {
			if (sq > 0)
				sq--;
		} else if (!isSpace(line[i]) && line[i] != '\n' && ab == 0 && sq == 0) {
			break;
		}
	}
	return(TRUE);
}

static long FindAndChangeCoreLex(char *line, AttTYPE *att, long isFound) {
	int i, pos;
	
	pos = 0;
	while (line[pos] != EOS) {
		if (line[pos] == '_') {
			line[pos] = ' ';
			isFound++;
		} else if (line[pos] == '[' && line[pos+1] == ':' && line[pos+2] == ' ') {
			if (errorReplace(line+pos)) {
				for (i=strlen(line); i >= pos+2; i--) {
					line[i+1] = line[i];
					att[i+1] = att[i];
				}
				line[pos+2] = ':';
				att[pos+2] = att[pos+1];
				isFound++;
			}
		} else if (line[pos] == '<' || 
				   (line[pos] == '[' && line[pos+1] == '/' && line[pos+2] == '/' && line[pos+3] == ']')) {
			if (isRetrace(line+pos, att+pos)) {
				isFound++;
			}
		}
		pos++;
	}
	return(isFound);
}

static int chstring_found(char *line, int pos, char *pat) {
	int len;
	register int n, m;

	if (strlen(line+pos) < strlen(pat))
		return(FALSE);
	else {
		len = pos;
		if (wildcard) {
/*
printf("1; pat=%s; line=%s;\n", pat, line+pos);
*/
			for (; *pat; pos++, pat++) {
/*
printf("2; pat=%s; line=%s", pat, line+pos);
*/
				if (*pat == '\\') {
					if (line[pos] != *++pat)
						break;
				} else if (*pat == '_') {				/* any character */
					if (line[pos+1] && *(pat+1))
						continue;
					else if (line[pos+1] == *(pat+1))
						return(pos-len);
					else
						return(FALSE);
				} else if (*pat == '*') {		/* wildcard */
					pat++;
					if (*pat == '\\')
						pat++;
rep:
					if (*pat == ' ') {
						while (line[pos] && line[pos] != *pat) {
							if (line[pos] == '\n' || uS.isskip(line, pos, &dFnt, MBF))
								break;
							pos++;
						}
					} else if (stringOriented == 0) {
/*
printf("2.5.0; *pat=%c; *line=%c;\n", *pat, line[pos]);
*/
						if (!*pat) {
							while (!uS.isskip(line, pos, &dFnt, MBF) && line[pos] != EOS)
								pos++;
							return((int)(pos-len));
						}
						while (line[pos] && line[pos] != *pat) {
							if (line[pos] == '\n' || uS.isskip(line, pos, &dFnt, MBF))
								return(FALSE);
							pos++;
/*
printf("2.5.1; *pat=%c; *line=%c;\n", *pat, line[pos]);
*/
						}
					} else {
						while (line[pos] && line[pos] != *pat)
							pos++;
					}

					if (!line[pos]) {
						if ((*pat == ' ' && !*(pat+1)) || !*pat) 
							return((int)(pos-len));
						else
							return(FALSE);
					}
					for (m=pos+1, n=1; line[m] && pat[n]; m++, n++) {
/*
printf("3; pat=%s; line=%s;\n", pat+n, line+m);
*/
						if (pat[n] == '*')
							break;
						else if (pat[n] == '_') ;
						else if (pat[n] == '\\') {
							if (!pat[++n])
								return(FALSE);
							else if (line[m] != pat[n]) {
								pos++;
								goto rep;
							}
						} else if (line[m] != pat[n]) {
							if (pat[n]==' ' && (line[m]=='\n' || uS.isskip(line,m,&dFnt,MBF))) ;
							else {
								pos++;
								goto rep;
							}
						}
					}
					if (!pat[n]) {
						if (pat[n-1] != '*') {
							pos = m;
							pat = pat + n;
							break;
						} else if (line[m]) {
							pos++;
							goto rep;
						}
					}
				}
				if (line[pos] != *pat) {
					if (*pat == ' ' && line[pos] != EOS) {
						if (line[pos] == '\n') {
							if (stringOriented == 0 && isSpace(*pat) && !isSpace(*(pat+1)) && line[pos+1] == '\t') {
								pos++;
							} else {
								if (stringOriented == 0)
									break;
								for (pos++; uS.isskip(line, pos, &dFnt, MBF) && line[pos] != EOS; pos++) ;
								pos--;
							}
						} else if (uS.isskip(line, pos, &dFnt, MBF)) {
							if (stringOriented == 0)
								break;
						} else
							break;
					} else
						break;
				}
			}
/*
printf("4; pat=%s; line=%s;\n", pat, line+pos);
*/
		} else {
			for (; *pat; pos++, pat++) {
				if (line[pos] != *pat) {
					if (*pat == ' ' && line[pos] != EOS) {
						if (line[pos] == '\n') {
							if (stringOriented == 0)
								break;
							for (pos++; uS.isskip(line, pos, &dFnt, MBF) && line[pos] != EOS; pos++) ;
							pos--;
						} else if (uS.isskip(line, pos, &dFnt, MBF)) {
							if (stringOriented == 0)
								break;
						} else
							break;
					} else
						break;
				}
			}
		}
		if (*pat == EOS) {
			if (stringOriented == 0) {
				if (uS.isskip(line, pos, &dFnt, MBF) || line[pos] == EOS)
					return(pos-len);
				else
					return(FALSE);
			} else
				return(pos-len);
		} else if (line[pos] == EOS && *pat == ' ' && *(pat+1) == EOS) {
			line[pos++] = ' ';
			line[pos] = EOS;
			return(pos-len);
		} else
			return(FALSE);
	}
}

static void make_replacement_str(char *old_pat, char *old_s, int old_s_len, char *new_pat, char *new_s) {
    register int j, k;
    register int n, m;
    int t, end;

	*new_s = EOS;
    if (old_s[0] == EOS)
    	return;
    for (j=0, k=0; old_pat[k]; j++, k++) {
		if (old_pat[k] == '\\') {
			k++;
		} else if (old_pat[k] == '_') {
			while (*new_pat != '_' && *new_pat != '*' && *new_pat != EOS)
				*new_s++ = *new_pat++;
			if (*new_pat != EOS) {
				new_pat++;
				*new_s++ = old_s[j];
			}
		} else if (old_pat[k] == '*') {	  /* wildcard */
			k++; t = j;
			if (old_pat[k] == '\\')
				k++;
f1:
			while (j < old_s_len && old_s[j] != old_pat[k]) j++;
			end = j;
			if (j < old_s_len) {
	    		for (m=j+1, n=k+1; m < old_s_len && old_pat[n]; m++, n++) {
					if (old_pat[n] == '*')
						break;
					else if (old_pat[n] == '_')
						break;
					else if (old_s[m] != old_pat[n]) {
		    			j++;
						goto f1;
					}
				}
				if (m < old_s_len && !old_pat[n]) {
					j++;
					goto f1;
				}
			}
			while (*new_pat != '_' && *new_pat != '*' && *new_pat != EOS) {
				if (*new_pat == '\\')
					new_pat++;
				*new_s++ = *new_pat++;
			}
			if (*new_pat != EOS) {
				new_pat++;
				while (t < end)
					*new_s++ = old_s[t++];
			}
			if (j >= old_s_len || old_pat[k] == EOS) break;
		}
	}
	while (*new_pat != EOS)
		*new_s++ = *new_pat++;
	*new_s = EOS;
}

static int change(char *line, AttTYPE *att, int pos, int wlen, char *from_word, char *to_word) {
	register int i;
	register int tlen;

	if (wildcard) {
		make_replacement_str(from_word, line+pos, wlen, to_word, templineC4);
		to_word = templineC4;
	}

	tlen = strlen(to_word);
	if (tlen > wlen) {
		int num;
		num = tlen - wlen;
		for (i=strlen(line); i >= pos; i--) {
			line[i+num] = line[i];
			att[i+num] = att[i];
		}
	} else if (tlen < wlen) {
		if (*to_word == EOS)
			att_cp(0, line+pos+tlen, line+pos+wlen, att+pos+tlen, att+pos+wlen);
		else
			att_cp(0, line+pos+tlen-1, line+pos+wlen-1, att+pos+tlen-1, att+pos+wlen-1);
	}

	if (*from_word == ' ' && *to_word == ' ') {
		pos++;
		to_word++;
		tlen--;
	}
	if (from_word[strlen(from_word)-1] == ' ' && to_word[tlen-1] == ' ') {
		for (; *(to_word+1) != EOS; pos++) {
			line[pos] = *to_word++;
			if (pos > 0)
				att[pos] = att[pos-1];
		}
	} else {
		for (; *to_word != EOS; pos++) {
			line[pos] = *to_word++;
			if (pos > 0)
				att[pos] = att[pos-1];
		}
	}
	return(pos-1);
}	

static long FindAndChange(char *line, AttTYPE *att, long isFound) {
	register int pos, lPos;
	char percentFound;
	struct words *nextone;
	int wlen;

	pos = 0;
	nextone = head;
	if (line[pos] == HIDEN_C && !isSearchForBullets) {
		lPos = pos;
		pos++;
		percentFound = (line[pos] == '%');
		for (; line[pos] != HIDEN_C && line[pos] != EOS; pos++) {
			if (!percentFound && !isdigit(line[pos]) && line[pos] != '_')
				break;
		}
		if (line[pos] == HIDEN_C)
			pos++;
		else
			pos = lPos;
	} else if (stringOriented == 2 && line[pos] == '"' && (pos == 0 || line[pos-1] == '\t')) {
		lPos = pos;
		pos++;
		for (; line[pos] != EOS; pos++) {
			if (line[pos] == '"' && line[pos+1] == ',')
				break;
		}
		if (line[pos] == '"')
			pos++;
		else
			pos = lPos;
	}
	if (line[pos] == '\n' || uS.isskip(line, pos, &dFnt, MBF)) {
		while (nextone != NULL) {
			if ((wlen=chstring_found(line,pos,nextone->word))) {
				isFound++;
				pos = change(line,att,pos,wlen,nextone->word,nextone->toword);
				break;
			}
			nextone = nextone->next;
		}
	} else {
		while (nextone != NULL) {
			if (isSpace(nextone->word[0])) {
				if ((wlen=chstring_found(line,pos,nextone->word+1))) {
					isFound++;
					if (nextone->toword[0] == ' ') 
						pos=change(line,att,pos,wlen,nextone->word,nextone->toword+1);
					else
						pos = change(line,att,pos,wlen,nextone->word,nextone->toword);
					break;
				}
			} else {
				if ((wlen=chstring_found(line,pos,nextone->word))) {
					isFound++;
					pos = change(line,att,pos,wlen,nextone->word,nextone->toword);
					break;
				}
			}
			nextone = nextone->next;
		}
	}
	if (stringOriented == 0) {
		while (!uS.isskip(line, pos, &dFnt, MBF))
			pos++;
		while (uS.isskip(line, pos, &dFnt, MBF))
			pos++;
	} else
		pos++;
	if (line[pos] == HIDEN_C && !isSearchForBullets) {
		lPos = pos;
		pos++;
		percentFound = (line[pos] == '%');
		for (; line[pos] != HIDEN_C && line[pos] != EOS; pos++) {
			if (!percentFound && !isdigit(line[pos]) && line[pos] != '_')
				break;
		}
		if (line[pos] == HIDEN_C)
			pos++;
		else
			pos = lPos;
	} else if (stringOriented == 2 && line[pos] == '"' && (pos == 0 || line[pos-1] == '\t')) {
		lPos = pos;
		pos++;
		for (; line[pos] != EOS; pos++) {
			if (line[pos] == '"' && line[pos+1] == ',')
				break;
		}
		if (line[pos] == '"')
			pos++;
		else
			pos = lPos;
	}
	while (line[pos] != EOS) {
		nextone = head;
		while (nextone != NULL) {
			if ((wlen=chstring_found(line,pos,nextone->word))) {
				isFound++;
				pos = change(line,att,pos,wlen,nextone->word,nextone->toword);
				break;
			} else
				nextone = nextone->next;
		}
		if (stringOriented == 0) {
			while (!uS.isskip(line, pos, &dFnt, MBF))
				pos++;
			while (uS.isskip(line, pos, &dFnt, MBF))
				pos++;
		} else
			pos++;
		if (line[pos] == HIDEN_C && !isSearchForBullets) {
			lPos = pos;
			pos++;
			percentFound = (line[pos] == '%');
			for (; line[pos] != HIDEN_C && line[pos] != EOS; pos++) {
				if (!percentFound && !isdigit(line[pos]) && line[pos] != '_')
					break;
			}
			if (line[pos] == HIDEN_C)
				pos++;
			else
				pos = lPos;
		} else if (stringOriented == 2 && line[pos] == '"' && (pos == 0 || line[pos-1] == '\t')) {
			lPos = pos;
			pos++;
			for (; line[pos] != EOS; pos++) {
				if (line[pos] == '"' && line[pos+1] == ',')
					break;
			}
			if (line[pos] == '"')
				pos++;
			else
				pos = lPos;
		}
	}
	return(isFound);
}

static void fixCodes(void) {
	long i, b;

	for (b=0L; utterance->line[b]; b++) {
		if (uS.isRightChar(utterance->line,b,'[',&dFnt,MBF)) {
			for (i=b; !uS.isRightChar(utterance->line,i,']',&dFnt,MBF) && utterance->line[i] != EOS; i++) ;
			if (uS.isRightChar(utterance->line,i,']',&dFnt,MBF)) {
				while (!uS.isRightChar(utterance->line,b,']',&dFnt,MBF)) {
					if (utterance->line[b] == '\t' || utterance->line[b] == '\n')
						utterance->line[b] = ' ';
					if (isspace(utterance->line[b])) {
						b++;
						for (i=b; isspace(utterance->line[i]); i++) ;
						if (i > b)
							att_cp(0, utterance->line+b, utterance->line+i, utterance->attLine+b, utterance->attLine+i);
					} else
						b++;
				}
			}
		}
	}
}

static char isIDTier(char *sp, char *line) {
//2015-03-30 	if (cutt_isCAFound) return(TRUE);
	for (; *line != EOS; line++) {
		if (*line == '\n' && isSpeaker(*(line+1)))
			return(TRUE);
	}
	return(FALSE);
}

static void cleanAtts(char *sp, char *line, AttTYPE *attSp, AttTYPE *attLine) {
	int i;
	char sb, bullet, preCode, uttFound;

	for (i=0; sp[i] != EOS; i++)
		attSp[i] = 0;
	if (sp[0] != '*' && !cutt_isBlobFound) {
		for (i=0; line[i] != EOS; i++)
			attLine[i] = 0;
		return;
	}
	sb = FALSE;
	bullet = FALSE;
	preCode = FALSE;
	uttFound = FALSE;
	for (i=0; line[i] != EOS; i++) {
		if (line[i] == '[') {
			if (!cutt_isBlobFound)
				sb = TRUE;
			preCode = FALSE;
			bullet = FALSE;
		} else if (line[i] == ']') {
			sb = FALSE;
			attLine[i] = 0;
		} else if (line[i] == HIDEN_C && !sb) {
			preCode = FALSE;
			bullet = !bullet;
			attLine[i] = 0;
		} else if (line[i] == '+' && !sb && !cutt_isBlobFound) {
			if (i == 0 || uS.isskip(line,i-1,&dFnt,C_MBF))
				preCode = TRUE;
		} else if (uS.IsUtteranceDel(line,i) && !sb) {
			if (!cutt_isBlobFound && !bullet && !uS.isPause(line, i, NULL, NULL))
				uttFound = TRUE;
		} else if (isSpace(line[i]) && !sb && !uttFound) {
			preCode = FALSE;
			bullet = FALSE;
		} else if (uS.isskip(line,i,&dFnt,C_MBF) && !sb && !cutt_isBlobFound) {
			attLine[i] = 0;
			preCode = FALSE;
			bullet = FALSE;
		}
		if (sb || bullet || preCode || uttFound)
			attLine[i] = 0;
	}
}

static void checkFixInitialZeroAge(char *line) { // lxs
	int i, cnt;

	cnt = 0;
	for (i=0; line[i] != EOS; i++) {
		if (line[i] == '|') {
			cnt++;
			if (cnt == 3) {
				for (i++; line[i] != EOS && line[i] != '|'; i++) {
					if (line[i] == ';' && isdigit(line[i+1]) && line[i+2] == '.') {
						i++;
						uS.shiftright(line+i, 1);
						line[i] = '0';
						for (; line[i] != EOS && line[i] != '|'; i++) {
							if (line[i] == '.' && isdigit(line[i+1]) && line[i+2] == '|') {
								i++;
								uS.shiftright(line+i, 1);
								line[i] = '0';
								return;
							}
						}
						return;
					} else if (line[i] == '.' && isdigit(line[i+1]) && line[i+2] == '|') {
						i++;
						uS.shiftright(line+i, 1);
						line[i] = '0';
						return;
					}
				}
			}
		}
	}
}

void call() {
	char RightSpeaker = FALSE;
	long tlineno = 0L;
	long isFound;
	int i, j;

	if (!chatmode) {
		lineonly = TRUE;
		tab = FALSE;
		chstring_isCoreLex = FALSE;
	}
	isFound = 0L;
	if (tab)
		isFound++;
	currentatt = 0;
	currentchar = (char)getc_cr(fpin, &currentatt);
	if (stringOriented == 2) {
		if (currentchar == (char)0xef) {
			if (currentchar != EOF)
				currentchar = (char)getc_cr(fpin, &currentatt);
			if (currentchar != EOF)
				currentchar = (char)getc_cr(fpin, &currentatt);
			if (currentchar != EOF)
				currentchar = (char)getc_cr(fpin, &currentatt);
		}
	}
	while (getwholeutter()) {
		if (!stout) {
			tlineno = tlineno + 1L;
			if (tlineno % PERIOD == 0)
				fprintf(stderr,"\r%ld ",tlineno);
		}
			
		if (tab || chstring_isCoreLex) {
			for (i=strlen(utterance->speaker)-1; i >= 0 && isSpace(utterance->speaker[i]); i--) ;
			utterance->speaker[i+1] = EOS;
//			if (utterance->speaker[i] == ':')
//				strcat(utterance->speaker,"\t");
			if (*utterance->speaker== '*')
				uS.uppercasestr(utterance->speaker+1, &dFnt, MBF);
			else if (*utterance->speaker == '%')
				uS.lowercasestr(utterance->speaker+1, &dFnt, MBF);
			else if (uS.partcmp(utterance->speaker,IDOF,FALSE,FALSE)) {
				checkFixInitialZeroAge(utterance->line);
			}
			cleanAtts(utterance->speaker,utterance->line,utterance->attSp,utterance->attLine);
			fixCodes();
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
			if (tab || chstring_isCoreLex) {
				if (!NO_CHANGE) {
					uS.remblanks(utterance->line);
					if (*utterance->line == EOS)
						uS.remblanks(utterance->speaker);
					printout(utterance->speaker,utterance->line,utterance->attSp,utterance->attLine,!isIDTier(utterance->speaker,utterance->line));
				} else
					printout(utterance->speaker,utterance->line,utterance->attSp,utterance->attLine,FALSE);
			} else {
				printout(utterance->speaker,utterance->line,utterance->attSp,utterance->attLine,FALSE);
			}
		} else {
			if (*utterance->speaker == '*')
				RightSpeaker = TRUE;
			if (chstring_isCoreLex) {
				isFound = FindAndChangeCoreLex(utterance->line, utterance->attLine, isFound);
			} else if (lineonly) {
				isFound = FindAndChange(utterance->line, utterance->attLine, isFound);
			} else if (headeronly) {
				isFound = FindAndChange(utterance->speaker, utterance->attSp, isFound);
			} else {
				att_cp(0, utterance->tuttline, utterance->speaker, tempAtt, utterance->attSp);
				att_cp(strlen(utterance->tuttline), utterance->tuttline, utterance->line, tempAtt, utterance->attLine);

/*
printf("0; uttline=%s", utterance->tuttline);
if (utterance->tuttline[strlen(utterance->tuttline)-1] != '\n') putchar('\n');
*/
				isFound = FindAndChange(utterance->tuttline, tempAtt, isFound);
				MkSpeaker(utterance->tuttline, tempAtt);
			}
/*
printf("1; sp=%s; uttline=%s", utterance->speaker, uttline);
if (uttline[strlen(uttline)-1] != '\n') putchar('\n');
*/
			if (chatmode) {
				for (j=0; isSpace(utterance->speaker[j]); j++) ;
			} else
				j = 0;

			if (NO_CHANGE) {
				if (tab || chstring_isCoreLex) {
					i = 0;
					if (/*//2015-03-30 !cutt_isCAFound && */!cutt_isBlobFound)
						for (; isSpace(utterance->line[i]); i++) ;
					printout(utterance->speaker+j,utterance->line+i,utterance->attSp+j,utterance->attLine+i,FALSE);
				} else {
					if (stringOriented != 0 && utterance->line[0] == '\n' && utterance->line[1] == EOS)
						utterance->line[0] = EOS;
					printout(utterance->speaker+j,utterance->line,utterance->attSp+j,utterance->attLine,FALSE);
				}
			} else {
				uS.remblanks(utterance->line);
				i = 0;
				if (/*//2015-03-30 !cutt_isCAFound && */!cutt_isBlobFound)
					for (; isSpace(utterance->line[i]); i++) ;
				if (!chatmode) {
					if (utterance->line[i] != EOS) {
						j = strlen(utterance->line) - 1;
						if (utterance->line[j] != '\n')
							fprintf(fpout, "%s\n", utterance->line);
						else
							fputs(utterance->line, fpout);
					} else
						fprintf(fpout, "\n");
				} else if (utterance->line[i] != EOS)
					printout(utterance->speaker+j,utterance->line+i,utterance->attSp+j,utterance->attLine+i,!isIDTier(utterance->speaker+j,utterance->line+i));
				else if (*(utterance->speaker+j) != '*' && *(utterance->speaker+j) != '%') {
					uS.remblanks(utterance->speaker+j);
					if (*(utterance->speaker+j) != EOS) {
						i = strlen(utterance->speaker+j) - 1;
						if (i >= 0) {
							if (utterance->speaker[j+i] != '\n') {
								strcpy(utterance->line, "\n");
								printout(utterance->speaker+j,utterance->line,utterance->attSp+j,NULL,0);
							} else
								printout(utterance->speaker+j,NULL,utterance->attSp+j,NULL,0);
						} else
							printout(utterance->speaker+j,NULL,utterance->attSp+j,NULL,0);
					}
				} else {
					uS.remblanks(utterance->speaker+j);
					i = strlen(utterance->speaker+j) - 1;
					if (i > 4 || (i == 4 && utterance->speaker[j+i] != ':')) {
						fprintf(stderr,"\n");
						fprintf(stderr,"*** File \"%s\": line %ld.\n", oldfname, lineno);
						fprintf(stderr, "Tier name is longer than 3 char\n");
						fprintf(stderr,"%s\n", utterance->speaker);
						chstring_cleanup();
						cutt_exit(0);
					}
				}
			}
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
	struct words *nextone;

	f++;
	switch(*f++) {
		case 'c':
				if (*f)
					uS.str2FNType(dicname, 0L, getfarg(f,f1,i));
				break;
		case 'd':
				NO_CHANGE = TRUE;
				no_arg_option(f);
				break;
		case 's':
				nextone = makenewsym(f);
				(*i)++;
				if (f1 == NULL) {
					fputs("Unexpected ending.\n",stderr);
					cutt_exit(0);
				} else if (*f1 == '-') 
					fprintf(stderr,"Warning: second string %s looks like an option\n",f1);
				nextone->toword = (char *)malloc(strlen(f1)+1);
				if (nextone->toword == NULL)
					chstring_overflow();
				strcpy(nextone->toword,f1);
				fprintf(stderr,"From string \"%s\" ",nextone->word);
				fprintf(stderr,"to string \"%s\"\n",nextone->toword);
				pars(nextone->toword);
				chstring_FirstTime = FALSE;
				break;
		case 'l':
				if (*f == 'x') {
					DispChanges = FALSE;
				} else {
					if (lineonly) {
						fprintf(stderr, "It is illegal to use options +l and +b together.\n");
						cutt_exit(0);
					}
					headeronly = TRUE;
					no_arg_option(f);
				}
				break;
		case 'b':
				if (headeronly) {
					fprintf(stderr,"It is illegal to use options +l and +b together.\n");
					cutt_exit(0);
				}
				lineonly = TRUE;
				no_arg_option(f);
				break;
		case 'q':
				if (chatmode) {
					if (*f == EOS) {
						tab = TRUE;
					} else if (*f == '1') {
						chstring_isCoreLex = TRUE;
					} else {
						fprintf(stderr,"Only +q or +q1 options are allowed\n");
						cutt_exit(0);
					}
				} else {
					fprintf(stderr,"+q option is not allowed with TEXT format\n");
					cutt_exit(0);
				}
//				no_arg_option(f);
				break;
		case 'L':
			int len;
			strcpy(lib_dir, f);
			len = strlen(lib_dir);
			if (len > 0 && lib_dir[len-1] != '/')
				strcat(lib_dir, "/");
			break;
		case 'w':
				if (*f == EOS)
					stringOriented = 1;
				else if (*f == '1')
					stringOriented = 2;
				else {
					fprintf(stderr,"Invalid argument for option: %s\n", f-2);
					fprintf(stderr,"Please choose \"-w\"\n");
//					fprintf(stderr,"Please choose \"-w\" or \"-w1\"\n");
					cutt_exit(0);
				}
				break;
		case 'x':
				wildcard = FALSE;
				no_arg_option(f);
				break;
		default:
				maingetflag(f-2,f1,i);
				break;
	}
}
