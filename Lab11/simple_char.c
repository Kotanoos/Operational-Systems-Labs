#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "simple_char"
#define BUF_SIZE 100

static dev_t dev_num;
static struct cdev my_cdev;
static char buffer[BUF_SIZE] = {0};
static int open_count = 0;

static int my_device_open(struct inode *inode, struct file *file) {
    open_count++;
    pr_info("%s: device opened %d times\n", DEVICE_NAME, open_count);
    return 0;
}

static int my_device_close(struct inode *inode, struct file *file) {
    pr_info("%s: device closed\n", DEVICE_NAME);
    return 0;
}

static ssize_t my_device_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos) {
    size_t to_copy;

    if (*ppos >= BUF_SIZE) {
        return 0;
    }

    to_copy = min(count, (size_t)(BUF_SIZE - *ppos));

    if (copy_to_user(user_buf, buffer + *ppos, to_copy)) {
        return -EFAULT;
    }

    *ppos += to_copy;
    pr_info("%s: %zu bytes read\n", DEVICE_NAME, to_copy);
    return to_copy;
}

static ssize_t my_device_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos) {
    size_t to_copy;

    if (*ppos >= BUF_SIZE) {
        return -ENOSPC;
    }

    to_copy = min(count, (size_t)(BUF_SIZE - *ppos));

    if (copy_from_user(buffer + *ppos, user_buf, to_copy)) {
        return -EFAULT;
    }

    *ppos += to_copy;
    pr_info("%s: %zu bytes written\n", DEVICE_NAME, to_copy);
    return to_copy;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_device_open,
    .release = my_device_close,
    .read = my_device_read,
    .write = my_device_write,
};

static int __init simple_char_init(void) {
    if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) {
        pr_err("%s: failed to allocate device number\n", DEVICE_NAME);
        return -1;
    }

    cdev_init(&my_cdev, &fops);

    if (cdev_add(&my_cdev, dev_num, 1) < 0) {
        unregister_chrdev_region(dev_num, 1);
        pr_err("%s: failed to add cdev\n", DEVICE_NAME);
        return -1;
    }

    pr_info("%s: driver registered with major %d\n", DEVICE_NAME, MAJOR(dev_num));
    return 0;
}

static void __exit simple_char_exit(void) {
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("%s: driver unregistered\n", DEVICE_NAME);
}

module_init(simple_char_init);
module_exit(simple_char_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kotanos");
MODULE_DESCRIPTION("drive");
