//
// Created by kamil on 30.11.2022.
//

#include "heap.h"

struct heap_t {
    struct element *p_head;// 8
    struct element *p_tail;// 8

    //intptr_t sbrk_start;  // 8
    intptr_t sbrk_size;   // 8
} __attribute__((aligned(8)));

struct element {
    struct element *p_prev;// 8
    struct element *p_next;// 8

    int size;
    short free; // 1-wolny; 0- zajety
    int objectSum;
}__attribute__((aligned(8)));

struct heap_t *heap_manager_t;

int heap_setup(void){
    size_t size = sizeof(struct heap_t) + sizeof(struct element) * 2;

    void *sbrkPtr = custom_sbrk(size);
    if (sbrkPtr == NULL) {
        return -1;
    }

    // Pozycje struktur
    struct heap_t *pHeap = (struct heap_t *) sbrkPtr;

    struct element *pHead = (struct element *) ((uint8_t *) sbrkPtr + sizeof(struct heap_t));              // Blok graniczny lewy
    struct element *pTail = (struct element *) ((uint8_t *) sbrkPtr + size - sizeof(struct element));// Blok graniczny prawy

    pHeap->p_head = pHead;
    pHeap->p_tail = pTail;
    pHeap->sbrk_size = size;

    pHeap->p_head->size = 0;
    pHeap->p_head->free = 1;
    pHeap->p_head->objectSum = calculateObjectSum(pHead);//dodac kazdy bajt struktury
    pHeap->p_head->p_next = pTail;
    pHeap->p_head->p_prev = NULL;

    pHeap->p_tail->size = 0;
    pHeap->p_tail->free = 1;
    pHeap->p_tail->objectSum = calculateObjectSum(pTail);//dodac kazdy bajt struktury
    pHeap->p_tail->p_next = NULL;
    pHeap->p_tail->p_prev = pHead;


    heap_manager_t = pHeap;

    return 0;
}

void *heap_malloc(size_t size) {
    if(size>0){
        NULL;
    }
    return NULL;
}

void *heap_calloc(size_t number, size_t size) {
    if(size>0 || number > 0){
        NULL;
    }
    return NULL;

}

void *heap_realloc(void *memblock, size_t count) {
    if(!memblock|| count>0){
        return NULL;
    }
    return NULL;

}

void heap_free(void *memblock) {
    if(!memblock){
        return;
    }

}

enum pointer_type_t get_pointer_type(const void *const pointer) {
    if(!pointer){
        return -1;
    }

    return 0;
}

int heap_validate(void) {

    return 0;
}

size_t heap_get_largest_used_block_size(void){
    return 1;

}

void heap_clean(void){
    if(!heap_manager_t){
        return;
    }

    size_t heap_size = heap_manager_t->sbrk_size;
    memset((void *) heap_manager_t, 0, heap_size);
    custom_sbrk(-heap_size);
    heap_manager_t = NULL;
}

int calculateObjectSum(struct element *ptr){
    if(!ptr){
        return -1;
    }

    uint32_t sum = 0;
    uint8_t *one_byte = (uint8_t *) ptr;

    for(size_t i = 0; i < sizeof(struct element) - 4; i++){
        sum += *(one_byte + i);
    }


    return sum;
}