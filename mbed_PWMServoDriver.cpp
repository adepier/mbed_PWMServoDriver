/*!
 *  @file mbed_PWMServoDriver.cpp
 *
 *  @mainpage Adafruit 16-channel PWM & Servo driver
 *
 *  @section intro_sec Introduction
 *
 *  This is a library for the 16-channel PWM & Servo driver.
 *
 *  Designed specifically to work with the Adafruit PWM & Servo driver.
 *
 *  Pick one up today in the adafruit shop!
 *  ------> https://www.adafruit.com/product/815
 *
 *  These displays use I2C to communicate, 2 pins are required to interface.
 *
 *  Adafruit invests time and resources providing this open source code,
 *  please support Adafruit andopen-source hardware by purchasing products
 *  from Adafruit!
 *
 *  @section author Author
 *
 *  Limor Fried/Ladyada (Adafruit Industries).
 *
 *  @section license License
 *
 *  BSD license, all text above must be included in any redistribution
 */

#include "mbed_PWMServoDriver.h" 

//#define ENABLE_DEBUG_OUTPUT

 /*!
 *  @brief  Instantiates a new PCA9685 PWM driver chip with the I2C address on a
 * TwoWire interface
 */
// mbed_PWMServoDriver::mbed_PWMServoDriver()
//     : _i2caddr(PCA9685_I2C_ADDRESS), _i2c(&Wire) {}

// /*!
//  *  @brief  Instantiates a new PCA9685 PWM driver chip with the I2C address on a
//  * TwoWire interface
//  *  @param  addr The 7-bit I2C address to locate this chip, default is 0x40
//  */
// mbed_PWMServoDriver::mbed_PWMServoDriver(const uint8_t addr)
//     : _i2caddr(addr), _i2c(&I2C() ) {}

/*!
 *  @brief  Instantiates a new PCA9685 PWM driver chip with the I2C address on a
 * TwoWire interface
 *  @param  addr The 7-bit I2C address to locate this chip, default is 0x40
 *  @param  i2c  A reference to a 'TwoWire' object that we'll use to communicate
 *  with
 */
mbed_PWMServoDriver::mbed_PWMServoDriver(const uint8_t addr,
                                                 I2C &i2c)
    : _i2caddr(addr << 1), _i2c(&i2c) {
      
    }

/*!
 *  @brief  Setups the I2C interface and hardware
 *  @param  prescale
 *          Sets External Clock (Optional)
 */
void mbed_PWMServoDriver::begin(uint8_t prescale) {
  
  reset();
  if (prescale) {
    setExtClk(prescale);
  } else {
    // set a default frequency
    setPWMFreq(1000);
  }
  // set the default internal frequency
  setOscillatorFrequency(FREQUENCY_OSCILLATOR);
}

/*!
 *  @brief  Sends a reset command to the PCA9685 chip over I2C
 */
void mbed_PWMServoDriver::reset() {
  write8(PCA9685_MODE1, MODE1_RESTART);
 ThisThread::sleep_for(chrono::milliseconds(10));
}

/*!
 *  @brief  Puts board into sleep mode
 */
void mbed_PWMServoDriver::sleep() {
  uint8_t awake = read8(PCA9685_MODE1);
  uint8_t sleep = awake | MODE1_SLEEP; // set sleep bit high
  write8(PCA9685_MODE1, sleep);
  ThisThread::sleep_for(chrono::milliseconds(5)); // wait until cycle ends for sleep to be active
}

/*!
 *  @brief  Wakes board from sleep
 */
void mbed_PWMServoDriver::wakeup() {
  uint8_t sleep = read8(PCA9685_MODE1);
  uint8_t wakeup = sleep & ~MODE1_SLEEP; // set sleep bit low
  write8(PCA9685_MODE1, wakeup);
}

/*!
 *  @brief  Sets EXTCLK pin to use the external clock
 *  @param  prescale
 *          Configures the prescale value to be used by the external clock
 */
void mbed_PWMServoDriver::setExtClk(uint8_t prescale) {
  uint8_t oldmode = read8(PCA9685_MODE1);
  uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP; // sleep
  write8(PCA9685_MODE1, newmode); // go to sleep, turn off internal oscillator

  // This sets both the SLEEP and EXTCLK bits of the MODE1 register to switch to
  // use the external clock.
  write8(PCA9685_MODE1, (newmode |= MODE1_EXTCLK));

  write8(PCA9685_PRESCALE, prescale); // set the prescaler

  ThisThread::sleep_for(chrono::milliseconds(5));
  // clear the SLEEP bit to start
  write8(PCA9685_MODE1, (newmode & ~MODE1_SLEEP) | MODE1_RESTART | MODE1_AI);

#ifdef ENABLE_DEBUG_OUTPUT
  printf("Mode now 0x %i \n",read8(PCA9685_MODE1));
   
#endif
}

/*!
 *  @brief  Sets the PWM frequency for the entire chip, up to ~1.6 KHz
 *  @param  freq Floating point frequency that we will attempt to match
 */
void mbed_PWMServoDriver::setPWMFreq(float freq) {
#ifdef ENABLE_DEBUG_OUTPUT
  printf("Attempting to set freq %f \n",freq);
  
#endif
  // Range output modulation frequency is dependant on oscillator
  if (freq < 1)
    freq = 1;
  if (freq > 3500)
    freq = 3500; // Datasheet limit is 3052=50MHz/(4*4095)

  float prescaleval = ((_oscillator_freq / (freq * 4095.0)) + 0.5) - 1;
  if (prescaleval < PCA9685_PRESCALE_MIN)
    prescaleval = PCA9685_PRESCALE_MIN;
  if (prescaleval > PCA9685_PRESCALE_MAX)
    prescaleval = PCA9685_PRESCALE_MAX;
  uint8_t prescale = (uint8_t)prescaleval;

#ifdef ENABLE_DEBUG_OUTPUT
  printf("Final pre-scale: %i",prescale);
   
#endif

  uint8_t oldmode = read8(PCA9685_MODE1);
  uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP; // sleep
  write8(PCA9685_MODE1, newmode);                             // go to sleep
  write8(PCA9685_PRESCALE, prescale); // set the prescaler
  write8(PCA9685_MODE1, oldmode);
  ThisThread::sleep_for(chrono::milliseconds(5));
  // This sets the MODE1 register to turn on auto increment.
  write8(PCA9685_MODE1, oldmode | MODE1_RESTART | MODE1_AI);

#ifdef ENABLE_DEBUG_OUTPUT
  printf("Mode now 0x %i \n",read8(PCA9685_MODE1));
  
#endif
}

/*!
 *  @brief  Sets the output mode of the PCA9685 to either
 *  open drain or push pull / totempole.
 *  Warning: LEDs with integrated zener diodes should
 *  only be driven in open drain mode.
 *  @param  totempole Totempole if true, open drain if false.
 */
void mbed_PWMServoDriver::setOutputMode(bool totempole) {
  uint8_t oldmode = read8(PCA9685_MODE2);
  uint8_t newmode;
  if (totempole) {
    newmode = oldmode | MODE2_OUTDRV;
  } else {
    newmode = oldmode & ~MODE2_OUTDRV;
  }
  write8(PCA9685_MODE2, newmode);
#ifdef ENABLE_DEBUG_OUTPUT
  printf("Setting output mode: ");
  printf(totempole ? "totempole" : "open drain");
  printf(" by setting MODE2 to %i \n",newmode);
  
#endif
}

/*!
 *  @brief  Reads set Prescale from PCA9685
 *  @return prescale value
 */
uint8_t mbed_PWMServoDriver::readPrescale(void) {
  return read8(PCA9685_PRESCALE);
}

/*!
 *  @brief  Gets the PWM output of one of the PCA9685 pins
 *  @param  num One of the PWM output pins, from 0 to 15
 *  @return requested PWM output value
 */
uint8_t mbed_PWMServoDriver::getPWM(uint8_t num) {
    
//   _i2c->requestFrom((int)_i2caddr, PCA9685_LED0_ON_L + 4 * num, (int)4);
//   return _i2c->read();
return read8(  PCA9685_LED0_ON_L + 4 * num );
}

/*!
 *  @brief  Sets the PWM output of one of the PCA9685 pins
 *  @param  num One of the PWM output pins, from 0 to 15
 *  @param  on At what point in the 4095-part cycle to turn the PWM output ON
 *  @param  off At what point in the 4095-part cycle to turn the PWM output OFF
 */
void mbed_PWMServoDriver::setPWM(uint8_t num, uint16_t on, uint16_t off) {
#ifdef ENABLE_DEBUG_OUTPUT
  printf("Setting PWM %i: %i->%i\n",num,on,off); 
#endif 
char cmd[5];
    cmd[0] = PCA9685_LED0_ON_L + 4 * num;
    cmd[1] = on;
    cmd[2] = on >> 8;
    cmd[3] = off;
    cmd[4] = off >> 8; 
  if (_i2c->write(_i2caddr, cmd, 5),false)
   {    
        printf("setPWM ERR: No ACK on i2c write pin %i!", num);
    };
 //printf("setPWM data:  %s \n ",  cmd); 
//   _i2c->beginTransmission(_i2caddr);
//   _i2c->write(PCA9685_LED0_ON_L + 4 * num);
//   _i2c->write(on);
//   _i2c->write(on >> 8);
//   _i2c->write(off);
//   _i2c->write(off >> 8);
//   _i2c->endTransmission();
}

/*!
 *   @brief  Helper to set pin PWM output. Sets pin without having to deal with
 * on/off tick placement and properly handles a zero value as completely off and
 * 4095 as completely on.  Optional invert parameter supports inverting the
 * pulse for sinking to ground.
 *   @param  num One of the PWM output pins, from 0 to 15
 *   @param  val The number of ticks out of 4095 to be active, should be a value
 * from 0 to 4095 inclusive.
 *   @param  invert If true, inverts the output, defaults to 'false'
 */
void mbed_PWMServoDriver::setPin(uint8_t num, uint16_t val, bool invert) {
  // Clamp value between 0 and 4095 inclusive.
  val = min(val, (uint16_t)4095);
  if (invert) {
    if (val == 0) {
      // Special value for signal fully on.
      setPWM(num, 4095, 0);
    } else if (val == 4095) {
      // Special value for signal fully off.
      setPWM(num, 0, 4095);
    } else {
      setPWM(num, 0, 4095 - val);
    }
  } else {
    if (val == 4095) {
      // Special value for signal fully on.
      setPWM(num, 4095, 0);
    } else if (val == 0) {
      // Special value for signal fully off.
      setPWM(num, 0, 4095);
    } else {
      setPWM(num, 0, val);
    }
  }
}

/*!
 *  @brief  Sets the PWM output of one of the PCA9685 pins based on the input
 * microseconds, output is not precise
 *  @param  num One of the PWM output pins, from 0 to 15
 *  @param  Microseconds The number of Microseconds to turn the PWM output ON
 */
void mbed_PWMServoDriver::writeMicroseconds(uint8_t num,
                                                uint16_t Microseconds) {
#ifdef ENABLE_DEBUG_OUTPUT
  printf("Setting PWM Via Microseconds on output %i: %i->\n",num,Microseconds);
   
#endif

  double pulse = Microseconds;
  double pulselength;
  pulselength = 1000000; // 1,000,000 us per second

  // Read prescale
  uint16_t prescale = readPrescale();

#ifdef ENABLE_DEBUG_OUTPUT
  
  printf("%i PCA9685 chip prescale\n",prescale);
  
#endif

  // Calculate the pulse for PWM based on Equation 1 from the datasheet section
  // 7.3.5
  prescale += 1;
  pulselength *= prescale;
  pulselength /= _oscillator_freq;

#ifdef ENABLE_DEBUG_OUTPUT
 
  printf("%f us per bit \n",pulselength);
 
#endif

  pulse /= pulselength;

#ifdef ENABLE_DEBUG_OUTPUT 
  printf("%f pulse for PWM \n",pulse); 
#endif

  setPWM(num, 0, pulse);
}

/*!
 *  @brief  Getter for the internally tracked oscillator used for freq
 * calculations
 *  @returns The frequency the PCA9685 thinks it is running at (it cannot
 * introspect)
 */
uint32_t mbed_PWMServoDriver::getOscillatorFrequency(void) {
  return _oscillator_freq;
}

/*!
 *  @brief Setter for the internally tracked oscillator used for freq
 * calculations
 *  @param freq The frequency the PCA9685 should use for frequency calculations
 */
void mbed_PWMServoDriver::setOscillatorFrequency(uint32_t freq) {
  _oscillator_freq = freq;
}

/******************* Low level I2C interface */

uint8_t mbed_PWMServoDriver::read8(uint8_t addr) {
    char data;
    if(_i2c->write(_i2caddr, (char *)&addr, 1, true))
     
        printf("I2C ERR: no ack on write before read.\n");
     
    if(_i2c->read(_i2caddr, &data, 1))
    
        printf("I2C ERR: no ack on read\n");
     
    return (uint8_t)data;
}

void mbed_PWMServoDriver::write8(uint8_t addr, uint8_t d) {
    char data[] = { addr, d };
    if(_i2c->write(_i2caddr, data, 2))
    {    
     
        printf("I2C ERR: No ACK on i2c write!");
    
    }
}