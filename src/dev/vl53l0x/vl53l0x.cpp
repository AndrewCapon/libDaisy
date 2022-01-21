/*!
 * @file Adafruit_VL53L0X.cpp
 *
 * @mainpage Adafruit VL53L0X time-of-flight sensor
 *
 * @section intro_sec Introduction
 *
 * This is the documentation for Adafruit's VL53L0X driver for the
 * Arduino platform.  It is designed specifically to work with the
 * Adafruit VL53L0X breakout: https://www.adafruit.com/product/3317
 *
 * These sensors use I2C to communicate, 2 pins (SCL+SDA) are required
 * to interface with the breakout.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section dependencies Dependencies
 *
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */

#include "dev/vl53l0x/vl53l0x.h"
#include "dev/vl53l0x/vl53l0x_api_core.h"

#define VERSION_REQUIRED_MAJOR 1 ///< Required sensor major version
#define VERSION_REQUIRED_MINOR 0 ///< Required sensor minor version
#define VERSION_REQUIRED_BUILD 1 ///< Required sensor build

#define STR_HELPER(x) #x     ///< a string helper
#define STR(x) STR_HELPER(x) ///< string helper wrapper

namespace daisy
{
/**************************************************************************/
/*!
    @brief  Setups the I2C interface and hardware
    @param config Device configuration
    @returns OK if device is set up, ERR on any failure
*/
/**************************************************************************/
Adafruit_VL53L0X::Result Adafruit_VL53L0X::Init(Adafruit_VL53L0X::Config config)
{
    config_ = config;

    uint32_t refSpadCount;
    uint8_t  isApertureSpads;
    uint8_t  VhvSettings;
    uint8_t  PhaseCal;

    // Initialize Comms
    InitI2C();

    Status = VL53L0X_DataInit(&MyDevice); // Data initialization

    if(!setAddress(config_.dev_addr))
    {
        return ERR;
    }

    Status = VL53L0X_GetDeviceInfo(&MyDevice, &DeviceInfo);

    if(Status == VL53L0X_ERROR_NONE)
    {
        if((DeviceInfo.ProductRevisionMajor != 1)
           || (DeviceInfo.ProductRevisionMinor != 1))
        {
            Status = VL53L0X_ERROR_NOT_SUPPORTED;
        }
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        Status = VL53L0X_StaticInit(pMyDevice); // Device Initialization
    }

    return Status == VL53L0X_ERROR_NONE ? OK : ERR;

    if(Status == VL53L0X_ERROR_NONE)
    {
        Status = VL53L0X_PerformRefSpadManagement(
            pMyDevice,
            &refSpadCount,
            &isApertureSpads); // Device Initialization
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        Status
            = VL53L0X_PerformRefCalibration(pMyDevice,
                                            &VhvSettings,
                                            &PhaseCal); // Device Initialization
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        Status = VL53L0X_SetDeviceMode(
            pMyDevice,
            VL53L0X_DEVICEMODE_SINGLE_RANGING); // Setup in single ranging mode
    }

    // call off to the config function to do the last part of configuration.
    if(Status == VL53L0X_ERROR_NONE)
    {
        configSensor(config_.vl_config);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        return OK;
    }
    else
    {
        return ERR;
    }
}

/**************************************************************************/
/*!
    @brief  Change the I2C address of the sensor
    @param  newAddr the new address to set the sensor to
    @returns True if address was set successfully, False otherwise
*/
/**************************************************************************/
bool Adafruit_VL53L0X::setAddress(uint8_t newAddr)
{
    newAddr &= 0x7F;

    Status = VL53L0X_SetDeviceAddress(pMyDevice, newAddr * 2); // 7->8 bit

    System::Delay(10);

    if(Status == VL53L0X_ERROR_NONE)
    {
        pMyDevice->I2cDevAddr = newAddr; // 7 bit addr
        return true;
    }
    return false;
}

/**************************************************************************/
/*!
    @brief  Configure the sensor for one of the ways the example ST
    sketches configure the sensors for different usages.
    @param  vl_config Which configureation you are trying to configure for
    It should be one of the following
        VL53L0X_SENSE_DEFAULT
        VL53L0X_SENSE_LONG_RANGE
        VL53L0X_SENSE_HIGH_SPEED,
        VL53L0X_SENSE_HIGH_ACCURACY

    @returns True if address was set successfully, False otherwise
*/
/**************************************************************************/
bool Adafruit_VL53L0X::configSensor(VL53L0X_Sense_config_t vl_config)
{
    // All of them appear to configure a few things

    // Enable/Disable Sigma and Signal check
    Status = VL53L0X_SetLimitCheckEnable(
        pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);

    if(Status == VL53L0X_ERROR_NONE)
    {
        Status = VL53L0X_SetLimitCheckEnable(
            pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
    }

    if(Status != VL53L0X_ERROR_NONE)
        return false;

    switch(vl_config)
    {
        case VL53L0X_SENSE_DEFAULT:
            // Taken directly from SDK vl5310x_SingleRanging_example.c
            // Maybe should convert to helper functions but...
            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_SetLimitCheckEnable(
                    pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, 1);
            }

            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_SetLimitCheckValue(
                    pMyDevice,
                    VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD,
                    (FixPoint1616_t)(1.5 * 0.023 * 65536));
            }
            break;
        case VL53L0X_SENSE_LONG_RANGE:
            Status = VL53L0X_SetLimitCheckValue(
                pMyDevice,
                VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
                (FixPoint1616_t)(0.1 * 65536));
            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_SetLimitCheckValue(
                    pMyDevice,
                    VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
                    (FixPoint1616_t)(60 * 65536));
            }
            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(
                    pMyDevice, 33000);
            }

            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_SetVcselPulsePeriod(
                    pMyDevice, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18);
            }
            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_SetVcselPulsePeriod(
                    pMyDevice, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14);
            }
            break;
        case VL53L0X_SENSE_HIGH_SPEED:
            Status = VL53L0X_SetLimitCheckValue(
                pMyDevice,
                VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
                (FixPoint1616_t)(0.25 * 65536));
            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_SetLimitCheckValue(
                    pMyDevice,
                    VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
                    (FixPoint1616_t)(32 * 65536));
            }
            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(
                    pMyDevice, 30000);
            }
            break;
        case VL53L0X_SENSE_HIGH_ACCURACY:
            // increase timing budget to 200 ms

            if(Status == VL53L0X_ERROR_NONE)
            {
                setLimitCheckValue(VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
                                   (FixPoint1616_t)(0.25 * 65536));
            }
            if(Status == VL53L0X_ERROR_NONE)
            {
                setLimitCheckValue(VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
                                   (FixPoint1616_t)(18 * 65536));
            }
            if(Status == VL53L0X_ERROR_NONE)
            {
                setMeasurementTimingBudgetMicroSeconds(200000);
            }
            // Not sure about ignore threshold, try turnning it off...
            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_SetLimitCheckEnable(
                    pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, 0);
            }

            break;
    }

    return (Status == VL53L0X_ERROR_NONE);
}

VL53L0X_RangingMeasurementData_t Adafruit_VL53L0X::getSingleRangingMeasurement()
{
    VL53L0X_RangingMeasurementData_t data;
    Status = VL53L0X_PerformSingleRangingMeasurement(pMyDevice, &data);
    return data;
}


uint16_t Adafruit_VL53L0X::readRange(void)
{
    VL53L0X_RangingMeasurementData_t measure;

    measure      = getSingleRangingMeasurement();
    _rangeStatus = measure.RangeStatus;

    if(Status == VL53L0X_ERROR_NONE)
        return measure.RangeMilliMeter;
    // Other status return something totally out of bounds...
    return 0xffff;
}

uint8_t Adafruit_VL53L0X::readRangeStatus(void)
{
    return _rangeStatus;
}

bool Adafruit_VL53L0X::startRange(void)
{
    /* This function will do a complete single ranging
   * Here we fix the mode! */
    // first lets set the device in SINGLE_Ranging mode
    Status
        = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING);

    if(Status == VL53L0X_ERROR_NONE)
    {
        // Lets start up the measurement
        Status = VL53L0X_StartMeasurement(pMyDevice);
    }
    return (Status == VL53L0X_ERROR_NONE);
}

bool Adafruit_VL53L0X::isRangeComplete(void)
{
    uint8_t NewDataReady = 0;
    Status = VL53L0X_GetMeasurementDataReady(pMyDevice, &NewDataReady);
    return ((Status != VL53L0X_ERROR_NONE) || (NewDataReady == 1));
}

bool Adafruit_VL53L0X::waitRangeComplete(void)
{
    Status = VL53L0X_measurement_poll_for_completion(pMyDevice);

    return (Status == VL53L0X_ERROR_NONE);
}

uint16_t Adafruit_VL53L0X::readRangeResult(void)
{
    VL53L0X_RangingMeasurementData_t measure; // keep our own private copy

    Status       = VL53L0X_GetRangingMeasurementData(pMyDevice, &measure);
    _rangeStatus = measure.RangeStatus;
    if(Status == VL53L0X_ERROR_NONE)
        Status = VL53L0X_ClearInterruptMask(pMyDevice, 0);

    if((Status == VL53L0X_ERROR_NONE) && (_rangeStatus != 4))
        return measure.RangeMilliMeter;

    return 0xffff; // some out of range value
}

bool Adafruit_VL53L0X::startRangeContinuous(uint16_t period_ms)
{
    /* This function will do a complete single ranging
   * Here we fix the mode! */
    // first lets set the device in SINGLE_Ranging mode
    Status = VL53L0X_SetDeviceMode(pMyDevice,
                                   VL53L0X_DEVICEMODE_CONTINUOUS_TIMED_RANGING);

    if(Status == VL53L0X_ERROR_NONE)
    {
        Status = VL53L0X_SetInterMeasurementPeriodMilliSeconds(pMyDevice,
                                                               period_ms);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        // Lets start up the measurement
        Status = VL53L0X_StartMeasurement(pMyDevice);
    }
    return (Status == VL53L0X_ERROR_NONE);
}

void Adafruit_VL53L0X::stopRangeContinuous(void)
{
    Status = VL53L0X_StopMeasurement(pMyDevice);

    // lets wait until that completes.
    uint32_t StopCompleted = 0;
    uint32_t LoopNb;

    // Wait until it finished
    // use timeout to avoid deadlock
    if(Status == VL53L0X_ERROR_NONE)
    {
        LoopNb = 0;
        do
        {
            Status = VL53L0X_GetStopCompletedStatus(pMyDevice, &StopCompleted);
            if((StopCompleted == 0x00) || Status != VL53L0X_ERROR_NONE)
            {
                break;
            }
            LoopNb = LoopNb + 1;
            VL53L0X_PollingDelay(pMyDevice);
        } while(LoopNb < VL53L0X_DEFAULT_MAX_LOOP);

        if(LoopNb >= VL53L0X_DEFAULT_MAX_LOOP)
        {
            Status = VL53L0X_ERROR_TIME_OUT;
        }
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        Status = VL53L0X_ClearInterruptMask(
            pMyDevice, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
    }
}

bool Adafruit_VL53L0X::setMeasurementTimingBudgetMicroSeconds(
    uint32_t budget_us)
{
    Status
        = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(pMyDevice, budget_us);
    return (Status == VL53L0X_ERROR_NONE);
}

uint32_t Adafruit_VL53L0X::getMeasurementTimingBudgetMicroSeconds()
{
    uint32_t budget_us;
    Status
        = VL53L0X_GetMeasurementTimingBudgetMicroSeconds(pMyDevice, &budget_us);
    return (budget_us);
}

bool Adafruit_VL53L0X::setVcselPulsePeriod(VL53L0X_VcselPeriod VcselPeriodType,
                                           uint8_t             VCSELPulsePeriod)
{
    Status = VL53L0X_SetVcselPulsePeriod(
        pMyDevice, VcselPeriodType, VCSELPulsePeriod);
    return (Status == VL53L0X_ERROR_NONE);
}

uint8_t
Adafruit_VL53L0X::getVcselPulsePeriod(VL53L0X_VcselPeriod VcselPeriodType)
{
    uint8_t cur_period;
    Status
        = VL53L0X_GetVcselPulsePeriod(pMyDevice, VcselPeriodType, &cur_period);
    return (cur_period);
}

bool Adafruit_VL53L0X::setLimitCheckEnable(uint16_t LimitCheckId,
                                           uint8_t  LimitCheckEnable)
{
    Status = VL53L0X_SetLimitCheckEnable(
        pMyDevice, LimitCheckId, LimitCheckEnable);
    return (Status == VL53L0X_ERROR_NONE);
}

uint8_t Adafruit_VL53L0X::getLimitCheckEnable(uint16_t LimitCheckId)
{
    uint8_t cur_limit;
    Status = VL53L0X_GetLimitCheckEnable(pMyDevice, LimitCheckId, &cur_limit);
    return (cur_limit);
}

bool Adafruit_VL53L0X::setLimitCheckValue(uint16_t       LimitCheckId,
                                          FixPoint1616_t LimitCheckValue)
{
    Status
        = VL53L0X_SetLimitCheckValue(pMyDevice, LimitCheckId, LimitCheckValue);
    return (Status == VL53L0X_ERROR_NONE);
}

FixPoint1616_t Adafruit_VL53L0X::getLimitCheckValue(uint16_t LimitCheckId)
{
    FixPoint1616_t LimitCheckValue;
    Status
        = VL53L0X_GetLimitCheckValue(pMyDevice, LimitCheckId, &LimitCheckValue);
    return (LimitCheckValue);
}


Adafruit_VL53L0X::Result Adafruit_VL53L0X::InitI2C()
{
    MyDevice.i2c = &i2c_;
    MyDevice.I2cDevAddr = VL53L0X_I2C_ADDR; // always start with the default address

    I2CHandle::Config i2c_config;
    i2c_config.mode   = I2CHandle::Config::Mode::I2C_MASTER;
    i2c_config.periph = config_.periph;
    i2c_config.speed  = config_.speed;

    i2c_config.pin_config.scl = config_.scl;
    i2c_config.pin_config.sda = config_.sda;

    return I2CHandle::Result::OK == i2c_.Init(i2c_config) ? OK : ERR;
}

} // namespace daisy