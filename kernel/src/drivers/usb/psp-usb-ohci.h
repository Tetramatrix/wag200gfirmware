#ifndef		_PSP_USB_OHCI_H 
#define		_PSP_USB_OHCI_H

#include <linux/pci.h>
#include <linux/config.h>


#ifdef CONFIG_USB_PSP_HOST11_VBUS
#define dma_to_pspbus(addr)        (PHYSADDR(addr))
#define pspbus_to_dma(addr)        (KSEG0ADDR(addr))/* verify whether KSEG0/1 */
#endif

#ifdef CONFIG_USB_PSP_HOST11_VLYNQ
/*On the Host processor side, base of Rx Window used for mapping RAM
 * Assuming Rx0 i used for mapping RAM on the host processor side.*/
extern unsigned int vlynq_host_rx_win_base ;

/* If Rx0 on host is not used for mapping the RAM, then cum.Size of 
 * all Rx prior to the one being used for mapping Ram is indicated by
 * this variable*/
extern unsigned int vlynq_host_rxsz_cum ;

/* device side Vlynq Portal -Tx Base */
extern unsigned int vlynq_remotedev_txbase ;


/*address translation is as follows:
 * VBUS to Vlynq = Phys(addr) -(rxsz_cum) - (rxwin_base) + (remotedev_txbase)
 * Vlynq to VBUS = Virt( (addr)-(remotedev_txbase) + (rxsz_cum) + (rxwin_base) )
 */
 
#define	dma_to_pspbus(addr)   (PHYSADDR(addr) - vlynq_host_rxsz_cum \
		            - vlynq_host_rx_win_base + vlynq_remotedev_txbase)

#define	pspbus_to_dma(addr)   (KSEG0ADDR( addr - vlynq_remotedev_txbase \
                              + vlynq_host_rxsz_cum + vlynq_host_rx_win_base ) )



extern void set_vlynq_rx_win_base(unsigned int addr);
extern void set_vlynq_rx_sz_cum(unsigned int cumulative_size);
extern void set_vlynq_remotedev_txbase(unsigned int addr);
extern void set_vlynq_offsets(unsigned int rxBase, unsigned int rxCumSz, unsigned int remTxBase);
#endif/* end of ifdef CONFIG_USB_PSP_HOST11_VLYNQ */



#define	pci_pool_create			psp_pci_pool_create
#define	pci_pool_destroy		psp_pci_pool_destroy
#define	pci_pool_free			psp_pci_pool_free
#define pci_pool_alloc			psp_pci_pool_alloc
#define pci_alloc_consistent	psp_pci_alloc_consistent
#define pci_free_consistent		psp_pci_free_consistent
#define pool_free_page			psp_pool_free_page
#define is_page_busy			psp_is_page_busy
#define pool_find_page			psp_pool_find_page
#define pool_alloc_page			psp_pool_alloc_page

#undef pci_map_single
#define pci_map_single psp_pci_map_single

#define  POOL_POISON_BYTE        0xa7
#define  POOL_TIMEOUT_JIFFIES    ((100 /* msec */ * HZ) / 1000)

//#define PRINT_DEBUG_MSG
/*  Enable this only if the debug messages are needed */

#ifdef PRINT_DEBUG_MSG
#define PSP_DEBUG	printk(KERN_ERR "%d: In Function %s\n",__LINE__,__FUNCTION__);
#endif

/* This is the structue filled by the pci_pool_create and sent back  */
struct psp_pci_pool {
	/* the pool */
	struct list_head        page_list;
	spinlock_t              lock;
	size_t                  blocks_per_page;
	size_t                  size;
	int                     flags;
	struct pci_dev          *dev;
	size_t                  allocation;
	char                    name [32];
	wait_queue_head_t       waitq;
};


struct psp_pci_page {
	/* cacheable header for 'allocation' bytes */
	struct list_head        page_list;
	void                    *vaddr;
	dma_addr_t              dma;
	unsigned long           bitmap [0];
};


struct psp_pci_pool * psp_pci_pool_create(const char *name, struct pci_dev *pdev,size_t size, size_t align, size_t allocation, int flags);
void pci_pool_destroy (struct psp_pci_pool *pool);
void psp_pci_pool_free (struct psp_pci_pool *pool, void *vaddr, dma_addr_t dma);
void * psp_pci_pool_alloc (struct psp_pci_pool *pool, int mem_flags, dma_addr_t *handle);
void * psp_pci_alloc_consistent(struct pci_dev *hwdev, size_t size, dma_addr_t * dma_handle);
void psp_pci_free_consistent(struct pci_dev *hwdev, size_t size, void *vaddr, dma_addr_t dma_handle);
static void psp_pool_free_page (struct psp_pci_pool *pool, struct psp_pci_page *page);
static inline int psp_is_page_busy (int blocks, unsigned long *bitmap);
static struct psp_pci_page * psp_pool_find_page (struct psp_pci_pool *pool, dma_addr_t dma);
static struct psp_pci_page * psp_pool_alloc_page (struct psp_pci_pool *pool, int mem_flags);
void set_pspbus_ohci_base(unsigned int addr);

/* Let this inline code be here  */

extern inline dma_addr_t psp_pci_map_single(struct pci_dev *hwdev, void *ptr, size_t size, int direction)
{

	if (direction == PCI_DMA_NONE)
		BUG();

#ifndef CONFIG_COHERENT_IO
	dma_cache_wback_inv((unsigned long)ptr, size);
#endif
#if  defined(CONFIG_USB_PSP_HOST11_VBUS) || defined(CONFIG_USB_PSP_HOST11_VLYNQ)
	return dma_to_pspbus(ptr);
#else
	return virt_to_bus(ptr);
#endif

}

#endif /* _PSP-USB-OHCI_H */
