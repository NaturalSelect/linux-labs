#include "linux/printk.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_DESCRIPTION("List current processes");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

static int my_proc_init(void)
{
	struct task_struct *p = get_current();
	pr_info("Current process pid: %d, name: %s\n", p->pid, p->comm);
	pr_info("All process:\n");
	// NOTE: at <linux/sched/signal.h>
	for_each_process (p) {
		pr_info("pid: %d, name: %s\n", p->pid, p->comm);
	}
	return 0;
}

static void my_proc_exit(void)
{
	struct task_struct *p = get_current();
	pr_info("Current process pid: %d, name: %s\n", p->pid, p->comm);
}

module_init(my_proc_init);
module_exit(my_proc_exit);
