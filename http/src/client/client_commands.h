/*
Fichero:client_commands.h
Javier García Pechero DNI 70906279Q
Álvaro García Labrador DNI 70913088V
*/
#ifndef __CLIENT_COMMANDS_H__
#define __CLIENT_COMMANDS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"

#define RESET(s, size) (memset(s, '\0', size))

//Other functions
int numLines(FILE *file);
void addCRLF(char *s, int size);
int removeCRLF(char *s);

#endif
