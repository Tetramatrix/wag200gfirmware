/*
 *  linux/fs/open.c
 *
 *  Copyright (C) 1998-1999, Stephen Tweedie and Bill Hawes
 *
 *  Manage the dynamic fd arrays in the process files_struct.
 */

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <asm/bitops.h>
#include <linux/rcupdate.h>

struct rcu_fd_array {
	struct rcu_head rh;
	struct file **array;   
	int nfds;
};

struct rcu_fd_set {
	struct rcu_head rh;
	fd_set *openset;
	fd_set *execset;
	int nfds;
};

/*
 * Allocate an fd array, using kmalloc or vmalloc.
 * Note: the array isn't cleared at allocation time.
 */
struct file ** alloc_fd_array(int num)
{
	struct file **new_fds;
	int size = num * sizeof(struct file *);

	if (size <= PAGE_SIZE)
		new_fds = (struct file **) kmalloc(size, GFP_KERNEL);
	else 
		new_fds = (struct file **) vmalloc(size);
	return new_fds;
}

void free_fd_array(struct file **array, int num)
{
	int size = num * sizeof(struct file *);

	if (!array) {
		printk (KERN_ERR __FUNCTION__ "array = 0 (num = %d)\n", num);
		return;
	}

	if (num <= NR_OPEN_DEFAULT) /* Don't free the embedded fd array! */
		return;
	else if (size <= PAGE_SIZE)
		kfree(array);
	else
		vfree(array);
}

static void fd_array_callback(void *arg) 
{
	struct rcu_fd_array *a = (struct rcu_fd_array *) arg; 
	free_fd_array(a->array, a->nfds);
	kfree(arg); 
}

/*
 * Expand the fd array in the files_struct.  Called with the files
 * spinlock held for write.
 */

int expand_fd_array(struct files_struct *files, int nr)
{
	struct file **new_fds = NULL;
	int error, nfds = 0;
	struct rcu_fd_array *arg = NULL;

	
	error = -EMFILE;
	if (files->max_fds >= NR_OPEN || nr >= NR_OPEN)
		goto out;

	nfds = files->max_fds;
	spin_unlock(&files->file_lock);

	/* 
	 * Expand to the max in easy steps, and keep expanding it until
	 * we have enough for the requested fd array size. 
	 */

	do {
#if NR_OPEN_DEFAULT < 256
		if (nfds < 256)
			nfds = 256;
		else 
#endif
		if (nfds < (PAGE_SIZE / sizeof(struct file *)))
			nfds = PAGE_SIZE / sizeof(struct file *);
		else {
			nfds = nfds * 2;
			if (nfds > NR_OPEN)
				nfds = NR_OPEN;
		}
	} while (nfds <= nr);

	error = -ENOMEM;
	new_fds = alloc_fd_array(nfds);
	arg = (struct rcu_fd_array *) kmalloc(sizeof(*arg), GFP_ATOMIC);

	spin_lock(&files->file_lock);
	if (!new_fds || !arg)
		goto out;

	/* Copy the existing array and install the new pointer */

	if (nfds > files->max_fds) {
		struct file **old_fds = files->fd;
		int i = files->max_fds;

		/* Don't copy/clear the array if we are creating a new
		   fd array for fork() */
		if (i) {
			memcpy(new_fds, old_fds, i * sizeof(struct file *));
			/* clear the remainder of the array */
			memset(&new_fds[i], 0,
			       (nfds-i) * sizeof(struct file *)); 
		}

		wmb();
		files->fd = new_fds;
		wmb();
		files->max_fds = nfds;
		
		if (i) {
			arg->array = old_fds;
			arg->nfds = i;
			call_rcu(&arg->rh, fd_array_callback, arg);
		} else 
			kfree(arg);
	} else {
		/* Somebody expanded the array while we slept ... */
		spin_unlock(&files->file_lock);
		free_fd_array(new_fds, nfds);
		kfree(arg);
		spin_lock(&files->file_lock);
	}

	return 0;
out:
	if (new_fds)
		free_fd_array(new_fds, nfds);
	if (arg)
		kfree(arg);

	return error;
}

/*
 * Allocate an fdset array, using kmalloc or vmalloc.
 * Note: the array isn't cleared at allocation time.
 */
fd_set * alloc_fdset(int num)
{
	fd_set *new_fdset;
	int size = num / 8;

	if (size <= PAGE_SIZE)
		new_fdset = (fd_set *) kmalloc(size, GFP_KERNEL);
	else
		new_fdset = (fd_set *) vmalloc(size);
	return new_fdset;
}

void free_fdset(fd_set *array, int num)
{
	int size = num / 8;

	if (!array) {
		printk (KERN_ERR __FUNCTION__ "array = 0 (num = %d)\n", num);
		return;
	}
	
	if (num <= __FD_SETSIZE) /* Don't free an embedded fdset */
		return;
	else if (size <= PAGE_SIZE)
		kfree(array);
	else
		vfree(array);
}

static void fd_set_callback (void *arg)
{
	struct rcu_fd_set *a = (struct rcu_fd_set *) arg; 
	free_fdset(a->openset, a->nfds);
	free_fdset(a->execset, a->nfds);
	kfree(arg);
}

/*
 * Expand the fdset in the files_struct.  Called with the files spinlock
 * held for write.
 */
int expand_fdset(struct files_struct *files, int nr)
{
	fd_set *new_openset = 0, *new_execset = 0;
	int error, nfds = 0;
	struct rcu_fd_set *arg = NULL;

	error = -EMFILE;
	if (files->max_fdset >= NR_OPEN || nr >= NR_OPEN)
		goto out;

	nfds = files->max_fdset;
	spin_unlock(&files->file_lock);

	/* Expand to the max in easy steps */
	do {
		if (nfds < (PAGE_SIZE * 8))
			nfds = PAGE_SIZE * 8;
		else {
			nfds = nfds * 2;
			if (nfds > NR_OPEN)
				nfds = NR_OPEN;
		}
	} while (nfds <= nr);

	error = -ENOMEM;
	new_openset = alloc_fdset(nfds);
	new_execset = alloc_fdset(nfds);
	arg = (struct rcu_fd_set *) kmalloc(sizeof(*arg), GFP_ATOMIC);
	spin_lock(&files->file_lock);
	if (!new_openset || !new_execset || !arg)
		goto out;

	error = 0;
	
	/* Copy the existing tables and install the new pointers */
	if (nfds > files->max_fdset) {
		fd_set * old_openset = files->open_fds;
		fd_set * old_execset = files->close_on_exec;
		int old_nfds = files->max_fdset;
		int i = old_nfds / (sizeof(unsigned long) * 8);
		int count = (nfds - old_nfds) / 8;
		
		/* 
		 * Don't copy the entire array if the current fdset is
		 * not yet initialised.  
		 */
		if (i) {
			memcpy (new_openset, old_openset, old_nfds/8);
			memcpy (new_execset, old_execset, old_nfds/8);
			memset (&new_openset->fds_bits[i], 0, count);
			memset (&new_execset->fds_bits[i], 0, count);
		}
		
		wmb();
		files->open_fds =  new_openset;
		files->close_on_exec = new_execset;
		wmb();
		files->max_fdset = nfds;

		arg->openset = old_openset;
		arg->execset = old_execset;
		arg->nfds = nfds;
		call_rcu(&arg->rh, fd_set_callback, arg);

		return 0;
	} 
	/* Somebody expanded the array while we slept ... */

out:
	spin_unlock(&files->file_lock);
	if (new_openset)
		free_fdset(new_openset, nfds);
	if (new_execset)
		free_fdset(new_execset, nfds);
	if (arg)
		kfree(arg);
	spin_lock(&files->file_lock);
	return error;
}

