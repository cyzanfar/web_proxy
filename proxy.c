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
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n\r\n";

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);
int parse_uri(char *uri, char *hostname, char *pathname, int *port);
void handle_request(int connfd);
void read_requesthdrs(rio_t *rp);


int main(int argc, char *argv[])
{
    int listenfd, connfd; /* listen on sockfd, let new connection on new_sockfd */

    char hostname[MAXLINE], port[MAXLINE];

    socklen_t clientlen;

    pthread_t tid;

    struct sockaddr_storage clientaddr;

    /* Check command line args number */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    printf("%s\n%s\nCreate TCP socket\n", user_agent_hdr, argv[1]);


    /* create TCP socket, bind and listen*/
    listenfd = Open_listenfd(argv[1]);


    printf("server: waiting for connections...\n");
    while (1) {
        clientlen = sizeof(clientaddr);

        /*connfd = Malloc(sizeof(int));*/

        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);

        printf("Accepted connection from (%s, %s)\n", hostname, port);

        Pthread_create(&tid, NULL, handle_request, connfd);
    }
}

void handle_request(int connfd) {
    rio_t rio_client;
    rio_t rio_server;

    size_t totalByteCount = 0;

    ssize_t n;

    int port = 0;

    int serverfd;

    char buf[MAXLINE], hostname[MAXLINE], path[MAXLINE], strport[MAXLINE],
            method[16], version[16], uri[MAXLINE], leadLine[MAXLINE];

    path[0] = '/';

    // handle first line, extract uri
    int stage_counter = 0;
    char *token;

    Rio_readinitb(&rio_client, connfd);
    n = Rio_readlineb(&rio_client, buf, MAXLINE);


    Rio_writen(connfd, buf, (size_t)n); /* writes the n bytes */


    // tokenize the url to send the path to parse_uri
    token = strtok(buf, " ");

    printf("TOKEN: %s", token);

    while (token != NULL) {
        switch (stage_counter++)
        {
            case 0:;
                strcpy(method, token);
                break;
            case 1:
                strcpy(uri, token);
                if (parse_uri(token, hostname, path+1, &port) == -1) {
                    Close(connfd);
                }
                break;
            case 2:
                strcpy(version, token);
                break;
        }
        token = strtok(NULL, " ");
    }

    read_requesthdrs(&rio_client);

    printf("token: %s, hostname: %s, path: %s, port: %d\n", token, hostname, path, port);

    sprintf(strport, "%d", port);
    printf("string port: %s\n", strport);

    if ((serverfd = open_clientfd(hostname, strport)) < 0)
    {
        Close(connfd);
    }

    /* associates the file descriptor with the read buffer */
    Rio_readinitb(&rio_server, serverfd);

    /* Write the initial header to the server */
    sprintf(leadLine, "%s %s %s", method, path, version);

    /* writes to the fd leadline i.e GET / HTTP/1.0 */
    Rio_writen(serverfd, leadLine, strlen(leadLine));

    /* reads line by line the header */
    while((n = Rio_readlineb(&rio_client, buf, MAXLINE)) > 0 &&
          buf[0] != '\r' && buf[0] != '\n') {
        printf("buffer: %s\n", buf);
        Rio_writen(serverfd, buf, n);
    }

    /* write end of header */
    Rio_writen(serverfd, "\r\n", 2);

    printf("\nRESPOND: \n%s", leadLine);

    /* Read response from server */
    while((n = Rio_readnb(&rio_server, buf, MAXLINE)) > 0) {
        printf("buffer1: %s\n", buf);
        Rio_writen(connfd, buf, n);
        totalByteCount += n;
    }

    Close(serverfd);
    Close(connfd);
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */

int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    *port = 80;
    if (*hostend == ':')
        *port = atoi(hostend + 1);

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    }
    else {
        pathbegin++;
        strcpy(pathname, pathbegin);
    }

    return 0;
}



void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
