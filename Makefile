#!/usr/bin/make

PRGM   = fatshell
SRCS   = shell.c fat_fs.c
LIBS   = 
CFLAGS = -std=c99 -g -Wall

#note to future self: do not modify below this line :)

CC     = gcc
ODIR   = bin
OBJS   = $(SRCS:%.c=$(ODIR)/%.o)
LFLAGS = $(LIBS:%=-l%)

$(PRGM): $(OBJS) $(ODIR)
	$(CC) -o $(PRGM) $(OBJS) $(LFLAGS) $(CFLAGS)

$(ODIR)/%.o: %.c $(ODIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR):
	mkdir $(ODIR)

all: clean $(PRGM)

clean:
	rm -rf $(PRGM) $(OBJS) $(ODIR)

release: $(PRGM)
	rm -rf $(OBJS) $(ODIR)
