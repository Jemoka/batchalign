/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#ifndef _CUTT_XML_H
#define _CUTT_XML_H

#ifdef __cplusplus
extern "C" {
#endif

/* XML Data structure */
#define Attributes struct ATTRIBUTES
struct ATTRIBUTES {
	char *name;
	char *value;
	Attributes *next;
} ;

#define Element struct ELEMENTS
struct ELEMENTS {
	char *name;
	Attributes *atts;
	char *cData;
	char isUsed;
	Element *data;
	Element *next;
} ;
/* XML Data structure */


extern int stackIndex;
extern Element *CurrentElem;

extern void ResetXMLTree(void);
extern Element *freeElements(Element *p);

#ifdef __cplusplus
}
#endif

#endif /* _CUTT_XML_H */
