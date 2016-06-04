/**
  * @file header.h
  * @author Vishal Chopra
  * @date 23 May 2016
  * @brief This filoe include all the header file required for the LKM to load 
  * into the kernel.
*/


#include <linux/init.h>		//< Required for the macro __init and __exit
#include <linux/module.h>	//< To Load LKM to the kernel
#include <linux/kernel.h>
#include <linux/gpio.h>		//< Required for the GPIOs function
#include <linux/interrupt.h>	//< Required for the IRQ code
#include <linux/kobject.h>	//< Using kobject for the sysfs binding
#include <linux/time.h>		//< Using the clock to measure time between button presses

#define DEBOUNCE_TIME 200	//< The default bounce time --200ms

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vishal Chopra");
MODULE_DESCRIPTION("A simple linux GPIO Button LKM for the BBB");
MODULE_VERSION("0.1");


