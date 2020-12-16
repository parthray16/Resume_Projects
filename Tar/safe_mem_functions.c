#include "safe_mem_functions.h"

#include <stdio.h>

void *safe_malloc(size_t size){
    void *new = NULL;
    if (!(new = malloc(size))){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return new;
}

void *safe_realloc(void *ptr, size_t size){
    void *new = NULL;
    if (!(new = realloc(ptr, size))){
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    return new;
}

void *safe_calloc(int nitems, size_t size){
    void *new = NULL;
    if (!(new = calloc(nitems, size))){
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    return new;
}