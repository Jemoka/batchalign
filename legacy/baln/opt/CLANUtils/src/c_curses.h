/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#ifndef _C_CURSES_
#define _C_CURSES_

#define SCROLL_BAR_SIZE	15
#define ActualWinSize(w) ((w == global_df->w1) ? global_df->EdWinSize : w->num_rows)

#define ATTMARKER		'\002'

// text atts begin; AttTYPE type
#define set_changed_to_1(x)		(AttTYPE)(x | 1)
#define is_changed(x)			(AttTYPE)(x & 1)
#define set_changed_to_0(x)		(AttTYPE)(x & 0xfffe)

#define set_reversed_to_1(x)	(AttTYPE)(x | 2)
#define is_reversed(x)			(AttTYPE)(x & 2)
#define set_reversed_to_0(x)	(AttTYPE)(x & 0xfffd)

#define underline_start			'\001'
#define underline_end			'\002'
#define set_underline_to_1(x)	(AttTYPE)(x | 4)
#define is_underline(x)			(AttTYPE)(x & 4)
#define set_underline_to_0(x)	(AttTYPE)(x & 0xfffb)

#define italic_start			'\003'
#define italic_end				'\004'
#define set_italic_to_1(x)		(AttTYPE)(x | 8)
#define is_italic(x)			(AttTYPE)(x & 8)
#define set_italic_to_0(x)		(AttTYPE)(x & 0xfff7)

#define error_start				'\005'
#define error_end				'\006'
#define set_error_to_1(x)		(AttTYPE)(x | 16)
#define is_error(x)				(AttTYPE)(x & 16)
#define set_error_to_0(x)		(AttTYPE)(x & 0xffef)

#define set_color_to_1(x)		(AttTYPE)(x | 32)
#define is_colored(x)			(AttTYPE)(x & 32)
#define set_color_to_0(x)		(AttTYPE)(x & 0xffdf)

#define bold_start				'\007'
#define bold_end				'\010'
#define set_bold_to_1(x)		(AttTYPE)(x | 64)
#define is_bold(x)				(AttTYPE)(x & 64)
#define set_bold_to_0(x)		(AttTYPE)(x & 0xffbf)


#define blue_color				1
#define blue_start				'\016'
#define red_color				2
#define red_start				'\017'
#define green_color				3
#define green_start				'\020'
#define magenta_color			4
#define magenta_start			'\021'
#define color_end				'\022'
extern  AttTYPE set_color_num(char num, AttTYPE att);// bits 8=128, 9=256, 10=512
#define is_word_color(x)		(AttTYPE)(x & 896)	 // bits 8=128, 9=256, 10=512
extern  char get_color_num(AttTYPE att);			 // bits 8=128, 9=256, 10=512
#define zero_color_num(x)		(AttTYPE)(x & 0xfc7f)// bits 8=128, 9=256, 10=512

/*
#define set_X_to_1(x)		(AttTYPE)(x | 1024)
#define is_X(x)				(AttTYPE)(x & 1024)
#define set_X_to_0(x)		(AttTYPE)(x & 0xfbff)

#define set_X_to_1(x)		(AttTYPE)(x | 2048)
#define is_X(x)				(AttTYPE)(x & 2048)
#define set_X_to_0(x)		(AttTYPE)(x & 0xf7ff)

#define set_X_to_1(x)		(AttTYPE)(x | 4096)
#define is_X(x)				(AttTYPE)(x & 4096)
#define set_X_to_0(x)		(AttTYPE)(x & 0xefff)

#define set_X_to_1(x)		(AttTYPE)(x | 8192)
#define is_X(x)				(AttTYPE)(x & 8192)
#define set_X_to_0(x)		(AttTYPE)(x & 0xdfff)

#define set_X_to_1(x)		(AttTYPE)(x | 16384)
#define is_X(x)				(AttTYPE)(x & 16384)
#define set_X_to_0(x)		(AttTYPE)(x & 0xbfff)

#define set_X_to_1(x)		(AttTYPE)(x | 32768)
#define is_X(x)				(AttTYPE)(x & 32768)
#define set_X_to_0(x)		(AttTYPE)(x & 0x7fff)
*/


#define get_text_att(x)			(AttTYPE)(x & 0xfffc)
#define set_text_att_to_0(x)	(AttTYPE)(x & 0x03)
// text atts end; AttTYPE type


// char type begin:
#define set_all_line_to_1(x)	(char)(x | 1)
#define is_all_line(x)			(char)(x & 1)
#define set_all_line_to_0(x)	(char)(x & 0xfe)

#define set_match_word_to_1(x)	(char)(x | 2)
#define is_match_word(x)		(char)(x & 2)
#define set_match_word_to_0(x)	(char)(x & 0xfd)

#define set_case_word_to_1(x)	(char)(x | 4)
#define is_case_word(x)			(char)(x & 4)
#define set_case_word_to_0(x)	(char)(x & 0xfb)

#define set_wild_card_to_1(x)	(char)(x | 8)
#define is_wild_card(x)			(char)(x & 8)
#define set_wild_card_to_0(x)	(char)(x & 0xf7)
// char type end:


#define COLORWORDLIST struct cwords
COLORWORDLIST {
	char color;
	COLORWORDLIST *nextCW;
} ;

#define NUMCOLORPOINTS 5
#define COLORTEXTLIST struct ctext
COLORTEXTLIST {
	char cWordFlag;
	unCH *keyWord;
	unCH *fWord;
	int len;
	int red;
	int green;
	int blue;
	int index;
	long sCol[NUMCOLORPOINTS], eCol[NUMCOLORPOINTS];
	COLORTEXTLIST *nextCT;
} ;

typedef struct _curses_win_ {
    int  num_rows, num_cols;
    int  cur_row,  cur_col;
	int  winPixelSize;
    int  textOffset;
    short LT_row, LT_col;
    unsigned long *lineno;
    FONTINFO **RowFInfo;
    char reverse;
	char isUTF;
    unCH **win_data;
    AttTYPE **win_atts;
    struct _curses_win_ *NextWindow;
} WINDOW;

extern "C"
{

extern NewFontInfo cedDFnt;

extern WINDOW *newwin(int num_rows, int num_cols, int LT_row, int LT_col, int tOff);
extern COLORTEXTLIST *FindColorKeywordsBounds(COLORTEXTLIST *lRootColorText, AttTYPE *sAtts,unCH *sData,int lnoff,int ecol,COLORTEXTLIST *cl);
extern COLORTEXTLIST *createColorTextKeywordsList(COLORTEXTLIST *lRootColorText, char *st);
extern void copyFontInfo(FONTINFO *des, FONTINFO *src, char isUse);
extern void copyNewToFontInfo(FONTINFO *des, NewFontInfo *src);
extern void copyNewFontInfo(NewFontInfo *, NewFontInfo *);
extern void createColorWordsList(char *st);
extern void FreeColorWord(COLORWORDLIST *RootColorWord);
extern void FreeColorText(COLORTEXTLIST *RootColorText);
extern void SetRdWKeyScript(void);
extern void SetDefFontKeyScript(void);
extern void ResetSelectFlag(char c);
extern void GetSoundWinDim(int *row, int *col);
extern void wdrawdot(WINDOW *w, int hp1, int lp1, int hp2, int lp2, int col);
extern void wdrawcontr(WINDOW *w, char on);
extern void MoveSoundWave(WINDOW *w, int dir, int left_lim, int right_lim);
extern void clear(void);
extern void endwin(void);
extern void delwin(WINDOW *w);
extern void delOneWin(WINDOW *w);
extern void touchwin(WINDOW *w);
extern void sp_touchwin(WINDOW *w);
extern void wrefresh(WINDOW *w);
extern void wmove(WINDOW *w, long row_win, long col_win);
extern void wsetlineno(WINDOW *w, int row, unsigned long lineno);
extern void mvwaddstr(WINDOW *w, int row, int col, unCH *s);
extern void waddstr(WINDOW *w, unCH *s);
extern void waddch(WINDOW *w, unCH c, AttTYPE *att, long col_chr, AttTYPE att1);
extern void mvwprintw(WINDOW *w,long row_win,long col_win,char *format,unCH *s);
extern void wstandout(WINDOW *w);
extern void wstandend(WINDOW *w);
extern void werase(WINDOW *w);
extern void wUntouchWin(WINDOW *w, int num);
extern void wclrtoeol(WINDOW *w, char isForce);
extern void wclrtobot(WINDOW *w);
extern void blinkCursorLine(void);
extern void DrawCursor(char TurnOn);
extern void DrawFakeHilight(char TurnOn);
extern void DrawSoundCursor(char TurnOn);
extern int  wgetch(WINDOW *w);
extern int  SpecialWindowOffset(short id);
extern int  SelectWordColor(void);
extern short ComputeHeight(WINDOW *w, int st, int en);
extern short FontTxtWidth(WINDOW *w, int row, unCH *s, int st, int en);
extern short TextWidthInPix(unCH *st, long beg, long len, FONTINFO *fnt, int textOffset);
#if (TARGET_API_MAC_CARBON == 1)
extern Boolean DrawUTFontMac(unCH *st, long len, FONTINFO *font, AttTYPE style);
#endif
}
#endif /* _C_CURSES_ */
