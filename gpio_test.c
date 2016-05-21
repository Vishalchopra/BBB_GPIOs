/** 
 * @file   gpio_test.c
 * @author Vishal Chopra
 * @date   19 May 2016
 * @breif A kernel module for controlling a GPIO LED/button pair. The device m
 * mountd via sysfs /sys/class/gpio/gpio115 and gpio49. LED is attached at the
 * GPIO 49 which is on P9_23 and the button is attached to GPIO 115 is on P9_27*/


#include <linux/init.h>		//<required for the __init and exit function
#include <linux/module.h>	//< Required to load the module to the kernel
#include <linux/kernel.h>
#include <linux/gpio.h>		//< required for the GPIO function
#include <linux/interrupt.h>	//< required for the IRQ code


MODULE_LICENCE("GPL");
MODULE_AUTHIR("Vishal Chopra");
MODULE_DESCRIPTION("A Button /LED test driver for the BBB");
MODULE_VERSION("0.1");

//--------------------------------global variables------------------------------------

static unsigned int gpioLED = 49;	//< hard coding the LED gpio to P9_23 pin(GPIO4)
static unsigned int gpioButton = 115;	//< hard coding the Button gpio to P9_27 to(GPIO115)
static unsigned int irqNumber;		//< Used to share IRQ number within this file
static unsigned int numberPresses = 0;	//< Store the number of time button press
Static bool ledOn = 0;			//< To find the status of the LED

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

//--------------------------------Intialization function------------------------------

/** @breif The LKM initialization function
 * The static keyword resticts the visibity of the function within the file. 
 * The __init means this function is used while initialization and memory freed up after that point
 * function setup the GPIOs and IRQ.
 * @return return 0 if successful.
 */

static int __init gpio_init(void)
{
	int result = 0;
	printk(KERN_INFO "GPIO_TEST: Initializing the GPIO_TEST LKM\n");
	if (!gpio_is_valid(gpioLED))i{
		printk(KERN_ERR "GPIO_TEST: invalid LED GPIO\n");
		return -ENODEV;
	}

	ledOn = true;  		// Set up the LED. It is a GPIO in output mode and will be on by default
	gpio_request(gpioLED, "sysfs");		// request the gpio for the LED
	gpio_direction_output(gpioLED, ledOn);	// set the gpio in output mode
	gpio_export(gpioLED, false);		// Cases gpio49 to appear in /sys/class/gpio

	gpio_request(gpioButton, "sysfs");	// Request the gpio for the Button
	gpio_direction_input(gpioButton);	// Set the gpioin the input mode
	gpio_set_debounce(gpioButton, 200);	// Bebounce the Button with a daley of 200ms
	gpio_export(gpioButton, false);		// Causes gpio115 to apeare in /sys/class/gpio
	// The bool argument prevent the direction from being changed
	printk(KERN_INFO "GPIO: The button state is currently: %d\n", gpio_get_value(gpioButton));

	irqNumber = gpio_to_irq(gpioButton);
	prink(KERN_INFO "GPIO_TEST: The button is mapped to IRQ: %d\n", irqNumber);

	result = request_irq(irqNumber,		//The interrupt number requested
			(irq_handler_t) gpio_irq_handler, //pointer to the handler function
			IRQ_TRIGGER_RISING, //Interrupt on rising edge(button press not release)
			"gpio_handler",   	//used in /proc/interrupt to identify the owner
			NULL);
	printk(KERN_INFO "GPIO_TEST: The return value of irq_request is %d\n", result);
	return result;
}

//---------------------------------Exit Function--------------------------------------

/** @brief The LKM Cleanup function
  * This function is used to release the GPIOs and display messages
*/

static void __exit gpio_exit(void)
{
	printk(KERN_INFO "GPIO_TEST: The button state is currently %d\n", gpio_get_value(gpioButton));
	printk(KERN_INFO: "GPIO_TEST: The number of time button press %d\n", numberPresses);
	gpio_set_value(gpioLED, 0);		// turn off the LED, make it clear that device is unloaded
	gpio_unexport(gpioLED);			// Uneport the LED gpio, remove the enter from the /proc/interrupt
	free_irq(irqNumber, NULL);		// Free the IRQ number
	gpio_unexport(gpioButton);		// Unexport the Button gpio, remove the enter from the /proc/interrupt
	gpio_free(gpioLED);			// Freed LED gpio
	gpio_free(gpioButton);			// Freed Button gpio
	printk(KERN_INFO "GPIO_TEST: Goodbye from the LKM");
}

//--------------------------------Interrupt Handler------------------------------------

/** @breif The interrupt handler function
  * The nterrupt handler attached to the GPIO. Used to perform function when interrupt is generated.
  * @param irq associated with the GPIO
  * @param dev_id Sturture is used to identify the interruot from which device, in this case it is NULL
  * @param regs  h/w specify register ---only use for the debugging
  * @return return IRQ_HANDLER if Successful otherwise IRQ_NONE
**/

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_reg *regs)
{
	ledOn = !ledOn;
	gpio_set_value(gpioLED, ledOn);
	numberPresses++;
	printk(KERN_INFO "Interrupt Button state is %d\n", gpio_get_value(gpioButton));
	return (irq_handler_t)IRQ_HANDLER;
}


module_init(gpio_init);
module_exit(gpio_exit);
