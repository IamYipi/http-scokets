/*
Fichero:server.h
Javier García Pechero DNI 70906279Q
Álvaro García Labrador DNI 70913088V
*/
#ifndef __SERVER_H__
#define __SERVER_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <stdbool.h>

#include "params.h"
#include "server_commands.h"

#define RESET(s, size) (memset(s, '\0', size))
#define MAXHOST 	128

extern int errno;

int addNewConexionToLog(struct sockaddr_in servaddr_in, struct sockaddr_in clientaddr_in, char *protocol);
int addCommandToLog(char *command, bool isResponse);

void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, struct sockaddr_in clientaddr_in);

void errout(char *);		/* declare error out routine */

#endif
