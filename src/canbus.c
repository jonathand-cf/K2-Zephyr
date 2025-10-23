#include "canbus.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(canbus, LOG_LEVEL_INF);

static const struct device *can_dev;

/* 29-bit Extended ID: (CMD << 8) | controller_id */
static inline uint32_t vesc_eid(uint8_t cmd, uint8_t cid)
{
    return ((uint32_t)cmd << 8) | (uint32_t)cid;
}

static inline void put_u32_be(uint8_t *dst, int32_t v)
{
    dst[0] = (uint8_t)(v >> 24);
    dst[1] = (uint8_t)(v >> 16);
    dst[2] = (uint8_t)(v >>  8);
    dst[3] = (uint8_t)(v >>  0);
}

int canbus_init(void)
{
    can_dev = DEVICE_DT_GET(DT_NODELABEL(can1));
    if (!device_is_ready(can_dev)) {
        LOG_ERR("CAN not ready");
        return -ENODEV;
    }
    int ret = can_set_mode(can_dev, CAN_MODE_NORMAL);
    if (ret) { LOG_ERR("can_set_mode=%d", ret); return ret; }
    ret = can_start(can_dev);
    if (ret) { LOG_ERR("can_start=%d", ret); return ret; }

    enum can_state st; struct can_bus_err_cnt cnt;
    if (!can_get_state(can_dev, &st, &cnt)) {
#if KERNEL_VERSION_MAJOR >= 4
        LOG_INF("CAN ready: state=%d REC=%u TEC=%u", st, cnt.rx_err_cnt, cnt.tx_err_cnt);
#else
        LOG_INF("CAN ready: state=%d REC=%u TEC=%u", st, cnt.rx_err_cnt, cnt.tx_err_cnt);
#endif
    }
    return 0;
}

int vesc_send_u32(enum vesc_can_cmd cmd, uint8_t controller_id, int32_t val)
{
    if (!can_dev) return -ENODEV;

    struct can_frame f = {
        .id    = vesc_eid((uint8_t)cmd, controller_id),  // 29-bit EID
        .dlc   = 4,
        .flags = CAN_FRAME_IDE                           // EXTENDED!
    };
    put_u32_be(f.data, val);

    int ret = can_send(can_dev, &f, K_MSEC(200), NULL, NULL);
    if (ret) {
        LOG_ERR("VESC send cmd=%u id=%u ret=%d", (unsigned)cmd, (unsigned)controller_id, ret);
    }
    return ret;
}

int vesc_set_duty(uint8_t id, float duty)
{
    /* VESC forventer duty * 100000 (signert 32-bit) */
    int32_t v = (int32_t)(duty * 100000.0f);
    return vesc_send_u32(VESC_CAN_SET_DUTY, id, v);
}

int vesc_set_current(uint8_t id, float amp)
{
    /* VESC forventer A * 1000 */
    int32_t v = (int32_t)(amp * 1000.0f);
    return vesc_send_u32(VESC_CAN_SET_CURRENT, id, v);
}

int vesc_set_brake(uint8_t id, float amp)
{
    int32_t v = (int32_t)(amp * 1000.0f);
    return vesc_send_u32(VESC_CAN_SET_CURRENT_BRAKE, id, v);
}

int vesc_set_rpm(uint8_t id, int32_t rpm)
{
    return vesc_send_u32(VESC_CAN_SET_RPM, id, rpm);   // u-skalert RPM
}

int vesc_set_pos(uint8_t id, int32_t pos)
{
    return vesc_send_u32(VESC_CAN_SET_POS, id, pos);
}
