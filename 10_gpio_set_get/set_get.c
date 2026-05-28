#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio/consumer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BrandonFors - LDD");
MODULE_DESCRIPTION("GPIO Request Example");

static const char *device_name = "gpio_ctrl";

#define LED_GPIO 24
#define BUTTON_GPIO 23

// offset correlates to gpiochip571 whos label is pinctrl-rp1 -> controls our exposed gpio pins
#define GPIO_OFFSET 571

//gpio descriptors are safer and easier to manage
static struct gpio_desc *led, *button;

static int led_gpio = (LED_GPIO + GPIO_OFFSET);
static int button_gpio = (BUTTON_GPIO + GPIO_OFFSET);


static int __init my_init(void){
    int status;
    led = gpio_to_desc(led_gpio);
    if(!led){
       pr_err("%s: Failed to request led gpio 24\n", device_name);
       return -1; 
    }

    button = gpio_to_desc(button_gpio);
    if(!button){
       pr_err("%s: Failed to request button gpio 23\n", device_name);
       return -1; 
    }
    status = gpiod_direction_input(button);
    if(status){
       pr_err("%s: Unable to set GPIO 23 as input\n", device_name);
       return -1; 
    }
    gpiod_set_value(led, 1);

    pr_info("%s: Button state is: %d\n", device_name, gpiod_get_value(button));

    pr_info("%s: GPIO request example loaded\n", device_name);
    return 0;
}

static void __exit my_exit(void){

    gpiod_set_value(led, 0);
    // no need to free gpios as we didn't request them in the first place
    
    pr_info("%s: GPIO example exit\n", device_name);

}

module_init(my_init);
module_exit(my_exit);