#ifndef FOS_KERN_KHEAP_H_
#define FOS_KERN_KHEAP_H_
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif
//====================================================================================



struct k_heap
{
	void* va;
	unsigned int size;
};

//#################free list functions######################################
void insert_in_free_list(void* va,unsigned int size);
void* get_best_fit(unsigned int size);
void sort_free_list_by_size();
void sort_free_list_by_virtual();
void init_k_heap_free_list();
//void kMSort(struct k_heap** A, int end, int start);
//void kMerge(int* A, int p, int q, int r);
//##########################################################################


void intialize_kheaplist();
void insert_in_k_h_list(void* va,unsigned int size);
void print_list();
//----------------------------------------------
void* kmalloc(unsigned int size);
void kfree(void* virtual_address);

unsigned int kheap_virtual_address(unsigned int physical_address);
unsigned int kheap_physical_address(unsigned int virtual_address);
int get_page_table_form_DIR(uint32 *ptr_page_directory,int index, uint32 **ptr_page_table);


#endif // FOS_KERN_KHEAP_H_
