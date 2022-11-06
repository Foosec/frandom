/*

 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <stddef.h>
#include <linux/random.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/kdev_t.h>

#include "frandom.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Foosec");
MODULE_DESCRIPTION("Fast random based on xorshift");


#define SUCCESS 0
#define DEVICE_NAME "frandom"

static dev_t dev_reg;
static struct class *dev_class;
static struct cdev c_dev;

static struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};

struct xorshift_state {
    uint32_t a;
    uint64_t advancements;
};

uint32_t advance_state(struct xorshift_state *state) {
    if(state->advancements > UINT_MAX){
        get_random_bytes(&state->a, sizeof(state->a));
        state->advancements = 0;
    }

	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = state->a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;

    state->advancements++;
	return state->a = x;
}

static struct xorshift_state state;

int init_module(void)
{
    printk(KERN_INFO "Initializing frandom!\n");
    
    //Allocates chrdev region with base 0 count 1. 
    int err = alloc_chrdev_region(&dev_reg, 0, 1, DEVICE_NAME);

    if (err < 0) {
        printk(KERN_ALERT "Registering char device failed\n");
        return -1;
    }

    dev_class = class_create(THIS_MODULE, DEVICE_NAME);
    if(dev_class == NULL){
        printk(KERN_ALERT "Class creation failed!\n");
        unregister_chrdev_region(dev_reg, 1);
        return -1;
    }
    struct device *device_t = device_create(dev_class, NULL, dev_reg, NULL, DEVICE_NAME);
    if(device_t == NULL){
        printk(KERN_ALERT "Device creation failed!\n");
        class_destroy(dev_class);
        unregister_chrdev_region(dev_reg, 1);
        return -1;
    }

    cdev_init(&c_dev, &fops);

    if(cdev_add(&c_dev, dev_reg, 1) == -1){
        printk(KERN_ALERT "Device addition failed!\n");
        device_destroy(dev_class, dev_reg);
        class_destroy(dev_class);
        unregister_chrdev_region(dev_reg, 1);
        return -1;
    }

  get_random_bytes(&state.a, sizeof(state.a));



  printk(KERN_INFO "Registered frandom");

  return SUCCESS;
}


void cleanup_module(void){

    printk(KERN_INFO "Unloading frandom\n");

    device_destroy(dev_class, dev_reg);
    class_destroy(dev_class);
    unregister_chrdev_region(dev_reg, 1);

}


static int device_open(struct inode *inode, struct file *filp){

  return SUCCESS;
}


static int device_release(struct inode *inode, struct file *filp){

  return SUCCESS;
}

static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */
                           char *buffer,      /* buffer to fill with data */
                           size_t length,     /* length of the buffer     */
                           loff_t *offset){

    char buf[1024];
    int to_write = length;
    while(to_write > 0 ){
        //Fill up buffer
        for(int i=0; i<sizeof(buf) / sizeof(uint32_t); i+=sizeof(uint32_t)) {
            uint32_t r = advance_state(&state);
            memcpy(buf+i, &r, sizeof(r));
        }

        if(to_write > sizeof(buf)){
            copy_to_user(buffer, buf, sizeof(buf));
            to_write -= sizeof(buf);
        }
        else{
            copy_to_user(buffer, buf, to_write);
            to_write = 0;
        }        
    }

    return length;

}

static ssize_t device_write(struct file *filp, const char *buf, size_t len, loff_t *off){
  printk(KERN_ALERT "Attempt to write to frandom\n");
  return -EINVAL;
}
