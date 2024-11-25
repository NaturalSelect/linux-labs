/*
 * Deferred Work
 *
 * Exercise #1, #2: simple timer
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Simple kernel timer");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define TIMER_TIMEOUT 1

static struct timer_list timer;

static void timer_handler(struct timer_list *tl)
{
	pr_info("current jiffies %ld", jiffies);

	mod_timer(&timer, jiffies + TIMER_TIMEOUT * HZ);
}

static int __init timer_init(void)
{
	pr_info("[timer_init] Init module\n");

	timer_setup(&timer, timer_handler, 0);

	mod_timer(&timer, jiffies + TIMER_TIMEOUT * HZ);

	return 0;
}

static void __exit timer_exit(void)
{
	pr_info("[timer_exit] Exit module\n");

	del_timer_sync(&timer);
}

module_init(timer_init);
module_exit(timer_exit);
