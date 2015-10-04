#include "distance.h"

const nrf_drv_timer_t TIMER_HCSR04 = NRF_DRV_TIMER_INSTANCE(1);

// counter
static volatile uint32_t tCount = 0;

// HC-SR04 Trigger pin
uint32_t pinTrig = 7;
// HC-SR04 Echo pin
uint32_t pinEcho = 8;

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

void timer_hcsr04_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    switch(event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
          tCount++;
          break;
        
        default:
          //Do nothing.
          break;
    }    
}

// set up and start Timer1
void init_dist_measurement(void)
{		
  nrf_gpio_pin_dir_set(pinTrig, NRF_GPIO_PIN_DIR_OUTPUT);
  nrf_gpio_pin_dir_set(pinEcho, NRF_GPIO_PIN_DIR_INPUT);

  uint32_t time_ticks;
  uint32_t err_code = NRF_SUCCESS;
    
  // Configure TIMER_HCSR04
  err_code = nrf_drv_timer_init(&TIMER_HCSR04, NULL, 
                                timer_hcsr04_event_handler);
  APP_ERROR_CHECK(err_code);
    
  time_ticks = 500;
  
  nrf_drv_timer_extended_compare(&TIMER_HCSR04, 
                                 NRF_TIMER_CC_CHANNEL0, 
                                 time_ticks, 
                                 NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

  nrf_drv_timer_enable(&TIMER_HCSR04);  
}





