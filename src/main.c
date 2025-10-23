#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>
#include "canbus.h"
#include "led.h"
#include "control.h"
#include "net.h"

LOG_MODULE_REGISTER(k2_app, LOG_LEVEL_INF);


void main(void)
{

    canbus_init();

    // Initialize the LED GPIO pin
    led_init();

    // Initialize ROV control system
    rov_control_init();
    
    // Initialize networking
    network_init();

    // Start ROV control thread
    rov_control_start();
    
    // Start UDP server thread
    udp_server_start();



    while (1) {

        vesc_set_rpm(68, 2000);
        vesc_set_rpm(37, 750);
        vesc_set_rpm(7, 1500);

        k_sleep(K_SECONDS(1));
    }
}
