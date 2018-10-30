//
// Created by Cyrus Ghazanfar and Kulvinder Lotay on 10/28/18.
//

#ifndef HW2_HANDOUT_CACHE_H
#define HW2_HANDOUT_CACHE_H

#include "csapp.h"

#define MAX_CACHE_SIZE 1098304
#define MAX_OBJECT_SIZE 102400




/* Cache node struct */
typedef struct cnode {
    char *host;
    char *path;
    char *payload;
    struct cnode *prev;
    struct cnode *next;
    size_t size;
    int port;
} cnode_t;

extern cnode_t *tail;
extern cnode_t *head;
extern int cache_count;
extern volatile size_t cache_load;
extern volatile int read_count;
extern sem_t read_lock, write_lock;

int cmp(cnode_t *node, char *host, int port, char *path);
void cache_init();
void delete(cnode_t *node);
void enqueue(cnode_t *node);
void dequeue();
cnode_t * new(char *host, int port, char *path, char *body, size_t size);
cnode_t * match(char *host, int port, char *path);
int cache_check();
void Cache_check();


#endif //HW2_HANDOUT_CACHE_H
