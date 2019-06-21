#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>
#include <inc/queue.h>
#include <inc/types.h>


uint32 kheap_ph[(1024*1024)];
//************************************************************************************
struct k_heap free_list[(KERNEL_HEAP_MAX-KERNEL_HEAP_START)/PAGE_SIZE];// must sorted BY 2 WAY 1) by size to use in best fit
int f_index=-1 ;//counter for free list          // 2) by virtual address to double to space together if they after each other
//int f_calls=0;
//---------------------------------------------------------------------------------------
struct k_heap k_allocated_list[(KERNEL_HEAP_MAX-KERNEL_HEAP_START)/PAGE_SIZE]; //this array hold allocated space in kernel heap and must be updated when allocating and when free
int index = -1 ;
//=====================================================================================
uint32 kheap_va_pa[(KERNEL_HEAP_MAX-KERNEL_HEAP_START)/PAGE_SIZE];

//#########################....[ WORKING ON KERNEL HEAP FREE LIST ]....##############################################
//THIS AREA BELONG TO KERNEL HEAP FREE LIST OPERATIONS...
//--------------------------------------------------------
//first inserting
void insert_in_free_list(void* va,unsigned int size)//belong to free list
{
	f_index++;
	free_list[f_index].va=va;
	free_list[f_index].size=size;

	//update list -> sort
	sort_free_list_by_virtual();
	sort_free_list_by_size();

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//second get best fit
void* get_best_fit(unsigned int size)
{

	if( size>(KERNEL_HEAP_MAX-KERNEL_HEAP_START))
	{
		return (void*)KERNEL_HEAP_MAX;
	}
	else
	{
		  unsigned int i,j,newSize,found=0;
		    void* newVa;
		    void* va;
			for(i=0;i<=f_index;i++)
			{
				if(free_list[i].size>=size)
				{
					found=1;
					va=free_list[i].va;
					//-------------------------
					if(free_list[i].size > size)
					{
						newSize=(free_list[i].size)-size;
						newVa=ROUNDUP(((free_list[i].va)+size),PAGE_SIZE);
						//----------------------------
						for(j=i;j<f_index;j++)
						{
							free_list[j].size=free_list[(j+1)].size;
							free_list[j].va=free_list[(j+1)].va;
						}
						free_list[f_index].va=NULL;
						free_list[f_index].size=0;
						f_index--;
						//------------------------------
						//maybe size available is the best fit but still more than the demanded size
						insert_in_free_list(newVa,newSize);
						break;
					}
					else
					{
						for(j=i;j<f_index;j++)
						{
							free_list[j].size=free_list[(j+1)].size;
							free_list[j].va=free_list[(j+1)].va;
						}
						free_list[f_index].va=NULL;
						free_list[f_index].size=0;
						f_index--;
						break;
					}
				}
			}
			if(found==0)
			{
				return (void*)KERNEL_HEAP_MAX;
			}
			return va;
	}

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//sort free list by size
 void sort_free_list_by_size()//temp bubble sort O(n*n) till implement quick or merge sort
{
	 int i,j;
	 struct k_heap swap;
	 for (i = 0 ; i < f_index; i++)
	  {
	    for (j = 0 ; j < f_index - i ; j++)
	    {
	      if (free_list[j].size >free_list[j+1].size) /* For decreasing order use < */
	      {
	        swap       = free_list[j];
	        free_list[j]   = free_list[j+1];
	        free_list[j+1] = swap;
	      }
	    }
	  }
}
 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 //sort free list by virtual then merge
 void sort_free_list_by_virtual()//temp bubble sort O(n*n) till implement quick or merge sort
 {
	 int i,j;
	 struct k_heap swap;
	 for (i = 0 ; i < f_index; i++)
	  {
		 for (j = 0 ; j < f_index - i ; j++)
		 {
			 if (free_list[j].va >free_list[j+1].va) /* For decreasing order use < */
		      {
		        swap       = free_list[j];
		        free_list[j]   = free_list[j+1];
		        free_list[j+1] = swap;
		      }
		 }
	  }
	 //=================================================================
	 //merge two space after each other
	 for(i=0;i<f_index;i++)
	 {
		 if (ROUNDUP(((free_list[i].va)+(free_list[i].size)),PAGE_SIZE) == free_list[i+1].va)
		 {
			 //merge
			 free_list[i].size+=free_list[i+1].size;
			 //shift elements and decrerment f_index
			 for(j=i+1;j<f_index;j++)
			 {
				 free_list[j].size=free_list[(j+1)].size;
				 free_list[j].va=free_list[(j+1)].va;
			 }
			 free_list[f_index].va=NULL;
			 free_list[f_index].size=0;
			 f_index--;
			 i--;
		 }
	 }
 }

 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 void init_k_heap_free_list()
 {
	 f_index++;
	 free_list[f_index].va=(void*)KERNEL_HEAP_START;
	 free_list[f_index].size=(unsigned int)(KERNEL_HEAP_MAX-KERNEL_HEAP_START);
 }
//#########################....[ END OF WORKING ON KERNEL HEAP FREE LIST ]....#######################################


 //%%%%%%%%%%%%%%%%%%%%%%%%....[ THIS AREA FOR WORKING ON KERNEL HEAP ALLOCATED LIST ]....%%%%%%%%%%%%%%%%%%%%%%%%%%%
//===================================================================================================================
void insert_in_k_allocated_list(void* va,unsigned int size)//belong to allocated list
{
	index++;
	k_allocated_list[index].va=va;
	k_allocated_list[index].size=size;
}
//===================================================================================================================
void print_list()
{
//---------------------------------
	int i=0;
	for(i=0;i<=f_index;i++)
	{
		cprintf("element[%d] va = %x , size = %d\n",(i+1),(free_list[i].va),(free_list[i].size));
	}
	cprintf("==============================================================\n");
}
//===================================================================================================================
//%%%%%%%%%%%%%%%%%%%%%%%%%....[ END OF WORKING ON KERNEL HEAP ALLOCATED LIST ]....%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



//2016: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)
//==================================================================================
static uint32 FirstFree_va_kheep=KERNEL_HEAP_START;//pointer to dynamic allocation in kernel heap this pointer to get first free place to allocate pages
void * kmalloc(unsigned int size)
{
	//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kmalloc()
	// Write your code here, remove the panic and write your code

	//get point to best fit space

	//FirstFree_va_kheep=(uint32)get_best_fit(size);
	//cprintf("best free va = %x\n",FirstFree_va_kheep);
	//print_list();
	size = ROUNDUP(size, PAGE_SIZE);
	uint32 start_alloc=FirstFree_va_kheep;//hold start before allocating to return it after allocate
	//====================================================================================
	if(((start_alloc)<(KERNEL_HEAP_MAX-size)) && (start_alloc)>= (KERNEL_HEAP_START-size) )//check if allocation size start above Kernel heap start and less
		//than kernel heap max
	{
		//NOTE: Allocation is continuous increasing virtual address
		//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)
		//refer to the project documentation for the detailed steps
		struct Frame_Info *frame;//used to allocate frame
		uint32 v_end=FirstFree_va_kheep+size;// calculate max length for allocating

		uint32 index=(FirstFree_va_kheep-KERNEL_HEAP_START)/PAGE_SIZE;
		//----------------------------------------------------------
		for(;FirstFree_va_kheep<v_end;FirstFree_va_kheep+=PAGE_SIZE,index++)//looping start from first free virtual address to max length
		{

			if(allocate_frame(&frame)!=0)//allocate frame if no free frames return and delete pervious frames
			{
				cprintf("NO FREE FRAME AT THE CURRENT MOMENT\n");
				//---------------------------------------------------
				//k9free(FirstFree_va_kheep);//delete the pervious allocated frame if there is no enough memory to continue allocate the size
				//-----------------------------------------------------
				FirstFree_va_kheep=start_alloc;
				return NULL;
			}
			kheap_ph[to_frame_number(frame)]=FirstFree_va_kheep;
			map_frame(ptr_page_directory,frame,(void*)FirstFree_va_kheep,PERM_WRITEABLE|PERM_PRESENT);//map the allocated frame
			//cprintf("addr = %x\n", to_physical_address(frame));
			kheap_va_pa[index]=to_physical_address(frame);

			//----------------------------------------------------------------------------------------------------------------
		}
		//=====================================================================================================
		insert_in_k_allocated_list((void*)start_alloc,size);
		//--------------------------------------------
		return (void*)start_alloc;
	}
	else
	{
		cprintf("The amount of size  is out of Range! \n");
		return NULL;
	}
	//====================================================================================
	//TODO: [PROJECT 2016 - BONUS1] Implement a Kernel allocation strategy
	// Instead of the continuous allocation/deallocation, implement one of
	// the strategies NEXT FIT, BEST FIT, .. etc

	//====================================================================================
	//change this "return" according to your answer
	return NULL;
}
//==================================================================================


//==================================================================================
void kfree(void* va)
{
	if(index>-1)
	{
		//print_list();
			//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kfree()
			// Write your code here, remove the panic and write your code
			unsigned int size=0,i,j,found=0;
			for(i=0;i<=index;i++)
			{
				if(k_allocated_list[i].va==va)
				{
					//--------------------------------------------
					//insert free space to free list
					insert_in_free_list(k_allocated_list[i].va,k_allocated_list[i].size);
					//--------------------------------------------
					found=1;
					size=k_allocated_list[i].size;
					for(j=i;j<index;j++)
					{
						k_allocated_list[j].size=k_allocated_list[(j+1)].size;
						k_allocated_list[j].va=k_allocated_list[(j+1)].va;
					}
					k_allocated_list[index].va=NULL;
					k_allocated_list[index].size=0;
					index--;
					break;
				}
			}

			if(found==1)
			{
				uint32 v_end=(uint32)va+size;
				uint32 v=(uint32)va;

				for(;v<v_end;v+=PAGE_SIZE)
				{
					kheap_va_pa[((v-KERNEL_HEAP_START)/0x1000)]=0;
					uint32 *ptr11;
					struct Frame_Info *frame=get_frame_info(ptr_page_directory,(void*)v,&ptr11);
					kheap_ph[to_frame_number(frame)]=0;
					unmap_frame(ptr_page_directory,(void*)v);
				}
			}
	}
	//print_list();

}
//==================================================================================

//==================================================================================
uint32 kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kheap_virtual_address()

	/*int fram_number=physical_address/PAGE_SIZE;
	uint32 *page_ptr,temp,i=0,j=0,v_address=0;

    for(i=0;i<1024;i++)
	{
    	//outter loop for each pagetable in  DIR table
    	if(get_page_table_form_DIR(ptr_page_directory,i,&page_ptr)!=0)//check if table existence
    	{
    		//INEER LOOP for each page in page table
    		for(j=0;j<1024;j++)
    		{
    			temp=page_ptr[j];//detect frame number connect to this page
    			temp=(temp>>12);//////////////////////////////////////////
    			if(temp == fram_number)
    			{
    				i=i<<22;//get DIR INDEX
    				j=j<<12;//get table index
    				v_address=i|j; //get virtual address DIR
    				//----------------------------------
    				//cprintf("found\n");
    				if(v_address>=KERNEL_HEAP_START && v_address<=KERNEL_HEAP_MAX)
    				{
    				return v_address;
    				}
    				else
    				{
    					return 0;
    				}
    				// break;
    			}
    		}//end of inner loop
    	}// end of table check existence
	}//end of outter loop*/
/*	uint32 v_address=0;
	int i=0;
	int maxsize=((FirstFree_va_kheep-KERNEL_HEAP_START)/0x1000);
	for(;i<maxsize;i++)
	{
		if(kheap_va_pa[i]==physical_address)
			return ((i*0x1000)+KERNEL_HEAP_START);
	}
	return v_address;*/
	return kheap_ph[(physical_address/PAGE_SIZE)];
}
//==================================================================================

//======================helper function=============================================
int get_page_table_form_DIR(uint32 *ptr_page_directory,int index, uint32 **ptr_page_table)
{
	// Fill this function in
	uint32 page_directory_entry = ptr_page_directory[index];

	*ptr_page_table = STATIC_KERNEL_VIRTUAL_ADDRESS(EXTRACT_ADDRESS(page_directory_entry));

	if (page_directory_entry == 0)
	{

			*ptr_page_table = 0;
			return 0;
	}
	return 1;
}
//==================================================================================

//==================================================================================
unsigned int kheap_physical_address(unsigned int virtual_address)
{
	/*//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kheap_physical_address()
	uint32 *page_ptr;
	struct Frame_Info *frame=get_frame_info(ptr_page_directory,(void*)virtual_address,&page_ptr);//get frame by virtual;
	//========================================================================================
	if(frame==NULL)
	{
		//cprintf("there is no frame connected to this virtual address\n");
		return 0;
	}
	//========================================================================================

	return to_physical_address(frame);//return the physical address of target frame.*/

	uint32 i=(virtual_address-KERNEL_HEAP_START)/PAGE_SIZE;
//	cprintf("addr = %x\n", kheap_va_pa[i]);

	return kheap_va_pa[i];
}
//==================================================================================
