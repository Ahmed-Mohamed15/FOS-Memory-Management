#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//2016: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

struct heapInfo{
	int numberOfPages;
	bool used;
};

struct PhysIndx{
	uint32 virtAdd;
};

bool PhsOne=0;
uint32 heapInfoIndex = 0;
uint32 startAddress = KERNEL_HEAP_START;
struct heapInfo heapInfoArr[(KERNEL_HEAP_MAX - KERNEL_HEAP_START)/PAGE_SIZE];
struct PhysIndx PhysArr[(uint32)(KERNEL_HEAP_MAX - KERNEL_BASE)/PAGE_SIZE];

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kmalloc()
	heapInfoArr[heapInfoIndex].numberOfPages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	heapInfoArr[heapInfoIndex].used=1;
	if((KERNEL_HEAP_MAX - startAddress) > size){
		uint32 returnAddress = startAddress;
		int i;
		for(i = 0; i < heapInfoArr[heapInfoIndex].numberOfPages; i++, startAddress+=PAGE_SIZE){
			uint32 *ptr_table = NULL;
			struct Frame_Info* ptr_frame_info = get_frame_info(ptr_page_directory, (void*)startAddress, &ptr_table);
			allocate_frame(&ptr_frame_info) ;
			map_frame(ptr_page_directory, ptr_frame_info, (void*)startAddress, PERM_WRITEABLE);
			uint32 PhysTmp=kheap_physical_address(startAddress);
			PhysArr[(PhysTmp-KERNEL_HEAP_START)/PAGE_SIZE].virtAdd=startAddress;
		}
		heapInfoIndex+=heapInfoArr[heapInfoIndex].numberOfPages;
		return (void*)returnAddress;
	}
	return NULL;
	//TODO: [PROJECT 2016 - BONUS1] Implement a Kernel allocation strategy
	// Instead of the continuous allocation/deallocation, implement one of
	// the strategies NEXT FIT, BEST FIT, .. etc
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kfree()

	int i=(uint32)(virtual_address-KERNEL_HEAP_START)/PAGE_SIZE;
	if(heapInfoArr[i].used==1){
		uint32 address = (uint32)virtual_address;
		int i1;
		for(i1 = 0; i1 < heapInfoArr[i].numberOfPages; i1++, address += PAGE_SIZE){
			uint32 * temp_page_table = NULL;
			struct Frame_Info* ptrFrameInfo = get_frame_info(ptr_page_directory, (void*)address, &temp_page_table);
			uint32 PhysTmp=kheap_physical_address(address);
			PhysArr[(PhysTmp-KERNEL_HEAP_START)/PAGE_SIZE].virtAdd=0;
			free_frame(ptrFrameInfo);
			unmap_frame(ptr_page_directory, (void*)address);
		}
		heapInfoArr[i].used=0;
	}
	//TODO: [PROJECT 2016 - BONUS1] Implement a Kernel allocation strategy
	// Instead of the continuous allocation/deallocation, implement one of
	// the strategies NEXT FIT, BEST FIT, .. etc

}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kheap_virtual_address()

	return PhysArr[(physical_address-KERNEL_HEAP_START)/PAGE_SIZE].virtAdd;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kheap_physical_address()

	uint32* pageTablePtr = NULL;
	get_page_table(ptr_page_directory, (uint32*)virtual_address, &pageTablePtr);
	return ((pageTablePtr[PTX(virtual_address)])>>12)*PAGE_SIZE;
}
