/*
 * Sean Middleditch
 * sean@sourcemud.org
 *
 * The author or authors of this code dedicate any and all copyright interest
 * in this code to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and successors. We
 * intend this dedication to be an overt act of relinquishment in perpetuity of
 * all present and future rights to this code under copyright law. 
 */

#if !defined(_POSIX_SOURCE)
#	define _POSIX_SOURCE
#endif
#if !defined(_BSD_SOURCE)
#	define _BSD_SOURCE
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>

#ifdef HAVE_ZLIB
#include "zlib.h"
#endif

#include "libtelnet.h"

static struct termios orig_tios;
static telnet_t *telnet;
static int do_echo;

static const telnet_telopt_t telopts[] = {
	{ TELNET_TELOPT_ECHO,		TELNET_WONT, TELNET_DO   },
	{ TELNET_TELOPT_TTYPE,		TELNET_WILL, TELNET_DONT },
	{ TELNET_TELOPT_COMPRESS2,	TELNET_WONT, TELNET_DO   },
	{ TELNET_TELOPT_MSSP,		TELNET_WONT, TELNET_DO   },
	{ -1, 0, 0 }
};

static void _cleanup(void) {
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &orig_tios);
}

static void _input(char *buffer, int size, char *protocol) {
	//printf("_input function entered\n");
	static char crlf[] = { '\r', '\n' };
	int i;

	for (i = 0; i != size; ++i) {
		/* if we got a CR or LF, replace with CRLF
		 * NOTE that usually you'd get a CR in UNIX, but in raw
		 * mode we get LF instead (not sure why)
		 */
		if (buffer[i] == '\r' || buffer[i] == '\n') {
			if (do_echo)
				printf("\r\n");
			//printf("Enter 1\n");
			telnet_send(telnet, crlf, 2);
			//printf("Enter 2\n");
		} else {
			if (do_echo)
				putchar(buffer[i]);
			//printf("Enter 3\n");
			telnet_send(telnet, buffer + i, 1);
			//printf("Enter 4\n");
		}
	}
	fflush(stdout);
	//printf("_input function exited\n");
}

static void _send(int sock, const char *buffer, size_t size) {
	int rs;

	/* send data */
		while (size > 0) {
		if ((rs = send(sock, buffer, size, 0)) == -1) {
			fprintf(stderr, "send() failed: %s\n", strerror(errno));
			exit(1);
		} else if (rs == 0) {
			fprintf(stderr, "send() unexpectedly returned 0\n");
			exit(1);
		}

		/* update pointer and size to see if we've got more to send */
		buffer += rs;
		size -= rs;
	}
}

static void _event_handler(telnet_t *telnet, telnet_event_t *ev,
		void *user_data) {
	int sock = *(int*)user_data;

	switch (ev->type) {
	/* data received */
	case TELNET_EV_DATA:
		if (ev->data.size && fwrite(ev->data.buffer, 1, ev->data.size, stdout) != ev->data.size) {
              		fprintf(stderr, "ERROR: Could not write complete buffer to stdout");
		}
		fflush(stdout);
		break;
	/* data must be sent */
	case TELNET_EV_SEND:
		_send(sock, ev->data.buffer, ev->data.size);
		break;
	/* request to enable remote feature (or receipt) */
	case TELNET_EV_WILL:
		/* we'll agree to turn off our echo if server wants us to stop */
		if (ev->neg.telopt == TELNET_TELOPT_ECHO)
			do_echo = 0;
		break;
	/* notification of disabling remote feature (or receipt) */
	case TELNET_EV_WONT:
		if (ev->neg.telopt == TELNET_TELOPT_ECHO)
			do_echo = 1;
		break;
	/* request to enable local feature (or receipt) */
	case TELNET_EV_DO:
		break;
	/* demand to disable local feature (or receipt) */
	case TELNET_EV_DONT:
		break;
	/* respond to TTYPE commands */
	case TELNET_EV_TTYPE:
		/* respond with our terminal type, if requested */
		if (ev->ttype.cmd == TELNET_TTYPE_SEND) {
			telnet_ttype_is(telnet, getenv("TERM"));
		}
		break;
	/* respond to particular subnegotiations */
	case TELNET_EV_SUBNEGOTIATION:
		break;
	/* error */
	case TELNET_EV_ERROR:
		fprintf(stderr, "ERROR: %s\n", ev->error.msg);
		exit(1);
	default:
		/* ignore */
		break;
	}
}

void init_carberry(struct pollfd pfd[], int rs, char buffer[512], int sock){

	printf("Carberry initialization started\n");
	int init_counter = 0;
	char buffAT[3] = {'A','T','\r'};
	char buffCanModeUser[14] = {'C','A','N',' ','M','O','D','E',' ','U','S','E','R','\r'};
	char buffOpenCh2[23] = {'C','A','N',' ','U','S','E','R',' ','O','P','E','N',' ','C','H','2',' ','5','0','0','K','\r'};
	char buffAlignRight[21] = {'C','A','N',' ','U','S','E','R',' ','A','L','I','G','N',' ','R','I','G','H','T','\r'};
	char buffOBDTXID[22] = {'O','B','D',' ','S','E','T',' ','T','X','I','D',' ','C','H','2',' ','0','7','E','0','\r'};
	char buffOBDRXID[22] = {'O','B','D',' ','S','E','T',' ','R','X','I','D',' ','C','H','2',' ','0','7','E','8','\r'};
	

	while(poll(pfd, 2, -1) != -1) {

			//printf("Entered while loop\n");
			if(pfd[0].revents & POLLIN) {
			//printf("Usao sam u prvi if\n");
			if((rs = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
			//printf("Usao sam u drugi if\n");
			if(init_counter == 0){
			printf("AT = Communication working? \n");
			_input(buffAT, 3, NULL);
			init_counter++;
			} else if (init_counter == 1){
			printf("Set Mode User\n");
			_input(buffCanModeUser,14,NULL);
			init_counter++;
			}else if(init_counter == 2){
			printf("Can user open channel 2 auto\n");
			_input(buffOpenCh2,23,NULL);
			init_counter++;
			}else if(init_counter == 3){
			printf("CAN USER ALIGN RIGHT\n");
			_input(buffAlignRight,21,NULL);
			init_counter++;
			} else if(init_counter == 4) {
			printf("OBD SET TXID CH2 07E0\n");
			_input(buffOBDTXID,22,NULL);
			init_counter++;
			} else if(init_counter == 5) {
			printf("OBD SET RXID CH2 07E8\n");
			_input(buffOBDRXID,22,NULL);
			init_counter++;
			} else if(rs == 0){
			break;
			} else {
			fprintf(stderr,"recv(server) failed: %s\n", strerror(errno));
			exit(1);
			}
			//printf("Jel mi se odgovor javio poslije sada?\n");
			//_input(buffer, rs, NULL);
			//printf("Jel mi se odgovor javio poslije ovde?");
			}
		}
		if(pfd[1].revents & POLLIN || init_counter == 6) {
			if((rs = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
			//printf("init_counter %d\n",init_counter);
			telnet_recv(telnet, buffer, rs);
			//printf("%c %c\n", buffer[0], buffer[1]);
			//printf("After telnet response\n");
			} else if (rs == 0) {
			break;
			} else {
			fprintf(stderr, "recv(client) failed: %s\n", strerror(errno));
			exit(1);
			}
		
		}
	if(init_counter == 6){
	printf("Exiting initialization for OBD2\n");
	break;} 
	//printf("End of while loop iterration\n");
	}

}

int main(int argc, char **argv) {
	char buffer[512];
	int rs;
	int sock;
	struct sockaddr_in addr;
	struct pollfd pfd[2];
	struct addrinfo *ai;
	struct addrinfo hints;
	struct termios tios;
	char buffRequestRPM[19] = {'O','B','D',' ','Q','U','E','R','Y',' ','C','H','2',' ','0','1','0','C','\r'};

	/* check usage */
	if (argc != 3) {
		fprintf(stderr, "Usage:\n ./telnet-client <host> <port>\n");
		return 1;
	}

	/* look up server host */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rs = getaddrinfo(argv[1], argv[2], &hints, &ai)) != 0) {
		fprintf(stderr, "getaddrinfo() failed for %s: %s\n", argv[1],
				gai_strerror(rs));
		return 1;
	}
	
	/* create server socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "socket() failed: %s\n", strerror(errno));
		return 1;
	}

	/* bind server socket */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		fprintf(stderr, "bind() failed: %s\n", strerror(errno));
		return 1;
	}

	/* connect */
	if (connect(sock, ai->ai_addr, ai->ai_addrlen) == -1) {
		fprintf(stderr, "connect() failed: %s\n", strerror(errno));
		return 1;
	}

	/* free address lookup info */
	freeaddrinfo(ai);

	/* get current terminal settings, set raw mode, make sure we
	 * register atexit handler to restore terminal settings
	 */
	//tcgetattr(STDOUT_FILENO, &orig_tios);
	//atexit(_cleanup);
	//tios = orig_tios;
	//cfmakeraw(&tios);
	//tcsetattr(STDOUT_FILENO, TCSADRAIN, &tios);

	/* set input echoing on by default */
	do_echo = 0;

	/* initialize telnet box */
	telnet = telnet_init(telopts, _event_handler, 0, &sock);

	/* initialize poll descriptors */
	memset(pfd, 0, sizeof(pfd));
	pfd[0].fd = STDIN_FILENO;
	pfd[0].events = POLLIN;
	pfd[1].fd = sock;
	pfd[1].events = POLLIN;

	char *protocol = NULL;

		printf("\n");
                printf("Please select supported protocol:\n");
                printf("1. LIN - to be implemented\n");
                printf("2. CAN\n");
                int choice;
                scanf("%d",&choice);
                if(choice == 2){

                printf("CAN selected,begining auto setup\nPress Enter\n");

		/*Function that initializes Carberry for OBD QUERY COMMAND*/
		init_carberry(pfd, rs, buffer, sock);
		/*RPM OBD Request*/
		_input(buffRequestRPM, 19, NULL);
		printf("RPM OBD COMMAND SENT\n");
		/*RPM OBD Response*/
		rs = recv(sock, buffer, sizeof(buffer), 0);
		telnet_recv(telnet, buffer, rs);

		printf("Response: %c%c%c%c%c%c%c%c%c%c%c\n", buffer[0],buffer[1],buffer[2],buffer[3],buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9],buffer[10]);
		}
          	else{
		printf("Wrong selection\n");
		}

		FILE *fp;
		//fp = fopen("OBD_RPM_QUERYS.txt","w");
		
		//printf("Writing to a file...\n");
		//fprintf(fp,"Nikola\n");
		//printf("End writing to a file\n");
		//fclose(fp);
		
		int write_counter = 0;

		
		while(1){
		/*RPM OBD Request in the while loop*/
		_input(buffRequestRPM, 19, NULL);
		printf("RPM OBD COMMAND SENT\n");
		/*RPM OBD Response in the while loop*/
		rs = recv(sock, buffer, sizeof(buffer), 0);
		telnet_recv(telnet, buffer, rs);
		printf("Response: %c%c%c%c%c%c%c%c%c%c%c\n", buffer[0],buffer[1],buffer[2],buffer[3],buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9] ,buffer[10]);
		fp = fopen("OBD_RPM_QUERY.txt","a");
		if(fp == NULL) {
			printf("Error while opening the file\n");
			return 0;
		}
		fprintf(fp, "%d: %c%c%c%c%c%c%c%c%c%c%c\n",write_counter, buffer[0],buffer[1],buffer[2],buffer[3],buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9] ,buffer[10]);
		fclose(fp);
		sleep(3);
		write_counter++;
		}

	/*Below while loop is used for sending commands to Carrbery manually*/

	/* loop while both connections are open */
	//while (poll(pfd, 2, -1) != -1) {
		//printf("Entered while loop\n");
		/*if (pfd[0].revents & POLLIN) {
			if ((rs = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
				_input(buffer, rs,NULL);
				//_input(array,rs);
			} else if (rs == 0) {
				break;
			} else {
				fprintf(stderr, "recv(server) failed: %s\n",
						strerror(errno));
				exit(1);
			}
		}

		// read from client	 
		if (pfd[1].revents & POLLIN) {
			if ((rs = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
				//printf("While response\n");
				printf("This is before telnet response\n");
				telnet_recv(telnet, buffer, rs);
				printf("%c%c%c%c%c%c%c%c%c%c%c\n", buffer[0],buffer[1],buffer[2],buffer[3],buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9],buffer[10]);
				//printf("This response is from while loop: %c%c\n",buffer[0],buffer[1]);
				printf("This is after telnet response\n");
			} else if (rs == 0) {
				break;
			} else {
				fprintf(stderr, "recv(client) failed: %s\n",
						strerror(errno));
				exit(1);
			}
		}*/
		//printf("End of while loop iteration\n");
	//}

	//clean up 
	telnet_free(telnet);
	close(sock);

	return 0;
}

