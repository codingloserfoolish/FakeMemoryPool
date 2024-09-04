#ifndef MEMORYPOOL_H_
#define MEMORYPOOL_H_

typedef unsigned char	uchar;
typedef unsigned int	uint;

#define mp_total_page_count		3
#define mp_min_block_size		32

#define mp_mid_part_mask		0xffffffff

#define mp_alignment_up(size,alignment_mask) ((size+alignment_mask-1)&(~(alignment_mask-1)))
#define mp_address_to_index(address,base,block_size) ((address-base)/block_size)

/*struct mp_page_block_chart
* 1.���block����
* 2.����block���úͲ�����
* 
* table��Ӧ��ϵ
* 00000000
* 76543210
*/
struct mp_page_block_chart_s
{
	uint _block_status_table[8];//���Ա�ʾ128����
};
/*
* β���ڴ�ȫ�����������²���
* �Ҳ�������-1
*/
int find_available_memory_index(mp_page_block_chart_s*table, uint size,uint block_size);
/*
* private 
*/
void set_block_dirty(mp_page_block_chart_s* table,int index);
void set_blocks_dirty(mp_page_block_chart_s* table, int index,int count);
void set_block_tidy(mp_page_block_chart_s* table, int index);
void set_blocks_tidy(mp_page_block_chart_s* table, int index, int count);
int get_block_dirty(mp_page_block_chart_s* table, int index);

/*
* map_page_s
* ��Ϊÿ��Ϊ32B,64B,128B�Ŀ�
* ÿ�㶼��128����
*/
struct mp_page_s
{
	mp_page_block_chart_s	_table;
	uint					_block_size;
	uchar*					_rest_memory;//β���ڴ��ַ
	uchar*					_alloc;//�ڴ��
};

uchar* mp_page_alloc(mp_page_s* page, uint size);
void mp_page_free(mp_page_s* page,uchar*ptr);
/*
* private
*/
void mp_page_set_min_available_address(mp_page_s* page);

/*
* mp_memory_pool_mamager
* �ᰴ��_page_part�±�ʶ��ÿ��Ĵ�С(index+1)*32
*/
struct mp_memory_pool_mamager
{
	mp_page_s _page_part[mp_total_page_count];
};

void initialize_memory_pool(mp_memory_pool_mamager* manager);
void release_memory_pool(mp_memory_pool_mamager* manager);

uchar* mp_alloc(mp_memory_pool_mamager*manager,uint size);
void mp_free(mp_memory_pool_mamager* manager, uchar*ptr);
#endif // !MEMORYPOOL_H_
