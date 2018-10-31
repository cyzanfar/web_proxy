//
// Created by Cyrus Ghazanfar and Kulvinder Lotay on 10/28/18.
//

#ifndef HW2_HANDOUT_CACHE_H
#define HW2_HANDOUT_CACHE_H

#include "csapp.h"

/* Set constants for max cache size (1MB) and object size (100KB) */
#define MAX_CACHE_SIZE 1098304
#define MAX_OBJECT_SIZE 102400

/* Cache node struct */
/* Deque */
typedef struct cnode {
    char *host;
    char *path;
    char *payload;
    struct cnode *prev;
    struct cnode *next;
    size_t size;
    int port;
} cnode_t;

/* Declare head and tail nodes */
extern cnode_t *tail;
extern cnode_t *head;

/* Declare cache count - items in cache */
extern int items_in_cache;

/* Total size of items in cache */
extern volatile size_t total_cache_size;
extern volatile int read_count;

/* Semaphores for read and write locks */
extern sem_t read_lock, write_lock;

/* Declare functions for compare, init, adding and removing from deque */
int compare_item(cnode_t *node, char *host, int port, char *path);
void cache_init();
void delete(cnode_t *node);
void add_to_deque(cnode_t *node);
void remove_from_deque();
cnode_t * new(char *host, int port, char *path, char *body, size_t size);
cnode_t * elem_match(char *host, int port, char *path);
int cache_check();
void Cache_check();


#endif //HW2_HANDOUT_CACHE_H
