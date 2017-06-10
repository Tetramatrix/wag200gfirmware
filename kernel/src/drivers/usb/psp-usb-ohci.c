#include "psp-usb-ohci.h"
#include "psp-usb-host-config.h"

/* usb-titan-ohci-nonpci.c  */
/* START  */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/pci.h>
#include <linux/proc_fs.h>

#include <asm/irq.h>
#include <asm/io.h>
#include "usb-ohci.h"

#include <asm/avalanche/avalanche_map.h>
#include <asm/avalanche/generic/avalanche_misc.h>
#include <asm/avalanche/generic/avalanche_intc.h>

#ifdef CONFIG_MIPS_TITAN
#include <asm/avalanche/titan/titan.h>
#include <asm/avalanche/titan/titan_clk_cntl.h>
#endif


#define PSP_USB_HOST_DRIVER_VERSION "0.0.1"

/* prototype definitions */

int __devinit hc_add_ohci(struct pci_dev *dev, int irq, void *membase, unsigned long flags,
	    ohci_t **ohci, const char *name, const char *slot_name);

static int  hostDriver_proc_version(char *buf, char **start, off_t offset, int count, int *eof, void *data);

static int  ohciHcRev_proc_version(char *buf, char **start, off_t offset, int count, int *eof, void *data);

extern void hc_remove_ohci(ohci_t *ohci);

static ohci_t *nonpci_ohci;

// Boot options
static unsigned int ohci_base=0, ohci_len=0;
static int ohci_irq=-1;
// bogus pci_dev
static struct pci_dev bogus_pcidev;
// ioremapped ohci_base
static void *mem_base;

/* prototypes */
static void usb_host_delay(unsigned long);

MODULE_PARM(ohci_base, "i");
MODULE_PARM(ohci_len, "i");
MODULE_PARM(ohci_irq, "i");
MODULE_PARM_DESC(ohci_base, "IO Base address of OHCI Oper. registers");
MODULE_PARM_DESC(ohci_len, "IO length of OHCI Oper. registers");
MODULE_PARM_DESC(ohci_irq, "IRQ for OHCI interrupts");

#ifdef CONFIG_USB_PSP_HOST11_VBUS

void host_reset()
{
	avalanche_reset_ctrl(AVALANCHE_USB_MASTER_RESET_BIT, IN_RESET);
	usb_host_delay(200);
	avalanche_reset_ctrl(AVALANCHE_USB_MASTER_RESET_BIT, OUT_OF_RESET);
	usb_host_delay(200);
}

#endif

/* END  */
/* usb-titan-ohci-nonpci.c  */


#ifdef CONFIG_USB_PSP_HOST11_VLYNQ
/*On the Host processor side, base of Rx Window used for mapping RAM
 * Assuming Rx0 i used for mapping RAM on the host processor side.*/
unsigned int vlynq_host_rx_win_base = 0;

/* If Rx0 on host is not used for mapping the RAM, then cum.Size of 
 * all Rx prior to the one being used for mapping Ram is indicated by
 * this variable*/
unsigned int vlynq_host_rxsz_cum = 0;

/* device side Vlynq Portal -Tx Base */
unsigned int vlynq_remotedev_txbase = 0;


void set_vlynq_rx_win_base(unsigned int addr)
{
	vlynq_host_rx_win_base = addr;
	return;
}

void set_vlynq_rx_sz_cum(unsigned int cumulative_size)
{
	vlynq_host_rxsz_cum = cumulative_size;
	return;
}


void set_vlynq_remotedev_txbase(unsigned int addr)
{
	vlynq_remotedev_txbase = addr;
	return;
}


void set_vlynq_offsets(unsigned int rxBase, unsigned int rxCumSz, unsigned int remTxBase)
{
	vlynq_remotedev_txbase = remTxBase;
	vlynq_host_rxsz_cum = rxCumSz;
	vlynq_host_rx_win_base = rxBase;
	return;
}
#endif/* end of ifdef CONFIG_USB_PSP_HOST11_VLYNQ */



/*

These macros are essentially the same as above. Just it uses the standard macros.
   
unsigned int sdram_base = 0; // Offset 1
unsigned int pspbus_base = 0; // Offset 2 

#define virt_to_pspbus(addr)	(PHYSADDR(addr) - sdram_base + pspbus_base )
#define pspbus_to_virt(addr)	KSEG0ADDR(addr - pspbus_base + sdram_base )

*/





struct psp_pci_pool * psp_pci_pool_create (const char *name, struct pci_dev *pdev, size_t size, size_t align, size_t allocation, int flags)
{
	struct psp_pci_pool	*retval;

	 /* Essentially making the size be in compliænce with the align value  */
	if (align == 0)
		align = 1;

	if (size == 0)
		return 0;
	else if (size < align)
		size = align;
	else if ((size % align) != 0) {
		size += align + 1;
		size &= ~(align - 1);
	}

	/* If the boundry is not given  */
	if (allocation == 0) {
		if (PAGE_SIZE < size)
		{
			 /* PAGE_SIZE is
			  * #define PAGE_SIZE (1UL << PAGE_SHIFT)
			  * and PAGE_SHIFT is 12. So the final value of PAGE_SIZE is 4096 .
			  * Its also confirmed through a printk*/
			allocation = size;
		}else{
			allocation = PAGE_SIZE;
		}
 /* Making sures the allocation does not go beyond the page size  */

		/* FIXME: round up for less fragmentation */

	} else if (allocation < size){
		return 0;
	}

	if (!(retval = kmalloc (sizeof *retval, flags)))
		return retval;

#ifdef	CONFIG_PCIPOOL_DEBUG
	flags |= SLAB_POISON;
#endif

	strncpy (retval->name, name, sizeof retval->name);
	retval->name [sizeof retval->name - 1] = 0;

	retval->dev = pdev;
	INIT_LIST_HEAD (&retval->page_list);
	spin_lock_init (&retval->lock);
	retval->size = size;
	retval->flags = flags;
	retval->allocation = allocation;
	retval->blocks_per_page = allocation / size;
	init_waitqueue_head (&retval->waitq);

#ifdef CONFIG_PCIPOOL_DEBUG
	printk (KERN_DEBUG "pcipool create %s/%s size %d, %d/page (%d alloc)\n",
		pdev ? pdev->slot_name : NULL, retval->name, size,
		retval->blocks_per_page, allocation);
#endif

	return retval;
}
	
 /* This function will free the area occupied by a device   */
void psp_pci_pool_destroy (struct psp_pci_pool *pool)
{
	unsigned long		flags;

#ifdef CONFIG_PCIPOOL_DEBUG
	printk (KERN_DEBUG "pcipool destroy %s/%s\n",
			pool->dev ? pool->dev->slot_name : NULL,
			pool->name);
#endif

	spin_lock_irqsave (&pool->lock, flags);

	 /* Remove all the allocations inside this pool  */
	while (!list_empty (&pool->page_list)) {
		struct psp_pci_page		*page;
		page = list_entry (pool->page_list.next,
				struct psp_pci_page, page_list);
		if (is_page_busy (pool->blocks_per_page, page->bitmap)) {
			printk (KERN_ERR "pci_pool_destroy %s/%s, %p busy\n",
					pool->dev ? pool->dev->slot_name : NULL,
					pool->name, page->vaddr);
			/* leak the still-in-use consistent memory */
			list_del (&page->page_list);
			kfree (page);
		} else
			pool_free_page (pool, page);
	}
	spin_unlock_irqrestore (&pool->lock, flags);
	kfree (pool);
}


void psp_pci_pool_free (struct psp_pci_pool *pool, void *vaddr, dma_addr_t dma)
{
	struct psp_pci_page		*page;
	unsigned long		flags;
	int			map, block;

	if ((page = pool_find_page (pool, dma)) == 0) {
		printk (KERN_ERR "pci_pool_free %s/%s, %p/%x (bad dma)\n",
				pool->dev ? pool->dev->slot_name : NULL,
				pool->name, vaddr, (int) (dma & 0xffffffff));
		return;
	}

#ifdef	CONFIG_PCIPOOL_DEBUG
	if (((dma - page->dma) + (void *)page->vaddr) != vaddr) {
		printk (KERN_ERR "pci_pool_free %s/%s, %p (bad vaddr)/%x\n",
				pool->dev ? pool->dev->slot_name : NULL,
				pool->name, vaddr, (int) (dma & 0xffffffff));
		return;
	}
#endif

	block = dma - page->dma;
	block /= pool->size;
	map = block / BITS_PER_LONG;
	block %= BITS_PER_LONG;

#ifdef	CONFIG_PCIPOOL_DEBUG
	if (page->bitmap [map] & (1UL << block)) {
		printk (KERN_ERR "pci_pool_free %s/%s, dma %x already free\n",
				pool->dev ? pool->dev->slot_name : NULL,
				pool->name, dma);
		return;
	}
#endif
	if (pool->flags & SLAB_POISON)
		memset (vaddr, POOL_POISON_BYTE, pool->size);

	spin_lock_irqsave (&pool->lock, flags);
	set_bit (block, &page->bitmap [map]);
	if (waitqueue_active (&pool->waitq))
		wake_up (&pool->waitq);
	/*
	 * Resist a temptation to do
	 *    if (!is_page_busy(bpp, page->bitmap)) pool_free_page(pool, page);
	 * it is not interrupt safe. Better have empty pages hang around.
	 */
	spin_unlock_irqrestore (&pool->lock, flags);
}


void * psp_pci_pool_alloc (struct psp_pci_pool *pool, int mem_flags, dma_addr_t *handle)
{
	unsigned long		flags;
	struct list_head	*entry;
	struct psp_pci_page		*page;
	int			map, block;
	size_t			offset;
	void			*retval;

restart:
	spin_lock_irqsave (&pool->lock, flags);
	list_for_each (entry, &pool->page_list) {
		int		i;
		page = list_entry (entry, struct psp_pci_page, page_list);
		/* only cachable accesses here ... */
		for (map = 0, i = 0;
				i < pool->blocks_per_page;
				i += BITS_PER_LONG, map++) {
			if (page->bitmap [map] == 0)
				continue;
			block = ffz (~ page->bitmap [map]);
			if ((i + block) < pool->blocks_per_page) {
				clear_bit (block, &page->bitmap [map]);
				offset = (BITS_PER_LONG * map) + block;
				offset *= pool->size;
				goto ready;
			}
		}
	}
	if (!(page = pool_alloc_page (pool, mem_flags))) {
		if (mem_flags == SLAB_KERNEL) {
			DECLARE_WAITQUEUE (wait, current);

			current->state = TASK_INTERRUPTIBLE;
			add_wait_queue (&pool->waitq, &wait);
			spin_unlock_irqrestore (&pool->lock, flags);

			schedule_timeout (POOL_TIMEOUT_JIFFIES);

			current->state = TASK_RUNNING;
			remove_wait_queue (&pool->waitq, &wait);
			goto restart;
		}
		retval = 0;
		goto done;
	}

	clear_bit (0, &page->bitmap [0]);
	offset = 0;
ready:
	retval = offset + page->vaddr;
	*handle = offset + page->dma;
done:
	spin_unlock_irqrestore (&pool->lock, flags);
	return retval;
}


void * psp_pci_alloc_consistent(struct pci_dev *hwdev, size_t size, dma_addr_t * dma_handle)
{
	  
	void *ret;

	int gfp = GFP_ATOMIC;

	if (hwdev == NULL || hwdev->dma_mask != 0xffffffff)
		gfp |= GFP_DMA;

	ret = (void *) __get_free_pages(gfp, get_order(size));

	if (ret != NULL) {
		memset(ret, 0, size);

#ifdef CONFIG_NONCOHERENT_IO
		dma_cache_wback_inv((unsigned long) ret, size);
		ret = KSEG1ADDR(ret);
#endif
		
#if defined(CONFIG_USB_PSP_HOST11_VBUS) || defined(CONFIG_USB_PSP_HOST11_VLYNQ)
		*dma_handle = dma_to_pspbus(ret);
#else
		*dma_handle = virt_to_bus(ret);
#endif
	}

	return ret;
}


void psp_pci_free_consistent(struct pci_dev *hwdev, size_t size, void *vaddr, dma_addr_t dma_handle)
{

	unsigned long addr = (unsigned long) vaddr;

#ifdef CONFIG_NONCOHERENT_IO
	addr = KSEG0ADDR(addr);
#endif

	free_pages(addr, get_order(size));

	return;
}

 /* EXTRA Functions, that were needed to make the above function work  */

static inline int psp_is_page_busy (int blocks, unsigned long *bitmap)
{
	while (blocks > 0) {
		if (*bitmap++ != ~0UL)
			return 1;
		blocks -= BITS_PER_LONG;
	}
	return 0;
}

static void psp_pool_free_page (struct psp_pci_pool *pool, struct psp_pci_page *page)
{

	dma_addr_t  dma = page->dma;

	if (pool->flags & SLAB_POISON)
		memset (page->vaddr, POOL_POISON_BYTE, pool->allocation);

	pci_free_consistent (pool->dev, pool->allocation, page->vaddr, dma);
	list_del (&page->page_list);
	kfree (page);
}

static struct psp_pci_page * psp_pool_find_page (struct psp_pci_pool *pool, dma_addr_t dma)
{
	unsigned long       flags;
	struct list_head    *entry;
	struct psp_pci_page     *page;

	spin_lock_irqsave (&pool->lock, flags);
	
	list_for_each (entry, &pool->page_list) {
		page = list_entry (entry, struct psp_pci_page, page_list);
		if (dma < page->dma)
			continue;
		if (dma < (page->dma + pool->allocation))
			goto done;
	}
	page = 0;

done:
	spin_unlock_irqrestore (&pool->lock, flags);
	
	return page;
}

static struct psp_pci_page * psp_pool_alloc_page (struct psp_pci_pool *pool, int mem_flags)
{
	struct psp_pci_page	*page;
	int		mapsize;

	mapsize = pool->blocks_per_page;
	mapsize = (mapsize + BITS_PER_LONG - 1) / BITS_PER_LONG;
	mapsize *= sizeof (long);

	page = (struct psp_pci_page *) kmalloc (mapsize + sizeof *page, mem_flags);
	if (!page)
		return 0;
	page->vaddr = pci_alloc_consistent (pool->dev,
			pool->allocation,
			&page->dma);
	if (page->vaddr) {
		memset (page->bitmap, 0xff, mapsize);	// bit set == free
		if (pool->flags & SLAB_POISON)
			memset (page->vaddr, POOL_POISON_BYTE, pool->allocation);
		list_add (&page->page_list, &pool->page_list);
	} else {
		kfree (page);
		page = 0;
	}
	return page;
}

/* usb-titan-ohci-nonpci.c  */

#ifndef MODULE	

static int __init
ohci_setup (char* options)
{
	char* this_opt;
	
	if (!options || !*options)
		return 0;

	for(this_opt=strtok(options, ","); this_opt; this_opt=strtok(NULL, ",")) {
		if (!strncmp(this_opt, "base:", 5)) {
			ohci_base = simple_strtoul(this_opt+5, NULL, 0);
		} else if (!strncmp(this_opt, "len:", 4)) {
			ohci_len = simple_strtoul(this_opt+4, NULL, 0);
		} else if (!strncmp(this_opt, "irq:", 4)) {
			ohci_irq = simple_strtoul(this_opt+4, NULL, 0);
		}
	}

	return 0;
}

__setup("usb_ohci=", ohci_setup);

#endif


static int __init nonpci_ohci_init(void)
{
	int ret;
	

        nonpci_ohci = NULL;

#if   !defined(CONFIG_USB_PSP_HOST11_VBUS) && !defined(CONFIG_USB_PSP_HOST11_VLYNQ)
	/* Actually this is not needed. It will never get compiled  */
	if (!ohci_base || !ohci_len || (ohci_irq < 0))
		return -ENODEV;

	if (!request_mem_region (ohci_base, ohci_len, "usb-ohci")) {
		dbg ("controller already in use");
		return -EBUSY;
	}

	mem_base = ioremap_nocache (ohci_base, ohci_len);
	if (!mem_base) {
		err("Error mapping OHCI memory");
		return -EFAULT;
	}

#else  /* CONFIG_MIPS_TITAN  */

#ifdef	CONFIG_USB_PSP_HOST11_VBUS

	host_reset(); /* put the module in reset and pull it out  */

	
	if(ohci_base == 0) ohci_base = AVALANCHE_USB_MASTER_CONTROL_BASE;
	mem_base = ohci_base;
	if(ohci_len == 0) ohci_len = 256;
	if(ohci_irq == -1)ohci_irq = (unsigned int)LNXINTNUM(AVALANCHE_USB_MASTER_INT);
	
	/*set_sdram_base_addr(0x0);
	set_pspbus_base_addr(0x0);*/

        /* configure clock frequency */	
	avalanche_clkc_set_freq(CLKC_USB, CLK_MHZ(48)); /* Common Code  */
	
#endif  /* CONFIG_USB_PSP_HOST11_VBUS  */

#ifdef CONFIG_USB_PSP_HOST11_VLYNQ
{
	VLYNQ_CFG_HDR * vHdr;

	vHdr = usb_host_get_config();
	if(ohci_base == 0) ohci_base = vHdr->baseaddress;
	mem_base = ohci_base;
	if(ohci_len == 0)ohci_len = 256;
	if(ohci_irq == -1) ohci_irq = vHdr->irq + AVALANCHE_INT_END_SECONDARY;

        /* put host module into reset */
	*(volatile unsigned int*)(vHdr->reset_base) &= (~(1<<vHdr->reset_bit));
	usb_host_delay(5000);

	/* pull th host module out of reset */
	*(volatile unsigned int*)(vHdr->reset_base) |= (1<<vHdr->reset_bit);
	usb_host_delay(5000);

	/* Configure Usb Host Clock */
	*(volatile unsigned int *)(vHdr->clock_base) |= vHdr->clock_val;
	usb_host_delay(5000);
	
        /* Configure endianess */
	*(volatile unsigned int *)(vHdr->endian_reg) |= vHdr->endian_val;
	usb_host_delay(5000);
	
	/* configure vlynq offsets for address translation */
	set_vlynq_offsets(0x14000000,0,0);
	
}
	
#endif /* end of ifdef CONFIG_USB_PSP_HOST11_VLYNQ block */
	
#endif /*end of else block for if !defined   */

	/*
	 * Fill in some fields of the bogus pci_dev.
	 */
	memset(&bogus_pcidev, 0, sizeof(struct pci_dev));
	strcpy(bogus_pcidev.name, "non-PCI OHCI");
	strcpy(bogus_pcidev.slot_name, "builtin");
	bogus_pcidev.resource[0].name = "OHCI Operational Registers";
	bogus_pcidev.resource[0].start = ohci_base;
	bogus_pcidev.resource[0].end = ohci_base + ohci_len;
	bogus_pcidev.resource[0].flags = 0;
	bogus_pcidev.irq = ohci_irq;

	/*
	 * Initialise the generic OHCI driver.
	 */
	ret = hc_add_ohci(&bogus_pcidev, ohci_irq, mem_base, 0, 
			   &nonpci_ohci, "usb-ohci", "builtin");

#if !defined(CONFIG_USB_PSP_HOST11_VBUS) && !defined(CONFIG_USB_PSP_HOST11_VLYNQ)
	if (ret) {
		iounmap(mem_base);
		release_mem_region(ohci_base, ohci_len);
	}
#else
/* create custom proc entries for PSP USB Host  */
	create_proc_read_entry ("avalanche/usb_host_driver_ver",0444,NULL,hostDriver_proc_version,NULL);
	create_proc_read_entry ("avalanche/ohci_HcRev",0444,NULL,ohciHcRev_proc_version,NULL);
#endif

	return ret;
}

static void __exit nonpci_ohci_exit(void)
{
	hc_remove_ohci(nonpci_ohci);
        remove_proc_entry("avalanche/usb_host_driver_ver",NULL);
	remove_proc_entry("avalanche/ohci_HcRev",NULL);
#if   !defined(CONFIG_USB_PSP_HOST11_VBUS) && !defined(CONFIG_USB_PSP_HOST11_VLYNQ)
	release_mem_region(ohci_base, ohci_len);
#endif	
}



volatile int host_delay_ctr = 0 ;
static void usb_host_delay(unsigned long clkTime)
{
	unsigned long i = 0;
	volatile int * ptr =  &host_delay_ctr;

	/* crude approx.loop.We assume that each cycle takes  about 
	 * 23 assembly instructions */
	for(i=0;i<(clkTime+23)/23;i++)
	{
		*ptr = *ptr + 1;
	}
}/* end of function usb_host_delay() */

/* functions for custom proc entries.These custome entries are created as 
 * part of /proc/avalanche  */
static int hostDriver_proc_version(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len=0;
/* Driver Version */
	len += sprintf(buf+len, "PSP USB Host Driver Version ");
	len += sprintf(buf+len, PSP_USB_HOST_DRIVER_VERSION" \n");
	return len;
}	

static int  ohciHcRev_proc_version(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    int len=0;
    unsigned int *addr;
    unsigned char val;

    /* HcRevision Register from OHCI */
    len += sprintf(buf+len, "OHCI: HCRevision  ");

    if(nonpci_ohci)
      {
	      addr = &(nonpci_ohci->regs->revision);
	      val = (unsigned char) ( (*addr) & 0xFF );
	      len += sprintf(buf+len,"0X%02x \n",val);
      }
    else
    {
	    len += sprintf(buf+len,"Error:OHCI driver initialization failed\n");
    }
    return len;
}


module_init(nonpci_ohci_init);
module_exit(nonpci_ohci_exit);
