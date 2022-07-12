/*
Fichero:params.h
Javier García Pechero DNI 70906279Q
Álvaro García Labrador DNI 70913088V
*/
#ifndef __PARAMS_H__
#define __PARAMS_H__

#define PORT 		6279   		// > Servers listening port
#define ADDRNOTFOUND 	0xffffffff	// > Addres for unfound host
#define TAMANO	2000		// > Max size for packets received

//Server and clientUDP
#define RETRIES	5			// > Number of times to retry before givin up
#define TIMEOUT 6			// > Max time for getting a response

//Carry Return and Line feed (CR and LF)
#define CR '\r'
#define LF '\n'

#define WRONG_COMMAND -1	

#endif
