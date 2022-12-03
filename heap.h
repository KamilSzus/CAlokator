//
// Created by kamil on 30.11.2022.
//

#ifndef SO2_HEAP_H
#define SO2_HEAP_H

#include <stddef.h>
#include <unistd.h>
#include <stdint.h>
#include <memory.h>
#include "custom_unistd.h"

enum pointer_type_t {
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

struct heap_t;
struct element;

int heap_setup(void);

void *heap_malloc(size_t size);

void *heap_calloc(size_t number, size_t size);

void *heap_realloc(void *memblock, size_t count);

void heap_free(void *memblock);

enum pointer_type_t get_pointer_type(const void *const pointer);

int heap_validate(void);

size_t heap_get_largest_used_block_size(void);

void heap_clean(void);

int calculateObjectSum(struct element *ptr);

struct element *findFirstFreeElement(size_t size);

int heapExpand(size_t size);

void heap_show(const struct heap_t *pheap);

void setPlotekInBlock(struct element *ptr);

#endif //SO2_HEAP_H
