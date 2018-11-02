# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall

# the build MAIN executable:
MAIN = view2d.exe

SRCS =  ctrans.c  heap.c  polygn.c  savevf.c  viewobs.c  viewunob.c \
	getdat.c  misc.c  readvf.c  readvs.c  test3d.c view3d.c viewpp.c \
	common.c \
	view2d.c test2d.c v2main.c

OBJS = $(SRCS:.c=.o)

LFLAGS = -lm -lview3d
INCLUDES = -I.

all: view2d.exe view3d.exe

libview3d.a: $(OBJS)
	ar rcs $@ $<


view2d.exe: libview3d.a
	$(CC) $(CFLAGS) $(INCLUDES) -L. -o $@ v2main.o $(LFLAGS) $(LIBS)

view3d.exe: libview3d.a
	$(CC) $(CFLAGS) $(INCLUDES) -L. -o $@ v3main.o $(LFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) view2d.exe view3d.exe libview3d.a $(OBJS)

.PHONY = all clean
