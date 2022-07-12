/*
Fichero:server_commands.h
Javier García Pechero DNI 70906279Q
Álvaro García Labrador DNI 70913088V
*/
#ifndef __SERVER_COMMANDS_H__
#define __SERVER_COMMANDS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>

#include "params.h"

//Other functions
int numLines(FILE *file);
void addCRLF(char *s, int size);
int removeCRLF(char *s);

#endif
