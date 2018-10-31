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
#include "cache.h"

#define BACKLOG 10

/* Misc constants */
#define	MAXLINE	 8192  /* Max text line length */
#define MAXBUF   8192  /* Max I/O buffer size */
#define LISTENQ  1024  /* Second argument to listen() */


/* You won't lose style points for including this long line in your code */
/* Set static header content for User-Agent, Connection, and Proxy-Connection */
static const char *user_agent_hdr = "%sUser-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "%sConnection: close\r\n";
static const char *proxy_connection_hdr = "%sProxy-Connection: close\r\n\r\n";

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);
int parse_uri(char *uri, char *hostname, char *pathname, int *port);
void handle_request(int connfd);
void read_requesthdrs(rio_t *rp, char*);
void *thread_handler(int fd);
void set_write_lock();
void rel_write_lock();
void set_read_lock();
void rel_read_lock();


int main(int argc, char *argv[])
{
    int listenfd, connfd; /* listen on sockfd, let new connection on new_sockfd */

    char hostname[MAXLINE], port[MAXLINE];

    socklen_t clientlen;

    struct sockaddr_storage clientaddr;

    /* Check command line args number */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    printf("%s\n%s\nCreate TCP socket\n", user_agent_hdr, argv[1]);


    /* create TCP socket, bind and listen*/
    listenfd = Open_listenfd(argv[1]);

    /* Initialize cache for all threads */
    cache_init();

    printf("server: waiting for connections...\n");
    while (1) {
        clientlen = sizeof(clientaddr);

        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);

        printf("Accepted connection from (%s, %s)\n", hostname, port);

        thread_handler(connfd);
    }

    return 0;
}

void *thread_handler(int fd){
    pthread_t tid;

    printf("Creating new thread handler for: %d\n", fd);

    Pthread_create(&tid, NULL, handle_request, fd);
}

void handle_request(int connfd) {
    /* Client and server IO handlers */
    rio_t rio_client, rio_server;

    cnode_t *node;

    /* Line and total cache byte count */
    size_t totalByteCount = 0;
    ssize_t line;

    /* Flag to check if cache was hit */
    int cache_hit = 0;

    int port = 0;

    int serverfd;

    char buf[MAXLINE], hostname[MAXLINE], path[MAXLINE], strport[MAXLINE], body[MAX_OBJECT_SIZE],
            method[16], version[16], uri[MAXLINE], leadLine[MAXLINE], request_header[MAXLINE];

    memset(buf, 0, sizeof(buf));
    memset(body, 0, sizeof(body));

    path[0] = '/';

    // handle first line, extract uri
    int stage_counter = 0;
    char *token;

    /* Associating file descriptor with a read buffer */
    Rio_readinitb(&rio_client, connfd);

    /* Read first line into buffer */
    Rio_readlineb(&rio_client, buf, MAXLINE);

    /* Return if buffer is empty */
    if (strcmp(buf, "") == 0)
        return;

    /* Extract the method i.e GET */
    token = strtok(buf, " ");

    /* this just assigns each part of the leadline to the corresponding variable (method, uri, version) */
    while (token != NULL) {
        switch (stage_counter++)
        {
            case 0:
                strcpy(method, token);
                break;
            case 1:
                strcpy(uri, token);
                if (parse_uri(token, hostname, path+1, &port) == -1) {
                    Close(connfd);
                }
                break;
            case 2:
                strcpy(version, "HTTP/1.0");
                break;
        }
        token = strtok(NULL, " ");
    }

    /* Set read and write to locks */
    set_read_lock();

    Cache_check();

    if ((node = elem_match(hostname, port, path)) != NULL) {
        printf("Item in Cache\n");

        delete(node);
        add_to_deque(node);
        Rio_writen(connfd, node->payload, node->size);
        cache_hit = 1;
    }

    rel_read_lock();

    if (cache_hit == 1) {
        printf("Cached item returned\n");
        return;
    }

    printf("Item not in Cache...\n");

    /* Read the request headers */
    read_requesthdrs(&rio_client, request_header);

    /* Check if method is other than GET */
    if (strcasecmp(method, "GET")) {
        printf("METHOD: %s\n", method);
        /* TODO figure out wtf is going on */
        clienterror(connfd, method, "501", "Not Implemented", "Method not implemented !");
        return;
    }

    /* Write the initial header to the server */
    sprintf(leadLine, "%s %s %s\r\n", method, path, version);

    printf("method: %s, hostname: %s, path: %s, port: %d\n", method, hostname, path, port);

    /* convert the int port into a string */
    sprintf(strport, "%d", port);

    if ((serverfd = Open_clientfd(hostname, strport)) < 0)
    {
        Close(connfd);
    }

    /* associates the file descriptor with the read buffer */
    Rio_readinitb(&rio_server, serverfd);

    /* writes to the fd leadline i.e GET / HTTP/1.0 */
    Rio_writen(serverfd, leadLine, strlen(leadLine));

    /* write the actual header */
    Rio_writen(serverfd, request_header, strlen(request_header));

    /* write end of header */
    Rio_writen(serverfd, "\r\n", 2);

    /* Read response from server */
    while((line = Rio_readnb(&rio_server, buf, MAXLINE)) > 0) {
        totalByteCount += line;

        if (totalByteCount <= MAX_OBJECT_SIZE) {
            strcat(body, buf);
            Rio_writen(connfd, buf, line);
        }
    }

    if (totalByteCount <= MAX_OBJECT_SIZE) {
        node = new(hostname, port, path, body, totalByteCount);

        set_write_lock();
        Cache_check();

        while (total_cache_size + totalByteCount > MAX_CACHE_SIZE) {
            printf("Evicting the Cache\n");
            remove_from_deque();
        }
        add_to_deque(node);
        printf("Cache load: %d bytes\n", total_cache_size);
        printf("Cache size: %d \n", items_in_cache);


        Cache_check();
        rel_write_lock();
    }

    Close(serverfd);
    Close(connfd);
}

void read_requesthdrs(rio_t *rp, char* header)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {

        if (strstr(buf, "User-Agent") != NULL) {
            sprintf(header, user_agent_hdr, header);
        }
        else if(strstr(buf, "Proxy-Connection") != NULL) {
            sprintf(header, proxy_connection_hdr, header);
        }
        else if(strstr(buf, "Connection") != NULL) {
            sprintf(header, connection_hdr, header);
        }
        else {
            sprintf(header, "%s%s", header, buf);
        }
        Rio_readlineb(rp, buf, MAXLINE);
    }

    return;
}

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

void set_write_lock() {
    P(&write_lock);
}

void rel_write_lock() {
    V(&write_lock);
}

void set_read_lock() {
    P(&read_lock);
    read_count++;
    if (read_count == 1) {
        set_write_lock();
    }
    V(&read_lock);
}

void rel_read_lock() {
    P(&read_lock);
    read_count--;
    if (read_count == 0) {
        rel_write_lock();
    }
    V(&read_lock);
}

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    printf("IN CLIENT ERROR\n");
    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    printf("111111\n");
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
