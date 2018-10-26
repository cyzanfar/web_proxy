#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define BACKLOG 10

/* Misc constants */
#define	MAXLINE	 8192  /* Max text line length */
#define MAXBUF   8192  /* Max I/O buffer size */
#define LISTENQ  1024  /* Second argument to listen() */

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char *argv[])
{
    int listenfd, connfd; /* listen on sockfd, let new connection on new_sockfd */
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    printf("%s\n%s\n", user_agent_hdr, argv[1]);

    printf("Create TCP socket\n");

    /* create TCP socket, bind and listen*/
    if ((listenfd = Open_listenfd(argv[1])) < 0){
        perror("socket");
        exit(-1);
    }

    printf("server: waiting for connections...\n");

    while (1) {
        clientlen = sizeof(clientaddr);

        /* accept incomming request */
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);

        printf("Accepted connection from (%s, %s)\n", hostname, port);
    }




    return 0;
}

