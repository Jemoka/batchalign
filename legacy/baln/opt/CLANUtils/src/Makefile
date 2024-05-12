# @@@IMPORTANT IMPORTANT@@@
# This is the *WINDOWS* makefile
# Windows MinGW Make doesn't take an argument for what the makefile is
# So we just use this as the Makefile.
# CLAN has to be 32bit app. Please use -m32 option when compiling CLAN.
#
#
# Other operating system specific makefiles are named Makefile.$ARCH


















# @@@@@ READ THE NOTE ABOVE @@@@@ #

DD  = ../unix/bin
OBJ = ../unix/obj

# for Apple Unix AND FreeBSD >= 3.2:
CC = g++

# for Apple prior to MacOS 11 Big Sur, uncomment next line
#	CFLAGS = -O -DUNX -DAPPLEUNX -fwritable-strings -fshort-wchar -Wcomment -Wdeprecated-declarations -Wempty-body -arch i386
# for Apple MacOS 11 Big Sur and later, uncomment next line
#	CFLAGS = -O -DUNX -DAPPLEUNX -fwritable-strings -fshort-wchar -Wcomment -Wdeprecated-declarations -Wempty-body
# Windows DOS MinGW-w64:
#   install "tdm64-gcc-10.3.0-2.exe" from "jmeubank.github.io/tdm-gcc/" and uncomment next line
CFLAGS = -O -D_DOS -fshort-wchar -Wcomment -Wdeprecated-declarations -Wempty-body -Wno-register

# Debian 'sid' distribution
#	CC = g++
# for Linux:
#	CC = g++
# for Linux g++ 4.4.1-4ubuntu9 compiler:
#	CC = g++
# Other machines:
#	CC = gcc
LIB = -lcurses -ltermcap

# for SUN SPARC station:
#	CFLAGS = -O2 -DUNX -fwritable-strings
# for SunOS sparc SUNW, Ultra-5_10
#	CFLAGS = -O2 -DUNX -DERRNO -fwritable-strings
# Linux on Intel and most other machines:
# 	CFLAGS = -O -DUNX -fwritable-strings -Wall
# for Linux g++ 4.4.1-4ubuntu9 compiler:
#	CFLAGS = -DUNX -Wno-deprecated -Wno-deprecated-declarations
# if previous doesn't work try:
#	CFLAGS = -DUNX -fwritable-strings
# OR
#	CFLAGS = -O -DUNX -DERRNO -fwritable-strings
#	CFLAGS = -DUNX -DERRNO -fwritable-strings

all:	chat2elan chat2praat chstring elan2chat fixbullets flo kwal lowcase praat2chat retrace

chat2elan:		$(DD)/chat2elan
chat2praat:		$(DD)/chat2praat
chstring:		$(DD)/chstring
elan2chat:		$(DD)/elan2chat
fixbullets:		$(DD)/fixbullets
flo:			$(DD)/flo
kwal:			$(DD)/kwal
lowcase:		$(DD)/lowcase
praat2chat:		$(DD)/praat2chat
retrace:		$(DD)/retrace


$(OBJ)/cut.o: cutt.cpp cu.h common.h mul.h c_curses.h
	$(CC) -c $(CFLAGS) -o $(OBJ)/cut.o cutt.cpp

$(OBJ)/stp.o: stringparser.cpp stringparser.h fontconvert.h
	$(CC) -c $(CFLAGS) -o $(OBJ)/stp.o stringparser.cpp

$(OBJ)/fntc.o: fontconvert.cpp fontconvert.h
	$(CC) -c $(CFLAGS) -o $(OBJ)/fntc.o fontconvert.cpp

$(OBJ)/cxml.o: cutt-xml.cpp cu.h common.h mul.h
	$(CC) -c $(CFLAGS) -o $(OBJ)/cxml.o cutt-xml.cpp


$(DD)/chat2elan: Chat2Elan.cpp cu.h common.h $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o
	$(CC) $(CFLAGS) Chat2Elan.cpp -o $(DD)/chat2elan $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o

$(DD)/chat2praat: Chat2Praat.cpp cu.h common.h $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o
	$(CC) $(CFLAGS) Chat2Praat.cpp -o $(DD)/chat2praat $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o

$(DD)/chstring: chstring.cpp cu.h common.h $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o
	$(CC) $(CFLAGS) chstring.cpp -o $(DD)/chstring $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o

$(DD)/elan2chat: Elan2Chat.cpp cu.h common.h $(OBJ)/cut.o $(OBJ)/cxml.o $(OBJ)/stp.o $(OBJ)/fntc.o
	$(CC) $(CFLAGS) Elan2Chat.cpp -o $(DD)/elan2chat $(OBJ)/cut.o $(OBJ)/cxml.o $(OBJ)/stp.o $(OBJ)/fntc.o

$(DD)/fixbullets: FixBullets.cpp cu.h common.h $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o
	$(CC) $(CFLAGS) FixBullets.cpp -o $(DD)/fixbullets $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o

$(DD)/flo: flo.cpp cu.h common.h $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o
	$(CC) $(CFLAGS) flo.cpp -o $(DD)/flo $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o

$(DD)/kwal: kwal.cpp cu.h common.h $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o
	$(CC) $(CFLAGS) kwal.cpp -o $(DD)/kwal $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o

$(DD)/lowcase: lowcase.cpp cu.h common.h $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o
	$(CC) $(CFLAGS) lowcase.cpp -o $(DD)/lowcase  $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o

$(DD)/praat2chat: Praat2Chat.cpp cu.h common.h $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o
	$(CC) $(CFLAGS) Praat2Chat.cpp -o $(DD)/praat2chat $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o

$(DD)/retrace: retrace.cpp cu.h common.h $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o
	$(CC) $(CFLAGS) retrace.cpp -o $(DD)/retrace $(OBJ)/cut.o $(OBJ)/stp.o $(OBJ)/fntc.o

clean:
	-rm -f $(DD)/* $(OBJ)/*

.PHONY: all clean \
	chat2elan \
	chat2praat \
	chstring \
	elan2chat \
	fixbullets \
	flo \
	kwal \
	lowcase \
	praat2chat \
	retrace \
