/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#include "cu.h"
#include "cutt-xml.h"

#define isSpaceCR(c)	((c) == (char)' ' || (c) == (char)'\t' || (c) == (char)'\n')

static int lastInput;
int  stackIndex = -1;
static Element *RootElem = NULL;
Element *CurrentElem = NULL;

static void freeAttributes(Attributes *p) {
	Attributes *t;

	while (p != NULL) {
		t = p;
		p = p->next;
		if (t->name)
			free(t->name);
		if (t->value)
			free(t->value);
		free(t);
	}
}

Element *freeElements(Element *p) {
	Element *t;

	while (p != NULL) {
		t = p;
		p = p->next;
		freeAttributes(t->atts);
		if (t->data)
			freeElements(t->data);
		if (t->cData)
			free(t->cData);
		if (t->name)
			free(t->name);
		free(t);
	}
	return(NULL);
}

void freeXML_Elements(void) {
	RootElem = freeElements(RootElem);
	CurrentElem = NULL;
	stackIndex = -1;
}

void ResetXMLTree(void) {
	CurrentElem = RootElem;
	stackIndex = -1;
}

static char *xml_mkstring(char *s) {  /* allocate space for string of text */
	char *p;
	int  i;

	for (i=0; isSpaceCR(s[i]); i++) ;
	if (i > 0)
		strcpy(s, s+i);
	uS.remblanks(s);
	i = strlen(s) - 1;
	if (s[0] == '"' && s[i] == '"') {
		s[i] = EOS;
		strcpy(s, s+1);
	}
	if ((p=(char *)malloc(strlen(s)+1)) == NULL)
		return(NULL);
	strcpy(p, s);
	return(p);
}

static Element *parsElem(Element *newElem, char *elem) {
	char t, *eqs;
	int beg, end;
	Attributes *tatt, *latt;
	char qf;

	beg = 0;
	while (elem[beg] != EOS) {
		while (isSpaceCR(elem[beg]))
			beg++;
		end = beg;
		qf = FALSE;
		while ((!isSpaceCR(elem[end]) || qf) && elem[end] != EOS) {
			if (elem[end] == '"')
				qf = !qf;
			end++;
		}
		t = elem[end];
		elem[end] = EOS;
		if (beg < end) {
			if (elem[beg] == '<') {
				beg++;
				if (elem[beg] == '/')
					beg++;
				newElem->name = xml_mkstring(elem+beg);
			} else {
				if ((tatt=NEW(Attributes)) != NULL) {
					tatt->name = NULL;
					tatt->value = NULL;
					tatt->next = NULL;
					if (newElem->atts == NULL)
						newElem->atts = tatt;
					else {
						for (latt=newElem->atts; latt->next != NULL; latt=latt->next) ;
						latt->next = tatt;
					}
					eqs = strchr(elem+beg, '=');
					if (eqs == NULL)
						tatt->name = xml_mkstring(elem+beg);
					else {
						*eqs = EOS;
						tatt->name = xml_mkstring(elem+beg);
						tatt->value = xml_mkstring(eqs+1);
						*eqs = '=';
					}
				}
			}
		}
		elem[end] = t;
		beg = end;
		if (elem[end] != EOS)
			beg = end + 1;
		else
			break;
	}
	return(newElem);
}

static Element *getNewElem(char *closingElem, FILE *fpXMLin) {
	int i;
	char st[256];
	Element *newElem;

	*closingElem = 0;
	newElem = NULL;
	while ((lastInput == 0 || isSpaceCR(lastInput)) && !feof(fpXMLin))
		lastInput = getc_cr(fpXMLin, NULL);
	if (feof(fpXMLin))
		return(NULL);
	i = 0;
	if (lastInput == '<') {
		templineC[i++] = lastInput;
		while ((lastInput=getc_cr(fpXMLin, NULL)) != '>' && !feof(fpXMLin) && i < UTTLINELEN)
			templineC[i++] = lastInput;
		if (lastInput == '>')
			lastInput = getc_cr(fpXMLin, NULL);
		templineC[i] = EOS;
		if (templineC[1] == '/')
			*closingElem = 1;
		else if (templineC[i-1] == '/') {
			*closingElem = 2;
			templineC[--i] = EOS;
		} else if (templineC[0] == '<' && templineC[1] == '!') {
			*closingElem = 2;
		} else if (!strncmp(templineC, "<?xml", 5) && templineC[i-1] == '?') {
			*closingElem = 2;
		}

		newElem = NEW(Element);
		if (newElem != NULL) {
			newElem->name = NULL;
			newElem->atts = NULL;
			newElem->cData = NULL;
			newElem->isUsed = FALSE;
			newElem->data = NULL;
			newElem->next = NULL;
			newElem = parsElem(newElem, templineC);
		}
	} else {
		templineC[i++] = lastInput;
		while ((lastInput=getc_cr(fpXMLin, NULL)) != '<' && !feof(fpXMLin) && i < UTTLINELEN)
			templineC[i++] = lastInput;
		templineC[i] = EOS;
		newElem = NEW(Element);
		if (newElem != NULL) {
			strcpy(st, "CONST");
			newElem->name = xml_mkstring(st);
			newElem->atts = NULL;
			newElem->cData = xml_mkstring(templineC);
			newElem->isUsed = FALSE;
			newElem->data = NULL;
			newElem->next = NULL;
		}
	}
	return(newElem);
}

static Element *AddElements(FILE *fpXMLin) {
	char closingElem;
	Element *newElem, *rElem, *cElem;

	rElem = NULL;
	newElem = getNewElem(&closingElem, fpXMLin);
	if (newElem == NULL)
		return(rElem);
	rElem = newElem;
	if (closingElem == 1) {
		if (rElem == newElem) {
			rElem = NULL;
		}
		newElem = freeElements(newElem);
		return(rElem);
	}
	cElem = rElem;
	do {
		cElem->next = NULL;
		if (closingElem == 0 && cElem->cData == NULL)
			cElem->data = AddElements(fpXMLin);
		newElem = getNewElem(&closingElem, fpXMLin);
		if (newElem == NULL)
			return(rElem);
		if (closingElem == 1) {
			if (rElem == newElem) {
				rElem = NULL;
			}
			newElem = freeElements(newElem);
			return(rElem);
		}
		cElem->next = newElem;
		cElem = cElem->next;
	} while (1) ;
	return(rElem);
}

char BuildXMLTree(FILE *fpXMLin) {
	freeXML_Elements();
	lastInput = 0;

	RootElem = AddElements(fpXMLin);
	CurrentElem = RootElem;
	stackIndex = -1;
	return(true);
}
