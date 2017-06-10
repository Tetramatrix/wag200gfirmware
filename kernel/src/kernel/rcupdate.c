/*
 * Read-Copy Update mechanism for mutual exclusion
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (c) International Business Machines Corp., 2001
 * Copyright (C) Andrea Arcangeli <andrea@suse.de> SuSE, 2001
 *
 * Author: Dipankar Sarma <dipankar@in.ibm.com>,
 *	   Andrea Arcangeli <andrea@suse.de>
 * 
 * Based on the original work by Paul McKenney <paul.mckenney@us.ibm.com>
 * and inputs from Andrea Arcangeli, Rusty Russell, Andi Kleen etc.
 * Papers:
 * http://www.rdrop.com/users/paulmck/paper/rclockpdcsproof.pdf
 * http://lse.sourceforge.net/locking/rclock_OLS.2001.05.01c.sc.pdf (OLS2001)
 *
 * For detailed explanation of Read-Copy Update mechanism see -
 * 		http://lse.sourceforge.net/locking/rcupdate.html
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/rcupdate.h>

/* Definition for rcupdate control block. */
static spinlock_t rcu_lock;
static struct list_head rcu_nxtlist;
static struct list_head rcu_curlist;
static struct tasklet_struct rcu_tasklet;
static unsigned long rcu_qsmask;
static int rcu_polling_in_progress;
static long rcu_quiescent_checkpoint[NR_CPUS];

/*
 * Register a new rcu callback. This will be invoked as soon
 * as all CPUs have performed a context switch or been seen in the
 * idle loop or in a user process.
 */
void call_rcu(struct rcu_head *head, void (*func)(void *arg), void *arg)
{
	head->func = func;
	head->arg = arg;

	spin_lock_bh(&rcu_lock);
	list_add(&head->list, &rcu_nxtlist);
	spin_unlock_bh(&rcu_lock);

	tasklet_hi_schedule(&rcu_tasklet);
}

static int rcu_prepare_polling(void)
{
	int stop;
	int i;

#ifdef DEBUG
	if (!list_empty(&rcu_curlist))
		BUG();
#endif

	stop = 1;
	if (!list_empty(&rcu_nxtlist)) {
		list_splice(&rcu_nxtlist, &rcu_curlist);
		INIT_LIST_HEAD(&rcu_nxtlist);

		rcu_polling_in_progress = 1;

		for (i = 0; i < smp_num_cpus; i++) {
			int cpu = cpu_logical_map(i);

			if (cpu != smp_processor_id()) {
				rcu_qsmask |= 1UL << cpu;
				rcu_quiescent_checkpoint[cpu] = RCU_quiescent(cpu);
				force_cpu_reschedule(cpu);
			}
		}
		stop = 0;
	}

	return stop;
}

/*
 * Invoke the completed RCU callbacks.
 */
static void rcu_invoke_callbacks(void)
{
	struct list_head *entry;
	struct rcu_head *head;

#ifdef DEBUG
	if (list_empty(&rcu_curlist))
		BUG();
#endif

	entry = rcu_curlist.prev;
	do {
		head = list_entry(entry, struct rcu_head, list);
		entry = entry->prev;

		head->func(head->arg);
	} while (entry != &rcu_curlist);

	INIT_LIST_HEAD(&rcu_curlist);
}

static int rcu_completion(void)
{
	int stop;

	rcu_polling_in_progress = 0;
	rcu_invoke_callbacks();

	stop = rcu_prepare_polling();

	return stop;
}

static int rcu_polling(void)
{
	int i;
	int stop;

	for (i = 0; i < smp_num_cpus; i++) {
		int cpu = cpu_logical_map(i);

		if (rcu_qsmask & (1UL << cpu))
			if (rcu_quiescent_checkpoint[cpu] != RCU_quiescent(cpu))
				rcu_qsmask &= ~(1UL << cpu);
	}

	stop = 0;
	if (!rcu_qsmask)
		stop = rcu_completion();

	return stop;
}

/*
 * Look into the per-cpu callback information to see if there is
 * any processing necessary - if so do it.
 */
static void rcu_process_callbacks(unsigned long data)
{
	int stop;

	spin_lock(&rcu_lock);
	if (!rcu_polling_in_progress)
		stop = rcu_prepare_polling();
	else
		stop = rcu_polling();
	spin_unlock(&rcu_lock);

	if (!stop)
		tasklet_hi_schedule(&rcu_tasklet);
}

/* Because of FASTCALL declaration of complete, we use this wrapper */
static void wakeme_after_rcu(void *completion)
{
	complete(completion);
}

/*
 * Initializes rcu mechanism.  Assumed to be called early.
 * That is before local timer(SMP) or jiffie timer (uniproc) is setup.
 */
void __init rcu_init(void)
{
	tasklet_init(&rcu_tasklet, rcu_process_callbacks, 0UL);
	INIT_LIST_HEAD(&rcu_nxtlist);
	INIT_LIST_HEAD(&rcu_curlist);
	spin_lock_init(&rcu_lock);
}

/*
 * Wait until all the CPUs have gone through a "quiescent" state.
 */
void synchronize_kernel(void)
{
	struct rcu_head rcu;
	DECLARE_COMPLETION(completion);

	/* Will wake me after RCU finished */
	call_rcu(&rcu, wakeme_after_rcu, &completion);

	/* Wait for it */
	wait_for_completion(&completion);
}

EXPORT_SYMBOL(call_rcu);
EXPORT_SYMBOL(synchronize_kernel);
