#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>

/*
oops_mod: loading out-of-tree module taints kernel.
before init
BUG: kernel NULL pointer dereference, address: 00000000
#PF: supervisor write access in kernel mode
#PF: error_code(0x0002) - not-present page
*pde = 00000000
Oops: 0002 [#1] SMP
CPU: 0 PID: 422 Comm: insmod Tainted: G           O      5.10.14+ #5
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS 1.15.0-1 04/01/2014
EIP: my_oops_init+0xd/0x22 [oops_mod]
Code: Unable to access opcode bytes at RIP 0xe085afe3.
EAX: 0000000b EBX: 00000000 ECX: dfbc0a44 EDX: 01000000
ESI: e085b000 EDI: 00000002 EBP: c2abdda8 ESP: c2abdda4
DS: 007b ES: 007b FS: 00d8 GS: 00e0 SS: 0068 EFLAGS: 00010282
CR0: 80050033 CR2: e085afe3 CR3: 04e12000 CR4: 00350e90
Call Trace:
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
EIP: 0xb7dea222
Code: 06 89 8a f0 02 00 00 c3 55 57 56 53 8b 6c 24 2c 8b 7c 24 28 8b 74 24 24 8b 54 24 20 8b 4c 24 1c 8b 5c 24 18 8b 44 24 140
EAX: ffffffda EBX: 00000003 ECX: 0104b160 EDX: 00000000
ESI: 00000100 EDI: 00000100 EBP: 00000000 ESP: bfd4839c
DS: 007b ES: 007b FS: 0000 GS: 0033 SS: 007b EFLAGS: 00000206
Modules linked in: oops_mod(O+)
CR2: 0000000000000000
---[ end trace 0f0241495c5456ec ]---
EIP: my_oops_init+0xd/0x22 [oops_mod]
Code: Unable to access opcode bytes at RIP 0xe085afe3.
EAX: 0000000b EBX: 00000000 ECX: dfbc0a44 EDX: 01000000
ESI: e085b000 EDI: 00000002 EBP: c2abdda8 ESP: c2abdda4
DS: 007b ES: 007b FS: 00d8 GS: 00e0 SS: 0068 EFLAGS: 00010282
CR0: 80050033 CR2: e085afe3 CR3: 04e12000 CR4: 00350e90
BUG: sleeping function called from invalid context at include/linux/cgroup-defs.h:753
in_atomic(): 0, irqs_disabled(): 1, non_block: 0, pid: 422, name: insmod
INFO: lockdep is turned off.
irq event stamp: 6458
hardirqs last  enabled at (6457): [<c10bd69c>] console_unlock+0x57c/0x5b0
hardirqs last disabled at (6458): [<c17bce00>] exc_page_fault+0x30/0x200
softirqs last  enabled at (6040): [<c1023877>] call_on_stack+0x17/0x60
softirqs last disabled at (6027): [<c1023877>] call_on_stack+0x17/0x60
CPU: 0 PID: 422 Comm: insmod Tainted: G      D    O      5.10.14+ #5
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS 1.15.0-1 04/01/2014
Call Trace:
 dump_stack+0x6d/0x8b
 ___might_sleep.cold+0xac/0xbd
 __might_sleep+0x32/0x90
 exit_signals+0x17/0xd0
 do_exit+0xa6/0x8e0
 rewind_stack_do_exit+0x11/0x14
EIP: 0xb7dea222
Code: 06 89 8a f0 02 00 00 c3 55 57 56 53 8b 6c 24 2c 8b 7c 24 28 8b 74 24 24 8b 54 24 20 8b 4c 24 1c 8b 5c 24 18 8b 44 24 140
EAX: ffffffda EBX: 00000003 ECX: 0104b160 EDX: 00000000
ESI: 00000100 EDI: 00000100 EBP: 00000000 ESP: bfd4839c
DS: 007b ES: 007b FS: 0000 GS: 0033 SS: 007b EFLAGS: 00000206
Killed

skels/kernel_modules/5-oops-mod/oops_mod.ko：     文件格式 elf32-i386


Disassembly of section .text.unlikely:

b7dea222 <init_module>:
objdump：警告： source file /home/nature/Workspace/linux/tools/labs/skels/./kernel_modules/5-oops-mod/oops_mod.c is more recent than object file
/*
oops_mod: loading out-of-tree module taints kernel.
before init
BUG: kernel NULL pointer dereference, address: 00000000
#PF: supervisor write access in kernel mode
#PF: error_code(0x0002) - not-present page
b7dea222:       55                      push   %ebp
b7dea223:       89 e5                   mov    %esp,%ebp <- 此处引发了空指针
*pde = 00000000
Oops: 0002 [#1] SMP
CPU: 0 PID: 422 Comm: insmod Tainted: G           O      5.10.14+ #5
b7dea225:       68 00 00 00 00          push   $0x0
b7dea22a:       e8 fc ff ff ff          call   b7dea22b <init_module+0x9>
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS 1.15.0-1 04/01/2014
EIP: my_oops_init+0xd/0x22 [oops_mod]
b7dea22f:       c6 05 00 00 00 00 61    movb   $0x61,0x0
Code: Unable to access opcode bytes at RIP 0xe085afe3.
b7dea236:       68 0f 00 00 00          push   $0xf
b7dea23b:       e8 fc ff ff ff          call   b7dea23c <init_module+0x1a>
EAX: 0000000b EBX: 00000000 ECX: dfbc0a44 EDX: 01000000
ESI: e085b000 EDI: 00000002 EBP: c2abdda8 ESP: c2abdda4
DS: 007b ES: 007b FS: 00d8 GS: 00e0 SS: 0068 EFLAGS: 00010282
b7dea240:       31 c0                   xor    %eax,%eax
b7dea242:       c9                      leave
b7dea243:       c3                      ret

b7dea244 <cleanup_module>:
CR0: 80050033 CR2: e085afe3 CR3: 04e12000 CR4: 00350e90
Call Trace:
 do_one_initcall+0x57/0x2d0
b7dea244:       55                      push   %ebp
b7dea245:       89 e5                   mov    %esp,%ebp
 ? do_init_module+0x1f/0x230
b7dea247:       68 1d 00 00 00          push   $0x1d
b7dea24c:       e8 fc ff ff ff          call   b7dea24d <cleanup_module+0x9>
 ? rcu_read_lock_sched_held+0x41/0x70
b7dea251:       58                      pop    %eax
b7dea252:       c9                      leave
b7dea253:       c3                      ret

Cannot unload module:
	rmmod: can't unload module 'oops_mod': Device or resource busy

lsmod:
	oops_mod 20480 1 - Loading 0xe085b000 (O+)
*/

MODULE_DESCRIPTION("Oops generating module");
MODULE_AUTHOR("So2rul Esforever");
MODULE_LICENSE("GPL");

static int my_oops_init(void)
{
	char *p = 0;

	pr_info("before init\n");
	// NOTE:
	*p = 'a';
	pr_info("after init\n");

	return 0;
}

static void my_oops_exit(void)
{
	pr_info("module goes all out\n");
}

module_init(my_oops_init);
module_exit(my_oops_exit);
