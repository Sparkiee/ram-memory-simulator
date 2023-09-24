#include "sim_mem.h"

// a function which checks if there's an available frame in the ram which isn't used (using the memory array)
int isMemoryAvailable(bool* memoryAllocation, int size) {
    for (int i = 0; i < size; ++i) {
        if(memoryAllocation[i] == false)
            return i;
    }
    // If we haven't returned by now, there aren't enough free pages
    return -1;
}

// moves to swap in the next swap point, once we get to the maximum pointer, we reset the pointer
// back to the beginning
void move_to_swap(int swapfile_fd, int* swap_pointer, page_descriptor** page_table,
                  int out, int in,
                  int page_size, int bss_size, int data_size, int heap_stack_size)
{
    if(*swap_pointer >= (bss_size + data_size + heap_stack_size)/page_size)
        *swap_pointer = 0;
    lseek(swapfile_fd, (*swap_pointer) * page_size, SEEK_SET);
    write(swapfile_fd, &main_memory[page_table[out][in].frame * page_size], page_size);
    page_table[out][in].swap_index = *swap_pointer;
    page_table[out][in].valid = false;
    (*swap_pointer)++;
}

// parses the address we receive into segment, page, offset
void parseAddress(int address, int size, int* offset, int* in, int* out)
{
    bitset<ADDRESS_SIZE> binaryNumber(address);
    string binary = binaryNumber.to_string();

    // grabbing the first two most significant digits from the address string
    // converting the string to an int and converting from binary to decimal
    bitset<ADDRESS_SIZE> BinaryIn(binary.substr(0, 2));
    *out = BinaryIn.to_ulong();

    int log2result = log(size) / log(2);

    bitset<ADDRESS_SIZE> BinaryOffset(binary.substr(12-log2result, ADDRESS_SIZE));
    *offset = BinaryOffset.to_ulong() % size;

    bitset<ADDRESS_SIZE> BinaryOut(binary.substr(2, 12));
    *in = BinaryOut.to_ulong() / size;
}

// buffer to know from where to read in exec in exec
// calculates the start gap of each segment
int sim_mem::exec_read_start_buffer(int segment)
{
    switch (segment) {
        case TEXT_SEGMENT:
            return 0;
        case DATA_SEGMENT:
            return text_size;
        case BSS_SEGMENT:
            return text_size + data_size;
        case HEAP_STACK_SEGMENT:
            return text_size + data_size + bss_size;
        default:
            return -1;
    }
    return -1;
}

// returns the number of pages in each segment
int sim_mem::numOfPages(int segment) {
    switch (segment) {
        case TEXT_SEGMENT: return text_size/page_size;
        case DATA_SEGMENT: return data_size/page_size;
        case BSS_SEGMENT: return bss_size/page_size;
        case HEAP_STACK_SEGMENT: return heap_stack_size/page_size;
        default: return -1;
    }
    return -1;
}

// checks which is the least recently used page and switches it up and returns its info
int sim_mem::evictPage(int* outter, int* inner) {
    int oldestPage = INT_MAX;
    int oldestSeg = INT_MAX;
    int oldestAccess = INT_MAX;

     // calculate the number of pages in this segment
    for(int seg = 0; seg < 4; seg++)
    {
        int num_pages = numOfPages(seg);
        for(int page = 0; page < num_pages; page++) {
            if(page_table[seg][page].last_access < oldestAccess && page_table[seg][page].frame != -1) {
                oldestPage = page;
                oldestSeg = seg;
                oldestAccess = page_table[seg][page].last_access;
            }
        }
    }
    *outter = oldestSeg;
    *inner = oldestPage;
    return page_table[*outter][*inner].frame;
}

// constructor
sim_mem::sim_mem(const char* exe_file_name, const char* swap_file_name, int text_size, int data_size, int bss_size, int heap_stack_size, int page_size) {
    //flushing all streams just in case
    fflush(NULL);


    this->text_size = text_size;
    this->data_size = data_size;
    this->bss_size = bss_size;
    this->heap_stack_size = heap_stack_size;
    this->page_size = page_size;

    for (int i = 0; i < MEMORY_SIZE; ++i) {
        main_memory[i] = '0';
    }

    if(exe_file_name == NULL)
    {
        cout << "ERR" << endl;
        exit(1);
    }


    this->program_fd = open(exe_file_name, O_RDONLY);

    if (this->program_fd == -1) {
        cout << "ERR";
        exit(1); // terminate with error
    }

    if(swap_file_name == NULL)
    {
        cout << "ERR" << endl;
        exit(1);
    }

    int swapLength = bss_size + data_size + heap_stack_size;


    // Open the file, truncating any existing content
    this->swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT | O_TRUNC, 0666);  // Read/write permissions for owner, read permissions for others
    if (this->swapfile_fd < 0) {
        perror("ERR");
        exit(1);
    }


    char zero = '0';
    // writing the 'zeros' into the file in the length of swap (bss + data + heap stack)
    while(swapLength > 0)
    {
        if (write(this->swapfile_fd, &zero, 1) != 1) {
            perror("ERR");
            exit(1);
        }
        swapLength--;
    }
    // initiating page_table variable
    this->page_table = new page_descriptor * [NUM_OF_SEGMENTS];
    this->page_table[TEXT_SEGMENT] = new page_descriptor[text_size/page_size];
    for (int i = 0; i < text_size/page_size; ++i) {
        page_table[TEXT_SEGMENT][i].valid = false;
        page_table[TEXT_SEGMENT][i].frame = -1;
        page_table[TEXT_SEGMENT][i].dirty = false;
        page_table[TEXT_SEGMENT][i].swap_index = -1;
        page_table[TEXT_SEGMENT][i].last_access = 0;
    }
    this->page_table[DATA_SEGMENT] = new page_descriptor[data_size/page_size];
    for (int i = 0; i < data_size/page_size; ++i) {
        page_table[DATA_SEGMENT][i].valid = false;
        page_table[DATA_SEGMENT][i].frame = -1;
        page_table[DATA_SEGMENT][i].dirty = false;
        page_table[DATA_SEGMENT][i].swap_index = -1;
        page_table[DATA_SEGMENT][i].last_access = 0;
    }
    this->page_table[BSS_SEGMENT] = new page_descriptor[bss_size/page_size];
    for (int i = 0; i < bss_size/page_size; ++i) {
        page_table[BSS_SEGMENT][i].valid = false;
        page_table[BSS_SEGMENT][i].frame = -1;
        page_table[BSS_SEGMENT][i].dirty = false;
        page_table[BSS_SEGMENT][i].swap_index = -1;
        page_table[BSS_SEGMENT][i].last_access = 0;
    }
    this->page_table[HEAP_STACK_SEGMENT] = new page_descriptor[heap_stack_size/page_size];
    for (int i = 0; i < heap_stack_size/page_size; ++i) {
        page_table[HEAP_STACK_SEGMENT][i].valid = false;
        page_table[HEAP_STACK_SEGMENT][i].frame = -1;
        page_table[HEAP_STACK_SEGMENT][i].dirty = false;
        page_table[HEAP_STACK_SEGMENT][i].swap_index = -1;
        page_table[HEAP_STACK_SEGMENT][i].last_access = 0;
    }

    this->memoryAllocation = new bool[MEMORY_SIZE/page_size];
    // Initialize the array
    for (int i = 0; i < MEMORY_SIZE / page_size; ++i) {
        memoryAllocation[i] = false; // false indicates no page is currently occupying this slot
    }

    this->swap_pointer = 0;
    this->clock = 0;
}


char sim_mem::load(int address) {
    // if negative address
    if(address < 0)
    {
        cout << "ERR" << endl;
        return '\0';
    }
    int offset, in, out;
    // out is external access in the page descriptor, in is internal
    parseAddress(address, page_size, &offset, &in, &out);

    // if the page we are trying to load is valid (inside the memory), we just return the character
    if(page_table[out][in].valid) {
        page_table[out][in].last_access = clock++;
        return main_memory[(page_table[out][in].frame)*page_size + offset];
    }
    else // page not in the memory
    {
        // if we have space in the memory
        int mem_slot = isMemoryAvailable(this->memoryAllocation, MEMORY_SIZE/page_size);
        // if we are in the text segment, we load it from exec as there's no reason to save it up in swap
        if(out == TEXT_SEGMENT) // page is text and then its in exec
        {
            // if there's no space in memory
            if(mem_slot == -1)
            {
                // we check use which page to evict
                int outter = 0, inner = 0;
                int mem_pos = evictPage(&outter, &inner);
                // we check if it's valid and dirty we need to send it to the swap to back it up
                if(this->page_table[outter][inner].valid && page_table[outter][inner].dirty && outter != TEXT_SEGMENT)
                {
                    move_to_swap(this->swapfile_fd, &this->swap_pointer, this->page_table,
                                 outter, inner, page_size, bss_size, data_size, heap_stack_size);
                    page_table[outter][inner].valid = false;
                    memoryAllocation[page_table[outter][inner].frame] = false;
                    page_table[outter][inner].frame = -1;

                    mem_slot = mem_pos;
                }
                else // no need to send to swap, we just load up the page and use it's frame
                {
                    mem_slot = page_table[outter][inner].frame;
                    page_table[outter][inner].valid = false;
                    page_table[outter][inner].frame = -1;
                    memoryAllocation[page_table[outter][inner].frame] = false;
                }
            }
            // since we are in text segment we load it up from the exec_file
            lseek(this->program_fd, in * page_size, SEEK_SET); //set the cursor at the beginning
            int read_result = read(this->program_fd, &main_memory[mem_slot *page_size], page_size);
            if(read_result == -1) {
                cout << "ERR" << endl;
                return '\0';
            }
            page_table[out][in].valid = true;
            page_table[out][in].frame = mem_slot;
            page_table[out][in].last_access = clock++;
            memoryAllocation[page_table[out][in].frame] = true;
            return main_memory[(page_table[out][in].frame)*page_size + offset];

        }
        else //not in text segment
        {
            // if dirty
            if (this->page_table[out][in].dirty) // dirty means the page is in the swap
            {
                // making a buffer to update the swap with 0s
                char buffer[this->page_size];
                for (int i = 0; i < this->page_size; ++i) {
                    buffer[i] = '0';
                }
                if(mem_slot == -1) //there's no space in the memory
                {
                    // grabbing the page we need to evict
                    int outter = 0, inner = 0;
                    int mem_pos = evictPage(&outter, &inner);
                    // we need to back it up in the swap if it's not a text and it's dirty and valid
                    if(this->page_table[outter][inner].valid && page_table[outter][inner].dirty && outter != TEXT_SEGMENT)
                    {
                        move_to_swap(this->swapfile_fd, &this->swap_pointer, this->page_table,
                                     outter, inner, page_size, bss_size, data_size, heap_stack_size);
                        page_table[outter][inner].valid = false;
                        memoryAllocation[page_table[outter][inner].frame] = false;
                        page_table[outter][inner].frame = -1;
                        mem_slot = mem_pos;
                    }
                    else // no need to send to swap
                    {
                        mem_slot = page_table[outter][inner].frame;
                        page_table[outter][inner].valid = false;
                        page_table[outter][inner].frame = -1;
                        memoryAllocation[page_table[outter][inner].frame] = false;
                    }
                }
                // loading from swap to main memory
                lseek(swapfile_fd, page_table[out][in].swap_index * page_size, SEEK_SET);
                int read_result = read(this->swapfile_fd, &main_memory[mem_slot * page_size], page_size);
                if(read_result == -1) {
                    cout << "ERR" << endl;
                    return '\0';
                }

                // filling the swap with zeros
                lseek(swapfile_fd, page_table[out][in].swap_index * page_size, SEEK_SET);
                int write_result = write(this->swapfile_fd, buffer, page_size);
                if(write_result == -1) { // error writing
                    cout << "ERR" << endl;
                    return '\0';
                }
                page_table[out][in].valid = true;
                page_table[out][in].frame = mem_slot;
                page_table[out][in].last_access = clock++;
                memoryAllocation[page_table[out][in].frame] = true;
                // double check once again the two below
                page_table[out][in].swap_index = -1;
                return main_memory[(page_table[out][in].frame)*page_size + offset];
            }
            else // not dirty = not in swap so we read from exec
            {
                // trying to load from heap_stack, err
                if(out == HEAP_STACK_SEGMENT) // trying to load from heap stack
                {
                    cout << "ERR" << endl;
                    return '\0';
                }
                // if there's no space in the main memory
                if(mem_slot == -1)
                {
                    // we look for which page to evict again
                    int outter = 0, inner = 0;
                    int mem_pos = evictPage(&outter, &inner);
                    // we need to back it up in the swap
                    if(this->page_table[outter][inner].valid && page_table[outter][inner].dirty && outter != TEXT_SEGMENT)
                    {
                        move_to_swap(this->swapfile_fd, &this->swap_pointer, this->page_table,
                                     outter, inner, page_size, bss_size, data_size, heap_stack_size);
                        page_table[outter][inner].valid = false;
                        memoryAllocation[page_table[outter][inner].frame] = false;
                        page_table[outter][inner].frame = -1;

                        mem_slot = mem_pos;
                    }
                    else // no need to send to swap
                    {
                        mem_slot = page_table[outter][inner].frame;
                        page_table[outter][inner].valid = false;
                        page_table[outter][inner].frame = -1;
                        memoryAllocation[page_table[outter][inner].frame] = false;
                    }
                }
                // grabbing it from exec file
                int start_buff = exec_read_start_buffer(out);
                lseek(this->program_fd, start_buff + in * page_size, SEEK_SET); //set the cursor at the beginning
                int read_result = read(this->program_fd, &main_memory[mem_slot * page_size], page_size);
                if(read_result == -1) {
                    cout << "ERR" << endl;
                    return '\0';
                }
                page_table[out][in].valid = true;
                page_table[out][in].frame = mem_slot;
                page_table[out][in].last_access = clock++;
                memoryAllocation[page_table[out][in].frame] = true;
                return main_memory[(page_table[out][in].frame)*page_size + offset];

            }
        }
    }
}

void sim_mem::store(int address, char value) {
    int offset, in, out;
    // out is external access in the page descriptor, in is internal
    parseAddress(address, page_size, &offset, &in, &out);

    // if the page we are trying to load is valid, we just return the character
    //if we are trying to store in text segment, we throw an error
    if(out == TEXT_SEGMENT)
    {
        cout << "ERR" << endl;
        return;
    }
    // if page is valid, we just update it
    if(page_table[out][in].valid) {
        page_table[out][in].last_access = clock++;
        main_memory[(page_table[out][in].frame)*page_size + offset] = value;
        page_table[out][in].dirty = true;
    }
    else
    {
        // if we are not in text segment
        if((out != TEXT_SEGMENT)) // the other segments
        {
            // checking if there's a spot in the ram
            int mem_slot = isMemoryAvailable(this->memoryAllocation,MEMORY_SIZE/page_size);
            if(!page_table[out][in].dirty) // if page is not dirty,
            {
                // if there's no space in the main memory
                if(mem_slot == -1) {
                    int outter = 0, inner = 0;
                    int mem_pos = evictPage(&outter, &inner);
                    // we don't need to send text to swap, we just take it from the exec
                    if (this->page_table[outter][inner].valid && page_table[outter][inner].dirty &&
                        outter != TEXT_SEGMENT) {
                        move_to_swap(this->swapfile_fd, &this->swap_pointer, this->page_table,
                                     outter, inner, page_size, bss_size, data_size, heap_stack_size);
                        page_table[outter][inner].valid = false;
                        memoryAllocation[page_table[outter][inner].frame] = false;
                        page_table[outter][inner].frame = -1;
                        mem_slot = mem_pos;
                    } else // no need to send to swap
                    {
                        mem_slot = page_table[outter][inner].frame;
                        page_table[outter][inner].valid = false;
                        page_table[outter][inner].frame = -1;
                        memoryAllocation[page_table[outter][inner].frame] = false;
                    }
                }
                // if we are in the heap stack segment, we first load a blank page (0s)
                if(out == HEAP_STACK_SEGMENT || out == BSS_SEGMENT) // if we are in heap_stack segment
                {
                    // we initiate a new page of zero
                    for (int i = 0; i < page_size; ++i) {
                        main_memory[mem_slot*page_size + i] = '0';
                    }
                }
                else
                {
                    // if we are in data, we load it up from exec_file
                    int start_buff = exec_read_start_buffer(out);
                    lseek(this->program_fd, start_buff + in * page_size, SEEK_SET); //set the cursor at the beginning
                    int read_result = read(this->program_fd, &main_memory[mem_slot * page_size], page_size);
                    if(read_result == -1) {
                        cout << "ERR" << endl;
                        return;
                    }
                }
                page_table[out][in].valid = true;
                page_table[out][in].frame = mem_slot;
                page_table[out][in].dirty = true;
                page_table[out][in].last_access = clock++;
                memoryAllocation[page_table[out][in].frame] = true;
                main_memory[mem_slot*page_size + offset] = value;

            }
            else // if page is dirty
            {
                char buffer[this->page_size];
                for (int i = 0; i < this->page_size; ++i) {
                    buffer[i] = '0';
                }

                if(mem_slot == -1) //there's no space in main memory
                {
                    // if there's no space in the main memory
                    int outter = 0, inner = 0;
                    int mem_pos = evictPage(&outter, &inner);
                    // if we need to back it up in the swap
                    if(this->page_table[outter][inner].valid && page_table[outter][inner].dirty && outter != TEXT_SEGMENT)
                    {
                        move_to_swap(this->swapfile_fd, &this->swap_pointer, this->page_table,
                                     outter, inner, page_size, bss_size, data_size, heap_stack_size);
                        page_table[outter][inner].valid = false;
                        memoryAllocation[page_table[outter][inner].frame] = false;
                        page_table[outter][inner].frame = -1;

                        mem_slot = mem_pos;
                    }
                    else // no need to send to swap
                    {
                        mem_slot = page_table[outter][inner].frame;
                        page_table[outter][inner].valid = false;
                        page_table[outter][inner].frame = -1;
                        memoryAllocation[page_table[outter][inner].frame] = false;
                    }

                }

                // loading from swap to main memory
                lseek(swapfile_fd, page_table[out][in].swap_index * page_size, SEEK_SET);
                int read_result = read(this->swapfile_fd, &main_memory[mem_slot * page_size], page_size);
                if(read_result == -1) {
                    cout << "ERR" << endl;
                    return;
                }
                // writing zeros into the swap
                lseek(swapfile_fd, page_table[out][in].swap_index * page_size, SEEK_SET);
                int write_result = write(this->swapfile_fd, buffer, page_size);
                if(write_result == -1) { // error writing
                    cout << "ERR" << endl;
                    return;
                }
                page_table[out][in].valid = true;
                page_table[out][in].dirty = true;
                page_table[out][in].frame = mem_slot;
                page_table[out][in].last_access = clock++;
                memoryAllocation[page_table[out][in].frame] = true;
                // double check once again the two below
                page_table[out][in].swap_index = -1;
                main_memory[(page_table[out][in].frame)*page_size + offset] = value;
            }
        }
    }
}

void sim_mem::print_memory() {
    printf("\n Physical memory:\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}

void sim_mem::print_swap() {
    char* str = (char*)malloc(this->page_size * sizeof(char));
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while(read(swapfile_fd, str, this->page_size) == this->page_size) {
        for (int i = 0; i < page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
}

void sim_mem::print_page_table() {
    int i;
    int num_of_txt_pages = text_size / page_size;
    int num_of_data_pages = data_size / page_size;
    int num_of_bss_pages = bss_size / page_size;
    int num_of_stack_heap_pages = heap_stack_size / page_size;
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for(i = 0; i < num_of_txt_pages; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[0][i].valid,
               page_table[0][i].dirty,
               page_table[0][i].frame ,
               page_table[0][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for(i = 0; i < num_of_data_pages; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[1][i].valid,
               page_table[1][i].dirty,
               page_table[1][i].frame ,
               page_table[1][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for(i = 0; i < num_of_bss_pages; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[2][i].valid,
               page_table[2][i].dirty,
               page_table[2][i].frame ,
               page_table[2][i].swap_index);
    }
    printf("Valid\t Dirty\t Frame\t Swap index\n");
    for(i = 0; i < num_of_stack_heap_pages; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[3][i].valid,
               page_table[3][i].dirty,
               page_table[3][i].frame ,
               page_table[3][i].swap_index);
    }
}

sim_mem::~sim_mem() {
    for (int i = 0; i < NUM_OF_SEGMENTS; i++) {
        delete [] page_table[i];
    }

    delete [] page_table;
    delete[] memoryAllocation;

    close(this->swapfile_fd);
    close(this->program_fd);
}

