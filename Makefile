.PHONY: build clean mkdir
CC= gcc
CFLAG= -c
OFLAG= -o
SDIR= src
DDIR= bin
RL= -lreadline


build: main exec vector utils mkdir
	@ $(CC) $(OFLAG) shell *.o $(RL) &&  mv *.o shell ./$(DDIR)

clean:
	@ rm -rf $(DDIR)

main: $(SDIR)/main.c $(SDIR)/vector.h $(SDIR)/utils.h
	@ $(CC) $(CFLAG) $(SDIR)/main.c

exec: $(SDIR)/exec.c $(SDIR)/exec.h
	@ $(CC) $(CFLAG) $(SDIR)/exec.c

vector: $(SDIR)/vector.c $(SDIR)/vector.h
	@ $(CC) $(CFLAG) $(SDIR)/vector.c

utils: $(SDIR)/utils.c $(SDIR)/utils.h
	@ $(CC) $(CFLAG) $(SDIR)/utils.c

mkdir:
	@ mkdir -p $(DDIR)
