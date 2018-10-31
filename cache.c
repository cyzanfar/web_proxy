//
// Created by Cyrus Ghazanfar and Kulvinder Lotay on 10/28/18.
//


#include "cache.h"

/* Declare head and tail nodes */
cnode_t *tail;
cnode_t *head;

/* Define cache count - items in cache */
int items_in_cache;

/* Define volatile values for concurrency protection */
volatile size_t total_cache_size;
volatile int read_count;

/* Define read and write locks */
sem_t read_lock, write_lock;

/* Construct new node */
cnode_t *new(char *host, int port, char *path, char *body, size_t size) {
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

/* Define cache initialization function */
void cache_init() {
    tail = NULL;
    head = NULL;

    /* Set total_cache_size, read_count, items_in_cache to 0 */
    items_in_cache = 0;
    read_count = 0;
    total_cache_size = 0;

    /* Initialize read and write locks */
    Sem_init(&read_lock, 0 , 1);
    Sem_init(&write_lock, 0, 1);
}

/* Define function to to check for item already in cache */
/* If item exists, return 1, else return 0 */
int compare_item(cnode_t *node, char *host, int port, char *path) {

    /* Check if host matches host in memory */
    if (strcasecmp(node->host, host))
        return 0;

    /* Check if port matches port in memory */
    if (node->port != port)
        return 0;

    /* Check if path matches path in memory*/
    if (strcasecmp(node->path, path))
        return 0;

    /* If all three match, return true */
    return 1;
}

/* Define delete function */
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
    total_cache_size -= node->size;
    items_in_cache--;
}

/* Add to deque */
void add_to_deque(cnode_t *node) {
    if (items_in_cache == 0) {
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
    total_cache_size += node->size;
    items_in_cache++;
}
/* Remove from deque */
void remove_from_deque() {
    cnode_t * res;
    if (items_in_cache == 0)
        return;
    else if (items_in_cache == 1) {
        res = head;
        head = NULL;
        tail = NULL;
    } else {
        res = head;
        (head->next)->prev = NULL;
        head = head->next;
    }
    total_cache_size -= res->size;
    items_in_cache--;
    Free(res->host);
    Free(res->path);
    Free(res->payload);
    Free(res);
}


/* Check for match in node */
cnode_t *match(char *host, int port, char *path) {
    cnode_t * res = tail;
    for (; res != NULL; res = res->prev) {
        if (compare_item(res, host, port, path)) {
            return res;
        }
    }
    return NULL;
}

/* Check cache for item */
int cache_check() {
    cnode_t * block;
    int count = 0;
    if (items_in_cache == 0)
        return 1;
    if (items_in_cache == 1) {
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

    if (count != items_in_cache) {
        printf("Cache count error, count = %d, cache_count = %d\n",
               count, items_in_cache);
        return 0;
    }
    return 1;
}

/* Wrapper function for cache check */
/* If item exists in cache, exit */
void Cache_check() {
    if (!cache_check())
        exit(0);
    return;
}