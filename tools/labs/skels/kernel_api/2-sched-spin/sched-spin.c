/*
 * Kernel API lab
 *
 * sched-spin.c: Sleeping in atomic context
 */

#include "linux/spinlock.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Sleep while atomic");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

/*
root@qemux86:~# insmod skels/kernel_api/2-sched-spin/sched-spin.ko
BUG: scheduling while atomic: insmod/627/0x00000002
1 lock held by insmod/627:
 #0: c4cb3d88 (&lock){+.+.}-{2:2}, at: sched_spin_init+0x32/0x90 [sched_spin]
Modules linked in: sched_spin(O+) [last unloaded: sched_spin]
CPU: 0 PID: 627 Comm: insmod Tainted: G        W  O      5.10.14+ #5
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS 1.15.0-1 04/01/2014
Call Trace:
 dump_stack+0x6d/0x8b
 __schedule_bug.cold+0x6e/0x81
 __schedule+0x5f7/0x760
 ? __mod_timer+0x1ed/0x320
 schedule+0x56/0xd0
 ? 0xe0831000
 schedule_timeout+0xaa/0x1c0
 ? trace_raw_output_hrtimer_start+0xa0/0xa0
 sched_spin_init+0x61/0x90 [sched_spin]
 ? sched_spin_init+0x32/0x90 [sched_spin]
 do_one_initcall+0x57/0x2d0
 ? do_init_module+0x1f/0x230
 ? rcu_read_lock_sched_held+0x41/0x70
 ? kmem_cache_alloc_trace+0x2cb/0x340
 do_init_module+0x4e/0x230
 load_module+0x2370/0x2820
 ? kernel_read+0x39/0x50
 ? kernel_read_file_from_fd+0x4c/0x90
 __ia32_sys_finit_module+0x9a/0xd0
 do_int80_syscall_32+0x2c/0x40
 entry_INT80_32+0xf7/0xf7
EIP: 0xb7dd8222
Code: 06 89 8a f0 02 00 00 c3 55 57 56 53 8b 6c 24 2c 8b 7c 24 28 8b 74 24 24 8b 54 24 20 8b 4c 24 1c 8b 5c 24 18 8b 44 24 140
EAX: ffffffda EBX: 00000003 ECX: 01d81160 EDX: 00000000
ESI: 00000100 EDI: 00000100 EBP: 00000000 ESP: bfbf2fbc
DS: 007b ES: 007b FS: 0000 GS: 0033 SS: 007b EFLAGS: 00000206
------------[ cut here ]------------
initcall sched_spin_init+0x0/0x90 [sched_spin] returned with preemption imbalance
WARNING: CPU: 0 PID: 627 at init/main.c:1230 do_one_initcall+0x207/0x2d0
Modules linked in: sched_spin(O+) [last unloaded: sched_spin]
CPU: 0 PID: 627 Comm: insmod Tainted: G        W  O      5.10.14+ #5
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS 1.15.0-1 04/01/2014
EIP: do_one_initcall+0x207/0x2d0
Code: 7c 00 64 ff c1 e9 7b fe c1 e9 7b fe 26 00 90 89 26 00 90 89 45 b0 89 44 45 b0 89 44 24 88 1a 96 24 88 1a 96 78 00 0f 0be
EAX: 00000052 EBX: 00000000 ECX: dfbc0a44 EDX: 01000000
ESI: e0831000 EDI: 00000000 EBP: c4cb3e0c ESP: c4cb3db0
DS: 007b ES: 007b FS: 00d8 GS: 00e0 SS: 0068 EFLAGS: 00010292
CR0: 80050033 CR2: b7dcb350 CR3: 04db0000 CR4: 00350e90
Call Trace:
 ? 0xe0831000
 ? rcu_read_lock_sched_held+0x41/0x70
 ? kmem_cache_alloc_trace+0x2cb/0x340
 do_init_module+0x4e/0x230
 load_module+0x2370/0x2820
 ? kernel_read+0x39/0x50
 ? kernel_read_file_from_fd+0x4c/0x90
 __ia32_sys_finit_module+0x9a/0xd0
 do_int80_syscall_32+0x2c/0x40
 entry_INT80_32+0xf7/0xf7
EIP: 0xb7dd8222
Code: 06 89 8a f0 02 00 00 c3 55 57 56 53 8b 6c 24 2c 8b 7c 24 28 8b 74 24 24 8b 54 24 20 8b 4c 24 1c 8b 5c 24 18 8b 44 24 140
EAX: ffffffda EBX: 00000003 ECX: 01d81160 EDX: 00000000
ESI: 00000100 EDI: 00000100 EBP: 00000000 ESP: bfbf2fbc
DS: 007b ES: 007b FS: 0000 GS: 0033 SS: 007b EFLAGS: 00000206
irq event stamp: 7559
hardirqs last  enabled at (7569): [<c10bd69c>] console_unlock+0x57c/0x5b0
hardirqs last disabled at (7578): [<c10bd4ad>] console_unlock+0x38d/0x5b0
softirqs last  enabled at (7254): [<c1023877>] call_on_stack+0x17/0x60
softirqs last disabled at (7217): [<c1023877>] call_on_stack+0x17/0x60
---[ end trace c76027ec63e8b8bf ]---
*/

static int sched_spin_init(void)
{
	spinlock_t lock;

	spin_lock_init(&lock);

	spin_lock(&lock);

	// NOTE: if we in spin lock, we can't sleep
	// because we can't be preempted

	set_current_state(TASK_INTERRUPTIBLE);
	/* Try to sleep for 5 seconds. */
	schedule_timeout(5 * HZ);

	spin_unlock(&lock);

	return 0;
}

static void sched_spin_exit(void)
{
}

module_init(sched_spin_init);
module_exit(sched_spin_exit);
