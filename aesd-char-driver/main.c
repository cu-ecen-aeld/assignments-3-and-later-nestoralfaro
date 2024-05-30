/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

// #include <asm-generic/errno-base.h>
#include <asm-generic/errno-base.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include <linux/uaccess.h>
// #include <sys/types.h>
// #include <stdint.h>
// #include <stdio.h>
// #include "aesd-circular-buffer.h"
#include "aesd-circular-buffer.h"
#include "aesd_ioctl.h"
#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("nestoralfaro"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
  PDEBUG("open");
  /**
   * TODO: handle open
   */
  filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);
  return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
  PDEBUG("read %zu bytes with offset %lld", count, *f_pos);
  /**
   * TODO: handle read
   */
  ssize_t retval = 0;
  struct aesd_dev *dev;
  struct aesd_buffer_entry *entry;
  size_t entry_offset = 0;

  // check input parameters
  if (!filp || !buf) {
    return -EINVAL;
  }

  // get the device structure from the file's private data
  dev = filp->private_data;
  if (!dev) {
    return -EINVAL;
  }

  // obtain mutex to protect the circular buffer
  if (mutex_lock_interruptible(&dev->lock)) {
    return -ERESTARTSYS;
  }

  // find the buffer entry corresponding the file position
  entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buffer, *f_pos, &entry_offset);
  if (!entry) {
    // no valid entry found, return 0 bytes read
    retval = 0;
    goto out;
  }

  // calculate the number of bytes to copy
  if (count > (entry->size - entry_offset)) {
    count = entry->size - entry_offset;
  }

  // copy data to user space
  if (copy_to_user(buf, entry->buffptr + entry_offset, count)) {
    retval = -EFAULT;
    goto out;
  }

  // update file position and return number of bytes read
  *f_pos += count;
  retval = count;

out:
  // release the mutex
  mutex_unlock(&dev->lock);
  // reset the file position to 0 if no bytes were read
  if (retval == 0) {
    *f_pos = 0;
  }

  return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
  PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
  /**
   * TODO: handle write
   */
  ssize_t retval = -ENOMEM;
  struct aesd_dev *dev;
  const char *presult;
  size_t write_offset;
  bool newline_found;
  size_t i;

  // check input params
  if (!filp || !buf || !f_pos) {
    return -EINVAL;
  }
  if (!access_ok(buf, count)) {
    return -EINVAL;
  }

  // get the device structure from the file's private data
  dev = filp->private_data;
  if (!dev) {
    return -EINVAL;
  }

  // allocate memory for the write data
  char *write_data = kmalloc(count, GFP_KERNEL);
  if (!write_data)
  {
      return -ENOMEM;
  }

  // copy data from user space to kernel space
  if (copy_from_user(write_data, buf, count))
  {
      kfree(write_data);
      return -EFAULT;
  }

  // acquire the mutex to protect the circular buffer
  if (mutex_lock_interruptible(&dev->lock))
  {
      kfree(write_data);
      return -ERESTARTSYS;
  }

  // search for a newline character in the data
  newline_found = false;
  for (i = 0; i < count; i++)
  {
      if (write_data[i] == '\n')
      {
          newline_found = true;
          break;
      }
  }

  // determine the amount of data to append based on newline's presence
  write_offset = newline_found ? i + 1 : count;

  // allocate memory for the temporary buffer if needed
  if (dev->entry.buffptr == NULL)
  {
      dev->entry.buffptr = kmalloc(write_offset, GFP_KERNEL);
      if (!dev->entry.buffptr)
      {
          kfree(write_data);
          mutex_unlock(&dev->lock);
          return -ENOMEM;
      }
      memcpy(dev->entry.buffptr, write_data, write_offset);
      dev->entry.size = write_offset;
  }
  else
  {
      // reallocate memory to accommodate the new data
      char *new_buffptr = krealloc(dev->entry.buffptr, dev->entry.size + write_offset, GFP_KERNEL);
      if (!new_buffptr)
      {
          kfree(write_data);
          mutex_unlock(&dev->lock);
          return -ENOMEM;
      }
      memcpy(new_buffptr + dev->entry.size, write_data, write_offset);
      dev->entry.buffptr = new_buffptr;
      dev->entry.size += write_offset;
  }

  // if a newline is found, add the entry to the circular buffer
  if (newline_found)
  {
      presult = aesd_circular_buffer_add_entry(&dev->circular_buffer, &dev->entry);
      if (presult)
      {
          kfree(presult);
      }
      dev->entry.buffptr = NULL;
      dev->entry.size = 0;
  }

  // reset file position to 0 and return number of bytes written
  f_pos = 0;
  retval = count;

  // free the temporary buffer
  kfree(write_data);

  // release the mutex
  mutex_unlock(&dev->lock);
  return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence) {
  struct aesd_dev *dev = filp->private_data;
  loff_t newpos;
  size_t total_size = 0;
  int i;

  if (mutex_lock_interruptible(&dev->lock)) {
    return -ERESTARTSYS;
  }

  // calculate the total size of data in the circular buffer
  for (i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; ++i) {
    total_size += dev->circular_buffer.entry[i].size;
  }

  switch (whence) {
    case SEEK_SET:
      newpos = off;
      break;
    case SEEK_CUR:
      newpos = filp->f_pos + off;
      break;
    case SEEK_END:
      newpos = total_size + off;
      break;
    default:
      mutex_unlock(&dev->lock);
      return -EINVAL;
  }

  if (newpos < 0 || newpos > total_size) {
    return -EINVAL;
  }

  filp->f_pos = newpos;
  mutex_unlock(&dev->lock);
  return newpos;
}

long aesd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) return -ENOTTY;
  if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) return -ENOTTY;

  struct aesd_dev *dev = filp->private_data;
  struct aesd_seekto seekto;
  struct aesd_buffer_entry *entry;
  size_t entry_offset, cumulative_size;
  int i;

  if (cmd == AESDCHAR_IOCSEEKTO) {
    if (copy_from_user(&seekto, (struct aesd_seekto __user *)arg, sizeof(seekto))) {
      return -EFAULT;
    }

    if (mutex_lock_interruptible(&dev->lock)) {
      return -ERESTARTSYS;
    }

    // validate write_cmd index
    if (seekto.write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED || seekto.write_cmd < 0) {
      mutex_unlock(&dev->lock);
      return -EINVAL;
    }

    // calculate cumulative size to the target command
    cumulative_size = 0;
    for (i = 0; i < seekto.write_cmd; ++i) {
      entry = &dev->circular_buffer.entry[(dev->circular_buffer.out_offs) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED];
      cumulative_size += entry->size;
    }

    // validate write_cmd_offset
    entry = &dev->circular_buffer.entry[(dev->circular_buffer.out_offs + seekto.write_cmd_offset) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED];
    if (seekto.write_cmd_offset >= entry->size) {
      mutex_unlock(&dev->lock);
      return -EINVAL;
    }

    // set new file position
    filp->f_pos = cumulative_size + seekto.write_cmd_offset;

    mutex_unlock(&dev->lock);
    return 0;
  }

  return -ENOTTY;
}

struct file_operations aesd_fops = {
  .owner =            THIS_MODULE,
  .read =             aesd_read,
  .write =            aesd_write,
  .open =             aesd_open,
  .release =          aesd_release,
  .llseek =           aesd_llseek,
  .unlocked_ioctl =   aesd_unlocked_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
  dev_t dev = 0;
  int result;

  // allocate a major number for the device
  result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
  aesd_major = MAJOR(dev);
  if (result < 0) {
    printk(KERN_WARNING "Can't get major %d\n", aesd_major);
    return result;
  }
  memset(&aesd_device, 0, sizeof(struct aesd_dev));

  /**
   * TODO: initialize the AESD specific portion of the device
   */
  aesd_circular_buffer_init(&aesd_device.circular_buffer);
  mutex_init(&aesd_device.lock);
  aesd_device.entry.buffptr = NULL;
  aesd_device.entry.size = 0;

  result = aesd_setup_cdev(&aesd_device);
  if(result) {
    unregister_chrdev_region(dev, 1);
  }
  return result;
}

void aesd_cleanup_module(void)
{
  dev_t devno = MKDEV(aesd_major, aesd_minor);
  uint8_t index;
  struct aesd_buffer_entry *entry;
  cdev_del(&aesd_device.cdev);

  /**
   * TODO: cleanup AESD specific poritions here as necessary
   */
  // free all buffers in the circular buffer
  AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, index) {
    if (entry->buffptr != NULL) {
      kfree(entry->buffptr);
    }
  }
  mutex_destroy(&aesd_device.lock);

  printk(KERN_INFO "AESD cleanup module completed\n");
  unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
