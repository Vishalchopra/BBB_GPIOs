


/** @file button.c
  * @author Vishal Chopra
  * @date 23 May 2016
  * @breif A kernel module for controlling a button that is connected to a GPIO
  * It has full support for the interrupt and for sysfs entries so that an inte
  * face can be created to the button or button can be configured from the linux
  * Userspace. The sysfs entery appears at /sys/ebb/gpio115
*/

#include "header.h"

static bool isRising = 1;	//< rising edge is the default IRQ property
module_param(isRising, bool, S_IRUGO);	//< Dynamic allocation S_IRUGO read only
MODULE_PARM_DESC(isRising, "Rising edge = 1 (default), Falling edge = 0");

static unsigned int gpioButton = 115;	//< hard coding GPIO to 115
module_param(gpioButton, uint, S_IRUGO);
MODULE_PARM_DESC(gpioButton, "GPIO Button number (default  115)");

static unsigned int gpioLED = 49;	//< hard coding GPIO to 49
module_param(gpioLED, uint, S_IRUGO);
MODULE_PARM_DESC(gpioLED, "GPIO LED number (default  49)");

static char gpioName[8] = "gpioXXX";
static int  irqNumber;			//< Used for Button interrupt
static int  numberPresses = 0;		//< Store number of presses
static bool ledOn = 0;			//< To check LED status
static bool isDebounce = 1;
static struct timespec ts_last, ts_current, ts_diff;

//Function Prototype for the interrupt handler

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

/** breif call back function to output the numberPresses varible
  * @param kobject represent a kernel object device that appear in the sysfs file system
  * @param attr pointer to the kob_attribute struct
  * @param buf from which read the number of presses
  * @param return total number of character written to the buffer
*/

static ssize_t numberPresses_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", numberPresses);
}


/** breif call back function to read in the numberPresses varible
  * @param kobject represent a kernel object device that appear in the sysfs file system
  * @param attr pointer to the kob_attribute struct
  * @param buf from which read the number of presses
  * @param count the number of character in the buffer
  * @param return total number of character written to the buffer
*/

static ssize_t numberPresses_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%du", &numberPresses);
	return count;
}

static ssize_t ledOn_show(struct kobject *kobj, struct kobj_attribute *atr, char *buf)
{
	return sprintf(buf, "%d\n", ledOn);
}

static ssize_t lastTime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%.2lu:%.2lu:%.2lu:%.9lu \n", (ts_last.tv_sec/3000)%24, (ts_last.tv_sec / 60)%60, (ts_last.tv_sec % 60), ts_last.tv_nsec);


}

/** @breif Display the time difference between push button time*/
static ssize_t diffTime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu.%.9lu\n", ts_diff.tv_sec, ts_diff.tv_sec);
}

/* @breif Displays if button Debouncing is on or off */
static ssize_t isDebounce_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", isDebounce);
}

/** @breif Store and set the Debunce state */
static ssize_t isDebounce_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, ssize_t count)
{
	unsigned int temp;
	sscanf(buf, "%du", &temp);
	gpio_set_debounce(gpioButton, 0);
	isDebounce = temp;
	if(isDebounce){
		gpio_set_debounce(gpioButton, DEBOUNCE_TIME);
		printk(KERN_INFO "EBB Button Debounceon\n");
	}else{
		gpio_set_debounce(gpioButton, 0);
		printk(KERN_INFO "EBB Button Debounce off\n");
	}
	return count;
}

/**  Use these helper macros to define the name and access levels of the kobj_attributes
*  The kobj_attribute has an attribute attr (name and mode), show and store function pointers
*  The count variable is associated with the numberPresses variable and it is to be exposed
*  with mode 0666 using the numberPresses_show and numberPresses_store functions above
*/

static struct kobj_attribute count_attr = __ATTR(numberPresses, 0666, numberPresses_show, numberPresses_store);
static struct kobj_attribute debounce_attr = __ATTR(isDebounce, 0666, isDebounce_show, isDebounce_store);

/** The __ATTR_RO define a read only attribute. There is no need to identify  that the function is 
  * called _show but it must be present.
*/

static struct kobj_attribute ledon_attr = __ATTR_RO(ledOn);
static struct kobj_attribute diff_attr = __ATTR_RO(diffTime);
static struct kobj_attribute time_attr = __ATTR_RO(lastTime);

/** The ebb_attr is an array of attributes that is used to create the attribute group
*/

static struct attribute *ebb_attrs[] = {
	&count_attr.attr,
	&ledon_attr.attr,
	&debounce_attr.attr,
	&time_attr.attr,
	&diff_attr.attr,
	NULL
};
static struct attribute_group attr_group = {
	.name = gpioName,
	.attrs = ebb_attrs,
};

static struct kobject *ebb_obj;

/** @brief The LKM initialization function
*  The static keyword restricts the visibility of the function to within this C file. The __init
*  macro means that for a built-in driver (not a LKM) the function is only used at initialization
*  time and that it can be discarded and its memory freed up after that point. In this example this
*  function sets up the GPIOs and the IRQ
*  @return returns 0 if successful
*/

static int __init ebbButton_init(void)
{
	int result = 0;
	unsigned long IRQflags = IRQF_TRIGGER_RISING;
	printk(KERN_INFO "EBB: Intializing the LKM");
	sprintf(gpioName, "gpio%d", gpioButton);

	// create entry ti the /sys
	ebb_obj = kobject_create_and_add("ebb", kernel_kobj->parent);
	if(!ebb_obj){
		printk(KERN_ERR "Unable to create entry in sysfs");
		return -ENOMEM;
	}
	//add attribute to /sys/ebb 
	result = sysfs_create_group(ebb_obj, &attr_group);
	if(result){
		printk(KERN_ERR "Failed to create sysfs group");
		kobject_put(ebb_obj);			// remove the kobject sysfs file entry
		return result;
	}
	getnstimeofday(&ts_last);
	ts_diff = timespec_sub(ts_last, ts_last);
	
	ledOn = true;
	gpio_request(gpioLED, "sysfs");
	gpio_direction_output(gpioLED, ledOn);
	gpio_export(gpioLED, false);
	
	gpio_request(gpioButton, "sysfs");
	gpio_direction_input(gpioButton);
	gpio_set_debounce(gpioButton, DEBOUNCE_TIME);
	gpio_export(gpioButton, false);

	printk(KERN_INFO "Currently gpioButton sate is %d\n", gpio_get_value(gpioButton));

	irqNumber = gpio_to_irq(gpioButton);
	printk(KERN_INFO "The Button is map to IRQ %d\n", irqNumber);
	
	if(!isRising){
		IRQflags = IRQF_TRIGGER_FALLING;	// Set the interrupt on the falling edge
	}
	result = request_irq(irqNumber,
				(irq_handler_t) gpio_irq_handler, 
				IRQflags,
				"Button_handler", 	// Used in /proc/interrupt to identify the owner
				NULL);				// The *dev_id for shared interrupt lines, NULL is okay

	return result;
}


static void __exit ebbButton_exit(void)
{
	printk(KERN_INFO "EBB Button the button was presses %d times\n", numberPresses);
	kobject_put(ebb_obj);
	gpio_set_value(gpioLED, 0);
	gpio_unexport(gpioLED);
	free_irq(irqNumber, NULL);
	gpio_unexport(gpioLED);
	gpio_free(gpioLED);
	gpio_free(gpioButton);
	printk(KERN_INFO "Goodbye from the EBB Button");

}

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
   ledOn = !ledOn;                      // Invert the LED state on each button press
   gpio_set_value(gpioLED, ledOn);      // Set the physical LED accordingly
   getnstimeofday(&ts_current);         // Get the current time as ts_current
   ts_diff = timespec_sub(ts_current, ts_last);   // Determine the time difference between last 2 presses
   ts_last = ts_current;                // Store the current time as the last time ts_last
   printk(KERN_INFO "EBB Button: The button state is currently: %d\n", gpio_get_value(gpioButton));
   numberPresses++;                     // Global counter, will be outputted when the module is unloaded
   return (irq_handler_t) IRQ_HANDLED;  // Announce that the IRQ has been handled correctly
}
 
// This next calls are  mandatory -- they identify the initialization function
// and the cleanup function (as above).
module_init(ebbButton_init);
module_exit(ebbButton_exit); 
