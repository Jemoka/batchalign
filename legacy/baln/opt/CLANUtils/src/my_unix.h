/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#ifndef _MY_UNIX
#define _MY_UNIX


extern "C"
{
	typedef	unsigned short mode_t;

	extern int my_chdir(const FNType *path);
	extern int my_mkdir(const FNType *path, mode_t mode);
	extern int my_access(const FNType *path, int mode);
	extern int my_unlink(const FNType *path);
	extern int my_rename(const FNType *f1, const FNType *f2);
	extern char *my_getcwd(FNType *path, size_t len);
	extern FILE *my_fopen(const FNType *name, const char *mode);

}

#endif // _MY_UNIX
