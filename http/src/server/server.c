/*
Fichero:server.c
Javier García Pechero 
Álvaro García Labrador
*/
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool END_LOOP = false;          
void endProgram(){ END_LOOP = true; }
int depurar(char *a);


void handler()
{
	printf("[SERV] Alarma recibida \n");
}


/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */
int main(int argc, char **argv)
{

	int s_TCP, s_UDP;		/* connected socket descriptor */
	int ls_TCP, ls_UDP;		/* listening socket descriptor */
	int br;		 		/* contains the number of bytes read */
     
	struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
    
	struct sockaddr_in myaddr_in;		/* for local socket address */
	struct sockaddr_in clientaddr_in;	/* for peer socket address */
	int addrlen;
	
	fd_set readmask;
	int numfds,s_bigger;

	char buffer[TAMANO];		/* buffer for packets to be read into */

	struct sigaction vec;

	
	/* Register SIGALARM */
	vec.sa_handler = (void *) handler;
	vec.sa_flags = 0;
	if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
		perror(" sigaction(SIGALRM)");
		fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
		exit(1);
	}	
	
	
	/* Create the listen TCP socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	
	/* Clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

    	addrlen = sizeof(struct sockaddr_in);

	/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
		/* The server should listen on the wildcard address,
		 * rather than its own internet address.  This is
		 * generally good practice for servers, because on
		 * systems which are connected to more than one
		 * network at once will be able to have one server
		 * listening on all networks at once.  Even when the
		 * host is connected to only one network, this is good
		 * practice, because it makes the server program more
		 * portable.
		 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PORT);


	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
	
	/* Initiate the listen on the socket so remote users
	 * can connect.  The listen backlog is set to 5, which
	 * is the largest currently supported.
	 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}
	
	
	/* Create the socket UDP. */
	ls_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (ls_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	}
	
	/* Bind the server's address to the socket. */
	if (bind(ls_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	}

	/* Now, all the initialization of the server is
	 * complete, and any user errors will have already
	 * been detected.  Now we can fork the daemon and
	 * return to the user.  We need to do a setpgrp
	 * so that the daemon will no longer be associated
	 * with the user's control terminal.  This is done
	 * before the fork, so that the child will not be
	 * a process group leader.  Otherwise, if the child
	 * were to open a terminal, it would become associated
	 * with that terminal as its control terminal.  It is
	 * always best for the parent to do the setpgrp.
	 */
	setpgrp();

	switch (fork()) {
		case -1:	/* Unable to fork, for some reason. */
			perror(argv[0]);
			fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
			exit(1);

		
		case 0:		/* The child process (daemon) comes here. */

			/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
			fclose(stdin);
			fclose(stderr);

			/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
			if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
		    		perror(" sigaction(SIGCHLD)");
		    		fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
		    		exit(1);
		    	}
		    
			/* Register SIGTERM to create an orderly completion  */
			vec.sa_handler = (void *) endProgram;
			vec.sa_flags = 0;
			if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
		    		perror(" sigaction(SIGTERM)");
	    			fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
		    		exit(1);
		    	}
		
			while (!END_LOOP) {
				/* Add both sockets to the mask */
				FD_ZERO(&readmask);
				FD_SET(ls_TCP, &readmask);
				FD_SET(ls_UDP, &readmask);
				
				/* Select the socket descriptor that has changed. It leaves a 
				   mark in the mask. */
				if (ls_TCP > ls_UDP) 	s_bigger=ls_TCP;
				else 		  	s_bigger=ls_UDP;

				if ( (numfds = select(s_bigger+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
					if (errno == EINTR) {
				    		END_LOOP=true;
				    		close(ls_TCP);
				    		close(ls_UDP);
				    		perror("\nFinalizando el servidor. Senial recibida en select\n"); 
					}
				}
				else { 
					/* Check if the selected socket is TCP */
					if (FD_ISSET(ls_TCP, &readmask)) {
						/* Note that addrlen is passed as a pointer
					     	 * so that the accept call can return the
					     	 * size of the returned address.
					     	 */
					     	 
						/* This call will block until a new
						 * connection arrives.  Then, it will
						 * return the address of the connecting
						 * peer, and a new socket descriptor, s,
						 * for that connection.
						 */
						s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
						if (s_TCP == -1) 
							exit(1);
						
						switch (fork()) {
							case -1:	/* Can't fork, just exit. */
								exit(1);
							
							case 0:		/* Child process comes here. */
								/* Close the listen socket inherited from the daemon. */
								close(ls_TCP);

								//Registers info of the new UDP "false connection"
								if(-1 == addNewConexionToLog(myaddr_in, clientaddr_in, "TCP")){
									perror("No se ha podido a�adir la connection a http.log");
								}	

								//Starts up the server
								serverTCP(s_TCP, clientaddr_in);
								exit(0);
							
							default:	/* Daemon process comes here. */
									/* The daemon needs to remember
									 * to close the new abrept socket
									 * after forking the child.  This
									 * prevents the daemon from running
									 * out of file descriptor space.  It
									 * also means that when the server
									 * closes the socket, that it will
									 * allow the socket to be destroyed
									 * since it will be the last close.
									 */
								close(s_TCP);
						}
						
					} /* End TCP*/
					
					
					
					/* Check if the selected socket is UDP */
					if (FD_ISSET(ls_UDP, &readmask)) {
						/* This call will block until a new
						* request arrives.  Then, it will create
						* a false "TCP" connection and working the same
						* as TCP works creating a new socket for that
						* false connection.
						*/
						br = recvfrom(ls_UDP, buffer, 1, 0, (struct sockaddr *)&clientaddr_in, &addrlen);
						if ( br == -1) {
						    perror(argv[0]);
						    printf("%s: recvfrom error (failed false connection UDP)\n", argv[0]);
						    exit(1);
						}
						
						/* When a new client sends a UDP datagram, his information is stored
						* in "clientaddr_in", so we can create a false connection by sending messages
						* manually with this information
						*/
						s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
						if (s_UDP == -1) {
							perror(argv[0]);
							printf("%s: unable to create new socket UDP for new client\n", argv[0]);
							exit(1);
						}

						/* Clear and set up address structure for new socket. 
						* Port 0 is specified to get any of the avaible ones, as well as the IP address.
						*/						
						memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
						myaddr_in.sin_family = AF_INET;
						myaddr_in.sin_addr.s_addr = INADDR_ANY;
						myaddr_in.sin_port = htons(0);
						
						/* Bind the server's address to the new socket for the client. */
						if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
							perror(argv[0]);
							printf("%s: unable to bind address new socket UDP for new client\n", argv[0]);
							exit(1);
						}
						
						/* As well as its done in TCP, a new thread is created for that false connection */
						switch (fork()) {
							case -1:	
								exit(1);
								
							case 0:		/* Child process comes here. */
								/* Child doesnt need the listening socket */
					    		close(ls_UDP); 
					    			
					    		/* Sends a message to the client for him to know the new port for 
								 * the false connection
					    		 */
					    		if (sendto(s_UDP, " ", 1, 0, (struct sockaddr *)&clientaddr_in, addrlen) == -1) {
									perror(argv[0]);
									fprintf(stderr, "%s: unable to send request to \"connect\" \n", argv[0]);
									exit(1);
								}

								//Registers info of the new UDP "false connection"
								if(-1 == addNewConexionToLog(myaddr_in, clientaddr_in, "UDP")){
									perror("No se ha podido a�adir la connection a http.log");
								}	
								
								//Starts up the server									
								serverUDP(s_UDP, clientaddr_in);
								exit(0);
							
							default:
								close(s_UDP);
						}
					
					} /* End UDP*/

				}

			}   /* End new clients loop */
			
			
			/* Close sockets before stopping the server */
			close(ls_TCP);
			close(ls_UDP);
		    
			printf("\nFin de programa servidor!\n");
		
		
		default:		/* Parent process comes here. */
			exit(0);
		}

	} //End switch	














/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	char hostname[MAXHOST];		/* remote host's name string */

	int len, len1, status;
	long timevar;			/* contains time returned by time() */
	struct linger linger;		/* allow a lingering, graceful close; */
				    	/* used when setting SO_LINGER */
			
	bool finish = false;	
	char command[TAMANO];
	

	
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	 
	status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in), hostname, MAXHOST,NULL,0,0);
	if(status){
		/* The information is unavailable for the remote
			 * host.  Just format its internet address to be
			 * printed out in the logging information.  The
			 * address will be shown in "internet dot format".
			 */
		 /* inet_ntop para interoperatividad con IPv6 */
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}
	
	/* Log a startup message. */
	time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	//printf("[SERV TCP] Startup from %s port %u at %s",
	//	hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

		/* Set the socket for a lingering, graceful close.
		 * This will cause a final close of this socket to wait until all of the
		 * data sent on it has been received by the remote host.
		 */
	linger.l_onoff  =1;
	linger.l_linger =1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) {
		errout(hostname);
	}

		/* Go into a loop, receiving requests from the remote
		 * client.  After the client has sent the last request,
		 * it will do a shutdown for sending, which will cause
		 * an end-of-file condition to appear on this end of the
		 * connection.  After all of the client's requests have
		 * been received, the next recv call will return zero
		 * bytes, signalling an end-of-file condition.  This is
		 * how the server will know that no more requests will
		 * follow, and the loop will be exited.
		 */
		 
		 
	//Main loop
	//Recibe todos los comandos
	while (len = recv(s, command, TAMANO, 0)) {
		if (len == -1) errout(hostname); /* error from recv */
			/* The reason this while loop exists is that there
			 * is a remote possibility of the above recv returning
			 * less than TAMANO bytes.  This is because a recv returns
			 * as soon as there is some data, and will not wait for
			 * all of the requested data to arrive.  Since TAMANO bytes
			 * is relatively small compared to the allowed TCP
			 * packet sizes, a partial receive is unlikely.  If
			 * this example had used 2048 bytes requests instead,
			 * a partial receive would be far more likely.
			 * This loop will keep receiving until all TAMANO bytes
			 * have been received, thus guaranteeing that the
			 * next recv at the top of the loop will start at
			 * the begining of the next request.
			 */
		//Hay veces que el TCP Recibe las cosas a trozos y por eso aqui se junta todo
		while (len < TAMANO) {
			len1 = recv(s, &command[len], TAMANO-len, 0);
			if (len1 == -1) errout(hostname);
			len += len1;
		}
		if(removeCRLF(command)){
			fprintf(stderr, "Command without CR-LF. Aborted connection\n");
			exit(1);
		}
//--------	/* Command is ok, just works :D */
		if(-1 == addCommandToLog(command, false)){
			perror("No se ha podido a�adir el comando recibido al fichero http.log");
		}
		FILE *ficheroParam;
		int count = 0;
		char fichero[20], *modo;
		bool numParams = false;
		int i = 0;
		
		char linea[TAMANO];
		char *nombreComando;
	
		nombreComando = strtok(command, " "); //Discards 'GET' (name of the command)
		
		//Invalid fichero
		strcpy(fichero,".");
		strcat(fichero,strtok(NULL, " "));		
		//Invalid hour
		modo = strtok(NULL, " ");
		
		if(modo == NULL){
			strcpy(modo,"c");
		}
		//Invalid number of arguments
		if(strtok(NULL, " ") != NULL) numParams = true;
		
		//INICIO
		char inicio[TAMANO];
		RESET(inicio,TAMANO);
		if(numParams || strcmp(nombreComando,"GET") != 0){
			strcpy(inicio,"HTTP/1.1 501 Not Implemented");
			i = 501;
		}
		//Sintax is correct, now parse command
		else if(NULL == (ficheroParam = fopen(fichero, "r"))){
			strcpy(inicio,"HTTP/1.1 404 Not Found");
			i = 404;
		}else{
			strcpy(inicio,"HTTP/1.1 200 OK");
			i = 200;
		}
		addCRLF(inicio,TAMANO);
		//Send 1
		send(s, inicio,TAMANO, 0);	
		if(-1 == addCommandToLog(inicio, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		
		//NOMBRE SERVIDOR
		char servidor[TAMANO];
		strcpy(servidor,"Server: Servidor de Javier");
		addCRLF(servidor,TAMANO);
		//Send 2
		send(s, servidor, TAMANO, 0);
		if(-1 == addCommandToLog(servidor, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		
		//CONEXION
		char conexion[TAMANO];
		if(strcmp(modo,"k") == 0){
			strcpy(conexion,"Connection: keep-alive");
		}else{
			strcpy(conexion,"Connection: close");
			finish=true;
		}
		addCRLF(conexion,TAMANO);
		//Send 3
		send(s, conexion, TAMANO, 0);
		if(-1 == addCommandToLog(conexion, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		//CONTENT LENGTH
		int longitud = 0;
		char content[TAMANO];
		strcpy(content,"Content-Length: ");
		if(i == 200){
			fseek(ficheroParam, 0, SEEK_END);
			longitud = ftell(ficheroParam);
			count = numLines(ficheroParam);
			longitud = longitud - count;
			char l[5];
			sprintf(l, "%d", longitud);
			strcat(content,l);
		}else if(i == 501 || i == 404){
			char l[5];
			longitud = 60;
			sprintf(l, "%d", longitud);
			strcat(content,l);
		}
		//Send 4
		send(s, content, TAMANO, 0);
		if(-1 == addCommandToLog(content, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		//VACIA
		char vacia[TAMANO];
		strcpy(vacia,"");
		addCRLF(vacia,TAMANO);
		//Send 5
		send(s, vacia, TAMANO, 0);
		if(-1 == addCommandToLog(vacia, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		//Si peta aqui +1
		char texto[longitud];
		if(i == 200){
			fseek(ficheroParam, 0, SEEK_SET);	
			while( fgets(linea, 200, ficheroParam) ){
				linea[strlen(linea) - 1] = '\0';
				strcat(texto,linea);
			}
			fclose(ficheroParam);
		}else if(i == 501){
			strcpy(texto,"<html><body><h1> 501 Not Implemented </h1></body></html>");
		}else if(i == 404){
			strcpy(texto,"<html><body><h1> 404 Not Implemented </h1></body></html>");
		}
		//Send 6
		send(s, texto, TAMANO, 0);
		if(-1 == addCommandToLog(texto, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		if(finish)break;
	}
		/* The loop has terminated, because there are no
		 * more requests to be serviced.  As mentioned above,
		 * this close will block until all of the sent replies
		 * have been received by the remote host.  The reason
		 * for lingering on the close is so that the server will
		 * have a better idea of when the remote has picked up
		 * all of the data.  This will allow the start and finish
		 * times printed in the log file to reflect more accurately
		 * the length of time this connection was used.
		 */
	close(s);

		/* Log a finishing message. */
	time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	//printf("[SERV TCP] Completed %s port %u  at %s",
	//	hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));
}





/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);     
}
















/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
 //void serverUDP(int s, struct sockaddr_in clientaddr_in){}
 
 
void serverUDP(int s, struct sockaddr_in clientaddr_in)
{
	char hostname[MAXHOST];		//remote host's name string 

	int status, n_retry;
	long timevar;			// contains time returned by time() 
	int addrlen = sizeof(struct sockaddr_in);
	
	bool finish = false;
	
	int i;	
	char command[TAMANO];
	
	
	

				
	// Look up the host information for the remote host
	 // that we have connected with.  Its internet address
	 // was returned by the accept call, in the main
	 //daemon loop above.
	 //
	 
	status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in), hostname, MAXHOST,NULL,0,0);
	if(status){
		// The information is unavailable for the remote
			 // host.  Just format its internet address to be
			 //printed out in the logging information.  The
			 // address will be shown in "internet dot format".
			 //
		 // inet_ntop para interoperatividad con IPv6
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}
	
	time (&timevar);
	//printf("[SERV UDP] Startup from %s port %u at %s",
	//	hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

	
	while (1) {
		//Receives next command from the client
		n_retry = RETRIES;
		while(n_retry > 0){
			alarm(TIMEOUT);
			if (recvfrom(s, command, TAMANO, 0, (struct sockaddr *)&clientaddr_in, &addrlen) == -1) {
				if (errno == EINTR) {
		 		     fprintf(stderr,"[SERV UDP] Alarm went off.\n");
		 		     n_retry--; 
				} else  {
					fprintf(stderr,"[SERV UDP] Unable to get response to \"connect\"\n");
					exit(1); 
				}
			}else{
				alarm(0);
				break;
			}
		if(n_retry == 0) break; 
	
	
	
		// Check if command has been received correctly 
		if(removeCRLF(command)){
			fprintf(stderr, "[SUDP] Command without CR-LF. Aborted \"connection\" \n");
			exit(1);
		}
		
		if(-1 == addCommandToLog(command, false)){
			perror("No se ha podido a�adir el comando recibido al fichero http.log");
		}
		
		
//--------	//Command is ok, just works :D 
		}
		FILE *ficheroParam;
		int count = 0;
		char fichero[20], *modo;
		bool numParams = false;
		
		char linea[TAMANO];
		char *nombreComando;
	
		nombreComando = strtok(command, " "); //Discards 'GET' (name of the command)
		
		//Invalid fichero
		strcpy(fichero,".");
		strcat(fichero,strtok(NULL, " "));		
		//Invalid hour
		modo = strtok(NULL, " ");
		
		if(modo == NULL){
			strcpy(modo,"c");
		}
		//Invalid number of arguments
		if(strtok(NULL, " ") != NULL) numParams = true;
		
		//INICIO
		char inicio[TAMANO];
		RESET(inicio,TAMANO);
		if(numParams || strcmp(nombreComando,"GET") != 0){
			strcpy(inicio,"HTTP/1.1 501 Not Implemented");
			i = 501;
		}
		//Sintax is correct, now parse command
		else if(NULL == (ficheroParam = fopen(fichero, "r"))){
			strcpy(inicio,"HTTP/1.1 404 Not Found");
			i = 404;
		}else{
			strcpy(inicio,"HTTP/1.1 200 OK");
			i = 200;
		}
		addCRLF(inicio,TAMANO);
		//Send 1
		sendto(s, inicio, TAMANO, 0,(struct sockaddr *)&clientaddr_in, addrlen);
		if(-1 == addCommandToLog(inicio, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		
		//NOMBRE SERVIDOR
		char servidor[TAMANO];
		strcpy(servidor,"Server: Servidor de Álvaro");
		addCRLF(servidor,TAMANO);
		//Send 2
		sendto(s, servidor, TAMANO, 0,(struct sockaddr *)&clientaddr_in, addrlen);
		if(-1 == addCommandToLog(servidor, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		
		//CONEXION
		char conexion[TAMANO];
		if(strcmp(modo,"k") == 0){
			strcpy(conexion,"Connection: close");//UDP SOLO EN CONECTION CLOSE
			finish=true;
		}else{
			strcpy(conexion,"Connection: close");
			finish=true;
		}
		addCRLF(conexion,TAMANO);
		//Send 3
		sendto(s, conexion, TAMANO, 0,(struct sockaddr *)&clientaddr_in, addrlen);
		if(-1 == addCommandToLog(conexion, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		//CONTENT LENGTH
		int longitud = 0;
		char content[TAMANO];
		strcpy(content,"Content-Length: ");
		if(i == 200){
			fseek(ficheroParam, 0, SEEK_END);
			longitud = ftell(ficheroParam);
			count = numLines(ficheroParam);
			longitud = longitud - count;
			char l[5];
			sprintf(l, "%d", longitud);
			strcat(content,l);
		}else if(i == 501 || i == 404){
			char l[5];
			longitud = 60;
			sprintf(l, "%d", longitud);
			strcat(content,l);
		}
		//Send 4
		sendto(s, content, TAMANO, 0,(struct sockaddr *) &clientaddr_in, addrlen);
		if(-1 == addCommandToLog(content, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		//VACIA
		char vacia[TAMANO];
		strcpy(vacia,"");
		addCRLF(vacia,TAMANO);
		//Send 5
		sendto(s, vacia, TAMANO, 0,(struct sockaddr *) &clientaddr_in, addrlen);
		if(-1 == addCommandToLog(vacia, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		//Si peta aqui +1
		char texto[longitud];
		if(i == 200){
			fseek(ficheroParam, 0, SEEK_SET);	
			while( fgets(linea, 200, ficheroParam) ){
				linea[strlen(linea) - 1] = '\0';
				strcat(texto,linea);
			}
			fclose(ficheroParam);
		}else if(i == 501){
			strcpy(texto,"<html><body><h1> 501 Not Implemented </h1></body></html>");
		}else if(i == 404){
			strcpy(texto,"<html><body><h1> 404 Not Implemented </h1></body></html>");
		}
		//Send 6
		sendto(s, texto, TAMANO, 0,(struct sockaddr *) &clientaddr_in, addrlen);
		if(-1 == addCommandToLog(texto, true)){
			perror("No se ha podido a�adir la respuesta al fichero http.log");
		}
		

		if(finish) break;
	}

	close(s);
	
	time (&timevar);
	//printf("[SERV UDP] Completed %s port %u  at %s",
	//	hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));
 }
 
/**
 * Adds info of the new connection to http.log
 */
int addNewConexionToLog(struct sockaddr_in servaddr_in, struct sockaddr_in clientaddr_in, char *protocol){
	FILE *logFile;
	long timevar;
	time_t t = time(&timevar);
	struct tm* ltime = localtime(&t);
	struct addrinfo hints, *res;
	struct in_addr reqaddr;
	char hostname[MAXHOST];  
	
	if(NULL == (logFile = fopen("../bin/logs/http.log", "a+"))){
		return -1;
	}
	  
	if(getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in), hostname, MAXHOST, NULL, 0, 0)){
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL){
			perror(" inet_ntop \n");
		}
	}		
	
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	if (getaddrinfo (hostname, NULL, &hints, &res) != 0){
		reqaddr.s_addr = ADDRNOTFOUND;
	}
	else {
		reqaddr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
		freeaddrinfo(res);
	}
	

	fprintf(logFile, "FECHA: %02d-%02d-%04d | ", ltime->tm_mday, ltime->tm_mon+1, ltime->tm_year+1900);
	fprintf(logFile, "HORA: %02d:%02d:%02d | ", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	fprintf(logFile, "NOMBRE HOST: %s | ", hostname);
	
	if (reqaddr.s_addr == ADDRNOTFOUND) 
		fprintf(logFile, "IP: %s | ", "DESCONOCIDA");

	else {
		if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
		fprintf(logFile, "IP: %s | ", hostname);
	}

	fprintf(logFile, "PROTOCOLO: %3s | ",protocol);
	fprintf(logFile, "PUERTO CLIENTE: %u\n", ntohs(clientaddr_in.sin_port));
	
	//lockf(fdescriptor, F_ULOCK, 0);
	fclose(logFile);	
	return 0;
}



/**
 * Adds info of the command received to http.log.
 */
int addCommandToLog(char *command, bool isResponse){
	FILE *logFile;
	long timevar;
	time_t t = time(&timevar);
	struct tm* ltime = localtime(&t);
	
	if(NULL == (logFile = fopen("../bin/logs/http.log", "a+"))){
		return -1;
	}

	fprintf(logFile, "FECHA: %02d-%02d-%04d | ", ltime->tm_mday, ltime->tm_mon+1, ltime->tm_year+1900);
	if(!isResponse) fprintf(logFile, "COMANDO: (%s)\n", command);
	else			fprintf(logFile, "RESPUESTA: (%s)\n", command);
	

	fclose(logFile);
	return 0;
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


