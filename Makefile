# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -DANSI -D_DEBUG

# the build MAIN executable:
MAIN = view2d.exe

SRCS =  ctrans.c  heap.c  polygn.c  savevf.c  viewobs.c  viewunob.c \
	getdat.c  misc.c  readvf.c  readvs.c  test3d.c view3d.c viewpp.c \
	common.c \
	view2d.c test2d.c

OBJS = $(SRCS:.c=.o)

LFLAGS = -lmingw32 -lview3d -lm
INCLUDES = -I.

LIBNAME = libview3d.dll.a

all: view2d.exe view3d.exe

lib: $(LIBNAME)

$(LIBNAME): $(OBJS)
	# ar rcs $@ $^
	gcc -o msys-view3d.dll -Wl,-no-undefined -g -shared \
		-Wl,--out-implib=$@ -Wl,--export-all-symbols \
		-Wl,--enable-auto-import -Wl,--whole-archive $(OBJS) \
		-Wl,--no-whole-archive -lm


view2d.exe: $(LIBNAME) v2main.o
	$(CC) $(CFLAGS) $(INCLUDES) -L. -o $@ v2main.o $(LFLAGS) $(LIBS)

view3d.exe: $(LIBNAME) v3main.o
	$(CC) $(CFLAGS) $(INCLUDES) -L. -o $@ v3main.o $(LFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) view2d.exe view3d.exe $(LIBNAME) $(OBJS) v3main.o v2main.o

.PHONY = all clean lib
