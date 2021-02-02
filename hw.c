#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


#define VM_ADDR 64
#define VM_PAGE 8

#define PTE_COUNT 8

#define MM_ADDR 32
#define MM_PAGE 4

#define MAX_CHAR 80

bool is_fifo = true;
bool is_lru = false;

// Memory entry for main and disk memory
struct m_entry {
    int page;
    int addr;
    int data;
}; 

// Page Table Entries
struct page_table_entry {
    int entry;
    int valid;
    int dirty;
    int page_num;
};

// struct singe_page {
//     int page_number;
//     int address;
// };


// page table
struct page_table_entry page_table[PTE_COUNT];

// main memory
struct m_entry main_memory[MM_ADDR];

// disk / virtual memory
struct m_entry disk[VM_ADDR];

int oldest_page = 0;
int page_accesses[8];



void initialize(){
    // initialize page table contents
    int i;
    for (i = 0; i < PTE_COUNT; i++) {
        page_table[i].entry = i;        
        page_table[i].page_num = i;
        page_table[i].valid = 0;
        page_table[i].dirty = 0;
    }

    // initialize main memory
    int j;
    int page_count = 0;
    for (j = 0; j < MM_ADDR; j++) {
        if (j % (8) == 0 && j != 0) {
            page_count += 1;
        }
        main_memory[j].page = page_count;
        main_memory[j].addr = j;
        main_memory[j].data = -1;
        
        
    }

    // initialize disk memory
    int k;
    page_count = 0;
    if (k % (8)== 0 && k != 0) {
            page_count += 1;
    }

    for (k = 0; k < VM_ADDR; k++) {
        disk[k].page = page_count;
        disk[k].addr = k;
        disk[k].data = -1;
    }
}

int find_available_page(){
    int i;
    int mark_pages[PTE_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
    
    for (i = 0; i < PTE_COUNT; i++) {
        if (page_table[i].valid == 1){
            mark_pages[page_table[i].page_num] = 1;
        }
    }

    for (i = 0; i < PTE_COUNT; i++){
        if (mark_pages[i] == 0){
            return i;
        }
    }

    return -1;
}

// return address of main memory that corresponds to correct page
int get_mm_addr(int avail_page){
    int i;
    for (i = 0; i < MM_ADDR; i++) {
        if (main_memory[i].page == avail_page) {
            return i;
        }
    }

    // not found
    return -1;
}

int fifo() {
    int victim_page;
//      Find a victim page in main memory
//  Copy victim to disk if it is dirty
//  Copy disk page to victim page
    victim_page = oldest_page++;

    if (victim_page == 3) {
        oldest_page = 0;
    }
    return victim_page;
}

int lru() {
//      Find a victim page in main memory
//  Copy victim to disk if it is dirty
//  Copy disk page to victim page
    int i;
    int victim_page = 0;
    int min_access = 255;
    for (i = 0; i < 8; i++){
        if (page_accesses[i] < min_access){
            victim_page = i;
            min_access = page_accesses[i];
        }
    }

    return victim_page;
}


int main(int argc, char* argv[]) {
    // check for page replacement algorithm
    if (argc == 2){
        if (strcmp(argv[1], "LRU") == 0){
            is_fifo = false;
            is_lru = true;
        }
    }

    initialize();
    
    while (!NULL) {
        char line[MAX_CHAR];
        char* args[13];

        fputs("> ", stdout);
        fgets(line, sizeof(line), stdin);

        int arg_count = 0;
        char* token = strtok(line, " \n");
        while (token != NULL){
            args[arg_count] = token;
            token = strtok(NULL, " \n");
            arg_count++;
        }
        args[arg_count] = NULL;

        if (strcmp(args[0], "quit") == 0){
            break;
        }
        else if (strcmp(args[0], "read") == 0) {
            // Argument is a virtual address
            // • Check if the virtual page is in memory
            // • If the page is in main memory, then read the data
            // from the correct byte in the page
            //  Read from main memory, not disk
            // • If the page is not in main memory then page fault
            // • After page fault, read the data from the correct
            // byte in the page
            int virtual_addr = atoi(args[1]);
            int virtual_page = virtual_addr >> 3;
            int ppage_num = page_table[virtual_page].page_num;
            int offset = virtual_addr % 8;
            int physical_addr = (ppage_num * 8) + offset;
            
            // page is in main memory
            if (page_table[virtual_page].valid == 1){
                printf("%d\n", main_memory[physical_addr].data);
                page_accesses[ppage_num]++;
            }
            // page is on disk
            else if (page_table[virtual_page].valid == 0){
                printf("A Page Fault Has Occurred\n");
                printf("%d\n", disk[physical_addr].data);
                
                int avail_page = find_available_page();
                
                if (avail_page == -1){
                    if (is_fifo){
                        // go to fifo function
                        avail_page = fifo();
                    }

                    else if (is_lru){
                        // go to lru function
                        avail_page = lru();
                    }
                }

                // next available page is found
                int physical_addr = get_mm_addr(avail_page);
                printf("%d AVAILABLE PAGE\n", physical_addr);
                int disk_addr = (virtual_page * 8);
                
                int i;
                int tempData[8];
                if (page_table[avail_page].dirty == 1) {
                    for (i = 0; i < 8; i++) {
                        tempData[i] = main_memory[physical_addr + i].data;
                        main_memory[physical_addr + i].data = disk[disk_addr + i].data;
                        disk[disk_addr + i].data = tempData[i];
                    }
                }
                else {
                    for (i = 0; i < 8; i++) {
                        main_memory[physical_addr + i].data = disk[disk_addr + i].data;
                    }
                }
                    
                page_table[virtual_page].valid = 1;
                page_table[virtual_page].page_num = avail_page;
                page_table[virtual_page].dirty = 0;

                page_accesses[avail_page]++;
            }
        }
        else if (strcmp(args[0], "write") == 0) {
            int virtual_addr = atoi(args[1]);
            int data = atoi(args[2]);
            int virtual_page = virtual_addr >> 3;
            int ppage_num = page_table[virtual_page].page_num;
            int offset = virtual_addr % 8;
            int physical_addr = (ppage_num * 8) + offset;

            // printf("%d VIRTUAL PAGW\n", virtual_page);
            
            // page is in main memory
            if (page_table[virtual_page].valid == 1){
                page_table[virtual_page].dirty = 1;
                main_memory[physical_addr].data = data;
                page_accesses[ppage_num]++;
            }
            // page is on disk
            else if (page_table[virtual_page].valid == 0){
                printf("A Page Fault Has Occurred\n");
                
                int avail_page = find_available_page();
                // printf("%d IHSFJSDNFJSDFJSNJK\n", avail_page);
                
                if (avail_page == -1){
                    if (is_fifo){
                        // go to fifo function
                        avail_page = fifo();
                    }

                    else if (is_lru){
                        // go to lru function
                        avail_page = lru();
                    }
                }

                // next available page is found                
                physical_addr = get_mm_addr(avail_page);
                // printf("%d DIS THE ADDRESS WE START AT \n", physical_addr);
                int disk_addr = (virtual_page * 8);
                
                int i;
                int tempData[8];

                if (page_table[avail_page].dirty == 1) {
                    for (i = 0; i < 8; i++) {
                        tempData[i] = main_memory[physical_addr + i].data;
                        main_memory[physical_addr + i].data = data;
                        disk[disk_addr + i].data = tempData[i];
                    }
                }
                else {
                    main_memory[physical_addr + offset].data = data;
                    page_table[virtual_page].dirty = 1;
                }
                
                page_table[virtual_page].valid = 1;
                page_table[virtual_page].page_num = avail_page;

                page_accesses[avail_page]++;
            }
        }

        else if (strcmp(args[0], "showmain") == 0) {
            int ppn = atoi(args[1]);
            int p_addr = ppn * 8;

            int i;
            for (i = 0; i < 8; i++) {
                printf("%d:%d\n", main_memory[p_addr + i].addr, main_memory[p_addr + i].data);
            }
        }
        else if (strcmp(args[0], "showdisk") == 0) {
            int dpn = atoi(args[1]);
            int d_addr = dpn * 8;

            int i;
            for (i = 0; i < 8; i++) {
                printf("%d: %d\n", disk[d_addr + i].addr, disk[d_addr + i].data);
            }
        }
        else if (strcmp(args[0], "showptable") == 0) {
            int i ;
            for (i = 0; i < PTE_COUNT; i++){
                printf("%d:%d:%d:%d\n", page_table[i].entry, page_table[i].valid, page_table[i].dirty, page_table[i].page_num);
            }
            
        }
                
    }
    return 0;
}
