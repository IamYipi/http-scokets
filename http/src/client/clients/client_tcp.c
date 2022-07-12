/*
Fichero:client_tcp.c
Javier García Pechero DNI 70906279Q
Álvaro García Labrador DNI 70913088V
*/
#include "../client_commands.h"
#include "client_tcp.h"
int depurar(char *a);

int recvTCP(int s, char *response, int size){
	int i, j;

	i = recv(s, response, size, 0);
	if (i == -1) 
		return -1;

	while (i < TAMANO) {
		j = recv(s, &response[i], size-i, 0);
		if (j == -1)
			return -1;
		i += j;
	}
	
	return 0;
}


int clienttcp(char** argv)
{
	int s;				/* connected socket descriptor */
	struct addrinfo hints, *res;
	long timevar;			/* contains time returned by time() */
	struct sockaddr_in myaddr_in;	/* for local socket address */
	struct sockaddr_in servaddr_in;	/* for server socket address */
	int addrlen, i, j, errcode;
	
	char command[TAMANO];	/* This example uses TAMANO byte messages. */
	char response[TAMANO];   
	FILE *commandsFile;		/* File that contains client http commands to be executed */

	FILE *clientLog;
	char clientLogName[50];
	char ordersPath[TAMANO];
	
	/* Create the socket. */
	s = socket (AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	/* Set up the peer address to which we will connect. */
	servaddr_in.sin_family = AF_INET;
	
	/* Get the host information for the hostname that the
	 * user passed in. */
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	
	
 	 /* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
	errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
	if (errcode != 0){
			/* Name was not found.  Return a
			 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				argv[0], argv[1]);
		exit(1);
	}
	else {
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}
	
	freeaddrinfo(res);

    /* PORT del servidor en orden de red*/
	servaddr_in.sin_port = htons(PORT);

		/* Try to connect to the remote server at the address
		 * which was just built into peeraddr.
		 */
	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
		exit(1);
	}
		/* Since the connect call assigns a free address
		 * to the local end of this connection, let's use
		 * getsockname to see what it assigned.  Note that
		 * addrlen needs to be passed in as a pointer,
		 * because getsockname returns the actual length
		 * of the address.
		 */
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	}

	/* Print out a startup message for the user. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */

	sprintf(clientLogName, "../bin/logs/%u.txt", ntohs(myaddr_in.sin_port)); 
	if(NULL == (clientLog = fopen(clientLogName, "a+"))){
		perror("Error al iniciar el fichero .txt del cliente");
		exit(1);
	}
	
	fprintf(clientLog,"[TCP] Connected to %s on port %u at %s\n",
			argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));
	fclose(clientLog);
//---------
	sprintf(ordersPath, "../orders/%s", argv[3]);		
	commandsFile = fopen(ordersPath, "r");
	if(commandsFile == NULL){
		fprintf(stderr, "[TCP] Cannot read http commands file\n");
		exit(1);
	}
	//AQUI CADA VEZ QUE ENTRA LEE UNA LINEA
	RESET(command, TAMANO);
	while( fgets(command, sizeof(command), commandsFile) != NULL){	
		command[strlen(command)-2] = '\0';		

		if(command[0] == '\0') 
			continue;	
		addCRLF(command, TAMANO);
		//Envia comando al server
		if (send(s, command, TAMANO, 0) != TAMANO) {
			fprintf(stderr, "%s: Connection aborted on error ", argv[0]);
			fprintf(stderr, "on send number %d\n", i);
			exit(1);
		}
		if(removeCRLF(command)){
			fprintf(stderr, "[TCP] Command without CR-LF. Aborted \"connection\" \n");
			exit(1);
		}
		if(NULL == (clientLog = fopen(clientLogName, "a+"))){
			perror("Error al iniciar el fichero .txt del cliente");
			exit(1);
		}
		fprintf(clientLog, "C: %s\n", command);
		fclose(clientLog);
		if(NULL == (clientLog = fopen(clientLogName, "a+"))){
			perror("Error al iniciar el fichero .txt del cliente");
			exit(1);
		}
		//Comienzan las llegadas
		RESET(response, TAMANO);
		//Received response 1
		if(-1 == recvTCP(s, response, TAMANO)){
			perror(argv[0]);
			fprintf(stderr, "[TCP] %s: error reading result 1\n", argv[0]);
			exit(1);
		}		
		if(removeCRLF(response)){
			fprintf(stderr, "[TCP] Response without CR-LF. Aborted connection\n");
			exit(1);
		}		
		fprintf(clientLog, "S: %s\n", response);
		fclose(clientLog);
		if(NULL == (clientLog = fopen(clientLogName, "a+"))){
			perror("Error al iniciar el fichero .txt del cliente");
			exit(1);
		}
		RESET(response, TAMANO);
		//Received response 2
		if(-1 == recvTCP(s, response, TAMANO)){
			perror(argv[0]);
			fprintf(stderr, "[TCP] %s: error reading result 2\n", argv[0]);
			exit(1);
		}		
		if(removeCRLF(response)){
			fprintf(stderr, "[TCP] Response without CR-LF. Aborted connection\n");
			exit(1);
		}		
		fprintf(clientLog, "S: %s\n", response);
		fclose(clientLog);
		if(NULL == (clientLog = fopen(clientLogName, "a+"))){
			perror("Error al iniciar el fichero .txt del cliente");
			exit(1);
		}
		RESET(response, TAMANO);
		//Received response 3
		if(-1 == recvTCP(s, response, TAMANO)){
			perror(argv[0]);
			fprintf(stderr, "[TCP] %s: error reading result 3\n", argv[0]);
			exit(1);
		}		
		if(removeCRLF(response)){
			fprintf(stderr, "[TCP] Response without CR-LF. Aborted connection\n");
			exit(1);
		}		
		fprintf(clientLog, "S: %s\n", response);
		fclose(clientLog);
		if(NULL == (clientLog = fopen(clientLogName, "a+"))){
			perror("Error al iniciar el fichero .txt del cliente");
			exit(1);
		}
		RESET(response, TAMANO);
		//Received response 4
		if(-1 == recvTCP(s, response, TAMANO)){
			perror(argv[0]);
			fprintf(stderr, "[TCP] %s: error reading result 4\n", argv[0]);
			exit(1);
		}		
		fprintf(clientLog, "S: %s\n", response);
		fclose(clientLog);
		if(NULL == (clientLog = fopen(clientLogName, "a+"))){
			perror("Error al iniciar el fichero .txt del cliente");
			exit(1);
		}
		RESET(response, TAMANO);
		//Received response 5
		if(-1 == recvTCP(s, response, TAMANO)){
			perror(argv[0]);
			fprintf(stderr, "[TCP] %s: error reading result 5\n", argv[0]);
			exit(1);
		}		
		if(removeCRLF(response)){
			fprintf(stderr, "[TCP] Response without CR-LF. Aborted connection\n");
			exit(1);
		}		
		fprintf(clientLog, "S: %s\n", response);
		fclose(clientLog);
		if(NULL == (clientLog = fopen(clientLogName, "a+"))){
			perror("Error al iniciar el fichero .txt del cliente");
			exit(1);
		}
		RESET(response, TAMANO);
		//Received response 6
		if(-1 == recvTCP(s, response, TAMANO)){
			perror(argv[0]);
			fprintf(stderr, "[TCP] %s: error reading result 6\n", argv[0]);
			exit(1);
		}		
		fprintf(clientLog, "S: %s\n", response);
		fclose(clientLog);
		RESET(response, TAMANO);
	}

	fclose(commandsFile);


//---------
	/* Now, shutdown the connection for further sends.
	 * This will cause the server to receive an end-of-file
	 * condition after it has received all the requests that
	 * have just been sent, indicating that we will not be
	 * sending any further requests.
	 */
	if (shutdown(s, 1) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
		exit(1);
	}


	if(NULL == (clientLog = fopen(clientLogName, "a+"))){
			perror("Error al iniciar el fichero .txt del cliente");
			exit(1);
		}
    /* Print message indicating completion of task. */
	time(&timevar);
	//printf("\n[TCP] All done at %s", (char *)ctime(&timevar));
	fprintf(clientLog, "\n[TCP] All done at %s", (char *)ctime(&timevar));
	fclose(clientLog);
}

int depurar(char *a){
	FILE *logDepurar;
	if(NULL == (logDepurar = fopen("../bin/logs/depurar.log", "a+"))){
		return -1;
	}
	fprintf(logDepurar,"%s\n",a);
	fclose(logDepurar);
	return 0;
}

