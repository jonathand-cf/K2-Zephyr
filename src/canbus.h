#pragma once
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>

/* VESC CAN commands (utdrag) */
enum vesc_can_cmd {
    VESC_CAN_SET_DUTY              = 0,
    VESC_CAN_SET_CURRENT           = 1,
    VESC_CAN_SET_CURRENT_BRAKE     = 2,
    VESC_CAN_SET_RPM               = 3,
    VESC_CAN_SET_POS               = 4,
    VESC_CAN_SET_CURRENT_REL       = 10,
    VESC_CAN_SET_CURRENT_BRAKE_REL = 11,
};

/* Init av CAN (starter i NORMAL) */
int  canbus_init(void);

/* VESC-sending: controller_id = VESC sin CAN ID (0..255) */
int  vesc_set_duty   (uint8_t controller_id, float duty);      // [-1.0..+1.0]
int  vesc_set_current(uint8_t controller_id, float amp);       // A
int  vesc_set_brake  (uint8_t controller_id, float amp);       // A (brems)
int  vesc_set_rpm    (uint8_t controller_id, int32_t rpm);     // RPM (heltall)
int  vesc_set_pos    (uint8_t controller_id, int32_t pos);     // valgfri

/* Valgfri: send rå VESC-kommando (int32 payload) */
int  vesc_send_u32(enum vesc_can_cmd cmd, uint8_t controller_id, int32_t val);
