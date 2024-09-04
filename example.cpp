#include<iostream>
#include<bitset>
#include"memoryPool.h"
int main()
{
	mp_memory_pool_mamager memory_pool;
	initialize_memory_pool(&memory_pool);

	int* a = (int*)mp_alloc(&memory_pool, 4096);
	std::cout << (int)memory_pool._page_part[1]._alloc<<" "<< (int)memory_pool._page_part[2]._alloc<<" "<< (int)a;

	release_memory_pool(&memory_pool);

	return 0;
}