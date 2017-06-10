#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/cache.h>
#include <linux/rtnetlink.h>
#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/skbuff.h>

#include <net/protocol.h>
#include <net/dst.h>
#include <net/sock.h>
#include <net/checksum.h>


#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/system.h>

/* NSP Optimizations: This definition controls the static memory allocation optimization. */

extern int sysctl_hot_list_len;
extern struct sk_buff_head* get_skb_buff_list_head(int cpu_id);
extern kmem_cache_t* get_skbuff_head_cache(void);
extern void skb_drop_fraglist(struct sk_buff *skb);

#undef DEBUG_SKB 

/* NSP Optimizations: This structure definition defines the data structure that is used
 * for static buffer allocations.  */

/* Max. Reserved headroom in front of each packet so that the headers can be added to
 * a packet. Worst case scenario would be PPPoE + 2684 LLC Encapsulation + Ethernet
 * header. */
#define MAX_RESERVED_HEADROOM       20

/* This is the MAX size of the static buffer for pure data. */
#define MAX_SIZE_STATIC_BUFFER      1600

typedef struct DRIVER_BUFFER
{
    /* Pointer to the allocated data buffer. This is the static data buffer 
     * allocated for the TI-Cache. 60 bytes out of the below buffer are required
     * by the SKB shared info. We always reserve at least MAX_RESERVED_HEADROOM bytes 
     * so that the packets always have sufficient headroom. */
     char					ptr_buffer[MAX_SIZE_STATIC_BUFFER + MAX_RESERVED_HEADROOM + sizeof(struct skb_shared_info)];

     /* List of the driver buffers. */
     struct DRIVER_BUFFER*	ptr_next;

}DRIVER_BUFFER;

typedef struct DRIVER_BUFFER_MCB
{
    /* List of the driver buffers. */
    DRIVER_BUFFER*	ptr_available_driver_buffers;

    /* The number of available buffers. */

    int                     num_available_buffers;

}DRIVER_BUFFER_MCB;

DRIVER_BUFFER_MCB	driver_mcb;

int num_of_buffers(char* buf)
{
   unsigned long flags;
   int  len = 0;
   
   local_irq_save(flags);
   
   len = sprintf(buf,"Numbers of free Buffers %d\n",driver_mcb.num_available_buffers);
   /* Criticial Section: Unlock interrupts. */
   local_irq_restore(flags);
   return len;
}

int read_num_of_free_buff(void)
{
   return driver_mcb.num_available_buffers;
}

/**************************************************************************
 * FUNCTION NAME : skb_head_to_pool
 **************************************************************************
 * DESCRIPTION   :
 *  This function was optimized to put the SKB back into the pool, the 
 *  orignal code would keep only 'sysctl_hot_list_len' entries in the list
 *  the remaining skb's would be released back to the SLAB.
 * 
 * Caution : this has to be maintained in sync with its twin in the skbuff.c
 ***************************************************************************/
static __inline__ void skb_head_to_pool(struct sk_buff *skb)
{
    struct sk_buff_head* list;
    unsigned long        flags;
    kmem_cache_t* skbuff_head_cache = get_skbuff_head_cache( );

    local_irq_save(flags);
//    list = &skb_head_pool[smp_processor_id()].list;
    list = get_skb_buff_list_head(smp_processor_id());
    if (skb_queue_len(list) < sysctl_hot_list_len) 
    {
       __skb_queue_head(list, skb);
       local_irq_restore(flags);
       return;
    }
    else
    {
       local_irq_restore(flags);
       kmem_cache_free(skbuff_head_cache, skb);
       return;
    }
}

/* NSP Optimizations: These function were added to retrieve and replace buffers from the 
 * static memory pools instead of the Linux Slab Manager. */

static void skbuff_data_invalidate(void *pmem, int size)
{
  unsigned int i,Size=(((unsigned int)pmem)&0xf)+size;

  for (i=0;i<Size;i+=16,pmem+=16)
  {
    __asm__(" .set mips3 ");
    __asm__("cache  17, (%0)" : : "r" (pmem));
    __asm__(" .set mips0 ");
  }
}


/**************************************************************************
 * FUNCTION NAME : ti_release_skb
 **************************************************************************
 * DESCRIPTION   :
 *  This function is called from the ti_alloc_skb when there were no more
 *  data buffers available. The allocated SKB had to released back to the 
 *  data pool. The reason why this function was moved from the fast path 
 *  below was because '__skb_queue_head' is an inline function which adds
 *  a large code chunk on the fast path.
 *
 * NOTES         :
 *  This function is called with interrupts disabled. 
 **************************************************************************/
static void ti_release_skb (struct sk_buff_head* list, struct sk_buff* skb)
{
    __skb_queue_head(list, skb);
    return;
}

/**************************************************************************
 * FUNCTION NAME : ti_alloc_skb
 **************************************************************************
 * DESCRIPTION   :
 *  The function is called to allocate memory from the static allocated 
 *  TI-Cached memory pool.
 *
 * RETURNS       :
 *      Allocated static memory buffer - Success
 *      NULL                           - Error.
 **************************************************************************/
struct sk_buff *ti_alloc_skb(unsigned int size,int gfp_mask)
{
    register struct sk_buff*    skb;
    unsigned long               flags;
    struct sk_buff_head*        list;
    DRIVER_BUFFER*	        ptr_node = NULL;
    kmem_cache_t* skbuff_head_cache = get_skbuff_head_cache( );

    /* Critical Section Begin: Lock out interrupts. */
    local_irq_save(flags);

    /* Get the SKB Pool list associated with the processor and dequeue the head. */
//    list = &skb_head_pool[smp_processor_id()].list;
    list = get_skb_buff_list_head(smp_processor_id());
    skb = __skb_dequeue(list);

    if(skb == NULL) 
    {
       skb = kmem_cache_alloc(skbuff_head_cache, gfp_mask & ~__GFP_DMA);
       if (skb == NULL)
       {
#ifdef DEBUG_SKB 
          printk ("DEBUG: No SKB memory available:  = Number of free buffer:%d.\n", driver_mcb.num_available_buffers);
#endif
         local_irq_restore(flags); 
         goto nohead;
       }
    }
    
    size = SKB_DATA_ALIGN(size);

    /* see if we are asked to allocate more than we can offer */
    if(size > MAX_SIZE_STATIC_BUFFER) {
        skb_head_to_pool (skb);
        local_irq_restore(flags);
        goto nohead;
    }

    /* Did we get one. */
    if (skb != NULL)
    {
        /* YES. Now get a data block from the head of statically allocated block. */
        ptr_node = driver_mcb.ptr_available_driver_buffers;
	    if (ptr_node != NULL)
        {
            /* YES. Got a data block. Advance the free list pointer to the next available buffer. */
	        driver_mcb.ptr_available_driver_buffers = ptr_node->ptr_next;
            ptr_node->ptr_next = NULL;

            /* Decrement the number of available data buffers. */
            driver_mcb.num_available_buffers = driver_mcb.num_available_buffers - 1;
        }
        else
        {
          /* NO. Was unable to get a data block. So put the SKB back on the free list. 
           * This is slow path. */
#ifdef DEBUG_SKB 
           printk ("DEBUG: No Buffer memory available: call to ti_release_skb  = Number of free buffer:%d.\n",
                    driver_mcb.num_available_buffers);
#endif
           /* put the skb on the hot list or free it up (depending on the space
            * available on the hot list) : calling ti_release_skb instead 
            * would have always placed it on to the hot list. 
            */
           skb_head_to_pool (skb);
           
        }
    }
    
    /* Critical Section End: Unlock interrupts. */
    local_irq_restore(flags);

    /* Did we get an SKB and data buffer. Proceed only if we were succesful in getting both else drop */
    if (skb != NULL && ptr_node != NULL)
    {
       	/* XXX: does not include slab overhead */ 
        skb->truesize = size + sizeof(struct sk_buff);
 
        /* Load the data pointers. */
     	skb->head = ptr_node->ptr_buffer;
        skb->data = ptr_node->ptr_buffer + MAX_RESERVED_HEADROOM;
	    skb->tail = ptr_node->ptr_buffer + MAX_RESERVED_HEADROOM;
        skb->end  = ptr_node->ptr_buffer + size + MAX_RESERVED_HEADROOM;

	    /* Set up other state */
    	skb->len    = 0;
    	skb->cloned = 0;
        skb->data_len = 0;
    
        /* Mark the SKB indicating that the SKB is from the TI cache. */
        skb->ti_skb_id = 0xfeedbeef;

    	atomic_set(&skb->users, 1); 
    	atomic_set(&(skb_shinfo(skb)->dataref), 1);
        skb_shinfo(skb)->nr_frags = 0;
	    skb_shinfo(skb)->frag_list = NULL;
        return skb;
    }
nohead:
    /* Debug Message: No more buffers available. */
    /*local_irq_restore(flags);*/

    return NULL;
    
}

/**************************************************************************
 * FUNCTION NAME : ti_skb_release_fragment
 **************************************************************************
 * DESCRIPTION   :
 *  This function is called to release fragmented packets. This is NOT in 
 *  the fast path and this function requires some work. 
 **************************************************************************/
static void ti_skb_release_fragment(struct sk_buff *skb)
{

    if (skb_shinfo(skb)->nr_frags)
    {
        /* PANKAJ TODO: This portion has not been tested. */
	    int i;
#ifdef DEBUG_SKB 
        printk ("DEBUG: Releasing fragments in TI-Cached code.\n");
#endif
    	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
#ifdef DEBUG_SKB
	        printk ("DEBUG: Fragmented Page = 0x%p.\n", skb_shinfo(skb)->frags[i].page);
#endif
            put_page(skb_shinfo(skb)->frags[i].page);
        }
    }

    /* Check if there were any fragments present and if so clean all the SKB's. 
     * This is required to recursivly clean the SKB's. */
    if (skb_shinfo(skb)->frag_list)
        skb_drop_fraglist(skb);

    return;
}

/**************************************************************************
 * FUNCTION NAME : ti_skb_release_data
 **************************************************************************
 * DESCRIPTION   :
 *  The function is called to release the SKB back into the TI-Cached static
 *  memory pool. 
 **************************************************************************/
void ti_skb_release_data(struct sk_buff *skb)
{
    DRIVER_BUFFER*	ptr_node;
    unsigned long   flags;


    /* The SKB data can be cleaned only if the packet has not been cloned and we
     * are the only one holding a reference to the data. */
    if (!skb->cloned || atomic_dec_and_test(&(skb_shinfo(skb)->dataref)))
    {
        /* Are there any fragments associated with the SKB ?*/
	    if ((skb_shinfo(skb)->nr_frags != 0) || (skb_shinfo(skb)->frag_list != NULL))
        {
            /* Slow Path: Try and clean up the fragments. */
            ti_skb_release_fragment (skb);
        }

        /* Cleanup the SKB data memory. This is fast path. */
        ptr_node = (DRIVER_BUFFER *)skb->head;

        /* Critical Section: Lock out interrupts. */
        local_irq_save(flags);

        /* Add the data buffer to the list of available buffers. */
        ptr_node->ptr_next = driver_mcb.ptr_available_driver_buffers;
	    driver_mcb.ptr_available_driver_buffers = ptr_node;

        /* Increment the number of available data buffers. */
        driver_mcb.num_available_buffers = driver_mcb.num_available_buffers + 1;


        /* Criticial Section: Unlock interrupts. */
        local_irq_restore(flags);

    }

    return;
}


#ifdef DEBUG_SKB 
static int tiskb_num_of_buf(char* buf, char **start, off_t offset, int count,int *eof, void *data)
{
    int  len = 0;

     len += sprintf(buf+len, "num of available TI SKB buffers : %d\n", driver_mcb.num_available_buffers);

   return len;
}
#endif

char *prom_getenv(char *envname);

void ti_skb_init(void)
{
    int i;
    struct sk_buff *skb;
   
    char*           ptr_num_buffers;
    int             num_buffers;
    kmem_cache_t*   skbuff_head_cache = get_skbuff_head_cache( );

    /* Initialize the master control block.	*/
    memset ((void *)&driver_mcb, 0 , sizeof(driver_mcb));

    /* NSP Optimizations: Statically allocate memory for the TI-Cached Memory pool. */

    /* Debug Message.. */
    printk ("TI Optimizations: Allocating TI-Cached Memory Pool.\n");

    /* Read the number of buffers from an environment variable. */
    ptr_num_buffers = prom_getenv("StaticBuffer");
    if(ptr_num_buffers)
    {
       num_buffers = simple_strtoul(ptr_num_buffers, (char **)NULL, 10);
    }
    else
    {
       printk ("Warning: Number of buffers is not configured.Setting default to 512\n");
       num_buffers = 512;
    }

#ifdef DEBUG_SKB 
    create_proc_read_entry("avalanche/tiskb_buf_available", 0, NULL,tiskb_num_of_buf , NULL);
#endif

    printk ("Using %d Buffers for TI-Cached Memory Pool.\n", num_buffers);

    /* PANKAJ TODO: Currently this is hardcoded, need to read from an environment 
     * variable. */
    for (i=0; i < num_buffers ; i++)
    {
        /* Allocate memory for the SKB. */
        skb = kmem_cache_alloc(skbuff_head_cache, GFP_KERNEL);
        if (skb == NULL)
        {
            printk ("Error: SKBuff allocation failed for SKB: %d.\n", i);
            return;
        }
        
        /* Add the SKB to the pool. */
        skb_head_to_pool(skb);
    }

    /* Initialize the master control block.	*/
    memset ((void *)&driver_mcb, 0 , sizeof(driver_mcb));

    /* Allocate memory for the TI-Cached Data buffers. */
    for (i=0; i< num_buffers ; i++)
    {
        DRIVER_BUFFER*	ptr_driver_buffer;

	/* Allocate memory for the driver buffer. */
	ptr_driver_buffer = (DRIVER_BUFFER *)kmalloc((sizeof(DRIVER_BUFFER)),GFP_KERNEL);
	if (ptr_driver_buffer == NULL)
	{
	    printk ("Error: Unable to allocate memory for the driver buffers Failed:%d.\n",i);
	    return;
	}

	/* Initialize the allocated block of memory. */
	memset ((void *)ptr_driver_buffer, 0, sizeof(DRIVER_BUFFER));

	/* Add the allocated block to the list of available blocks */
	if (driver_mcb.ptr_available_driver_buffers == NULL)
	{
	    driver_mcb.ptr_available_driver_buffers = ptr_driver_buffer;
	}
	else
	{
	    ptr_driver_buffer->ptr_next = driver_mcb.ptr_available_driver_buffers;
	    driver_mcb.ptr_available_driver_buffers = ptr_driver_buffer;
	}

        /* Increment the number of available data buffers. */
        driver_mcb.num_available_buffers = driver_mcb.num_available_buffers + 1;
        

        /* Mick: Cache Invalidate. */
        skbuff_data_invalidate ((void *)ptr_driver_buffer->ptr_buffer,
                                MAX_SIZE_STATIC_BUFFER + MAX_RESERVED_HEADROOM + 60);
    }

    printk ("NSP Optimizations: Succesfully allocated TI-Cached Memory Pool.\n");

    return;
}
