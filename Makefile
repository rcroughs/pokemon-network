CC=gcc
OPT=-Wall -Wextra -std=gnu11 -O2
LIB_SERV=-L ./img-dist/ -limg-dist -lm -lpthread
LIB_CLIENT= -lpthread

# Code du serveur (img-search)
DIR_SERV=serveur

# Code du client (pokedex-client)
DIR_CLIENT=client

# Code commun au client & serveur (optionnel)
DIR_COMMON=commun

OBJS_SERV= serveur/filelist.o serveur/imageio.o serveur/server.o serveur/thread.o
OBJS_CLIENT= client/thread.o client/network.o

all: img-search pokedex-client

libimg-dist.a:
	(cd img-dist ; make)

img-search: libimg-dist.a $(DIR_SERV)/main.c $(OBJS_SERV)
	$(CC) $(OPT) $(DIR_SERV)/main.c -o img-search $(OBJS_SERV) $(LIB_SERV)

pokedex-client: $(DIR_CLIENT)/main.c $(OBJS_CLIENT)
	$(CC) $(OPT) $(DIR_CLIENT)/main.c -o pokedex-client $(OBJS_CLIENT) $(LIB_CLIENT)

%.o: %.c
	$(CC) $(OPT) -g -c $< -o $@

.PHONY: clean

clean:
	rm -f serveur/*.o
	rm -f client/*.o
	rm -f img-search
	rm -f pokedex-client
