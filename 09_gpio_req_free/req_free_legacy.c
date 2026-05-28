#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BrandonFors - LDD");
MODULE_DESCRIPTION("GPIO Request Example");

static const char *device_name = "gpio_ctrl";

#define LED_GPIO 24
#define BUTTON_GPIO 23

// offset correlates to gpiochip571 whos label is pinctrl-rp1 -> controls our exposed gpio pins
#define GPIO_OFFSET 571

static int led_gpio = (LED_GPIO + GPIO_OFFSET);
static int button_gpio = (BUTTON_GPIO + GPIO_OFFSET);


static int __init my_init(void){
    int status;
    // request led gpio pin
    status = gpio_request(led_gpio, "led_gpio");
    if(status){
       pr_err("%s: Failed to request led gpio 24\n", device_name);
       return -status; 
    }
    gpio_direction_output(led_gpio, 0);

    // request button gpio pin
    status = gpio_request(button_gpio, "button_gpio");
    if(status){
       pr_err("%s: Failed to request button gpio 23\n", device_name);
       gpio_free(led_gpio);
       return -status; 
    }
    gpio_direction_input(button_gpio);

    gpio_set_value(led_gpio, 1);

    pr_info("%s: Button state is: %d\n", device_name, gpio_get_value(button_gpio));

    pr_info("%s: GPIO request example loaded\n", device_name);
    return 0;
}

static void __exit my_exit(void){

    gpio_set_value(led_gpio, 0);
    gpio_free(led_gpio);
    gpio_free(button_gpio);
    
    pr_info("%s: GPIO example exit\n", device_name);

}

module_init(my_init);
module_exit(my_exit);