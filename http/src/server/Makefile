#
#Fichero:MakeFile
#Javier García Pechero DNI 70906279Q
#Álvaro García Labrador DNI 70913088V
#
CC=gcc
CFLAGS=-g -Wunused

.PHONY: server
server: server.c server.h params.h server_commands.c server_commands.h
	$(CC) $(CFLAGS) $^ -o ../../bin/server -lm
	
