#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("Command-line args module");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

static char *str = "the worm";

module_param(str, charp, 0000);
MODULE_PARM_DESC(str, "A simple string");

/*
root@qemux86:~# insmod skels/kernel_modules/6-cmd-mod/cmd_mod.ko str=tired
cmd_mod: loading out-of-tree module taints kernel.
Early bird gets tired
*/

static int __init cmd_init(void)
{
	pr_info("Early bird gets %s\n", str);
	return 0;
}

static void __exit cmd_exit(void)
{
	pr_info("Exit, stage left\n");
}

module_init(cmd_init);
module_exit(cmd_exit);
