/*
 * Set I2C Slave address. You can have multiple sensors with different addresses
 */
#define I2C_SLAVE_ADDRESS 0x13

#include <TinyWireS.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif

#define STATUS_PIN_1 4
#define ADC_PIN A3

volatile uint8_t i2c_regs[] =
{
    0, //older 8
    0 //younger 8
};

volatile byte reg_position = 0;
const byte reg_size = sizeof(i2c_regs);

/*
 * 0=16ms
 * 1=32ms
 * 2=64ms
 * 3=128ms
 * 4=250ms
 * 5=500ms
 * 6=1 sec,
 * 7=2 sec,
 * 8=4 sec,
 * 9= 8sec
 */
void setup_watchdog(int ii) {

  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}

/*
 * Watchdog Interrupt Service is executed when  watchdog timed out
 */
ISR(WDT_vect) {
  digitalWrite(STATUS_PIN_1, LOW);
}

volatile unsigned int lightMeter;

/**
 * This function is executed when there is a request to read sensor
 * To get data, 2 reads of 8 bits are required
 * First requests send 8 older bits of 16bit unsigned int
 * Second request send 8 lower bytes
 * Measurement is executed when request for first batch of data is requested
 */
void requestEvent()
{  
  
  if (reg_position == 0) {
    lightMeter = analogRead(ADC_PIN);
    i2c_regs[0] = lightMeter >> 8;
    i2c_regs[1] = lightMeter & 0xFF;
  }
  
  TinyWireS.send(i2c_regs[reg_position]);

  reg_position++;
  if (reg_position >= reg_size)
  {
      reg_position = 0;
  }
  digitalWrite(STATUS_PIN_1, HIGH);
}

void setup() {
  /*
   * Setup I2C
   */
  TinyWireS.begin(I2C_SLAVE_ADDRESS);
  TinyWireS.onRequest(requestEvent);

  /*
   * Set pins
   */
  pinMode(STATUS_PIN_1, OUTPUT);
  pinMode(ADC_PIN, INPUT);

  /*
   * Start watchdog timer
   */
  setup_watchdog(5);
}

void loop() {
  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
  sleep_enable();
  sleep_mode();
  sleep_disable(); 
}
