// Hana Lee 40847074
// Rebecca Huang 42285382

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

// page table
struct page_table_entry page_table[PTE_COUNT];

// main memory
struct m_entry main_memory[MM_ADDR];

// disk / virtual memory
struct m_entry disk[VM_ADDR];

int tracker_index = 0;
int page_tracker[4] = {-1, -1, -1, -1};


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
    for (k = 0; k < VM_ADDR; k++) {
        if (k % (8)== 0 && k != 0) {
            page_count += 1;
        }
        disk[k].page = page_count;
        disk[k].addr = k;
        disk[k].data = -1;
    }
}

int find_available_page(){
    int i;
    int mark_memory[4] = {0, 0, 0, 0};
    
    for (i = 0; i < 8; i++) {
        if (page_table[i].valid == 1){
            mark_memory[page_table[i].page_num] = 1;
        }
    }

    for (i = 0; i < 4; i++){
        if (mark_memory[i] == 0){
            return i;
        }
    }

    return -1;
}

int fifo() {
    int victim_page = tracker_index++;

    if (victim_page == 3){
        tracker_index = 0;
    }
    
    return victim_page;
}

int lru() {
    int i;
    int victim_page;
    int max_count = -1;
    for (i = 0; i < 4; i++){
        if (page_tracker[i] > max_count && page_tracker[i] != -1){
            victim_page = i;
            max_count = page_tracker[i];
        }
    }

    return victim_page;
}

void increment_page_tracker(int num){
    int i;
    for (i = 0; i < 4; i++){
        if (page_tracker[i] != -1){
            page_tracker[i]++;
        }
    }
    if (page_tracker[num] == -1){
        page_tracker[num]++;
    }
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
        // parse through input
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
            int virtual_addr = atoi(args[1]);
            int virtual_page = virtual_addr >> 3;
            int ppage_num = page_table[virtual_page].page_num;
            int offset = virtual_addr % 8;
            int physical_addr = (ppage_num * 8) + offset;
            
            // page is in main memory
            if (page_table[virtual_page].valid == 1){
                printf("%d\n", main_memory[physical_addr].data);
                
                page_tracker[ppage_num] = -1;
                increment_page_tracker(ppage_num);
            }
            // page is on disk
            else if (page_table[virtual_page].valid == 0){
                printf("A Page Fault Has Occurred\n");
                
                // find next available page
                int avail_page = find_available_page();
                
                int disk_page = virtual_page;

                if (avail_page == -1){
                    if (is_fifo){
                        // go to fifo function
                        avail_page = fifo();
                    }

                    else if (is_lru){
                        // go to lru function
                        avail_page = lru();
                    }
                    
                    // if its dirty, new disk location
                    int i;
                    for (i = 0; i < 8; i++){
                        if (page_table[i].page_num == avail_page && page_table[i].valid == 1){
                            disk_page = i;
                            break;
                        }
                    }
                }

                int disk_addr = (disk_page * 8);
                int physical_addr = (avail_page * 8);

                int tempData[8];
                int i;

                // if dirty in disk
                if (page_table[disk_page].dirty == 1) {
                    for (i = 0; i < 8; i++) {
                        tempData[i] = main_memory[physical_addr + i].data;
                        main_memory[physical_addr + i].data = disk[disk_addr + i].data;
                        disk[disk_addr + i].data = tempData[i];
                    }
                    
                }
                // not dirty, so no swapping
                else {
                    for (i = 0; i < 8; i++) {
                        main_memory[physical_addr + i].data = disk[disk_addr + i].data;
                    }
                }

                // reset dirty entry
                page_table[disk_page].valid = 0;
                page_table[disk_page].dirty = 0;
                page_table[disk_page].page_num = disk_page;

                page_table[virtual_page].valid = 1;
                page_table[virtual_page].dirty = 0;
                page_table[virtual_page].page_num = avail_page;
                    
                printf("%d\n", main_memory[physical_addr + offset].data);
                
                page_tracker[avail_page] = -1;
                increment_page_tracker(avail_page);
            }
        }
        else if (strcmp(args[0], "write") == 0) {
            int virtual_addr = atoi(args[1]);
            int data = atoi(args[2]);
            int virtual_page = virtual_addr >> 3;
            int ppage_num = page_table[virtual_page].page_num;
            int offset = virtual_addr % 8;
            int physical_addr = (ppage_num * 8) + offset;

            
            // page is in main memory
            if (page_table[virtual_page].valid == 1){
                page_table[virtual_page].dirty = 1;
                main_memory[physical_addr].data = data;
                
                page_tracker[ppage_num] = -1;
                increment_page_tracker(ppage_num);
            }
            // page is on disk
            else if (page_table[virtual_page].valid == 0){
                printf("A Page Fault Has Occurred\n");
                
                int avail_page = find_available_page();
                int disk_page = virtual_page;
                
                if (avail_page == -1){
                    if (is_fifo){
                        // go to fifo function
                        avail_page = fifo();
                    }

                    else if (is_lru){
                        // go to lru function
                        avail_page = lru();
                    }

                    // if its dirty, new disk location
                    int i;
                    for (i = 0; i < 8; i++){
                        if (page_table[i].page_num == avail_page && page_table[i].valid == 1){
                            disk_page = i;
                            break;
                        }
                    }
                }

                // next available page is found                
                int disk_addr = (disk_page * 8);
                int physical_addr = (avail_page * 8);
                
                int tempData[8];
                int i;
                if (page_table[disk_page].dirty == 1) {
                    for (i = 0; i < 8; i++) {
                        tempData[i] = main_memory[physical_addr + i].data;
                        main_memory[physical_addr + i].data = disk[disk_addr + i].data;
                        disk[disk_addr + i].data = tempData[i];
                    }
                }

                // write into main memory after swapping
                main_memory[physical_addr + offset].data = data;

                // reset dirty entry
                page_table[disk_page].valid = 0;
                page_table[disk_page].dirty = 0;
                page_table[disk_page].page_num = disk_page;

                page_table[virtual_page].dirty = 1;
                page_table[virtual_page].valid = 1;
                page_table[virtual_page].page_num = avail_page;
                
                page_tracker[avail_page] = -1;
                increment_page_tracker(avail_page);
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
