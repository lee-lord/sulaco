#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#if defined(USE_BARO) && defined(USE_BARO_SPI_LPS)

#include "build/build_config.h"

#include "barometer.h"
#include "barometer_lps.h"

#include "drivers/bus_spi.h"
#include "drivers/io.h"
#include "drivers/time.h"

//====================================Registers Addresses=========================================//
#define LPS_REF_P_XL    0x08
#define LPS_REF_P_L     0x09
#define LPS_REF_P_H     0x0A
#define LPS_WHO_AM_I    0x0F
/*
 * RES_CONF (10h)
 *
 *  Bits 3-2 AVGT - Temperature internal average configuration.
 *      Default: 11 (64)
 *  Bits 1-0 AVGP - Pressure internal average configuration.
 *      Default: 11 (512)
 *
 */
#define LPS_RES_CONF    0x10
/*
 * CTRL_REG1(20h)
 *
 *  Bit 7 PD - Power-down control.
 *      Default value: 0
 *      (0: power-down mode; 1: active mode)
 *  Bits 6-4 ODR - Output data rate selection.
 *      Default value: 000 (One shot mode)
 *      000 - One Shot
 *      001 - 1Hz
 *      010 - 7Hz
 *      011 - 12.5Hz
 *      100 - 25Hz
 *      101 - Resv
 *  Bit 3 DIFF_EN - Interrupt generation enable.
 *      Default value: 0
 *      (0: interrupt generation disabled; 1: interrupt generation enabled)
 *  Bit 2 BDU - Block data update.
 *      Default value: 0
 *      (0: continuous update; 1: output registers not updated until MSB and LSB have been read)
 *  Bit 1 RESET_AZ - Reset Autozero function.
 *      Default value: 0
 *      (0: normal mode; 1: reset Autozero function)
 *  Bit 0 SIM - SPI Serial Interface Mode selection.
 *      Default value: 0
 *      (0: 4-wire interface; 1: 3-wire interface)
 *
 */
#define LPS_CTRL1       0x20
/*
 * CTRL_REG2(21h)
 *
 *  Bit 7 BOOT - Reboot memory content.
 *      Default value: 0.
 *      (0: normal mode; 1: reboot memory content).
 *      The bit is self-cleared when the BOOT is completed.
 *  Bit 6 FIFO_EN - FIFO enable.
 *      Default value: 0.
 *      (0: disable; 1: enable)
 *  Bit 5 STOP_ON_FTH - Enable the FTH_FIFO bit in FIFO_STATUS (2Fh) for monitoring of FIFO level.
 *      Default value: 0
 *      (0: disable; 1: enable).
 *  Bit 4 FIFO_MEAN_DEC - Enable to decimate the output pressure to 1Hz with FIFO Mean mode.
 *      Default value: 0
 *      (0: disable / 1: enable)
 *  Bit 3 I2C_DIS - I2C interface enabled.
 *      Default value 0.
 *      (0: I2C enabled;1: I2C disabled)
 *  Bit 2 SWRESET - Software reset.
 *      Default value: 0.
 *      (0: normal mode; 1: software reset).
 *      The bit is self-cleared when the reset is completed.
 *  Bit 1 AUTOZERO - Autozero enable.
 *      Default value: 0.
 *      (0: normal mode; 1: Autozero enabled)
 *  Bit 0 ONE_SHOT - One shot mode enable.
 *      Default value: 0.
 *      (0: idle mode; 1: a new dataset is acquired)
 *
 */
#define LPS_CTRL2       0x21
/*
 * CTRL_REG3(21h)
 *
 *  Bit 7 INT_H_L - Interrupt active high, low.
 *      Default value: 0.
 *      (0: active high; 1: active low)
 *  Bit 6 PP_OD - Push-pull/open drain selection on interrupt pads.
 *      Default value: 0.
 *      (0: push-pull; 1: open drain)
 *  Bits 1-0 INT_S - Data signal on INT_DRDY pin control bits.
 *      Default value: 00.
 */
#define LPS_CTRL3       0x22
/*
 * CTRL_REG4(23h)
 *
 *  Bit 3 F_EMPTY - FIFO empty flag on INT_DRDY pin.
 *      Default value: 0.
 *      (0: disable; 1: enable)
 *  Bit 2 F_FTH - FIFO threshold (watermark) status on INT_DRDY pin to indicate that FIFO is filled up to the threshold level.
 *      Default value: 0.
 *      (0: disable; 1: enable)
 *  Bit 1 F_OVR - FIFO overrun interrupt on INT_DRDY pin to indicate that FIFO is full in FIFO mode or that an overrun occurred in Stream mode.
 *      Default value: 0.
 *      (0: disable; 1: enable)
 *  Bit 0 DRDY - Data-ready signal on INT_DRDY pin.
 *      Default value: 0.
 *      (0: disable; 1: enable)
 */
#define LPS_CTRL4       0x23
#define LPS_INT_CFG     0x24
#define LPS_INT_SOURCE  0x25
#define LPS_STATUS      0x27
#define LPS_OUT_XL      0x28
#define LPS_OUT_L       0x29
#define LPS_OUT_H       0x2A
#define LPS_TEMP_OUT_L  0x2B
#define LPS_TEMP_OUT_H  0x2C
#define LPS_FIFO_CTRL   0x2E
#define LPS_FIFO_STATUS 0x2F
#define LPS_THS_PL      0x30
#define LPS_THS_PH      0x31
#define LPS_RPDS_L      0x39
#define LPS_RPDS_H      0x3A
//=======================================Constants=============================================//
#define LPS22_ID        0xB1
#define LPS25_ID        0xBD
#define LPS33_ID        0xB1
#define LPS35_ID        0xB1
#define LPS_READ        0x80
#define LPS_MULT        0x40
//======================================ODR Values=============================================//
// CTRL1 value
#define LPS_ODR_ONE_SHOT        0x00
#define LPS_ODR_1               0x10
#define LPS_ODR_7               0x20
#define LPS_ODR_12_5            0x30
#define LPS_ODR_25              0x40
//======================================Average number=============================================//
#define LPS_AVP_8               0x00
#define LPS_AVP_32              0x01
#define LPS_AVP_128             0x02
#define LPS_AVP_512             0x03
#define LPS_AVT_8               0x00
#define LPS_AVT_16              0x04
#define LPS_AVT_32              0x08
#define LPS_AVT_64              0x0C
//======================================Moving average number=============================================//
#define LPS_NO_MA               0x00
#define LPS_MA_2                0x01
#define LPS_MA_4                0x03
#define LPS_MA_8                0x07
#define LPS_MA_16               0x0F
#define LPS_MA_32               0x1F

//Raw register values
static uint32_t rawP = 0;
static uint16_t rawT = 0;

bool lpsWriteCommand(busDevice_t *busdev, uint8_t cmd, uint8_t byte)
{
    return spiBusWriteRegister(busdev, cmd, byte);
}

bool lpsReadCommand(busDevice_t *busdev, uint8_t cmd, uint8_t *data, uint8_t len)
{
    return spiBusReadRegisterBuffer(busdev, cmd | 0x80 | 0x40, data, len);
}

bool lpsWriteVerify(busDevice_t *busdev, uint8_t cmd, uint8_t byte)
{
    uint8_t temp = 0xff;
    spiBusWriteRegister(busdev, cmd, byte);
    spiBusReadRegisterBuffer(busdev, cmd, &temp, 1);
    if (byte == temp) return true;
    return false;
}

static void lpsOn(busDevice_t *busdev, uint8_t CTRL1_val)
{
    lpsWriteCommand(busdev, LPS_CTRL1, CTRL1_val | 0x80);
    //Instead of delay let's ready status reg
}

static void lpsOff(busDevice_t *busdev)
{
    lpsWriteCommand(busdev, LPS_CTRL1, 0x00 | (0x01 << 2));
}

static void lps_nothing(baroDev_t *baro)
{
    UNUSED(baro);
    return;
}

static void lps_read(baroDev_t *baro)
{
    uint8_t status = 0x00;
    lpsReadCommand(&baro->busdev, LPS_STATUS, &status, 1);
    if (status & 0x03) {
        uint8_t temp[5];
        lpsReadCommand(&baro->busdev, LPS_OUT_XL, temp, 5);

        /* Build the raw data */
        rawP = temp[0] | (temp[1] << 8) | (temp[2] << 16) | ((temp[2] & 0x80) ? 0xff000000 : 0);
        rawT = (temp[4] << 8) | temp[3];
    } else {
        rawP = 0;
        rawT = 0;
    }
}

static void lps_calculate(int32_t *pressure, int32_t *temperature)
{
    *pressure = (int32_t)rawP * 100 / 4096;
    *temperature = (int32_t)rawT * 10 / 48 + 4250;
}

bool lpsDetect(baroDev_t *baro)
{
    //Detect
    busDevice_t *busdev = &baro->busdev;
    IOInit(busdev->busdev_u.spi.csnPin, OWNER_BARO_CS, 0);
    IOConfigGPIO(busdev->busdev_u.spi.csnPin, IOCFG_OUT_PP);
    IOHi(busdev->busdev_u.spi.csnPin); // Disable
    spiSetDivisor(busdev->busdev_u.spi.instance, SPI_CLOCK_STANDARD); // Baro can work only on up to 10Mhz SPI bus

    uint8_t temp = 0x00;
    lpsReadCommand(&baro->busdev, LPS_WHO_AM_I, &temp, 1);
    if (temp != LPS25_ID && temp != LPS22_ID && temp != LPS33_ID && temp != LPS35_ID) {
        return false;
    }

    //Init, if writeVerify is false fallback to false on detect
    bool ret = false;
    lpsOff(busdev);
    ret = lpsWriteVerify(busdev, LPS_CTRL2, (0x00 << 1)); if (ret != true) return false;
    ret = lpsWriteVerify(busdev, LPS_RES_CONF, (LPS_AVT_64 | LPS_AVP_512)); if (ret != true) return false;
    ret = lpsWriteVerify(busdev, LPS_CTRL4, 0x01); if (ret != true) return false;
    lpsOn(busdev, (0x04 << 4) | (0x01 << 1) | (0x01 << 2) | (0x01 << 3));

    lpsReadCommand(busdev, LPS_CTRL1, &temp, 1);

    baro->ut_delay = 1;
    baro->up_delay = 1000000 / 24;
    baro->start_ut = lps_nothing;
    baro->get_ut = lps_nothing;
    baro->start_up = lps_nothing;
    baro->get_up = lps_read;
    baro->calculate = lps_calculate;
    uint32_t timeout = millis();
    do {
        lps_read(baro);
        if ((millis() - timeout) > 500) return false;
    } while (rawT == 0 && rawP == 0);
    rawT = 0;
    rawP = 0;
    return true;
}
#endif
