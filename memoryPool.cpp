#include"memoryPool.h"
#include<stdlib.h>
#include<string.h>

int find_available_memory_index(mp_page_block_chart_s* table,uint size, uint block_size)
{
	int carry_index = 0;
	int meet_cout = mp_alignment_up(size, block_size) / block_size;
	uint block_status = 0;
	uint mask = 1;
	int current_count = 0;
	int current_index = 0;
	for (; carry_index < 8; ++carry_index)
	{
		block_status = table->_block_status_table[carry_index];
		for (; current_index < 16; ++current_index)
		{
			if (!(block_status & mask))
			{
				++current_count;
				if (current_count == meet_cout)
				{
					int start_index = carry_index * 16 + current_index - meet_cout + 1;
					set_blocks_dirty(table, start_index, meet_cout);
					return start_index;
				}
			}
			else
			{
				if (current_count != 0)current_count = 0;
			}
			mask <<= 2;
		}
		current_index = 0;
		mask = 1;
	}
	return -1;
}

void set_block_dirty(mp_page_block_chart_s* table, int index)
{
	int carry_index = index / 16;
	int mod_index = index % 16;
	uint mask = 1;
	if (carry_index >= 8)return;
	table->_block_status_table[carry_index] |= (mask<<(2*mod_index));
}

void set_blocks_dirty(mp_page_block_chart_s* table, int index, int count)
{
	int carry_index = index / 16;
	int mod_start_index = index % 16;
	uint start_mask = mp_mid_part_mask << (2*mod_start_index);
	int mod_start_count = 16 - mod_start_index;
	int mod_mid_count = (count - mod_start_count) / 16;
	int mod_final_count = (count - mod_start_count) % 16;

	//头填充
	uint orginal = table->_block_status_table[carry_index];
	table->_block_status_table[carry_index] |= start_mask;
	//中间填充
	for (; mod_mid_count; --mod_mid_count)
	{
		++carry_index;
		table->_block_status_table[carry_index] |= mp_mid_part_mask;
	}
	//尾填充
	if (mod_final_count < 0)
	{
		
		table->_block_status_table[carry_index] &= ~(mp_mid_part_mask << (2 * (mod_final_count + 16)));
		//消除，分割
		table->_block_status_table[carry_index] &= ~(((uint)1) << (2 * (mod_final_count + 15) + 1));
		table->_block_status_table[carry_index] |= orginal;
	}
	else if (mod_final_count == 0)
	{
		table->_block_status_table[carry_index] &= ~((uint)1 << 31);
	}
	else
	{
		++carry_index;
		orginal = table->_block_status_table[carry_index];
		table->_block_status_table[carry_index] |= (mp_mid_part_mask >> (2 * (16 - mod_final_count)));
		//消除，分割
		table->_block_status_table[carry_index] &= ~(((uint)1) << (2 * (mod_final_count - 1) + 1));
		table->_block_status_table[carry_index] |= orginal;
	}
}

void set_block_tidy(mp_page_block_chart_s* table, int index)
{
	int carry_index = index / 16;
	int mod_start_index = index % 16;
	uint mask = 1;
	table->_block_status_table[carry_index] &= ~(mask<<(2*mod_start_index));
}

void set_blocks_tidy(mp_page_block_chart_s* table, int index, int count)
{
	int carry_index = index / 16;
	int mod_start_index = index % 16;
	int mod_start_count = 16 - mod_start_index;
	int mod_mid_count = (count - mod_start_count) / 16;
	int mod_fin_count= (count - mod_start_count) % 16;
	uint orginal = table->_block_status_table[carry_index];
	//头
	table->_block_status_table[carry_index] &= ~(mp_mid_part_mask<<(2*mod_start_index));
	//中间
	for (; mod_mid_count; --mod_mid_count)
	{
		++carry_index;
		table->_block_status_table[carry_index] = 0;
	}
	//尾
	if (mod_fin_count <= 0)
	{
		table->_block_status_table[carry_index] |= (orginal&(mp_mid_part_mask << (2*(16 + mod_fin_count))));
	}
	else
	{
		++carry_index;
		table->_block_status_table[carry_index] &= ~(mp_mid_part_mask >> (2 * (16-mod_fin_count)));
	}
}

int get_block_dirty(mp_page_block_chart_s* table, int index)
{
	uint mask = 1;
	int carry_index = index / 16;
	int mod_index = index % 16;
	if (carry_index >= 8)return -1;
	return table->_block_status_table[carry_index] & (mask << (2 * mod_index));
}

uchar* mp_page_alloc(mp_page_s* page, uint size)
{
	uint total_size = page->_block_size * 128;
	uint occupied = (uint)(page->_rest_memory - page->_alloc);
	uint rest_container = total_size - occupied;
	if (rest_container<=size||occupied>= total_size)
	{
		int ret = find_available_memory_index(&(page->_table), size, page->_block_size);
		if (ret == -1)return 0;
		return ret*page->_block_size + page->_alloc;
	}
	uchar* address = page->_rest_memory;
	int require_size = mp_alignment_up(size, page->_block_size);
	int block_count = require_size / page->_block_size;
	int index = occupied / page->_block_size;
	if (block_count == 1)
	{
		set_block_dirty(&page->_table, index);
	}
	else
	{
		set_blocks_dirty(&page->_table, index, block_count);
	}
	page->_rest_memory = address + require_size;
	return address;
}

void mp_page_free(mp_page_s* page, uchar* ptr)
{
	int index = (uint)(ptr - page->_alloc) / page->_block_size;
	int carry_index = index / 16;
	int mod_start_index = index % 16;
	uint mask = 1<<(2*mod_start_index);
	int block_count = 0;
	for (; carry_index < 8 && (page->_table._block_status_table[carry_index] & (mask));)
	{
		++block_count;
		page->_table._block_status_table[carry_index] &= ~mask;
		++mod_start_index;
		mask <<= 1;
		if (mod_start_index >= 16)
		{
			++carry_index;
			mod_start_index = 0;
			mask = 1;
		}
	}
	block_count = (block_count + 1) / 2;
	if (ptr + block_count * page->_block_size == page->_rest_memory)
		page->_rest_memory = ptr;
	mp_page_set_min_available_address(page);
}

void mp_page_set_min_available_address(mp_page_s* page)
{
	uint index = (uint)(page->_rest_memory - page->_alloc)/page->_block_size;
	int carry_index = index / 16;
	int mod_end_index = index % 16;
	uint mask = 1;
	while (carry_index>=0&&(!(page->_table._block_status_table[carry_index] & (mask << mod_end_index))))
	{
		--mod_end_index;
		if (mod_end_index == 0)
		{
			--carry_index;
		}
	}
	if (carry_index < 0)return;
	page->_rest_memory = page->_alloc + page->_block_size * (carry_index * 16 + mod_end_index+2);
}

void initialize_memory_pool(mp_memory_pool_mamager* manager)
{
	for (int i = 0; i < mp_total_page_count; ++i)
	{
		manager->_page_part[i]._block_size = (i + 1) * mp_min_block_size;
		manager->_page_part[i]._alloc = (uchar*)malloc(manager->_page_part[i]._block_size*128);
		manager->_page_part[i]._rest_memory = manager->_page_part[i]._alloc;
		memset(manager->_page_part[i]._table._block_status_table,0,32);
	}
}

void release_memory_pool(mp_memory_pool_mamager* manager)
{
	for (int i = 0; i < mp_total_page_count; ++i)
	{
		free(manager->_page_part[i]._alloc);
	}
}

uchar* mp_alloc(mp_memory_pool_mamager* manager, uint size)
{
	uchar* alloc = 0;
	if (size == 0)return 0;
	int alloc_page_index = mp_alignment_up(size, mp_min_block_size)/(64*mp_min_block_size);//内存大于该层一半就用下一层
	for (int i = 1; i < mp_total_page_count; ++i)
	{
		if (alloc_page_index > 1)alloc_page_index >>= 1;
		else break;
	}
	for (;!alloc&&alloc_page_index < mp_total_page_count; ++alloc_page_index)
	{
		alloc = mp_page_alloc(manager->_page_part + alloc_page_index, size);
	}
	return alloc;
}

void mp_free(mp_memory_pool_mamager* manager, uchar* ptr)
{
	int belong_page_index = 0;
	for (; belong_page_index < mp_total_page_count; ++belong_page_index)
	{
		if (manager->_page_part[belong_page_index]._alloc <= ptr && manager->_page_part[belong_page_index]._rest_memory > ptr)break;
	}
	mp_page_free(manager->_page_part + belong_page_index, ptr);
}
