#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

MODULE_DESCRIPTION("KBD");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

#define MODULE_NAME "kbd"
#define KBD_DEVICE_NAME MODULE_NAME
#define KBD_MAJOR 42
#define KBD_MINOR 0
#define KBD_NR_MINORS 1

#define I8042_KBD_IRQ 1
#define I8042_STATUS_REG 0x64
#define I8042_DATA_REG 0x60

#define BUFFER_SIZE 1024
#define SCANCODE_RELEASED_MASK 0x80

struct kbd {
	struct cdev cdev;
	spinlock_t lock;
	char buf[BUFFER_SIZE];
	size_t put_idx, get_idx, count;
} devs[1];

/*
 * Checks if scancode corresponds to key press or release.
 */
static int is_key_press(unsigned int scancode)
{
	return !(scancode & SCANCODE_RELEASED_MASK);
}

/*
 * Return the character of the given scancode.
 * Only works for alphanumeric/space/enter; returns '?' for other
 * characters.
 */
static int get_ascii(unsigned int scancode)
{
	static char *row1 = "1234567890";
	static char *row2 = "qwertyuiop";
	static char *row3 = "asdfghjkl";
	static char *row4 = "zxcvbnm";

	scancode &= ~SCANCODE_RELEASED_MASK;
	if (scancode >= 0x02 && scancode <= 0x0b)
		return *(row1 + scancode - 0x02);
	if (scancode >= 0x10 && scancode <= 0x19)
		return *(row2 + scancode - 0x10);
	if (scancode >= 0x1e && scancode <= 0x26)
		return *(row3 + scancode - 0x1e);
	if (scancode >= 0x2c && scancode <= 0x32)
		return *(row4 + scancode - 0x2c);
	if (scancode == 0x39)
		return ' ';
	if (scancode == 0x1c)
		return '\n';
	return '?';
}

static void put_char(struct kbd *data, char c)
{
	if (data->count >= BUFFER_SIZE)
		return;

	data->buf[data->put_idx] = c;
	data->put_idx = (data->put_idx + 1) % BUFFER_SIZE;
	data->count++;
}

static bool get_char(char *c, struct kbd *data)
{
	if (data->count > 0) {
		*c = data->buf[data->get_idx];
		data->get_idx = (data->get_idx + 1) % BUFFER_SIZE;
		data->count--;
		return true;
	}
	return false;
}

static void reset_buffer(struct kbd *data)
{
	data->count = 0;
	data->get_idx = 0;
	data->put_idx = 0;
}

/*
 * Return the value of the DATA register.
 */
static inline u8 i8042_read_data(void)
{
	u8 val;
	val = inb(I8042_DATA_REG);
	return val;
}

irqreturn_t kbd_interrupt_handler(int irq_no, void *data)
{
	if (data != &devs[0]) {
		return IRQ_NONE;
	}
	u8 scancode;
	char ascii_code;
	struct kbd *kbd_data = (struct kbd *)data;

	scancode = i8042_read_data();
	if (!is_key_press(scancode)) {
		return IRQ_HANDLED;
	}
	ascii_code = get_ascii(scancode);
	pr_info("IRQ:% d, scancode = 0x%x, ascii(%c)\n", irq_no, scancode,
		ascii_code);

	spin_lock(&kbd_data->lock);
	put_char(kbd_data, ascii_code);
	spin_unlock(&kbd_data->lock);
	return IRQ_HANDLED;
}

static int kbd_open(struct inode *inode, struct file *file)
{
	struct kbd *data = container_of(inode->i_cdev, struct kbd, cdev);

	file->private_data = data;
	pr_info("%s opened\n", MODULE_NAME);
	return 0;
}

static int kbd_release(struct inode *inode, struct file *file)
{
	pr_info("%s closed\n", MODULE_NAME);
	return 0;
}

static ssize_t kbd_write(struct file *file, const char __user *user_buffer,
			 size_t size, loff_t *offset)
{
	struct kbd *data = (struct kbd *)file->private_data;
	unsigned long irq;
	pr_info("reset buffer");

	spin_lock_irqsave(&data->lock, irq);
	reset_buffer(data);
	spin_unlock_irqrestore(&data->lock, irq);
	return size;
}

static ssize_t kbd_read(struct file *file, char __user *user_buffer,
			size_t size, loff_t *offset)
{
	unsigned long irq;
	struct kbd *data = (struct kbd *)file->private_data;
	size_t read = 0;
	char buffer[BUFFER_SIZE];

	spin_lock_irqsave(&data->lock, irq);
	while (read < size && read < BUFFER_SIZE) {
		if (get_char(buffer + read, data) == true) {
			pr_info("read char %c", buffer[read]);
			read++;
			continue;
		}
		break;
	}
	spin_unlock_irqrestore(&data->lock, irq);

	if (read != 0) {
		if (copy_to_user(user_buffer, buffer, read)) {
			pr_err("copy_to_user failed\n");
			return -EFAULT;
		}
	}
	return read;
}

static const struct file_operations kbd_fops = {
	.owner = THIS_MODULE,
	.open = kbd_open,
	.release = kbd_release,
	.read = kbd_read,
	.write = kbd_write,
};

static int kbd_init(void)
{
	int err;

	err = register_chrdev_region(MKDEV(KBD_MAJOR, KBD_MINOR), KBD_NR_MINORS,
				     MODULE_NAME);
	if (err != 0) {
		pr_err("register_region failed: %d\n", err);
		goto out;
	}
	if (!request_region(I8042_STATUS_REG + 1, 1, KBD_DEVICE_NAME)) {
		pr_err("request_region failed for status register\n");
		goto out_unregister;
	}
	if (!request_region(I8042_DATA_REG + 1, 1, KBD_DEVICE_NAME)) {
		pr_err("request_region failed for data register\n");
		goto out_relase_status;
	}

	spin_lock_init(&devs[0].lock);
	devs[0].count = 0;
	devs[0].get_idx = 0;
	devs[0].put_idx = 0;
	err = request_irq(I8042_KBD_IRQ, kbd_interrupt_handler, IRQF_SHARED,
			  MODULE_NAME, &devs[0]);
	if (err) {
		goto out_release_data;
	}
	cdev_init(&devs[0].cdev, &kbd_fops);
	cdev_add(&devs[0].cdev, MKDEV(KBD_MAJOR, KBD_MINOR), 1);

	pr_notice("Driver %s loaded\n", MODULE_NAME);
	return 0;
out_release_data:
	release_region(I8042_DATA_REG + 1, 1);
out_relase_status:
	release_region(I8042_STATUS_REG + 1, 1);
out_unregister:
	unregister_chrdev_region(MKDEV(KBD_MAJOR, KBD_MINOR), KBD_NR_MINORS);
out:
	return err;
}

static void kbd_exit(void)
{
	cdev_del(&devs[0].cdev);

	free_irq(I8042_KBD_IRQ, &devs[0]);
	release_region(I8042_STATUS_REG + 1, 1);
	release_region(I8042_DATA_REG + 1, 1);
	unregister_chrdev_region(MKDEV(KBD_MAJOR, KBD_MINOR), KBD_NR_MINORS);
	pr_notice("Driver %s unloaded\n", MODULE_NAME);
}

module_init(kbd_init);
module_exit(kbd_exit);
