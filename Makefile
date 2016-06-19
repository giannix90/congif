HDR = term.h mbf.h gif.h
SRC = ${HDR:.h=.c}
EHDR = default.h cs_vtg.h cs_437.h
ESRC = main.c
BIN=/bin

all: congif
	
congif: $(HDR) $(EHDR) $(SRC) $(ESRC)
	$(CC) $(CFLAGS) -o $@ $(SRC) $(ESRC) -lm

install:
	@echo \install..\N
	@sudo cp congif $(BIN) && echo Install OK

clean:
	$(RM) congif
