/*
 * $Id: avalanche-flash.c,v 1.1.1.1 2006-01-16 08:12:25 jeff Exp $
 *
 * Normal mappings of chips in physical memory
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>



#define WINDOW_ADDR CONFIG_MTD_AVALANCHE_START
#define WINDOW_SIZE CONFIG_MTD_AVALANCHE_LEN
#define BUSWIDTH CONFIG_MTD_AVALANCHE_BUSWIDTH

#define MTD4_FLASH_4M "0x903f0000,0x90400000"
#define MTD4_FLASH_2M "0x901f0000,0x90200000"

#include <asm/mips-boards/prom.h>
extern char *prom_getenv(char *name);

static int create_mtd_partitions(void);
//static void __exit avalanche_mtd_cleanup(void);
	
/* avalanche__partition_info gives details on the logical partitions that splits
 * the flash device into. If the size if zero we use up to the end of
 * the device. */
#define MAX_NUM_PARTITIONS 6
static struct mtd_partition avalanche_partition_info[MAX_NUM_PARTITIONS];
static int num_of_partitions = 0;

static struct mtd_info *avalanche_mtd_info;

int avalanche_mtd_ready=0;

static map_word avalanche_read16(struct map_info *map, unsigned long ofs)
{
        map_word val;
        val.x[0] = __raw_readw(map->map_priv_1 + ofs);
        return val;
}

static void avalanche_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, map->map_priv_1 + from, len);
}

static void avalanche_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	__raw_writew(d, map->map_priv_1 + adr);
	mb();
}

static void avalanche_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio(map->map_priv_1 + to, from, len);
}

struct map_info avalanche_map = {
	.name=  "Physically mapped flash",
	.size= WINDOW_SIZE,
	.bankwidth = BUSWIDTH,
	.read = avalanche_read16,
	.copy_from = avalanche_copy_from,
	.write = avalanche_write16,
	.copy_to =  avalanche_copy_to,
};

static int __init avalanche_mtd_init(void)
{
       	printk(KERN_NOTICE "avalanche flash device: 0x%lx at 0x%lx.\n", (unsigned long)WINDOW_SIZE, (unsigned long)WINDOW_ADDR);
	avalanche_map.map_priv_1 = (unsigned long)ioremap_nocache(WINDOW_ADDR, WINDOW_SIZE);

	if (!avalanche_map.map_priv_1) {
		printk("Failed to ioremap\n");
		return -EIO;
	}
		
	avalanche_mtd_info = do_map_probe("cfi_probe", &avalanche_map);
	if (!avalanche_mtd_info)
	{
		avalanche_mtd_cleanup();
		return -ENXIO;
	}
	
	avalanche_mtd_info->owner = THIS_MODULE;

	if (!create_mtd_partitions())
		add_mtd_device(avalanche_mtd_info);
	else		
		add_mtd_partitions(avalanche_mtd_info, avalanche_partition_info, num_of_partitions);

	avalanche_mtd_ready=1;
	
	return 0;
}

static char *strdup(char *str)
{
	int n = strlen(str)+1;
	char *s = kmalloc(n, GFP_KERNEL);
	if (!s) return NULL;
	return strcpy(s, str);
}


static int create_mtd_partitions(void)
{
	unsigned int offset;
	unsigned int size;
	unsigned int found;
	unsigned char *flash_base;
	unsigned char *flash_end;
	char *env_ptr;
	char *base_ptr;
	char *end_ptr;

	do {
		char	env_name[20];

		found = 0;

		/* get base and end addresses of flash file system from environment */
		sprintf(env_name, "mtd%1u", num_of_partitions);
printk("Looking for mtd device :%s:\n", env_name);
		env_ptr = prom_getenv(env_name);

		/*  add for support old FW */
		if(num_of_partitions==4 && env_ptr == NULL){
			env_ptr=prom_getenv("flashsize");
			if(env_ptr==NULL)
				break;
			if(strcmp(env_ptr,"0x00400000")==0)/* 4M */
				env_ptr = strdup(MTD4_FLASH_4M);
			else /* 2M */
				env_ptr = strdup(MTD4_FLASH_2M);
			
		}/*  add for support old FW */
			
		if(env_ptr == NULL) {
			/* No more partitions to find */
			break;
		}

		/* Extract the start and stop addresses of the partition */
		base_ptr = strtok(env_ptr, ",");
		end_ptr = strtok(NULL, ",");
		if ((base_ptr == NULL) || (end_ptr == NULL)) {	
			printk("JFFS2 ERROR: Invalid %s start,end.\n", env_name);
			break;
		}

		flash_base = (unsigned char*) simple_strtol(base_ptr, NULL, 0);
		flash_end = (unsigned char*) simple_strtol(end_ptr, NULL, 0);
		if((!flash_base) || (!flash_end)) {
			printk("JFFS2 ERROR: Invalid %s start,end.\n", env_name);
			break;
		}

		offset = virt_to_bus(flash_base) - WINDOW_ADDR;
		size = flash_end - flash_base;
		printk("Found a %s image (0x%x), with size (0x%x).\n",env_name, offset, size);

		/* Setup the partition info. We duplicate the env_name for the partiton name */
		avalanche_partition_info[num_of_partitions].name = strdup(env_name);
		avalanche_partition_info[num_of_partitions].offset = offset;
		avalanche_partition_info[num_of_partitions].size = size;
		avalanche_partition_info[num_of_partitions].mask_flags = 0;
		num_of_partitions++;
	} while (num_of_partitions < MAX_NUM_PARTITIONS);

	return num_of_partitions;
}

static void __exit avalanche_mtd_cleanup(void)
{
	avalanche_mtd_ready=0;

	if (avalanche_mtd_info) {
		del_mtd_partitions(avalanche_mtd_info);
		del_mtd_device(avalanche_mtd_info);
		map_destroy(avalanche_mtd_info);
	}

	if (avalanche_map.map_priv_1) {
		iounmap((void *)avalanche_map.map_priv_1);
		avalanche_map.map_priv_1 = 0;
	}
}

module_init(avalanche_mtd_init);
module_exit(avalanche_mtd_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Snehaprabha Narnakaje");
MODULE_DESCRIPTION("Avalanche CFI map driver");
