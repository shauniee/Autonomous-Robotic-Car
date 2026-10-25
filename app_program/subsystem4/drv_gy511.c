/*
 * drv_gy511.c - Bounded I2C0 implementation for GY-511 (LSM303DLHC).
 */

#include "drv_gy511.h"
#include "../app.h"
#include "../common/robot_time.h"
#include "../../device/i2c/sysdepend/rp2040/i2c_sysdep.h"

/* Registers for LSM303DLHC */
#define REG_CTRL_REG1_A     0x20u
#define REG_CTRL_REG4_A     0x23u
#define REG_OUT_X_L_A       0x28u
#define REG_AUTO_INC_A      0x80u

#define REG_CRA_REG_M       0x00u
#define REG_CRB_REG_M       0x01u
#define REG_MR_REG_M        0x02u
#define REG_OUT_X_H_M       0x03u

/* Wait with timeout for I2C status flag */
static bool wait_i2c_status(uint32_t mask, bool match_set, uint32_t timeout_us) {
    uint32_t start = robot_time_us();
    while (true) {
        uint32_t status = in_w(I2C0_BASE + I2Cx_STATUS);
        bool condition = (status & mask) != 0;
        if (condition == match_set) {
            return true;
        }
        if ((uint32_t)(robot_time_us() - start) >= timeout_us) {
            return false;
        }
    }
}

/* Transmit 1 byte with timeout */
static bool i2c0_write_byte(uint8_t byte, bool stop_bit) {
    if (!wait_i2c_status(I2C_STATUS_TFNF, true, GY511_TIMEOUT_US)) {
        return false;
    }
    uint32_t cmd = byte | (stop_bit ? I2C_DATA_CMD_STOP : 0);
    out_w(I2C0_BASE + I2Cx_DATA_CMD, cmd);
    return true;
}

/* Write single register to slave */
static bool i2c0_write_reg(uint8_t slave_addr, uint8_t reg, uint8_t val) {
    out_w(I2C0_BASE + I2Cx_ENABLE, 0);                 /* Disable to set target */
    out_w(I2C0_BASE + I2Cx_TAR, slave_addr);
    out_w(I2C0_BASE + I2Cx_ENABLE, 1);                 /* Enable */

    if (!i2c0_write_byte(reg, false)) return false;
    if (!i2c0_write_byte(val, true)) return false;

    /* Wait for transmission to finish */
    return wait_i2c_status(I2C_STATUS_ACTIVITY, false, GY511_TIMEOUT_US);
}

/* Read multi-byte buffer from slave */
static bool i2c0_read_buffer(uint8_t slave_addr, uint8_t reg, uint8_t *buf, uint8_t len) {
    out_w(I2C0_BASE + I2Cx_ENABLE, 0);
    out_w(I2C0_BASE + I2Cx_TAR, slave_addr);
    out_w(I2C0_BASE + I2Cx_ENABLE, 1);

    /* Send register sub-address */
    if (!i2c0_write_byte(reg, false)) return false;

    /* Issue read commands into FIFO */
    for (uint8_t i = 0; i < len; ++i) {
        bool is_last = (i == (len - 1));
        uint32_t cmd = I2C_DATA_CMD_CMD_READ | (is_last ? I2C_DATA_CMD_STOP : 0);
        if (!wait_i2c_status(I2C_STATUS_TFNF, true, GY511_TIMEOUT_US)) {
            return false;
        }
        out_w(I2C0_BASE + I2Cx_DATA_CMD, cmd);
    }

    /* Collect returned bytes */
    for (uint8_t i = 0; i < len; ++i) {
        if (!wait_i2c_status(I2C_STATUS_RFNE, true, GY511_TIMEOUT_US)) {
            return false;
        }
        buf[i] = (uint8_t)(in_w(I2C0_BASE + I2Cx_DATA_CMD) & 0xFF);
    }

    return true;
}

bool drv_gy511_init(void) {
    /* 1. Release I2C0 and IO_BANK0 resets */
    clr_w(RESETS_RESET, RESETS_RESET_I2C0);
    uint32_t start = robot_time_us();
    while (!(in_w(RESETS_RESET_DONE) & RESETS_RESET_I2C0)) {
        if ((uint32_t)(robot_time_us() - start) >= GY511_TIMEOUT_US) return false;
    }

    /* 2. Configure GP4 (SDA) and GP5 (SCL) mux to I2C (func 3) with pull-ups */
    for (unsigned p = 4; p <= 5; ++p) {
        out_w(GPIO_CTRL(p), GPIO_CTRL_FUNCSEL_I2C);
        clr_w(GPIO(p), GPIO_OD);
        set_w(GPIO(p), GPIO_IE | GPIO_PUE);
    }

    /* 3. Configure Synopsys I2C0 controller (400 kHz Fast Mode at 125 MHz sys_clk) */
    out_w(I2C0_BASE + I2Cx_ENABLE, 0);
    out_w(I2C0_BASE + I2Cx_CON, I2C_CON_MASTER_MODE | I2C_CON_SPEED_FAST | 
                                I2C_CON_RESTART_EN | I2C_CON_SLAVE_DISABLE | I2C_CON_TX_EMPTY_CTRL);

    /* Clock divider counts for 400 kHz at 125 MHz */
    out_w(I2C0_BASE + I2Cx_FS_SCL_HCNT, 90);
    out_w(I2C0_BASE + I2Cx_FS_SCL_LCNT, 215);

    out_w(I2C0_BASE + I2Cx_ENABLE, 1);

    /* 4. Configure Accelerometer: 100 Hz, all axes enabled, +/- 2g High Res */
    if (!i2c0_write_reg(GY511_I2C_ADDR_ACCEL, REG_CTRL_REG1_A, 0x57u)) return false;
    if (!i2c0_write_reg(GY511_I2C_ADDR_ACCEL, REG_CTRL_REG4_A, 0x08u)) return false;

    /* 5. Configure Magnetometer: 30 Hz, continuous mode */
    i2c0_write_reg(GY511_I2C_ADDR_MAG, REG_CRA_REG_M, 0x14u);
    i2c0_write_reg(GY511_I2C_ADDR_MAG, REG_CRB_REG_M, 0x20u);
    i2c0_write_reg(GY511_I2C_ADDR_MAG, REG_MR_REG_M, 0x00u);

    return true;
}

bool drv_gy511_read_accel(Gy511RawAxes *out_raw) {
    uint8_t buf[6] = {0};
    /* Auto-increment bit 7 required on LSM303 */
    if (!i2c0_read_buffer(GY511_I2C_ADDR_ACCEL, REG_OUT_X_L_A | REG_AUTO_INC_A, buf, 6)) {
        return false;
    }
    out_raw->x = (int16_t)((buf[1] << 8) | buf[0]) >> 4;
    out_raw->y = (int16_t)((buf[3] << 8) | buf[2]) >> 4;
    out_raw->z = (int16_t)((buf[5] << 8) | buf[4]) >> 4;
    return true;
}

bool drv_gy511_read_mag(Gy511RawAxes *out_raw) {
    uint8_t buf[6] = {0};
    if (!i2c0_read_buffer(GY511_I2C_ADDR_MAG, REG_OUT_X_H_M, buf, 6)) {
        return false;
    }
    out_raw->x = (int16_t)((buf[0] << 8) | buf[1]);
    out_raw->z = (int16_t)((buf[2] << 8) | buf[3]);
    out_raw->y = (int16_t)((buf[4] << 8) | buf[5]);
    return true;
}
