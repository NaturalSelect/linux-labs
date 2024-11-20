/*
 * SO2 lab3 - task 3
 */

#include "asm/current.h"
#include "linux/jiffies.h"
#include "linux/list.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_DESCRIPTION("Memory processing");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

struct task_info {
	pid_t pid;
	unsigned long timestamp;
};

static struct task_info *ti1, *ti2, *ti3, *ti4;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;

	ti = kmalloc(sizeof(struct task_info), GFP_KERNEL);
	ti->pid = pid;
	ti->timestamp = jiffies;
	return ti;
}

static void task_info_free(struct task_info *ti)
{
	kfree(ti);
}

static void print_task_info(struct task_info *ti)
{
	pr_info("PID: %d, timestamp: %lu\n", ti->pid, ti->timestamp);
}

static int memory_init(void)
{
	struct task_struct *curr = get_current();
	struct task_struct *parent = curr->parent;
	struct task_struct *next = next_task(curr);
	struct task_struct *nextOfNext = next_task(next);

	ti1 = task_info_alloc(curr->pid);
	ti2 = task_info_alloc(parent->pid);
	ti3 = task_info_alloc(next->pid);
	ti4 = task_info_alloc(nextOfNext->pid);
	return 0;
}

static void memory_exit(void)
{
	print_task_info(ti1);
	print_task_info(ti2);
	print_task_info(ti3);
	print_task_info(ti4);

	task_info_free(ti1);
	task_info_free(ti2);
	task_info_free(ti3);
	task_info_free(ti4);
}

module_init(memory_init);
module_exit(memory_exit);
