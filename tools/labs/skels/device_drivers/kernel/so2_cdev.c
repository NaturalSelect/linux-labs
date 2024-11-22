/*
 * Character device drivers lab
 *
 * All tasks
 */

#include "linux/printk.h"
#include "linux/types.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define EXTRA
#define LOG_LEVEL KERN_INFO

#define MY_MAJOR 42
#define MY_MINOR 0
#define NUM_MINORS 1
#define MODULE_NAME "so2_cdev"
#define MESSAGE "hello\n"
#define IOCTL_MESSAGE "Hello ioctl"

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif

struct so2_device_data {
	struct cdev cdev;
	char buffer[BUFSIZ];
	ssize_t size;
	atomic_t busy;
	int block_at_ioctl;
	wait_queue_head_t ioctl_wq;
	int block_io;
	wait_queue_head_t read_wq;
};

struct so2_device_data devs[NUM_MINORS];

static int so2_cdev_open(struct inode *inode, struct file *file)
{
	struct so2_device_data *data;
	struct cdev *cdev;

	pr_info("so2_cdev: device opened\n");

	cdev = inode->i_cdev;
	data = container_of(cdev, struct so2_device_data, cdev);
	file->private_data = data;

#ifndef EXTRA
	if (atomic_cmpxchg(&data->busy, 0, 1)) {
		return -EBUSY;
	}
#endif

	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(10 * HZ);

	return 0;
}

static int so2_cdev_release(struct inode *inode, struct file *file)
{
	pr_info("so2_cdev: device closed\n");

#ifndef EXTRA
	struct so2_device_data *data =
		(struct so2_device_data *)file->private_data;

	atomic_set(&data->busy, 0);

#endif
	return 0;
}

static ssize_t so2_dev_readable(struct file *file, loff_t *offset, size_t size)
{
	struct so2_device_data *data =
		(struct so2_device_data *)file->private_data;

	return min(data->size - *offset, size);
}

static ssize_t so2_cdev_read(struct file *file, char __user *user_buffer,
			     size_t size, loff_t *offset)
{
	pr_info("read message");
	struct so2_device_data *data =
		(struct so2_device_data *)file->private_data;
	ssize_t len = so2_dev_readable(file, offset, size);
	if (len <= 0) {
		if (data->block_io) {
			wait_event_interruptible(
				data->read_wq,
				(len = so2_dev_readable(file, offset, size)) >
					0);
		} else {
			return -EWOULDBLOCK;
		}
	}

	if (copy_to_user(user_buffer, data->buffer + *offset, len)) {
		return -EFAULT;
	}
	*offset += len;
	return len;
}

static ssize_t so2_cdev_write(struct file *file, const char __user *user_buffer,
			      size_t size, loff_t *offset)
{
	pr_info("write message");
	struct so2_device_data *data =
		(struct so2_device_data *)file->private_data;
	ssize_t len = min(BUFSIZ - *offset, size);
	if (len <= 0) {
		pr_info("no space left");
		return 0;
	}

	if (copy_from_user(data->buffer + *offset, user_buffer, len)) {
		return -EFAULT;
	}
	*offset += len;
	data->size = max(data->size, *offset);
	wake_up_interruptible(&data->read_wq);
	return size;
}

static long so2_cdev_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	struct so2_device_data *data =
		(struct so2_device_data *)file->private_data;
	int ret = 0;
	int remains;
	char *buf;

	switch (cmd) {
	case MY_IOCTL_PRINT:
		pr_info("%s", IOCTL_MESSAGE);
		break;
	case MY_IOCTL_SET_BUFFER:
		buf = (char *)arg;
		data->size = BUFFER_SIZE;
		if (copy_from_user(data->buffer, buf, BUFFER_SIZE)) {
			ret = -EFAULT;
		}
		break;
	case MY_IOCTL_GET_BUFFER:
		buf = (char *)arg;
		if (copy_to_user(buf, data->buffer, BUFFER_SIZE)) {
			ret = -EFAULT;
		}
		break;
	case MY_IOCTL_UP:
		data->block_at_ioctl = 0;
		wake_up_interruptible(&data->ioctl_wq);
		break;
	case MY_IOCTL_DOWN:
		data->block_at_ioctl = 1;
		wait_event_interruptible(data->ioctl_wq,
					 data->block_at_ioctl == 0);
		break;
	case F_SETFL:
		if (arg & O_NONBLOCK) {
			data->block_io = 0;
		} else {
			data->block_io = 1;
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct file_operations so2_fops = {
	.owner = THIS_MODULE,
	.open = so2_cdev_open,
	.release = so2_cdev_release,
	.read = so2_cdev_read,
	.write = so2_cdev_write,
	.unlocked_ioctl = so2_cdev_ioctl,
};

static int so2_cdev_init(void)
{
	int err;
	int i, j;

	pr_info("register char dev %s", MODULE_NAME);
	err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS,
				     MODULE_NAME);
	if (err != 0) {
		return err;
	}

	for (i = 0; i < NUM_MINORS; i++) {
#ifdef EXTRA
		strcpy(devs[i].buffer, MESSAGE);
		devs[i].size = sizeof(MESSAGE);
#endif
		atomic_set(&devs[i].busy, 0);
		cdev_init(&devs[i].cdev, &so2_fops);
		init_waitqueue_head(&devs[i].ioctl_wq);
		devs[i].block_at_ioctl = 0;
		devs[i].block_io = 1;
		init_waitqueue_head(&devs[i].read_wq);
		err = cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, i), 1);
		if (err != 0) {
			for (j = 0; j < i; ++j) {
				cdev_del(&devs[j].cdev);
			}
			return err;
		}
	}

	return 0;
}

static void so2_cdev_exit(void)
{
	int i;

	for (i = 0; i < NUM_MINORS; i++) {
		devs[i].block_at_ioctl = 0;
		wake_up_interruptible(&devs[i].ioctl_wq);
		wake_up_interruptible(&devs[i].read_wq);
		cdev_del(&devs[i].cdev);
	}

	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
}

module_init(so2_cdev_init);
module_exit(so2_cdev_exit);
