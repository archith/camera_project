#include <linux/kernel.h>				/* printk() */
#include <linux/slab.h>					/* kmalloc() */
#include <linux/fs.h>						/* everything... */
#include <linux/errno.h>				/* error codes */
#include <linux/types.h>				/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>				/* O_ACCMODE */
#include <linux/pci.h>					/* O_ACCMODE */
#include <linux/delay.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/module.h>
#include <linux/init.h>					/* module_init/module_exit */
#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/vmalloc.h>

#include <asm/system.h>					/* cli(), *_flags */
#include "cachetest.h"					/* local definitions */

int quantum = 512;
int offset = 0;

devfs_handle_t CacheTS_devfs;
static char devname[7] = "CacheTS";
CacheTS_Dev *CacheTS_devices; /* allocated in CacheTS_init */
typedef int (*pF)(u32 length);

//clean_dcache_range
int Clean_Test(u32 mem_length)
{
	u32 *uncached_addr = 0;
	u32 *real_uncached = 0;
	dma_addr_t dma_handle;
	u32 *cached_alias = 0;
	u32 cached_preload[4];
	int i = 0;

	real_uncached = (u32*)consistent_alloc(GFP_KERNEL, mem_length, &dma_handle);
	cached_alias = (u32*)bus_to_virt(dma_handle);
	uncached_addr = real_uncached;

	printk("uncached_addr @ %#010x cached_alias@ %#010x\n", (u32)uncached_addr,(u32)cached_alias);

	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_alias[i] = 0;
		uncached_addr[i] = 0;
	}
	cli();
	/*
	 * Force a cache preload since we read-allocate
	 */
	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		uncached_addr[i] = 0xBBBBBBBB;
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(uncached_addr[i] != 0xBBBBBBBB){
			printk("before clean_dcache_range uncached_addr[%d]=%x \n",i,uncached_addr[i]);
			break;
		}

	}
	
	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_preload [i%4]= cached_alias[i];
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		cached_alias[i] = 0xAAAAAAAA;
	}
	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(uncached_addr[i] != 0xAAAAAAAA){
			printk("after assign 0xAAAAAAAA cached_alias uncached_addr[%d]=%x \n",i,uncached_addr[i]);
			break;
		}

	}
 for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xAAAAAAAA){
			printk("after assign 0xAAAAAAAA cached_alias cached_alias[%d]=%x \n",i,cached_alias[i]);
			break;
		}
	}
	
	clean_dcache_range(cached_alias,(cached_alias+mem_length/sizeof(u32)));

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(uncached_addr[i] != 0xAAAAAAAA){
			if(!(i%4)){
				printk("after clean_dcache_range uncached_addr[%d]=%x \n",i,uncached_addr[i]);
				break;
			}
		}
	}

	sti();

	consistent_free(real_uncached, mem_length, (u32)&dma_handle);


	return 0;
}
//flush_dcache_page
int FlushPage_Test(u32 mem_length)
{
	struct page *p_start, *p_end;
	u32 *uncached_addr = 0;
	u32 *real_uncached = 0;
	dma_addr_t dma_handle;
	u32 *cached_alias = 0;
	u32 cached_preload[4];
	int i = 0;

	printk("==>FlushPage_Test \n");

	real_uncached = (u32*)consistent_alloc(GFP_KERNEL, mem_length, &dma_handle);
	cached_alias = (u32*)bus_to_virt(dma_handle);
	uncached_addr = real_uncached;	
	printk("uncached_addr @ %#010x cached_alias@ %#010x\n", (u32)uncached_addr,(u32)cached_alias);
	printk("phsical address 0x%08x \n",dma_handle);

	for(i = 0; i < 16/sizeof(u32); i++){
		cached_alias[i] = 0;
		uncached_addr[i] = 0;
	}
	cli();
	/*
	 * Force a cache preload since we read-allocate
	 */
	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		cached_alias[i] = 0xCCCCCCCC;
	}

	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_preload [i%4]= cached_alias[i];
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		cached_alias[i] = 0xAAAAAAAA;
	}

	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_preload [i%4]= cached_alias[i];
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xAAAAAAAA){
			if(!(i%4)){
				printk("after assign data_A cached_alias[%d]=%x \n",i,cached_alias[i]);
			}
		}

	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		uncached_addr[i] = 0xBBBBBBBB;
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xAAAAAAAA){
			if(!(i%4)){
				printk("before flush_dcache_page cached_alias[%d]=%x \n",i,cached_alias[i]);
			}
		}

	}	

	/////////////////////////////////////////////////////////////////////////////////////////////////
	{	
		p_start = virt_to_page(cached_alias);
		p_end = virt_to_page(cached_alias+mem_length/sizeof(u32));
		
		while (p_start <= p_end) {
			printk("call flush_dcache_page page 0x%08x\n",(u32)p_start);
			flush_dcache_page(p_start);
			p_start++;
		}
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xBBBBBBBB){
			if(!(i%4)){
				printk("after invalidate_dcache_range cached_alias[%d]=%x \n",i,cached_alias[i]);
			}
		}
	}

	sti();

	consistent_free(real_uncached, mem_length, (u32)&dma_handle);


	printk("<==FlushPage_Test \n");
	return 0;
}
//clean_dcache_entry
int FlushClean_Test()
{

	u32 *uncached_addr = 0;
	u32 *real_uncached = 0;
	dma_addr_t dma_handle;
	u32 *cached_alias = 0;
	u32 cached_preload[4];
	int i = 0;

	real_uncached = (u32*)consistent_alloc(GFP_KERNEL, 16, &dma_handle);
	cached_alias = (u32*)bus_to_virt(dma_handle);
	uncached_addr = real_uncached;

	printk("uncached_addr @ %#010x cached_alias@ %#010x\n", (u32)uncached_addr,(u32)cached_alias);

	for(i = 0; i < 16/sizeof(u32); i++){
		cached_alias[i] = 0;
		uncached_addr[i] = 0;
	}
	cli();
	/*
	 * Force a cache preload since we read-allocate
	 */
	for(i = 0; i < 16/sizeof(u32); i++){
		cached_preload [i%4]= cached_alias[i];
	}

	for(i = 0; i < 16/sizeof(u32); i++)
	{
		uncached_addr[i] = 0xAAAAAAAA;
	}

	for(i = 0; i < 16/sizeof(u32); i++)
	{
		cached_alias[i] = 0xBBBBBBBB;
	}

//	printk("before clean_dcache_entry\n");

	for(i = 0; i < 16/sizeof(u32); i++)
	{
		if(uncached_addr[i] != 0xAAAAAAAA){
			if(!(i%4)){
				printk("before clean_dcache_entry uncached_addr[%d]=%x \n",i,cached_alias[i]);
			}
		}

	}

	clean_dcache_entry(cached_alias);


	for(i = 0; i < 16/sizeof(u32); i++)
	{
		if(uncached_addr[i] != 0xBBBBBBBB){
			printk("after clean_dcache_entry uncached_addr[%d]=%x \n",i,cached_alias[i]);
			break;
		}
	}

	sti();

	consistent_free(real_uncached, 16, (u32)&dma_handle);

	return 0;
}

//flush_dcache_range
int Flush_Test(u32 mem_length)
{
	u32 *uncached_addr = 0;
	u32 *real_uncached = 0;
	dma_addr_t dma_handle;
	u32 *cached_alias = 0;
	u32 cached_preload[4];
	int i = 0;

	real_uncached = (u32*)consistent_alloc(GFP_KERNEL, mem_length, &dma_handle);
	cached_alias = (u32*)bus_to_virt(dma_handle);
	uncached_addr = real_uncached;

	printk("uncached_addr @ %#010x cached_alias@ %#010x\n", (u32)uncached_addr,(u32)cached_alias);

	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_alias[i] = 0;
		uncached_addr[i] = 0;
	}
	cli();
	/*
	 * Force a cache preload since we read-allocate
	 */
	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_preload [i%4]= cached_alias[i];
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		cached_alias[i] = 0xAAAAAAAA;
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		uncached_addr[i] = 0xBBBBBBBB;
	}

//	printk("before flush_dcache_range\n");

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xAAAAAAAA){
			if(!(i%4)){
				printk("before flush_dcache_range cached_alias[%d]=%x \n",i,cached_alias[i]);
			}
		}

	}

//	printk("cached_alias_star @ %#010x cached_alias_end@ %#010x\n", cached_alias,(cached_alias+mem_length/sizeof(u32)));

	flush_dcache_range(cached_alias,(cached_alias+mem_length/sizeof(u32)));

//	printk("after flush_dcache_range\n");

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xBBBBBBBB){
			printk("after flush_dcache_range cached_alias[%d]=%x \n",i,cached_alias[i]);
			break;
		}
	}

	sti();

	consistent_free(real_uncached, mem_length, (u32)&dma_handle);

	return 0;
}

//invalidate_dcache_range
int InvalidAlignment_Test(u32 mem_length)
{
	u32 *uncached_addr = 0;
	u32 *real_uncached = 0;
	dma_addr_t dma_handle;
	u32 *cached_alias = 0;
	u32 cached_preload[4];

	int i = 0;

	real_uncached = (u32*)consistent_alloc(GFP_KERNEL, mem_length+offset, &dma_handle);
	cached_alias = (u32*)bus_to_virt(dma_handle);
  cached_alias +=offset;
	uncached_addr = real_uncached+offset;

	printk("uncached_addr @ %#010x cached_alias@ %#010x\n", (u32)uncached_addr,(u32)cached_alias);

	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_alias[i] = 0;
		uncached_addr[i] = 0;
	}
	cli();

	cpu_cache_clean_invalidate_all();
	/*
	 * Force a cache preload since we read-allocate
	 */

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		cached_alias[i] = 0xCCCCCCCC;
	}

	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_preload [i%4]= cached_alias[i];
	}

  for(i = 0; i < mem_length/sizeof(u32); i++)
	{		
		cached_alias[i] = 0xAAAAAAAA;
	}

	for(i = mem_length/sizeof(u32)-1; i >0 ; i--){
		cached_preload [i%4]= cached_alias[i];
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xAAAAAAAA){
			if(!(i%4)){
				printk("after assign data_A cached_alias[%d]=%x \n",i,cached_alias[i]);
			}
		}

	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(i%2)
		uncached_addr[i] = 0xBBBBBBBB;
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xAAAAAAAA){
			if(!(i%4)){
				printk("before invalidate_dcache_range cached_alias[%d]=%x \n",i,cached_alias[i]);
			}
		}

	}

	invalidate_dcache_range(cached_alias,(cached_alias+mem_length/sizeof(u32)));
	
	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
			if(i%2){
					if(cached_alias[i] != 0xBBBBBBBB){
						if(!(i%4)){
							printk("after invalidate_dcache_range cached_alias[%d]=%x \n",i,cached_alias[i]);
						}
					}
			}else{
					if(cached_alias[i] != 0xAAAAAAAA){
						if(!(i%4)){
						printk("after invalidate_dcache_range cached_alias[%d]=%x \n",i,cached_alias[i]);
						}
					}
			}				
	}
	
	sti();

	consistent_free(real_uncached, mem_length, (u32)&dma_handle);

	return 0;
}

int Boundary_Test()
{
	u32 *uncached_addr1 = 0;
	u32 *uncached_addr2 = 0;

	u32 *real_uncached1 = 0;
	u32 *real_uncached2 = 0;
	dma_addr_t dma_handle1,dma_handle2;
	u32 *cached_alias1 = 0;
	u32 *cached_alias2 = 0;
	u32 cached_preload[4];
	int i = 0;

	real_uncached1 = (u32*)consistent_alloc(GFP_KERNEL, 8*1024, &dma_handle1);
	cached_alias1 = (u32*)bus_to_virt(dma_handle1);
	uncached_addr1 = real_uncached1;

	real_uncached2 = (u32*)consistent_alloc(GFP_KERNEL, 8*1024, &dma_handle2);
	cached_alias2 = (u32*)bus_to_virt(dma_handle2);
	uncached_addr2 = real_uncached2;

	printk("uncached_addr1 @ %#010x cached_alias1@ %#010x\n", (u32)uncached_addr1,(u32)cached_alias1);
	printk("uncached_addr2 @ %#010x cached_alias2@ %#010x\n", (u32)uncached_addr2,(u32)cached_alias2);


	for(i = 0; i < 8*1024/sizeof(u32); i++){
		uncached_addr2[i] = 0;
		uncached_addr1[i] = 0;
	}
	cli();
	/*
	 * Force a cache preload since we read-allocate
	 */
	for(i = 8*1024/sizeof(u32)-1; i >0 ; i--){
		cached_preload [i%4]= cached_alias1[i];
	}

	for(i = 8*1024/sizeof(u32)-1; i >0; i--){
		cached_preload [i%4]= cached_alias2[i];
	}

	for(i = 0; i < 8*1024/sizeof(u32); i++)
	{
		cached_alias2[i] = 0xBBBBBBBB;
	}

	for(i = 0; i < 8*1024/sizeof(u32); i++)
	{
		cached_alias1[i] = 0xAAAAAAAA;
	}

	for(i = 4*1024; i < 8*1024/sizeof(u32); i++)
	{
		uncached_addr1[i] = 0xCCCCCCCC;
	}

	for(i = 0; i < 4*1024/sizeof(u32); i++)
	{
		uncached_addr2[i] = 0xCCCCCCCC;
	}

	for(i = 0; i < 8*1024/sizeof(u32); i++){
			if(cached_alias1[i] != 0xAAAAAAAA){
					if(!(i%4)){
							printk("before invalidate cached_alias1[%d]=%x \n",i,cached_alias1[i]);
							break;
					}
			}

			if(cached_alias2[i] != 0xBBBBBBBB){
					if(!(i%4)){
							printk("before invalidate cached_alias2[%d]=%x \n",i,cached_alias2[i]);
							break;
					}
			}
	}

	invalidate_dcache_range(cached_alias1,(cached_alias1+8*1024/sizeof(u32)));
	invalidate_dcache_range(cached_alias2,(cached_alias2+8*1024/sizeof(u32)));

	printk("after invalidate\n");

	for(i = 0; i < 4*1024/sizeof(u32); i++){
			if(cached_alias1[i] != 0xAAAAAAAA){
					if(!(i%4)){
							printk("after invalidate cached_alias1[%d]=%x \n",i,cached_alias1[i]);
					}
			}

			if(cached_alias2[i] != 0xCCCCCCCC){
					if(!(i%4)){
							printk("after invalidate cached_alias2[%d]=%x \n",i,cached_alias2[i]);
							break;
					}
			}
	}

	for(i = 4*1024; i < 8*1024/sizeof(u32); i++){
			if(cached_alias1[i] != 0xCCCCCCCC){
					if(!(i%4)){
							printk("cached_alias1[%d]=%x \n",i,cached_alias1[i]);
							break;
					}
			}

			if(cached_alias2[i] != 0xBBBBBBBB){
					if(!(i%4)){
							printk("cached_alias2[%d]=%x \n",i,cached_alias2[i]);
							break;
					}
			}
	}

	sti();

	consistent_free(real_uncached1, 8*1024, (u32)&dma_handle1);
	consistent_free(real_uncached2, 8*1024, (u32)&dma_handle2);

	return 0;
}


//invalidate_dcache_range
int Invalid_Test(u32 mem_length)
{
	u32 *uncached_addr = 0;
	u32 *real_uncached = 0;
	dma_addr_t dma_handle;
	u32 *cached_alias = 0;
	u32 cached_preload[4];

	int i = 0;

	real_uncached = (u32*)consistent_alloc(GFP_KERNEL, mem_length+offset, &dma_handle);
	cached_alias = (u32*)bus_to_virt(dma_handle);
  cached_alias +=offset;
	uncached_addr = real_uncached+offset;

//	printk("uncached_addr @ %#010x cached_alias@ %#010x\n", (u32)uncached_addr,(u32)cached_alias);

	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_alias[i] = 0;
		uncached_addr[i] = 0;
	}
	cli();

	cpu_cache_clean_invalidate_all();	
	/*
	 * Force a cache preload since we read-allocate	 
	 */

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		cached_alias[i] = 0xCCCCCCCC;
	}
	
	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_preload [i%4]= cached_alias[i];
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		cached_alias[i] = 0xAAAAAAAA;
	}

	for(i = mem_length/sizeof(u32)-1; i >0 ; i--){
		cached_preload [i%4]= cached_alias[i];
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xAAAAAAAA){
			if(!(i%4)){
				printk("after assign data_A cached_alias[%d]=%x \n",i,cached_alias[i]);
			}
		}

	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		uncached_addr[i] = 0xBBBBBBBB;
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xAAAAAAAA){
			if(!(i%4)){
				printk("before invalidate_dcache_range cached_alias[%d]=%x \n",i,cached_alias[i]);
			}
		}

	}

	invalidate_dcache_range(cached_alias,(cached_alias+mem_length/sizeof(u32)));
	
	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xBBBBBBBB){
			if(!(i%4)){
				printk("after invalidate_dcache_range cached_alias[%d]=%x \n",i,cached_alias[i]);
			}
		}
	}

	sti();

	consistent_free(real_uncached, mem_length, (u32)&dma_handle);

	return 0;
}

//cpu_cache_clean_invalidate_all();
int invalidall_test(u32 mem_length)
{
	u32 *uncached_addr = 0;
	u32 *real_uncached = 0;
	dma_addr_t dma_handle;
	u32 *cached_alias = 0;
	u32 cached_preload[4];
	int i = 0;

	real_uncached = (u32*)consistent_alloc(GFP_KERNEL, mem_length, &dma_handle);
	cached_alias = (u32*)bus_to_virt(dma_handle);
	uncached_addr = real_uncached;

	printk("uncached_addr @ %#010x cached_alias@ %#010x\n", (u32)uncached_addr,(u32)cached_alias);

	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_alias[i] = 0;
		uncached_addr[i] = 0;
	}
	cli();

	/*
	 * Force a cache preload since we read-allocate
	 */
	for(i = 0; i < mem_length/sizeof(u32); i++){
		cached_preload [i%4]= cached_alias[i];
	}
	
	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		cached_alias[i] = 0xAAAAAAAA;		
	}

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		uncached_addr[i] = 0xBBBBBBBB;
	}		

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xAAAAAAAA){
			if(!(i%4)){
				printk("before cpu_cache_clean_invalidate_all cached_alias[%d]=%x \n",i,cached_alias[i]);
			}
		}
					
	}
	
	cpu_cache_clean_invalidate_all();

	for(i = 0; i < mem_length/sizeof(u32); i++)
	{
		if(cached_alias[i] != 0xBBBBBBBB){
			printk("after cpu_cache_clean_invalidate_all cached_alias[%d]=%x \n",i,cached_alias[i]);
			break;
		}
	}
		
	sti();
	
	consistent_free(real_uncached-offset, mem_length+offset, (u32)&dma_handle);
	
	return 0;
}

void Show1(u32 length)
{
	int i = 0;
	int j = 0;
	unsigned char *pLED = rLED_BASE;

	*pLED = 0x99;

	for(i=0;i<length;i++)
	{
			j++;
			j*=j/j;
	}	

	for(i=0;i<length;i++)
	{
			j*=j/j;
	}

	for(i=0;i<length;i++)
	{
			j--;
	}

	for(i=0;i<length;i++)
	{
			j++;
			j/=j/j;
	}

	for(i=0;i<length;i++)
	{
			j++;
			j*=j/j;
	}

	for(i=0;i<length;i++)
	{
			j*=j/j;
	}

	for(i=0;i<length;i++)
	{
			j--;
	}

	for(i=0;i<length;i++)
	{
			j++;
			j/=j/j;
	}

	for(i=0;i<length;i++)
	{
			j++;
			j*=j/j;
	}

	for(i=0;i<length;i++)
	{
			j*=j/j;
	}

	for(i=0;i<length;i++)
	{
			j--;
	}

	for(i=0;i<length;i++)
	{
			j++;
			j/=j/j;
	}		
   
	return 0;
}

void Show2(u32 length)
{
	int i = length;
	int j = 0;
	unsigned char *pLED = rLED_BASE;

	*pLED = 0x66;

	for(i=0;i<length;i++)
	{
			j++;
			j/=j/j;
	}	

	for(i=0;i<length;i++)
	{
			j*=j/j;
	}

	for(i=0;i<length;i++)
	{
			j--;
	}

	for(i=0;i<length;i++)
	{
			j++;
			j*=j/j;
	}

	for(i=0;i<length;i++)
	{
			j*=j/j;
	}

	for(i=0;i<length;i++)
	{
			j--;
	}

	for(i=0;i<length;i++)
	{
			j++;
			j*=j/j;
	}

	for(i=0;i<length;i++)
	{
			j*=j/j;
	}

	for(i=0;i<length;i++)
	{
			j--;
	}

	return 0;
}

void Dummy()
{
	return 0;
}

void ICache_Test(u32 length)
{

	dma_addr_t dma_handle;
	unsigned char *pLED = rLED_BASE;
	
	pF	pFun_uncached = consistent_alloc(GFP_KERNEL, 4096, &dma_handle);
	pF	pFun_cached = bus_to_virt(dma_handle);

	printk("==>ICache_test\n");

	printk("uncached_addr @ %#010x cached_alias@ %#010x\n", (u32)pFun_uncached,(u32)pFun_cached);
	printk("Show1 size %d\n", Show2-Show1);

	memset(pFun_uncached,0,4096);
	memset(pFun_cached,0,4096);

	cli();
	memcpy(pFun_cached,Show1,Show2-Show1);
	pFun_cached(length);

	udelay(2000);
	
	*pLED = 0xF0;
	memcpy(pFun_cached,Show2,Show2-Dummy);
	pFun_cached(length);
	udelay(2000);
	*pLED = 0x0F;

	/////////////////////////////////////////////////
	flush_icache_range(pFun_cached,(u8)pFun_cached+4096);
	
	pFun_cached(length);

	sti();

	consistent_free(pFun_uncached, 4096, (u32)&dma_handle);

	printk("<==ICache_test\n");

}

/*
 * Open and close
 */

int CacheTS_open (struct inode *inode, struct file *filp)
{    
    return 0;
}

int CacheTS_release (struct inode *inode, struct file *filp)
{

    return 0;
}


/*
 * Data management: read and write
 */

ssize_t CacheTS_read (struct file *filp, char *buf, size_t count,
                loff_t *f_pos)
{    
    ssize_t retval = 0;
    return retval;
}



ssize_t CacheTS_write (struct file *filp, const char *buf, size_t count,
                loff_t *f_pos)
{
		ssize_t retval = 0;   
    return retval;
}

/*
 * The ioctl() implementation
 */

int CacheTS_ioctl (struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{

    int err= 0, ret = 0, tmp;

    printk("==>CacheTS_ioctl cmd = %x\n",cmd);

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (_IOC_TYPE(cmd) != CACHETEST_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > CACHETEST_IOC_MAXNR) return -ENOTTY;

    /*
     * the type is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. Note that the type is user-oriented, while
     * verify_area is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
//    if (_IOC_DIR(cmd) & _IOC_READ)
//        err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
//    else if (_IOC_DIR(cmd) & _IOC_WRITE)
//        err =  !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
//    if (err) return -EFAULT;

    switch(cmd) {                  
      case CACHETEST_IOCOFFSET:
      	ret = __get_user(offset, (int *) arg);
				printk("ioctl call CACHETEST_IOCOFFSET offset = %d\n",offset);
      	break;

      case CACHETEST_IOCINVAILD:     
				ret = __get_user(quantum, (int *) arg);
				printk("ioctl call CACHETEST_IOCINVAILD memlength = %d\n",quantum);
        Invalid_Test(quantum);
        break;
              
      case CACHETEST_IOCINVAILDALIGN:
				ret = __get_user(quantum, (int *) arg);
				printk("ioctl call CACHETEST_IOCINVAILDALIGN memlength = %d\n",quantum);
        InvalidAlignment_Test(quantum);
        break;
        
      case CACHETEST_IOCCLEAN:
				ret = __get_user(quantum, (int *) arg);
				printk("ioctl call CACHETEST_IOCCLEAN memlength = %d\n",quantum);
        Clean_Test(quantum);
        break;
      case CACHETEST_IOCFLUSH:
				ret = __get_user(quantum, (int *) arg);
				printk("ioctl call CACHETEST_IOCFLUSH memlength = %d\n",quantum);
        Flush_Test(quantum);
        break;
      case CACHETEST_IOCFLUSHCLEAN:
				ret = __get_user(quantum, (int *) arg);
				printk("ioctl call CACHETEST_IOCFLUSHCLEAN \n");
        FlushClean_Test();
        break;
      case CACHETEST_IOCFLUSHPAGE:
				ret = __get_user(quantum, (int *) arg);
				printk("ioctl call CACHETEST_IOCFLUSHPAGE \n");
        FlushPage_Test(quantum);
        break;
      case CACHETEST_IOCFLUSHBOUNDARY:
				ret = __get_user(quantum, (int *) arg);
				printk("ioctl call CACHETEST_IOCFLUSHBOUNDARY \n");
        Boundary_Test();
        break;
      case CACHE_IOCICACHE:
				ret = __get_user(quantum, (int *) arg);
				printk("ioctl call CACHE_IOCICACHE \n");
        ICache_Test(quantum);
        break;

      default:  /* redundant, as cmd was checked against MAXNR */
        return -ENOTTY;
    }
		printk("<==CacheTS_ioctl\n");
    return ret;

}

/*
 * The fops
 */

struct file_operations CacheTS_fops = {
    read: CacheTS_read,
    write: CacheTS_write,
    ioctl: CacheTS_ioctl,
    open: CacheTS_open,
    release: CacheTS_release,
};


/*
 * Finally, the module stuff
 */

static int __init CacheTS_init(void)
{
    int result, i;

    printk("==>CacheTS_init \n");

//    SET_MODULE_OWNER(&CacheTS_fops);

    /* 
     * allocate the devices -- we can't have them static, as the number
     * can be specified at load time
     */
    CacheTS_devices = kmalloc(4 * sizeof (CacheTS_Dev), GFP_KERNEL);
    
    if (!CacheTS_devices) {
        return -ENOMEM;
    }
    
    memset(CacheTS_devices, 0, 4 * sizeof (CacheTS_Dev));
    
		CacheTS_devfs = devfs_register(	NULL,
																		devname,
																		DEVFS_FL_AUTO_DEVNUM,
																		0,
																		0,
																		S_IFCHR| S_IRUGO | S_IWUGO,
																		&CacheTS_fops,
																		CacheTS_devices);
    
   
    printk("<==CacheTS_init \n");

    return 0; /* succeed */   
}

void CacheTS_cleanup(void)
{
		devfs_unregister(CacheTS_devfs);
		kfree(CacheTS_devices);		
}

module_init(CacheTS_init);
module_exit(CacheTS_cleanup);
