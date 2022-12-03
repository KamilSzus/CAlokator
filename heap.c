//
// Created by kamil on 30.11.2022.
//

#include <z3.h>
#include "heap.h"
#include "tested_declarations.h"
#include "rdebug.h"

struct heap_t {
    struct element *p_head;// 8
    struct element *p_tail;// 8

    //intptr_t sbrk_start;  // 8
    intptr_t sbrk_size;   // 8
} __attribute__((aligned(8)));

struct element {
    struct element *p_prev;// 8
    struct element *p_next;// 8

    size_t size; //zalokowana ilość pamięci
    short free; // 1-wolny; 0- zajety
    int objectSum;
}__attribute__((aligned(8)));

struct heap_t *heap_manager_t;

void heap_show(const struct heap_t* pheap) {
    if (pheap == NULL)
        return;

    const struct element* pcurrent;
    int counter = 0;
    int free_count = 0;
    for (pcurrent = pheap->p_head; pcurrent != NULL; pcurrent = pcurrent->p_next) {
        printf("Blok %d : %zu bajtów, stan %s\n", ++counter, pcurrent->size,pcurrent->free ? "WOLNY" : "ZAJETY");
        free_count += pcurrent->free ? pcurrent->size : 0;
    }
    printf("Na stercie jest wolnych %d bajtów\n", free_count);
    printf("\n");
}

int heap_setup(void) {
    size_t size = sizeof(struct heap_t) + sizeof(struct element) * 2;//*2 jezeli będzie tail

    void *sbrkPtr = custom_sbrk(size);
    if (sbrkPtr == NULL) {
        return -1;
    }

    // Pozycje struktur
    struct heap_t *pHeap = (struct heap_t *) sbrkPtr;

    struct element *pHead = (struct element *) ((uint8_t *) sbrkPtr +
                                                sizeof(struct heap_t));              // Blok graniczny lewy
    struct element *pTail = (struct element *) ((uint8_t *) sbrkPtr + size -
                                                sizeof(struct element));// Blok graniczny prawy

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

struct element *findFirstFreeElement(size_t size) {

    struct element *freeElement = heap_manager_t->p_head;

    while (1) {
        if (freeElement->free == 1 && freeElement->size >= size) {
            break;
        }

        if (freeElement->p_next == NULL) {
            return NULL;
        }

        freeElement = freeElement->p_next;
    }

    return freeElement;
}

void *heap_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    if (heap_validate() != 0) {
        return NULL;
    }

    struct element *firstFreeElement = findFirstFreeElement(size);

    if (firstFreeElement == NULL) {
        if (heapExpand(size)) {
            return NULL;
        }

        firstFreeElement = findFirstFreeElement(size);
    }

    firstFreeElement->free = 0;
    firstFreeElement->size = size;

    firstFreeElement->objectSum = calculateObjectSum(firstFreeElement);

    void *result = (uint8_t *) firstFreeElement + sizeof(struct element);

    heap_show(heap_manager_t);
    return result;
}

int heapExpand(size_t size) {
    if (heap_manager_t == NULL) {
        return 1;
    }

    size_t sizeNeededForUserAndStruck = size + sizeof(struct element);

    if (custom_sbrk(sizeNeededForUserAndStruck) == (void *) -1) {
        return 1;
    }


    struct element *newElement = (struct element *) (heap_manager_t->sbrk_size + (uint8_t *) heap_manager_t);

    heap_manager_t->sbrk_size += sizeNeededForUserAndStruck;

    newElement->size = size;
    newElement->p_next = heap_manager_t->p_tail;
    heap_manager_t->p_tail->p_prev->p_next = newElement;
    newElement->p_prev = heap_manager_t->p_tail->p_prev;
    newElement->free = 1;
    newElement->objectSum = calculateObjectSum(newElement);

    heap_manager_t->p_tail->p_prev = newElement;

    newElement->p_prev->objectSum = calculateObjectSum(newElement->p_prev);

    //heap_show(heap_manager_t);

    return 0;
}

void *heap_calloc(size_t number, size_t size) {
    if (size > 0 || number > 0) {
        NULL;
    }
    return NULL;

}

void *heap_realloc(void *memblock, size_t count) {
    if (!memblock || count > 0) {
        return NULL;
    }
    return NULL;

}

void heap_free(void *memblock) {
    if (!memblock) {
        return;
    }

}

enum pointer_type_t get_pointer_type(const void *const pointer) {
    if (pointer == NULL)
        return pointer_null;// 0

    if (heap_validate())
        return pointer_heap_corrupted;// 1

    //if (check_if_ptr_is_in_block_t(pointer) || check_if_ptr_is_in_heap_t(pointer))
    //    return pointer_control_block;// 2
//
    //if (check_if_ptr_is_in_plotki(pointer))
    //    return pointer_inside_fences;// 3
//
    //if (check_if_ptr_is_in_user_memory(pointer))
    //    return pointer_inside_data_block;// 4
//
    //if (!check_if_ptr_is_allocated(pointer))
    //    return pointer_unallocated;// 5

    return pointer_valid;// 6

}

int heap_validate(void) {

    return 0;
}

size_t heap_get_largest_used_block_size(void) {
    return 1;

}

void heap_clean(void) {
    if (!heap_manager_t) {
        return;
    }

    size_t heap_size = heap_manager_t->sbrk_size;
    memset((void *) heap_manager_t, 0, heap_size);
    custom_sbrk(-heap_size);
    heap_manager_t = NULL;
}

int calculateObjectSum(struct element *ptr) {
    if (!ptr) {
        return -1;
    }

    uint32_t sum = 0;
    uint8_t *one_byte = (uint8_t *) ptr;

    for (size_t i = 0; i < sizeof(struct element) - 4; i++) {
        sum += *(one_byte + i);
    }


    return sum;
}

void setPlotekInBlock(struct element *ptr)
{
    if (ptr == NULL || heap_manager_t == NULL)
        return;

    if (ptr == heap_manager_t->p_head || ptr == heap_manager_t->p_tail)
        return;


    uint8_t *leftPlotek = ((uint8_t *) ptr + sizeof(struct element));
    uint8_t *rightPlotek = ((uint8_t *) ptr + sizeof(struct element) + 2 + ptr->size);

    for (int i = 0; i < 2; ++i) {
        *(leftPlotek + i) = '#';
    }

    for (int i = 0; i < 2; ++i) {
        *(rightPlotek + i) = '#';

    }
}
