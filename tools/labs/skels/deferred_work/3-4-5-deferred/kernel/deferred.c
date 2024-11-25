/*
 * SO2 - Lab 6 - Deferred Work
 *
 * Exercises #3, #4, #5: deferred work
 *
 * Code skeleton.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched/task.h>
#include "../include/deferred.h"

#define MY_MAJOR 42
#define MY_MINOR 0
#define MODULE_NAME "deferred"

#define TIMER_TYPE_NONE -1
#define TIMER_TYPE_SET 0
#define TIMER_TYPE_ALLOC 1
#define TIMER_TYPE_MON 2

MODULE_DESCRIPTION("Deferred work character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

struct mon_proc {
	struct task_struct *task;
	struct list_head list;
};

static struct my_device_data {
	struct cdev cdev;
	struct timer_list timer;
	atomic_t flag;
	struct workqueue_struct *wq;
	struct list_head mon_procs;
	spinlock_t lock;
	atomic_t stop;
} dev;

struct work_item {
	struct work_struct work;
	void *private_data;
};

static void alloc_io(void)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(5 * HZ);
	pr_info("Yawn! I've been sleeping for 5 seconds.\n");
}

static struct mon_proc *get_proc(pid_t pid)
{
	struct task_struct *task;
	struct mon_proc *p;

	rcu_read_lock();
	task = pid_task(find_vpid(pid), PIDTYPE_PID);
	rcu_read_unlock();
	if (!task)
		return ERR_PTR(-ESRCH);

	p = kmalloc(sizeof(*p), GFP_ATOMIC);
	if (!p)
		return ERR_PTR(-ENOMEM);

	get_task_struct(task);
	p->task = task;

	return p;
}

static void timer_work(struct work_struct *work)
{
	struct task_struct *curr;
	int64_t t;
	struct work_item *wi;
	struct mon_proc *p, *n;
	wi = container_of(work, struct work_item, work);
	t = (int64_t)wi->private_data;
	size_t cnt = 0;
	pr_info("[timer_work] current jiffies %ld type %lld\n", jiffies, t);
	switch (t) {
	case TIMER_TYPE_SET:
		curr = get_current();
		if (curr == NULL) {
			pr_err("[timer_work] current task is NULL\n");
			goto out;
		}
		pr_info("[timer_work] pid %d, name %s\n", curr->pid,
			curr->comm);
		goto out;
	case TIMER_TYPE_ALLOC:
		alloc_io();
		pr_info("[timer_work] io finish\n");
		goto out;
	case TIMER_TYPE_MON:
		pr_info("[timer_work] monitoring task\n");
		spin_lock(&dev.lock);
		list_for_each_entry_safe (p, n, &dev.mon_procs, list) {
			cnt++;
			if (p->task->state == TASK_DEAD) {
				pr_info("[timer_work] pid %d, name %s is dead\n",
					p->task->pid, p->task->comm);
				put_task_struct(p->task);
				list_del(&p->list);
				kfree(p);
			}
		}
		spin_unlock(&dev.lock);
		if (cnt == 0) {
			goto out;
		}
		goto remod;
	}
remod:
	if (atomic_read(&dev.stop) == 0) {
		mod_timer(&dev.timer, jiffies + msecs_to_jiffies(1000));
	}
out:
	kfree(wi);
}

#define ALLOC_IO_DIRECT
/* TODO 3: undef ALLOC_IO_DIRECT*/

static void timer_handler(struct timer_list *tl)
{
	struct work_item *work;
	pr_info("[timer_handler] current jiffies %ld\n", jiffies);
	work = kmalloc(sizeof(*work), GFP_ATOMIC);
	if (!work) {
		pr_err("[timer_handler] failed to alloc task\n");
		return;
	}
	pr_info("[timer_handler] scheduling work\n");
	INIT_WORK(&work->work, timer_work);
	work->private_data = (void *)atomic_read(&dev.flag);
	if (queue_work(dev.wq, &work->work) == false) {
		pr_err("[timer_handler] failed to queue work\n");
		goto err;
	}
	pr_info("[timer_handler] work queued\n");
	return;
err:
	kfree(work);
}

static int deferred_open(struct inode *inode, struct file *file)
{
	struct my_device_data *my_data =
		container_of(inode->i_cdev, struct my_device_data, cdev);
	file->private_data = my_data;
	pr_info("[deferred_open] Device opened\n");
	return 0;
}

static int deferred_release(struct inode *inode, struct file *file)
{
	pr_info("[deferred_release] Device released\n");
	return 0;
}

static long deferred_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	struct my_device_data *my_data =
		(struct my_device_data *)file->private_data;
	struct mon_proc *p;
	pr_info("[deferred_ioctl] Command: %s\n", ioctl_command_to_string(cmd));

	switch (cmd) {
	case MY_IOCTL_TIMER_SET:
		atomic_set(&my_data->flag, TIMER_TYPE_SET);
		pr_info("[deferred_ioctl] timer %ld sec\n", arg);
		mod_timer(&my_data->timer,
			  jiffies + msecs_to_jiffies(1000) * arg);
		break;
	case MY_IOCTL_TIMER_CANCEL:
		del_timer_sync(&my_data->timer);

		break;
	case MY_IOCTL_TIMER_ALLOC:
		atomic_set(&my_data->flag, TIMER_TYPE_ALLOC);
		pr_info("[deferred_ioctl] timer %ld sec\n", arg);
		mod_timer(&my_data->timer,
			  jiffies + msecs_to_jiffies(1000) * arg);
		break;
	case MY_IOCTL_TIMER_MON: {
		p = get_proc(arg);
		if (!IS_ERR(p)) {
			spin_lock(&dev.lock);
			list_add_tail(&p->list, &dev.mon_procs);
			spin_unlock(&dev.lock);
			atomic_set(&my_data->flag, TIMER_TYPE_MON);
			mod_timer(&my_data->timer,
				  jiffies + msecs_to_jiffies(1000));
		}
		break;
	}
	default:
		return -ENOTTY;
	}
	return 0;
}

struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = deferred_open,
	.release = deferred_release,
	.unlocked_ioctl = deferred_ioctl,
};

static int deferred_init(void)
{
	int err;

	pr_info("[deferred_init] Init module\n");
	err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1, MODULE_NAME);
	if (err) {
		pr_info("[deffered_init] register_chrdev_region: %d\n", err);
		return err;
	}

	atomic_set(&dev.flag, TIMER_TYPE_NONE);
	atomic_set(&dev.stop, 0);
	dev.wq = create_singlethread_workqueue("timer-worker");
	INIT_LIST_HEAD(&dev.mon_procs);
	spin_lock_init(&dev.lock);
	timer_setup(&dev.timer, timer_handler, 0);
	cdev_init(&dev.cdev, &my_fops);
	cdev_add(&dev.cdev, MKDEV(MY_MAJOR, MY_MINOR), 1);

	return 0;
}

static void deferred_exit(void)
{
	struct mon_proc *p, *n;

	pr_info("[deferred_exit] Exit module\n");

	cdev_del(&dev.cdev);
	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), 1);
	atomic_set(&dev.stop, 1);

	spin_lock(&dev.lock);
	list_for_each_entry_safe (p, n, &dev.mon_procs, list) {
		pr_info("[deferred_exit] pid %d, name %s\n", p->task->pid,
			p->task->comm);
		put_task_struct(p->task);
		list_del(&p->list);
		kfree(p);
	}
	spin_unlock(&dev.lock);

	del_timer_sync(&dev.timer);
	flush_workqueue(dev.wq);
	destroy_workqueue(dev.wq);
}

module_init(deferred_init);
module_exit(deferred_exit);
