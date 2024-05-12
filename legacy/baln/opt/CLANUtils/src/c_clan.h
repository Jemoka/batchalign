/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/

#ifndef C_CLAN
#define C_CLAN

#define ALIAS_FILE_D	"0aliases.cut"
#define ALIAS_FILE_U	"aliases.cut"
#define	MAX_ARGS		500L
#define EXPANSION_SIZE	5000L /* This number must be larger than 1024 */

#define isOrgUnicodeFont(x) (x == WINArialUC || x == MACArialUC)

extern "C"
{

extern FNType	wd_dir[],   // working directory
				od_dir[];   // output directory

extern short lastCommand;

extern char *fbuffer;
extern FNType *StdInWindow;
extern const char *StdInErrMessage, *StdDoneMessage;

extern int  get_clan_prog_num(char *s, char isLoad);
extern int  rename_each_file(FNType *s, FNType *t, char isChangeCase);
extern int  GetNumberOfCommands(void);
extern int  getCommandNumber(char *cmd);

extern char init_clan(void);
extern char *NextArg(char *s);
extern char MakeArgs(char *com);
extern char selectChoosenFont(NewFontInfo *finfo, char isForce, char isKeepSize);

extern void cutt_exit(int i);
extern void att_cp(long pos, char *desSt, const char *srcSt, AttTYPE *desAtt, AttTYPE *srcAtt);
extern void att_shiftright(char *srcSt, AttTYPE *srcAtt, long num);
extern void execute(char *inputBuf, char tDataChanged);
extern void func_init(void);
extern void init_commands(void);
extern void free_commands(void);
extern void set_commands(char *text);
extern void set_lastCommand(short num);
extern void write_commands1977(FILE *fp);
extern void OpenHelpWindow(const FNType *fname);
extern void OpenWebWindow(void);
extern void OpenCharsWindow(void);
extern void show_info(char isJustComm);
extern void InitOptions(void);
extern void SysEventCheck(long);
extern void SetDefaultThaiFinfo(NewFontInfo *finfo);
extern void SetDefaultCAFinfo(NewFontInfo *finfo);
extern void re_readAliases(void);
extern void readAliases(char isInit);
extern void free_aliases(void);
extern char getAliasProgName(char *inBuf, char *progName, int size);
extern char SetDefaultUnicodeFinfo(NewFontInfo *finfo);
}

#endif // C_CLAN
