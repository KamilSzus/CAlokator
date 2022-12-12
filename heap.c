//
// Created by kamil on 30.11.2022.
//

#include <z3.h>
#include "heap.h"
#include "tested_declarations.h"
#include "rdebug.h"

struct heap_t {
    struct element *pHead;// 8
    struct element *pTail;// 8

    intptr_t sbrkSize;   // 8
} __attribute__((aligned(8)));

struct element {
    struct element *pPrev;// 8
    struct element *pNext;// 8

    size_t size; //ilość pamięci
    size_t sizeAllocated; //zalokowana ilość pamięci

    short isFree; // 1-wolny; 0- zajety
    int objectSum;
}__attribute__((aligned(8)));

struct heap_t *heapManagerT;

void heapShow(const struct heap_t *pheap) {
    if (pheap == NULL)
        return;

    const struct element *pCurrent;
    int counter = 0;
    int free_count = 0;
    for (pCurrent = pheap->pHead; pCurrent != NULL; pCurrent = pCurrent->pNext) {
        printf("Blok %d : %zu bajtów, stan %s\n", ++counter, pCurrent->size, pCurrent->isFree ? "WOLNY" : "ZAJETY");
        free_count += pCurrent->isFree ? pCurrent->size : 0;
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
                                                sizeof(struct heap_t));
    struct element *pTail = (struct element *) ((uint8_t *) sbrkPtr + size -
                                                sizeof(struct element));

    pHeap->pHead = pHead;
    pHeap->pTail = pTail;
    pHeap->sbrkSize = size;

    pHeap->pHead->size = 0;
    pHeap->pHead->sizeAllocated = 0;
    pHeap->pHead->isFree = 0;
    pHeap->pHead->pNext = pTail;
    pHeap->pHead->pPrev = NULL;
    pHeap->pHead->objectSum = calculateObjectSum(pHead);//dodac kazdy bajt struktury

    pHeap->pTail->size = 0;
    pHeap->pTail->sizeAllocated = 0;
    pHeap->pTail->isFree = 0;
    pHeap->pTail->pNext = NULL;
    pHeap->pTail->pPrev = pHead;
    pHeap->pTail->objectSum = calculateObjectSum(pTail);//dodac kazdy bajt struktury


    heapManagerT = pHeap;

    return 0;
}

struct element *findFirstFreeElement(size_t size) {

    struct element *freeElement = heapManagerT->pHead;

    while (1) {
        if (freeElement->isFree == 1 && freeElement->size >= size) {
            break;
        }

        if (freeElement->pNext == NULL) {
            return NULL;
        }

        freeElement = freeElement->pNext;
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

    firstFreeElement->isFree = 0;
    firstFreeElement->size = size;

    firstFreeElement->objectSum = calculateObjectSum(firstFreeElement);
    setPlotekInBlock(firstFreeElement);

    void *result = (uint8_t *) firstFreeElement + sizeof(struct element) + 2;

    uint8_t *startCurrencyElement = ((uint8_t *) firstFreeElement + sizeof(struct element));
    uint8_t *endCurrencyElement = (startCurrencyElement + 2 + firstFreeElement->size);

    if (endCurrencyElement == startCurrencyElement) {
        return result;
    }

    if (heap_validate() != 0) {
        return NULL;
    }

    //heapShow(heapManagerT);
    return result;
}

int heapExpand(size_t size) {
    if (heapManagerT == NULL) {
        return 1;
    }

    size_t sizeNeededForUserAndStruck = size + sizeof(struct element) + 4;

    if (custom_sbrk(sizeNeededForUserAndStruck) == (void *) -1) {
        return 1;
    }


    struct element *newElement = (struct element *) (heapManagerT->sbrkSize + (uint8_t *) heapManagerT);

    heapManagerT->sbrkSize += sizeNeededForUserAndStruck;

    newElement->size = size;
    newElement->sizeAllocated = size;
    newElement->pNext = heapManagerT->pTail;
    heapManagerT->pTail->pPrev->pNext = newElement;
    newElement->pPrev = heapManagerT->pTail->pPrev;
    newElement->isFree = 1;
    newElement->objectSum = calculateObjectSum(newElement);

    heapManagerT->pTail->pPrev = newElement;

    newElement->pPrev->objectSum = calculateObjectSum(newElement->pPrev);

    //heapShow(heapManagerT);

    return 0;
}

void *heap_calloc(size_t number, size_t size) {
    if (size > 0 || number > 0) {
        NULL;
    }

    void *newHeap = heap_malloc(number * size);
    if (!newHeap) {
        return NULL;
    }

    memset(newHeap, 0, number * size);

    if (heap_validate() != 0) {
        return NULL;
    }

    return newHeap;

}

void *heap_realloc(void *memblock, size_t count) {
    if (count == 0) {
        heap_free(memblock);
        return NULL;
    }

    enum pointer_type_t pointer_type = get_pointer_type(memblock);
    if ((pointer_type != pointer_null && pointer_type != pointer_valid)) {
        return NULL;
    }

    if (heap_validate() != 0) {
        return NULL;
    }

    if (!memblock) {
        return heap_malloc(count);
    }

    struct element *pElement = (struct element *) ((uint8_t *) memblock - sizeof(struct element) - 2);

    if (pElement->size == count) {
        return memblock;
    } else if (pElement->size > count) {
        pElement->size = count;
        pElement->objectSum = calculateObjectSum(pElement);
        setPlotekInBlock(pElement);

        return memblock;
    } else if (pElement->pNext == heapManagerT->pTail) {
        if (heapExpand(count - pElement->size)) {
            return NULL;
        }

        intptr_t startNextElement = (intptr_t) ((uint8_t *) pElement->pNext);
        intptr_t endNextElement = startNextElement + sizeof(struct element) + 4 + pElement->pNext->size;

        if (endNextElement - startNextElement < (intptr_t) sizeof(struct element) + 4) {

            struct element *futureNextElement = (struct element *) ((uint8_t *) pElement + 4 + sizeof(struct element) +
                                                                    count);
            memcpy(futureNextElement, pElement->pNext, sizeof(struct element));//potencjalnie tutaj

            futureNextElement->size =
                    ((intptr_t) futureNextElement->pNext - (intptr_t) futureNextElement) -
                    sizeof(struct element) + 4;
            //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
            //^                      ^
            futureNextElement->sizeAllocated = futureNextElement->size;

            pElement->size = count;
            pElement->sizeAllocated = count;

            futureNextElement->pPrev = pElement;
            futureNextElement->pNext->pPrev = futureNextElement;
            pElement->pNext = futureNextElement;


            pElement->objectSum = calculateObjectSum(pElement);
            futureNextElement->objectSum = calculateObjectSum(futureNextElement);
            futureNextElement->pNext->objectSum = calculateObjectSum(futureNextElement->pNext);

            setPlotekInBlock(futureNextElement);
            setPlotekInBlock(pElement);
            if (heap_validate() != 0) {
                return NULL;
            }
        } else {

            pElement->size = count;
            pElement->sizeAllocated = pElement->size;

            pElement->pNext = pElement->pNext->pNext;
            pElement->pNext->pPrev = pElement;

            pElement->objectSum = calculateObjectSum(pElement);
            pElement->pNext->objectSum = calculateObjectSum(pElement->pNext);
            pElement->pNext->pPrev->objectSum = calculateObjectSum(pElement->pNext->pPrev);

            setPlotekInBlock(pElement);
            if (heap_validate() != 0) {
                return NULL;
            }

        }

        if (heap_validate() != 0) {
            return NULL;
        }

        return memblock;
    } else if (pElement->sizeAllocated + pElement->pNext->sizeAllocated + sizeof(struct element) + 4 +
               sizeof(struct element) + 4 >= count + sizeof(struct element) + 4 &&
               pElement->pNext->isFree) {

        intptr_t startNextElement = (intptr_t) ((uint8_t *) pElement->pNext);
        intptr_t endNextElement = startNextElement + sizeof(struct element) + 4 + pElement->pNext->size;

        if (endNextElement - startNextElement < (intptr_t) sizeof(struct element) + 4) {

            struct element *futureNextElement = (struct element *) ((uint8_t *) pElement + 4 + sizeof(struct element) +
                                                                    count);

            memcpy(futureNextElement, pElement->pNext, sizeof(struct element));

            futureNextElement->size =
                    ((intptr_t) futureNextElement->pNext - (intptr_t) futureNextElement) -
                    sizeof(struct element) + 4;

            futureNextElement->sizeAllocated = futureNextElement->size;

            pElement->size = count;
            pElement->sizeAllocated = count;

            futureNextElement->pPrev = pElement;
            futureNextElement->pNext->pPrev = futureNextElement;
            pElement->pNext = futureNextElement;


            pElement->objectSum = calculateObjectSum(pElement);
            futureNextElement->objectSum = calculateObjectSum(futureNextElement);
            futureNextElement->pNext->objectSum = calculateObjectSum(futureNextElement->pNext);

            setPlotekInBlock(futureNextElement);
            setPlotekInBlock(pElement);
            if (heap_validate() != 0) {
                return NULL;
            }
        } else {

            pElement->size = count;
            pElement->sizeAllocated = pElement->size;

            pElement->pNext = pElement->pNext->pNext;
            pElement->pNext->pPrev = pElement;

            pElement->objectSum = calculateObjectSum(pElement);
            pElement->pNext->objectSum = calculateObjectSum(pElement->pNext);
            pElement->pNext->pPrev->objectSum = calculateObjectSum(pElement->pNext->pPrev);

            setPlotekInBlock(pElement);
            if (heap_validate() != 0) {
                return NULL;
            }
        }

        return memblock;
    } else {
        void *pNew = heap_malloc(count);
        if (pNew == NULL) {
            return NULL;
        }
        memcpy(pNew, memblock, pElement->size);
        heap_free(memblock);

        if (heap_validate() != 0) {
            return NULL;
        }
        return pNew;
    }
}

void heap_free(void *memblock) {
    if (!memblock || !heapManagerT || get_pointer_type(memblock) != 6) {
        return;
    }

    struct element *pElement = (struct element *) ((uint8_t *) memblock - sizeof(struct element) - 2);
    pElement->isFree = 1;
    pElement->size = pElement->sizeAllocated;

    pElement->objectSum = calculateObjectSum(pElement);
    //heapShow(heapManagerT);
    if (pElement->pNext->isFree && pElement->pNext != heapManagerT->pTail) {
        //pElement->size += pElement->pNext->size ;
        pElement->size += pElement->pNext->size + (sizeof(struct element) + 2 * 2);
        pElement->sizeAllocated = pElement->size;

        struct element *pNext = pElement->pNext->pNext;

        pElement->pNext = pNext;
        pNext->pPrev = pElement;

        pNext->objectSum = calculateObjectSum(pNext);
        pElement->objectSum = calculateObjectSum(pElement);

        setPlotekInBlock(pElement);
        //heapShow(heapManagerT);
    }

    if (pElement->pPrev->isFree && pElement->pPrev != heapManagerT->pHead) {
        struct element *pPrev = pElement->pPrev;
        pPrev->size += pElement->size + (sizeof(struct element) + 2 * 2);
        pPrev->sizeAllocated = pPrev->size;

        // pElement->pPrev = pPrev;
        pElement->pNext->pPrev = pPrev;
        pPrev->pNext = pElement->pNext;

        pPrev->objectSum = calculateObjectSum(pPrev);
        pPrev->pNext->objectSum = calculateObjectSum(pPrev->pNext);

        setPlotekInBlock(pPrev);
        //   heapShow(heapManagerT);
    }

}

enum pointer_type_t get_pointer_type(const void *const pointer) {
    if (pointer == NULL) {
        return pointer_null;
    }

    if (heap_validate()) {
        return pointer_heap_corrupted;
    }

    struct element *current = heapManagerT->pHead->pNext;

    while (current != heapManagerT->pTail) {

        intptr_t startCurrencyElement = (intptr_t) ((uint8_t *) current);
        intptr_t endCurrencyElement = startCurrencyElement + sizeof(struct element);

        if (startCurrencyElement <= (intptr_t) pointer && endCurrencyElement > (intptr_t) pointer) {
            return pointer_control_block;
        }

        current = current->pNext;
    }

    current = heapManagerT->pHead->pNext;
    while (current != heapManagerT->pTail) {

        if (current->isFree) {
            current = current->pNext;
            continue;
        }
        intptr_t startCurrencyElement = (intptr_t) ((uint8_t *) current) + sizeof(struct element);
        intptr_t endCurrencyElement = startCurrencyElement + current->size + 2;

        if (startCurrencyElement <= (intptr_t) pointer && startCurrencyElement + 2 > (intptr_t) pointer) {
            return pointer_inside_fences;
        }

        if (endCurrencyElement <= (intptr_t) pointer && endCurrencyElement + 2 > (intptr_t) pointer) {
            return pointer_inside_fences;
        }

        current = current->pNext;
    }


    current = heapManagerT->pHead->pNext;

    while (current != heapManagerT->pTail) {

        if (current->isFree) {
            current = current->pNext;
            continue;
        }
        intptr_t startCurrencyElement = (intptr_t) ((uint8_t *) current) + sizeof(struct element) +
                                        3;//dodac trzeba dwa plotki plus przesunąć o 1 bajt w pamieci
        intptr_t endCurrencyElement = startCurrencyElement + current->size;

        if (startCurrencyElement <= (intptr_t) pointer && endCurrencyElement > (intptr_t) pointer) {
            return pointer_inside_data_block;
        }

        current = current->pNext;
    }

    if (checkIfPointerIsAllocated(pointer) == 0) {
        return pointer_unallocated;
    }

    return pointer_valid;

}

int checkIfPointerIsAllocated(const void *const pVoid) {

    struct element *current = heapManagerT->pHead;

    while (current != NULL) {

        if ((intptr_t) current > (intptr_t) pVoid) {
            return 0;
        }

        intptr_t startCurrencyElement = (intptr_t) ((uint8_t *) current + sizeof(struct element) + 2);
        intptr_t endCurrencyElement = startCurrencyElement + current->size;
        //heapShow(heapManagerT);

        if (startCurrencyElement <= (intptr_t) pVoid && endCurrencyElement > (intptr_t) pVoid) {
            if (current->isFree == 0) {
                return 1;
            } else {
                return 0;
            }
        }

        current = current->pNext;
    }

    return 0;
}

int heap_validate(void) {
    if (!heapManagerT) {
        return 2;
    }

    struct element *current = heapManagerT->pHead->pNext;

    while (current != heapManagerT->pTail) {

        if (calculateObjectSum(current) != current->objectSum) {
            return 3;
        }

        uint8_t *startCurrencyElement = ((uint8_t *) current + sizeof(struct element));
        uint8_t *endCurrencyElement = (startCurrencyElement + 2 + current->size);

        for (int i = 0; i < 2; i++) {
            if (*(startCurrencyElement + i) != '#') {
                return 1;
            }
            if (*(endCurrencyElement + i) != '#') {
                return 1;
            }
        }

        current = current->pNext;
    }


    return 0;
}

size_t heap_get_largest_used_block_size(void) {
    if (heap_validate() != 0) {
        return 0;
    }

    struct element *current = heapManagerT->pHead;
    struct element *result = current;

    while (current != NULL) {

        //heapShow(heapManagerT);

        if ((current->size > result->size) && current->isFree == 0) {
            result = current;
        }

        current = current->pNext;
    }

    return result->size;

}

void heap_clean(void) {
    if (!heapManagerT) {
        return;
    }

    size_t heap_size = heapManagerT->sbrkSize;
    memset((void *) heapManagerT, 0, heap_size);
    custom_sbrk(-heap_size);
    heapManagerT = NULL;
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

void setPlotekInBlock(struct element *ptr) {
    if (ptr == NULL || heapManagerT == NULL)
        return;

    if (ptr == heapManagerT->pHead || ptr == heapManagerT->pTail)
        return;


    uint8_t *leftPlotek = ((uint8_t *) ptr + sizeof(struct element));
    uint8_t *rightPlotek = (leftPlotek + 2 + ptr->size);

    for (int i = 0; i < 2; i++) {
        *(leftPlotek + i) = '#';
        *(rightPlotek + i) = '#';

    }
}
