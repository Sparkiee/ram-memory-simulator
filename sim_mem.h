//
// Created by Maxim Shteingard on 11/06/2023.
//

#ifndef OS_EX4_SIM_MEM_H
#define OS_EX4_SIM_MEM_H

#include <iostream>
#include <fcntl.h> // for open
#include <unistd.h>
#include <string>
#include <climits>

#define ADDRESS_SIZE 12
#define MEMORY_SIZE 16
extern char main_memory[MEMORY_SIZE];

#define NUM_OF_SEGMENTS 4

#define TEXT_SEGMENT 0
#define DATA_SEGMENT 1
#define BSS_SEGMENT 2
#define HEAP_STACK_SEGMENT 3

using namespace std;

typedef struct page_descriptor {
    bool valid;
    int frame;
    bool dirty;
    int swap_index;
    int last_access; // LRU
} page_descriptor;

class sim_mem {
    int swapfile_fd;
    int program_fd;
    int text_size;
    int data_size;
    int bss_size;

    int heap_stack_size;
    int page_size;

    int clock;
    bool *memoryAllocation;
    int swap_pointer;

    page_descriptor **page_table; // pointer to page table

public:
    sim_mem(const char*, const char*, int, int, int, int, int);
    char load(int address);
    void store(int address, char value);

    void print_memory();
    void print_swap();
    void print_page_table();
    int numOfPages(int);
    int evictPage(int*, int*);
    int exec_read_start_buffer(int segment);
    ~sim_mem();
};


#endif //OS_EX4_SIM_MEM_H
