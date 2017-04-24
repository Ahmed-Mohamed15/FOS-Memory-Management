
#include <inc/lib.h>
/*
* Simple malloc()
*
* The address space for the dynamic allocation is
* from "USER_HEAP_START" to "USER_HEAP_MAX"-1
* Pages are allocated ON 4KB BOUNDARY
* On succeed, return void pointer to the allocated space
* return NULL if
*	-there's no suitable space for the required allocation
*/
// malloc()
//	This function use both NEXT FIT and BEST FIT strategies to allocate space in heap
//  with the given size and return void pointer to the start of the allocated space

//	To do this, we need to switch to the kernel, allocate the required space
//	in Page File then switch back to the user again.
//
//	We can use sys_allocateMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls allocateMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the allocateMem function is empty, make sure to implement it.
uint32 lastWritingAddress = USER_HEAP_START;
struct userHeapInfo {
	uint32  end;
	bool allocated;
};
char Mode;
struct userHeapInfo userHeap[(USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE];

uint32 getFreeSize(uint32 strt,uint32 AllocateSize) {
	uint32 size = 0;
	while (strt<USER_HEAP_MAX&&userHeap[(strt - USER_HEAP_START) / PAGE_SIZE].allocated == 0) {
		if(size>=AllocateSize&&Mode!='B'&&Mode!='W')// if it best fit or worst fit -> check all free size not the allocated only
			break;
	size += PAGE_SIZE;
	strt += PAGE_SIZE;
	}
	return size;
}

void* malloc(uint32 size)
{
	//TODO: [PROJECT 2016 - Dynamic Allocation] malloc() [User Side]

	if (lastWritingAddress >= USER_HEAP_MAX)
		lastWritingAddress = USER_HEAP_START;
	size = ROUNDUP(size, PAGE_SIZE);
	bool one = 0;
	uint32 stopSearch;
	if (sys_isUHeapPlacementStrategyNEXTFIT()) {
		Mode='N';
		int i;
		for (i = lastWritingAddress;i<USER_HEAP_MAX;i += PAGE_SIZE) {
			if(one==1&&i>=stopSearch)// when searching again stop from where it starts
				break;
			uint32 tmp = getFreeSize(i,size);
			if (tmp >= size) {
				userHeap[(i - USER_HEAP_START) / PAGE_SIZE].allocated = 1;
				userHeap[(i - USER_HEAP_START) / PAGE_SIZE].end = i + size;
				lastWritingAddress = userHeap[(i - USER_HEAP_START) / PAGE_SIZE].end;
				sys_allocateMem(i, size);
				return (void*)i;
			}
			if (userHeap[(i - USER_HEAP_START) / PAGE_SIZE].allocated == 1)// if allocated frame -> i=end of the allocated size
				i = userHeap[(i - USER_HEAP_START) / PAGE_SIZE].end - PAGE_SIZE;
			else// else it is a free frame so -> i=end of free space(tmp)
				i += tmp - PAGE_SIZE;

			if (i + PAGE_SIZE >= USER_HEAP_MAX && one == 0) {// start searching from begin again on time only
				i = USER_HEAP_START - PAGE_SIZE;
				one = 1;
				stopSearch=lastWritingAddress;
			}
		}
	}
	if (sys_isUHeapPlacementStrategyBESTFIT()) {
		Mode='B';
		int i;
		uint32 bestAdd = 0, bestSize = 0;
		for (i = USER_HEAP_START;i<USER_HEAP_MAX;i += PAGE_SIZE) {
			uint32 tmp = getFreeSize(i,size);
			if (tmp >= size) {
				if (bestSize == 0 || (tmp<bestSize)) {
					bestSize = tmp;
					bestAdd = i;
				}
			}
			if (userHeap[(i - USER_HEAP_START) / PAGE_SIZE].allocated == 1)
				i = userHeap[(i - USER_HEAP_START) / PAGE_SIZE].end - PAGE_SIZE;
			else
				i += tmp - PAGE_SIZE;
		}
		if (bestSize == 0)
			return NULL;
		userHeap[(bestAdd - USER_HEAP_START) / PAGE_SIZE].allocated = 1;
		userHeap[(bestAdd - USER_HEAP_START) / PAGE_SIZE].end = bestAdd + size;
		sys_allocateMem(bestAdd, size);
		return (void*)bestAdd;

	}

	else if (sys_isUHeapPlacementStrategyWORSTFIT()) {
			Mode='W';
			int i;
			uint32 worstAdd = 0, worstSize = 0;
			for (i = USER_HEAP_START;i<USER_HEAP_MAX;i += PAGE_SIZE) {
				uint32 tmp = getFreeSize(i,size);
				if (tmp >= size) {
					if (worstSize == 0 || (tmp > worstSize)) {
						worstSize = tmp;
						worstAdd = i;
					}
				}
				if (userHeap[(i - USER_HEAP_START) / PAGE_SIZE].allocated == 1)
					i = userHeap[(i - USER_HEAP_START) / PAGE_SIZE].end - PAGE_SIZE;
				else
					i += tmp - PAGE_SIZE;
			}
			if (worstSize == 0)
				return NULL;
			userHeap[(worstAdd - USER_HEAP_START) / PAGE_SIZE].allocated = 1;
			userHeap[(worstAdd - USER_HEAP_START) / PAGE_SIZE].end = worstAdd + size;
			sys_allocateMem(worstAdd, size);
			return (void*)worstAdd;

		}
	else if (sys_isUHeapPlacementStrategyFIRSTFIT()) {
		Mode='F';
		int i;
		for (i = USER_HEAP_START;i<USER_HEAP_MAX;i += PAGE_SIZE) {
			uint32 tmp = getFreeSize(i,size);
			if (tmp >= size) {
				userHeap[(i - USER_HEAP_START) / PAGE_SIZE].allocated = 1;
				userHeap[(i - USER_HEAP_START) / PAGE_SIZE].end = i + size;
				sys_allocateMem(i, size);
				return (void*)i;
			}
			if (userHeap[(i - USER_HEAP_START) / PAGE_SIZE].allocated == 1)
				i = userHeap[(i - USER_HEAP_START) / PAGE_SIZE].end - PAGE_SIZE;
			else
				i += tmp - PAGE_SIZE;
		}
	}
	return NULL;

	//TODO: [PROJECT 2016 - BONUS2] Apply FIRST FIT and WORST FIT policies
}



// free():
//	This function frees the allocation of the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from page file and main memory then switch back to the user again.
//
//	We can use sys_freeMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls freeMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the freeMem function is empty, make sure to implement it.

void free(void* virtual_address)
{
	//TODO: [PROJECT 2016 - Dynamic Deallocation] free() [User Side]

	sys_freeMem((uint32)virtual_address, userHeap[(uint32)(virtual_address - USER_HEAP_START) / PAGE_SIZE].end - (uint32)virtual_address);
	userHeap[(uint32)(virtual_address - USER_HEAP_START) / PAGE_SIZE].allocated = 0;
	userHeap[(uint32)(virtual_address - USER_HEAP_START) / PAGE_SIZE].end = 0;
}


//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// realloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_moveMem(uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		which switches to the kernel mode, calls moveMem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		in "memory_manager.c", then switch back to the user mode here
//	the moveMem function is empty, make sure to implement it.

void *realloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT 2016 - BONUS4] realloc() [User Side]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");

}

