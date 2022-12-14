/*
Fichero:client_udp.c
Javier García Pechero 
Álvaro García Labrador 
*/
#include "../client_commands.h"
#include "client_udp.h"


void handler()
{
	printf("Alarma recibida \n");
}



int recvUDP(int s, char *response, int size, struct sockaddr_in *servaddr_in, int *addrlen){
	int n_retry;

	n_retry = RETRIES;
	while(n_retry > 0){
		alarm(TIMEOUT);
		if (recvfrom (s, response, size, 0, (struct sockaddr *) servaddr_in, addrlen) == -1) {
			if (errno == EINTR) {
	     			fprintf(stderr,"Alarm went off.\n");
	     			n_retry--; 
			} else  {
				fprintf(stderr,"Unable to get response to \"connect\"\n");
				return -1;
			}
		}else{
			alarm(0);
			break;
		}
	}
	if(n_retry == 0) return -1;
		
	return 0;
}



int clientudp(char **argv)
{
	int i, errcode;
	int s;				// socket descriptor 
	long timevar;                   // contains time returned by time() 
	struct sockaddr_in myaddr_in;	// for local socket address 
	struct sockaddr_in servaddr_in;	// for server socket address 
	struct sockaddr_in servaddr_in_copia;
	int	addrlen;
	struct sigaction vec;
	struct addrinfo hints, *res;
	
	char command[TAMANO];	//This example uses TAMANO byte messages. 
	char response[TAMANO];  
	char tmp[TAMANO];
	FILE *commandsFile;		//File that contains client NNTP commands to be executed 

	FILE *clientLog;
	char clientLogName[TAMANO];
	char ordersPath[TAMANO];

	bool finishedFalseConex = false;


	//Create the socket. 
	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	
	 //clear out address structures 
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in_copia, 0, sizeof(struct sockaddr_in));
	
	/*
	 Bind socket to some local address so that the
	  server can send the reply back.  A port number
	 of zero will be used so that the system will
	  assign any available port number.  An address
	  of INADDR_ANY will be used so we do not have to
	  look up the internet address of the local host.
	*/
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_port = 0;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
		exit(1);
	}
	
	
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	}

	//Print out a startup message for the user. 
	time(&timevar);

	/* The port number must be converted first to host byte
	 order before printing.  On most hosts, this is not
	 necessary, but the ntohs() call is included here so
	that this program could easily be ported to a host
	that does require it.*/
	
    	//printf("[UDP] \"False connected\" to %s on port %u at %s", 
    	//	argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

	 //Set up the server address. 
	servaddr_in_copia.sin_family = AF_INET;
	
	
	/* Get the host information for the server's hostname that the
	 user passed in.*/
	 
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	
	
	//esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta
	errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
	if (errcode != 0){
		 /*Name was not found.  Return a
		 special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				argv[0], argv[1]);
		exit(1);
	}
	else {
		//Copy address of host 
		servaddr_in_copia.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}

	freeaddrinfo(res);
	//PORT del servidor en orden de red
	servaddr_in_copia.sin_port = htons(PORT);

   	//Registrar SIGALRM para no quedar bloqueados en los recvfrom
	vec.sa_handler = (void *) handler;
	vec.sa_flags = 0;
	if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
		perror(" sigaction(SIGALRM)");
		fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
		exit(1);
	}
	
	//
	
	

//---------
	sprintf(ordersPath, "../orders/%s", argv[3]);		
	commandsFile = fopen(ordersPath, "r");
	if(commandsFile == NULL){
		fprintf(stderr, "[UDP] Cannot read http commands file\n");
		exit(1);
	}	
	//AQUI CADA VEZ QUE ENTRA LEE UNA LINEA
	RESET(command, TAMANO);
	while( fgets(command, sizeof(command), commandsFile) != NULL){
			//Send a "false connection" message to the UDP server listening socket (ls_UDP)
		if (sendto (s, " ", 1,0, (struct sockaddr *)&servaddr_in_copia, addrlen) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to send request to \"connect\" \n", argv[0]);
			exit(1);
		}	
		
		//Waits for the response of the server with the new socket it has to talk to
		if(-1 == recvUDP(s, tmp, 1, &servaddr_in, &addrlen)){
			exit(1);
		}	
		

		sprintf(clientLogName, "../bin/logs/%u.txt", ntohs(myaddr_in.sin_port)); 
		if(NULL == (clientLog = fopen(clientLogName, "a+"))){
			perror("Error al iniciar el fichero .txt del cliente");
			exit(1);
		}

		fprintf(clientLog,"[UDP] \"False connected\" to %s on port %u at %s\n",
				argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));
		
		command[strlen(command)-2] = '\0';

		if(command[0] == '\0') 
			continue;
			
		addCRLF(command, TAMANO);
		//Envia al server
		if (sendto(s, command, TAMANO, 0, (struct sockaddr *)&servaddr_in, addrlen) == -1) {
			fprintf(stderr, "%s: Connection aborted on error ", argv[0]);
			fprintf(stderr, "on send number %d\n", i);
			exit(1);
		}
		
		if(removeCRLF(command)){
			fprintf(stderr, "[UDP] Command without CR-LF. Aborted \"connection\" \n");
			exit(1);
		}
		
		fprintf(clientLog, "\nC: %s\n", command);
		fclose(clientLog);
		if(NULL == (clientLog = fopen(clientLogName, "a+"))){
			perror("Error al iniciar el fichero .txt del cliente");
			exit(1);
		}
		//Comienzan las llegadas
		RESET(response, TAMANO);
		//Received response 1
		if(-1 == recvUDP(s, response, TAMANO,&servaddr_in,&addrlen)){
			perror(argv[0]);
			fprintf(stderr, "[UDP] %s: error reading result 1\n", argv[0]);
			exit(1);
		}		
		if(removeCRLF(response)){
			fprintf(stderr, "[UDP] Response without CR-LF. Aborted connection\n");
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
		if(-1 == recvUDP(s, response, TAMANO,&servaddr_in,&addrlen)){
			perror(argv[0]);
			fprintf(stderr, "[UDP] %s: error reading result 2\n", argv[0]);
			exit(1);
		}		
		//Change CRLF to '\0' to work with response as a string
		if(removeCRLF(response)){
			fprintf(stderr, "[UDP] Response without CR-LF. Aborted connection\n");
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
		if(-1 == recvUDP(s, response, TAMANO,&servaddr_in,&addrlen)){
			perror(argv[0]);
			fprintf(stderr, "[UDP] %s: error reading result 3\n", argv[0]);
			exit(1);
		}		
		if(removeCRLF(response)){
			fprintf(stderr, "[UDP] Response without CR-LF. Aborted connection\n");
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
		if(-1 == recvUDP(s, response, TAMANO,&servaddr_in,&addrlen)){
			perror(argv[0]);
			fprintf(stderr, "[UDP] %s: error reading result 4\n", argv[0]);
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
		if(-1 == recvUDP(s, response, TAMANO,&servaddr_in,&addrlen)){
			perror(argv[0]);
			fprintf(stderr, "[UDP] %s: error reading result 5\n", argv[0]);
			exit(1);
		}		

		if(removeCRLF(response)){
			fprintf(stderr, "[UDP] Response without CR-LF. Aborted connection\n");
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
		if(-1 == recvUDP(s, response, TAMANO,&servaddr_in,&addrlen)){
			perror(argv[0]);
			fprintf(stderr, "[UDP] %s: error reading result 6\n", argv[0]);
			exit(1);
		}		

		fprintf(clientLog, "S: %s\n", response);
		fclose(clientLog);
		RESET(command, TAMANO);
	}

	fclose(commandsFile);

//---------

 //Print message indicating completion of task.
 	if(NULL == (clientLog = fopen(clientLogName, "a+"))){
		perror("Error al iniciar el fichero .txt del cliente");
		exit(1);
	} 
	time(&timevar);
	fprintf(clientLog, "\n[UDP] All done at %s", (char *)ctime(&timevar));
	fclose(clientLog);
	
}


