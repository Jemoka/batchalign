/**********************************************************************
	"Copyright 1990-2022 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#ifndef _INC_WSTRING
#define _INC_WSTRING

extern "C"
{

	#define is_unCH_alnum iswalnum
	#define is_unCH_alpha iswalpha
	#define is_unCH_digit iswdigit
	#define is_unCH_space iswspace
	#define is_unCH_lower iswlower
	#define is_unCH_upper iswupper
	#define to_unCH_lower towlower
	#define to_unCH_upper towupper

	#define unCH	wchar_t

	extern unCH *cl_T(const char *);

#ifndef _MAX_FNAME
	#define _MAX_FNAME	256
#endif
#ifndef _MAX_PATH
	#define _MAX_PATH	2048
#endif

	#define CLType char

	#define FNSize _MAX_PATH+FILENAME_MAX

	#define FNType char

}

#endif  /* _INC_STRING */
