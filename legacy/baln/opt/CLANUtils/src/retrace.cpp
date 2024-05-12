/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


/************************************************************************/
/* RETRACE: Produces a %ret line which codes consecutive utterance	*/
/* line repetitions according to CHAT format. Test case results are 	*/
/* listed in the manual entry.						*/
/************************************************************************/

/* Created by Jeff Sokolov, January 1988				*/

/* Please send reports of any bugs to sokolov@andrew.cmu.edu		*/

/* To compile: cc -g -c retrace.c cutt.c -o retrace			*/

/* BUGS: 
*  1. If there is a carriage-return in the utterance at precisely the
*	 end of a retrace, the program will overgenerate coding symbols:
*	 
*	 *ADA:   mine didn't too. mine-'s@n went all the way down [#] <to the train>
*		  [//] [#] to the train track.
*	 %ret:   mine didn't too. mine-'s@n went all the way down [#] <to the train
*		  [/]> [//] [#] to the train track.
*
*/

/* Complaints about CLAN: What constitutes word-like and non word-like characters
   is defined in too many different places:
   a. addword: Allows the CUTT user to add or delete "words"
   b. geteachword: Skips over parentheses
   c. uS.isskip: Skips over punctuation only
   d. punctuation: ,.;:?!\"[]{}<>" (what about: @$%^&*+=|/~`)
*/

/* This eventually needs to be changed to "$SOURCEDIR/cu.h" */
#include "cu.h"

#define IS_WIN_MODE FALSE
#include "mul.h" 

#define CHAT_MODE 2

extern struct tier *defheadtier;

#define WORDLST struct WordsList
struct WordsList {
	char *word;
	char isCont;
	int sp, ep;
	WORDLST *nextWord;
} ;

WORDLST *root_word;

char ret_substitute_flag = FALSE;


void usage() {
	puts("Retrace produces a %ret line which codes consecutive utterance");
	puts("  line repetitions according to CHAT format");
	printf("Usage: retrace [c %s] filename(s)\n",mainflgs());
	puts("+c: substitute retrace line for main line");

	mainusage(TRUE);
}

void getflag(char *f, char *f1, int *i) {
	f++;
	switch(*f++) {
	  case 'c':
				ret_substitute_flag = TRUE; /* Retrace line will replace main line */
				break;
	  default:
				maingetflag(f-2,f1,i);	/* Parse the other standard clan args */
				break;
	}
}

CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = RETRACE;
	OnlydataLimit = 0;
	UttlineEqUtterance = FALSE;
	bmain(argc,argv,NULL);
}

void init(char first) {
	if (first) {
		addword('\0','\0',"+<#>");
		addword('\0','\0',"+xxx");
		addword('\0','\0',"+yyy");
		addword('\0','\0',"+www");
		addword('\0','\0',"+xxx@s*");
		addword('\0','\0',"+yyy@s*");
		addword('\0','\0',"+www@s*");
		addword('\0','\0',"+unk|xxx");
		addword('\0','\0',"+unk|yyy");
		addword('\0','\0',"+*|www");
		addword('\0','\0',"+</?>");
		addword('\0','\0',"+</->");
		addword('\0','\0',"+<///>");
		addword('\0','\0',"+<//>");
		addword('\0','\0',"+</>");  // list R6 in mmaininit funtion in cutt.c
		maininitwords();
		mor_initwords();
		stout = FALSE;
		onlydata = 1;
		LocalTierSelect = TRUE;
		if (defheadtier != NULL) {
			if (defheadtier->nexttier != NULL)
				free(defheadtier->nexttier);
			free(defheadtier);
			defheadtier = NULL;
		}
	}
}

static void FreeWordList(void) {
	WORDLST *t;

	while (root_word) {
		t = root_word;
		root_word = root_word->nextWord;
		if (t->word)
			free(t->word);
		free(t);
	}
	root_word = NULL;
}

static void retrace_error(const char *mess) {
	FreeWordList();
	if (*mess)
		fprintf(stderr, "Retrace: %s", mess);
	cutt_exit(0);
}

static void AddWordToList(char *word, int sp, int ep, char isCont) {
	WORDLST *t;

	if (root_word == NULL) {
		root_word = NEW(WORDLST);
		t = root_word;
	} else {
		t = root_word;
		while (t->nextWord != NULL) t = t->nextWord;
		t->nextWord = NEW(WORDLST);
		t = t->nextWord;
	}
	if (t == NULL)
		retrace_error("out of memory");
	t->nextWord = NULL;
	t->word = (char *)malloc(strlen(word)+1);
	if (t->word == NULL)
		retrace_error("out of memory");
	strcpy(t->word, word);
	t->sp = sp;
	t->ep = ep;
	t->isCont = isCont;
}

static void findWordInfo(int sp, int oldEp, int *ep, char *isCont) {
	char sqb;

	if (sp == oldEp) {
		*isCont = FALSE;
	} else {
		sqb = FALSE;
		for (sp--; sp > oldEp; sp--) {
			if (utterance->line[sp] == ']')
				sqb = TRUE;
			else if (utterance->line[sp] == '[')
				sqb = FALSE;
			if (!uS.isskip(utterance->line, sp, &dFnt, MBF) && !sqb)
				break;
		}
		if (sp == oldEp)
			*isCont = TRUE;
		else
			*isCont = FALSE;
	}
	if (*ep != 0) {
		if (uttline[sp] == ']')
			sqb = TRUE;
		else
			sqb = FALSE;
		for (sp=(*ep)-1; (uS.isskip(uttline, sp, &dFnt, MBF) || sqb) && sp >= 0; sp--) {
			if (uttline[sp] == ']')
				sqb = TRUE;
			else if (uttline[sp] == '[')
				sqb = FALSE;
		}
		*ep = sp + 1;
	}
}

static void addRetraceToOutput(WORDLST *s, WORDLST *e, int lp, char *retrace_line) {
	int i, j = 0;
	WORDLST *l;

	for (l=s; l->nextWord != e; l=l->nextWord) ;
	for (i=0; utterance->line[i] && i < s->sp; i++) {
		retrace_line[j++] = utterance->line[i];
	}
	if (s != l)
		retrace_line[j++] = '<';
	for (; utterance->line[i] && i < l->ep; i++) {
		retrace_line[j++] = utterance->line[i];
	}
	if (s != l)
		retrace_line[j++] = '>';
	retrace_line[j++] = ' ';
	retrace_line[j++] = '[';
	retrace_line[j++] = '/';
	retrace_line[j++] = ']';
	retrace_line[j++] = ' ';
	retrace_line[j++] = '\001';
	for (; utterance->line[i]; i++) {
		retrace_line[j++] = utterance->line[i];
		if (i == lp)
			retrace_line[j++] = '\002';
	}
	if (i == lp)
		retrace_line[j++] = '\002';
	retrace_line[j] = EOS;
}

static char singleMatch(WORDLST *s, WORDLST *e, char *retrace_line) {
	WORDLST *isMatch, *orgS;

	orgS = s;
	isMatch = NULL;
	strcpy(templineC2, s->word);
	for (s=s->nextWord; s != e; s=s->nextWord) {
		strcpy(templineC3, s->word);
		if (strcmp(templineC2, templineC3) == 0)
			isMatch = s;
		else
			break;
	}
	if (isMatch) {
		addRetraceToOutput(orgS, isMatch, isMatch->ep, retrace_line);
		return(TRUE);
	} else
		return(FALSE);
}

static char doubleMatch(WORDLST *s, WORDLST *e, char *retrace_line) {
	WORDLST *isMatch, *lastMatch = NULL, *orgS, *lastS;

	orgS = s;
	isMatch = NULL;
	strcpy(templineC2, s->word);
	s = s->nextWord;
	if (s != e) {
		strcat(templineC2, s->word);
		s = s->nextWord;
	}
	for (; s != e; s=s->nextWord) {
		lastS = s;
		strcpy(templineC3, s->word);
		s = s->nextWord;
		if (s == e)
			break;
		strcat(templineC3, s->word);
		if (strcmp(templineC2, templineC3) == 0) {
			isMatch = lastS;
			lastMatch = s;
		} else
			break;
	}
	if (isMatch && lastMatch != NULL) {
		addRetraceToOutput(orgS, isMatch, lastMatch->ep, retrace_line);
		return(TRUE);
	} else
		return(FALSE);
}

static char tripleMatch(WORDLST *s, WORDLST *e, char *retrace_line) {
	WORDLST *isMatch, *lastMatch = NULL, *orgS, *lastS;

	orgS = s;
	isMatch = NULL;
	strcpy(templineC2, s->word);
	s = s->nextWord;
	if (s == e)
		return(FALSE);
	strcat(templineC2, s->word);
	s = s->nextWord;
	if (s == e)
		return(FALSE);
	strcat(templineC2, s->word);
	s = s->nextWord;
	for (; s != e; s=s->nextWord) {
		lastS = s;
		strcpy(templineC3, s->word);
		s = s->nextWord;
		if (s == e)
			break;
		strcat(templineC3, s->word);
		s = s->nextWord;
		if (s == e)
			break;
		strcat(templineC3, s->word);
		if (strcmp(templineC2, templineC3) == 0) {
			isMatch = lastS;
			lastMatch = s;
		} else
			break;
	}
	if (isMatch && lastMatch != NULL) {
		addRetraceToOutput(orgS, isMatch, lastMatch->ep, retrace_line);
		return(TRUE);
	} else
		return(FALSE);
}

static char findRetraceInCluster(WORDLST *s, WORDLST *e, char *retrace_line) {
	if (s->nextWord == e)
		return(TRUE);

/*
WORDLST *t;
for (t=s; t != e; t=t->nextWord)
	printf("%s(%d,%d,%d) ", t->word, t->sp, t->ep, t->isCont);
printf("\n");
*/
	for (; s != e; s=s->nextWord) {
		if (singleMatch(s, e, retrace_line))
			return(FALSE);
		if (doubleMatch(s, e, retrace_line))
			return(FALSE);
		if (tripleMatch(s, e, retrace_line))
			return(FALSE);
	}
	return(TRUE);
}

static char makeretrace(char *retrace_line) {
	char isCont, done;
	char clan_word[BUFSIZ];
	int index, sp, ep, oldEp;
	WORDLST *s, *e;

	retrace_line[0] = EOS;
	root_word = NULL;
	isCont = FALSE;
	for (index=0; uS.isskip(uttline,index,&dFnt,MBF); index++) ;
	sp = index;
	oldEp = index;
	while ((index=getword(utterance->speaker, uttline, clan_word, NULL, index)) != 0) {
		ep = index;
		findWordInfo(sp, oldEp, &ep, &isCont);
		AddWordToList(clan_word, sp, ep, isCont);
		oldEp = ep;
		for (; uS.isskip(uttline,index,&dFnt,MBF); index++) ;
		sp = index;
	}
/*
for (s=root_word; s != NULL; s=s->nextWord)
	printf("%s(%d,%d,%d) ", s->word, s->sp, s->ep, s->isCont);
printf("\n");
*/
	done = TRUE;
	e = root_word;
	while (e != NULL) {
		s = e;
		for (e=e->nextWord; e != NULL && e->isCont == TRUE; e=e->nextWord) ;
		done = findRetraceInCluster(s, e, retrace_line);
		if (!done)
			break;
	}
	FreeWordList();
	return(done);
}

static void filterLastRetrace(char *line) {
	while (*line != EOS) {
		if (*line == '\001') {
			for (; *line != EOS && *line != '\002'; line++)
				*line = ' ';
			if (*line == '\002') {
				*line = ' ';
				line++;
			}
		} else
			line++;
	}
}

static void filterFinalOutput(char *line) {
	while (*line != EOS) {
		if (*line == '\001' || *line == '\002')
			strcpy(line, line+1);
		else
			line++;
	}
}

void call() {
	char done, retFound;
	long outlineno;
	extern long tlineno;
	extern long MAXOUTCOL;

	outlineno = 1L;
	currentatt = 0;
	currentchar = (char)getc_cr(fpin, &currentatt);
	while (getwholeutter()) {
		if (*utterance->speaker == '*') {
			strcpy(spareTier1, utterance->line);
			if (!ret_substitute_flag) {
				printout(utterance->speaker,utterance->line, NULL, NULL, FALSE);
				outlineno += tlineno;
			}

			retFound = FALSE;
			do {
				done = makeretrace(spareTier2);
				if (!done) {
					strcpy(utterance->line, spareTier2);
					if (uttline != utterance->line)
						strcpy(uttline,utterance->line);
					filterData(utterance->speaker,uttline);
					filterLastRetrace(uttline);
					retFound = TRUE;
				}
			} while (!done) ;

			if (retFound) {
				filterFinalOutput(utterance->line);
				printf("-------------------------------------\n");
				printf("*** File \"%s\": line %ld.\n", oldfname, lineno);
				printf("ORG: %s", spareTier1);
				if (!stout)
					printf("*** File \"%s\": line %ld.\n", newfname, outlineno);
				printf("NEW: %s", utterance->line);
			}
			if (!ret_substitute_flag)
				strcpy(utterance->speaker, "%ret:");
			printout(utterance->speaker,utterance->line, NULL, NULL, TRUE);
			outlineno += ((strlen(utterance->line) + strlen(utterance->speaker)) / MAXOUTCOL) + 1L;
		} else {
			printout(utterance->speaker,utterance->line, NULL, NULL, FALSE);
			outlineno += tlineno;
		}
	}
}
