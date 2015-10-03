#include "distance.h"

// counter
static volatile uint32_t tCount = 0;

// HC-SR04 Trigger - P0.01
uint32_t pinTrig = 1;
// HC-SR04 Echo - P0.02
uint32_t pinEcho = 2;

// count to us (micro seconds) conversion factor
// set in start_timer()
static volatile float countToUs = 1;

// get distance measurement from HC-SR04:
// Send a 10us HIGH pulse on the Trigger pin.
// The sensor sends out a “sonic burst” of 8 cycles.
// Listen to the Echo pin, and the duration of the next HIGH 
// signal will give you the time taken by the sound to go back 
// and forth from sensor to target.
// returns true only if a valid distance is obtained
bool getDistance(float* dist)
{
  // send 12us trigger pulse
  //    _
  // __| |__
  nrf_gpio_pin_clear(pinTrig);
  nrf_delay_us(20);
  nrf_gpio_pin_set(pinTrig);
  nrf_delay_us(12);
  nrf_gpio_pin_clear(pinTrig);
  nrf_delay_us(20);

  // listen for echo and time it
  //       ____________
  // _____|            |___
  
  // wait till Echo pin goes high
  while(!nrf_gpio_pin_read(pinEcho));
  // reset counter
  tCount = 0;
  // wait till Echo pin goes low
  while(nrf_gpio_pin_read(pinEcho));
  
  // calculate duration in us
  float duration = countToUs*tCount;
 
  // dist = duration * speed of sound * 1/2
  // dist in cm = duration in us * 10^-6 * 340.29 * 100 * 1/2
  float distance = duration*0.017;
  
  // check value
  if(distance < 400.0) {

    // save
    *dist = distance;

    return true;
  }
  else {
    return false;
  }
}

// set up and start Timer1
void init_dist_measurement(void)
{		
  nrf_gpio_pin_dir_set(pinTrig, NRF_GPIO_PIN_DIR_OUTPUT);
  nrf_gpio_pin_dir_set(pinEcho, NRF_GPIO_PIN_DIR_INPUT);

  NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;  
  NRF_TIMER1->TASKS_CLEAR = 1;
  // set prescalar n
  // f = 16 MHz / 2^(n)
  uint8_t prescaler = 0;
	NRF_TIMER1->PRESCALER = prescaler; 
	NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_16Bit;

  // 16 MHz clock generates timer tick every 1/(16000000) s = 62.5 nano s
  // With compare enabled, the interrupt is fired every: 62.5 * comp1 nano s
  // = 0.0625*comp1 micro seconds
  // multiply this by 2^(prescalar)

  uint16_t comp1 = 500;
  // set compare
	NRF_TIMER1->CC[1] = comp1;

  // set conversion factor
  countToUs = 0.0625*comp1*(1 << prescaler);

  printf("timer tick = %f us\n", countToUs);

  // enable compare 1
	NRF_TIMER1->INTENSET = 
    (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);

  // use the shorts register to clear compare 1
  NRF_TIMER1->SHORTS = (TIMER_SHORTS_COMPARE1_CLEAR_Enabled << 
                        TIMER_SHORTS_COMPARE1_CLEAR_Pos);

  // enable IRQ
  NVIC_EnableIRQ(TIMER1_IRQn);
		
  // start timer
  NRF_TIMER1->TASKS_START = 1;
}

// Timer 1 IRQ handler
// just increment count
void TIMER1_IRQHandler_dummy(void)
{
	if (NRF_TIMER1->EVENTS_COMPARE[1] && 
      NRF_TIMER1->INTENSET & TIMER_INTENSET_COMPARE1_Msk) {

    // clear compare register event	
    NRF_TIMER1->EVENTS_COMPARE[1] = 0;

    // increment count
    tCount++;
  }
}
