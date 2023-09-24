# RAM Memory Simulator
**Author:** Maxim Shteingard  

---

## Description

The provided code offers a simulation of a computer's memory system, incorporating a basic paging system with a Least Recently Used (LRU) page replacement algorithm. The memory is split into fixed-sized pages, and these are mapped to physical addresses using a page table.

### Page Table

The page table is a two-dimensional array `page_table[out][in]` that maintains the mapping from virtual to physical memory. Each page entry contains:
- A boolean `valid` flag indicating the page's presence in main memory.
- An integer `frame` showing the index in main memory where the page resides.
- An integer `last_access` recording the last time the page was accessed.
- An integer `swap_index` pointing to the location in the swap file if the page has been moved to swap.

### Paging

When a non-resident page in main memory is requested (page fault), the code checks for available memory. If memory is saturated, the LRU page replacement algorithm determines which page to evict. The `evictPage()` function pinpoints the least recently used page, transferring it to the swap file if needed.

### Swapping

The `move_to_swap()` function transfers a page to the swap file if evicted from main memory for a new page and subsequently updates the page table. Pages are retrieved back to main memory from the swap file when requested and absent in main memory.

### Address Translation

The `parseAddress()` function accepts a virtual address and decomposes it into an offset and two indices (`in` and `out`) for the page table.

### Loading and Storing

The `load()` and `store()` functions are utilized for memory reading and writing, respectively. They handle page faults, load required pages into memory, and refresh the page table and clock.

### Clock

The system adopts a global variable `clock` to monitor the "time" progression for the LRU algorithm. Every page load or store increments the clock.

### Segmentation

Memory is segmented into: 
- `TEXT_SEGMENT`
- `DATA_SEGMENT`
- `BSS_SEGMENT`
- `HEAP_STACK_SEGMENT`

This simulation offers foundational insights into OS memory management, page faults, and swapping, serving as a bedrock for deeper exploration into memory management strategies and OS designs.

---

## Input

- `load(int address)`: Reads the byte at the provided virtual address, where `address` denotes the virtual memory address to be read.

- `store(int address, char value)`: Writes the byte value to the specified virtual address. Here, `address` signifies the virtual memory address to be written, and `value` is the byte (character) inscribed at this location.

---

## Output

The program vividly demonstrates the state of physical memory, the swap, and the page table at every step.

---

## Compilation & Execution

To compile:

\```bash
make
\```

**Note**: Remember to insert `exec_file` into the `cmake-build-debug` if executing the code in CLion.
