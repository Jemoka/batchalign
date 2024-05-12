/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


/* mul.h  combines all varieties */

/* set up pointers to different varieties of common functions */

/* constants for all the programs */

#ifndef MULDEF
#define MULDEF

#define BeginningOfData 0
#define BeginningOfTier 1
#define MiddleOfTier	2
#define EndOfTier		3
#define EndOfData		4

extern FNType openFileName[];

	#define CLAN_MAIN_RETURN int
	extern int  main(int argc, char *argv[]);
extern void init(char); 	
extern void usage(void);
extern void getflag(char *, char *, int *);
extern void call(void);
extern void main_clan(int argc, char *argv[]);

#endif /* MULDEF */
