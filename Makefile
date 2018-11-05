# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -DANSI -D_DEBUG -DMAIN -ansi

SRCS =  ctrans.c  heap.c  polygn.c  savevf.c  viewobs.c  viewunob.c \
	getdat.c  misc.c  readvf.c  readvs.c  test3d.c view3d.c viewpp.c \
	common.c \
	view2d.c test2d.c v3lib.c

OBJS = $(SRCS:.c=.o)

LFLAGS = -lview3d -lm
ifeq ($(OS),Windows_NT)
LFLAGS += -lmingw32
endif

INCLUDES = -I.

ifeq ($(OS),Windows_NT)
LIBNAME = libview3d.dll.a
else
LIBNAME = libview3d.a
endif

all: view2d.exe view3d.exe viewht.exe

lib: $(LIBNAME)

$(LIBNAME): $(OBJS) config.h
ifeq ($(OS),Windows_NT)
	gcc -o msys-view3d.dll -Wl,-no-undefined -g -shared \
		-Wl,--out-implib=$@ -Wl,--export-all-symbols \
		-Wl,--enable-auto-import -Wl,--whole-archive $(OBJS) \
		-Wl,--no-whole-archive -lm
else
	ar rcs $@ $<
endif

config.h:
	echo "#ifndef V3D_CONFIG_H" > config.h
	echo "#define V3D_CONFIG_H" >> config.h
	echo "" >> config.h
	echo "#define V3D_VERSION \"3.5\"" >> config.h
	echo "" >> config.h
	echo "#endif" >> config.h

view2d.exe: $(LIBNAME) v2main.o config.h
	$(CC) $(CFLAGS) $(INCLUDES) -L. -o $@ v2main.o $(LFLAGS) $(LIBS)

view3d.exe: $(LIBNAME) v3main.o config.h
	$(CC) $(CFLAGS) $(INCLUDES) -L. -o $@ v3main.o $(LFLAGS) $(LIBS)

viewht.exe: $(LIBNAME) viewht.o config.h
	$(CC) $(CFLAGS) $(INCLUDES) -L. -o $@ viewht.o $(LFLAGS) $(LIBS)

%.o: %.c config.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) view2d.exe view3d.exe viewht.exe $(LIBNAME) $(OBJS) v3main.o v2main.o viewht.o msys-view3d.dll config.h

.PHONY = all clean lib
