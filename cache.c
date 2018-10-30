//
// Created by Cyrus Ghazanfar and Kulvinder Lotay on 10/28/18.
//




#include "cache.h"


cnode_t *tail;
cnode_t *head;
int cache_count;
volatile size_t cache_load;
volatile int read_count;
sem_t read_lock, write_lock;


int cmp(cnode_t *node, char *host, int port, char *path) {
    if (strcasecmp(node->host, host))
        return 0;
    if (node->port != port)
        return 0;
    if (strcasecmp(node->path, path))
        return 0;
    return 1;
}

/*
 */
/* $begin cache_init */
void cache_init() {
    tail = NULL;
    head = NULL;
    cache_load = 0;
    read_count = 0;
    cache_count = 0;
    Sem_init(&read_lock, 0 , 1);
    Sem_init(&write_lock, 0, 1);
}
/* $end cache_init */

/*
 * delete - Delete the given cache node from the list
 */
/* $begin delete */
void delete(cnode_t *node) {
    if (head == tail) {
        head = NULL;
        tail = NULL;
    } else if (node->prev == NULL) {
        head = node->next;
        (node->next)->prev = NULL;
    } else if (node->next == NULL) {
        tail = node->prev;
        (node->prev)->next = NULL;
    } else {
        (node->prev)->next = node->next;
        (node->next)->prev = node->prev;
    }
    cache_load -= node->size;
    cache_count--;
}
/* $end delete */

/*
 * enqueue - Enqueue the given cache node
 */
/* $begin enqueue */
void enqueue(cnode_t *node) {
    if (cache_count == 0) {
        head = node;
        tail = node;
        node->next = NULL;
        node->prev = NULL;
    } else {
        tail->next = node;
        node->prev = tail;
        node->next = NULL;
        tail = node;
    }
    cache_load += node->size;
    cache_count++;
}
/* $end enqueue */

/*
 * dequeue - Dequeue the given cache node
 */
/* $begin dequeue */
void dequeue() {
    cnode_t * res;
    if (cache_count == 0)
        return;
    else if (cache_count == 1) {
        res = head;
        head = NULL;
        tail = NULL;
    } else {
        res = head;
        (head->next)->prev = NULL;
        head = head->next;
    }
    cache_load -= res->size;
    cache_count--;
    Free(res->host);
    Free(res->path);
    Free(res->payload);
    Free(res);
}
/* $end dequeue */

/*
 * new - New node constructor
 * return: the ptr to the node constructed
 */
/* $begin new */
cnode_t * new(char *host, int port, char *path, char *body, size_t size) {
    cnode_t * res = Malloc(sizeof(cnode_t));
    res->host = Malloc(strlen(host) + 1);
    strcpy(res->host, host);
    res->path = Malloc(strlen(path) + 1);
    strcpy(res->path, path);
    res->port = port;
    res->payload = Malloc(strlen(body) + 1);
    strcpy(res->payload, body);
    res->size = size;
    return res;
}
/* $end new */


/*
 * match - Try to match a node in the cache
 * return: the node matched
 *         NULL on no matching
 */
/* $begin match */
cnode_t * match(char *host, int port, char *path) {
    cnode_t * res = tail;
    for (; res != NULL; res = res->prev) {
        if (cmp(res, host, port, path)) {
            return res;
        }
    }
    return NULL;
}
/* $end match */


/*
 * cache_check - Try the cache
 * return: 0 on error
 *         1 on good
 */
/* $begin cache_check */
int cache_check() {
    cnode_t * block;
    int count = 0;
    if (cache_count == 0)
        return 1;
    if (cache_count == 1) {
        if (head != tail) {
            printf("When count === 1, head should equal tail\n");
            return 0;
        }
        if (head->prev != NULL) {
            printf("The prev of head should be NULL\n");
            return 0;
        }
        if (tail->next != NULL) {
            printf("The next of tail should be NULL\n");
            return 0;
        }
        return 1;
    }

    if (tail->next != NULL) {
        printf("The next of tail should be NULL\n");
        return 0;
    }
    count++;
    for (block = tail; block->prev != NULL; block = block->prev) {
        count++;
        if (block != (block->prev)->next) {
            printf("Adjacent blocks' ptr should be consistent\n");
            return 0;
        }
    }

    if (block != head) {
        printf("Head is not reachable\n");
        return 0;
    }

    if (head->prev != NULL) {
        printf("The prev of head should be NULL\n");
        return 0;
    }

    if (count != cache_count) {
        printf("Cache count error, count = %d, cache_count = %d\n",
               count, cache_count);
        return 0;
    }
    return 1;
}
/* $end cache_check */


/*
 * Cache_check - Wrapper for cache_check
 */
/* $begin Cache_check */
void Cache_check() {
    if (!cache_check())
        exit(0);
    return;
}
/* $end cache_check */