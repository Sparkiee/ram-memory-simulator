# Ram Memory Simulator
Authored by Maxim Shteingard
209171156

== Description ==
	The provided code manages a simplified model of a computer's memory system, implementing a basic paging system
	with a Least Recently Used (LRU) page replacement algorithm. It divides memory into fixed-size pages and maps
	these to physical addresses using a page table.

	    Page Table:
	        The page table is a two-dimensional array page_table[out][in] that keeps track of the mapping
	        from virtual to physical memory. Each page entry contains a boolean valid flag indicating whether
	        the page is in main memory or not, an integer frame that represents the index in main memory where
	        the page is stored, an integer last_access that keeps track of the last time the page was accessed,
	        and an integer swap_index that keeps track of the location in the swap file if the page has been moved
	        to swap.

        Paging:
            When a page is requested that isn't in main memory (a page fault), the code checks if there is available
             memory. If memory is full, it uses the LRU page replacement algorithm to decide which page to evict.
             The evictPage() function identifies the page that was least recently used and moves it to
             the swap file if necessary.

        Swapping:
            If a page is moved out of main memory to make room for a new page, the move_to_swap() function moves
            the page to the swap file and updates the page table. Pages are written back to main memory from the
            swap file if they are requested and not in main memory.

        Address Translation:
            The parseAddress() function takes a virtual address and splits it into an offset and two indices in and
            out for the page table.

        Loading and Storing:
            The load() and store() functions are used to read and write to memory, respectively.
            Both functions handle page faults by loading pages into memory as needed and updating the page table
            and clock.

        Clock:
            The system uses a global variable clock to keep track of the passage of "time" for the LRU algorithm.
            Each time a page is loaded or stored, the clock is incremented.

        Segmentation:
            The memory is divided into three segments: TEXT_SEGMENT, DATA_SEGMENT, BSS_SEGMENT, and HEAP_STACK_SEGMENT.

    This project provides a basic simulation of how operating systems handle memory management,
    page faults, and swapping. It can be used as a foundation for further exploration into memory management
    techniques and operating system design.

==input==:
	    load(int address): This method reads the byte at the given virtual address.
	        The address is an integer representing the virtual memory address to be read.

        store(int address, char value):
            This method writes the byte value to the given virtual address. The address is an integer representing
            the virtual memory address to be written, and value is the byte (character) to be written to this
            location.

==output==:
        The program displays how would the physical memory, the swap and the page table look like at each
        step of the way.


==How to compile & run==:
	Type: make
  Make sure to add 'exec_file' into the cmake-build-debug if you're running the code in CLion
