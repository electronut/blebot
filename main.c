/* 
   
   nRF51-RGB-L298/main.c

   Controlling motors using L298.
   
   Demonstrates PWM and NUS (Nordic UART Service).

   Author: Mahesh Venkitachalam
   Website: electronut.in


   Reference:

   http://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk51.v9.0.0%2Findex.html

 */

#include "ble_init.h"

extern ble_nus_t m_nus;                                  


// These are based on default values sent by Nordic nRFToolbox app
// Modify as neeeded
#define FORWARD "FastForward"
#define REWIND "Rewind"
#define STOP "Stop"
#define PAUSE "Pause"
#define PLAY "Play"
#define START "Start"
#define END "End"
#define RECORD "Rec"
#define SHUFFLE "Shuffle"

// flip directions
bool flipA = false;
bool flipB = false;

// min/max motos speeds - PWM duty cycle
const uint32_t speedMin = 90;
const uint32_t speedMax = 90;
// current motor speeds
uint32_t speedA = 90;
uint32_t speedB = 90;

bool stopMotors = false;

bool changed = false;

// Function for handling the data from the Nordic UART Service.
static void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, 
                             uint16_t length)
{
  if (strstr((char*)(p_data), RECORD)) {
    flipA = !flipA;
  }
  else if (strstr((char*)(p_data), SHUFFLE)) {
    flipB = !flipB;
  }
  else if (strstr((char*)(p_data), STOP)) {
    stopMotors = true;
  }
  else if (strstr((char*)(p_data), PLAY)) {
    stopMotors = false;
  }
  
  // set changed flag
  changed = true;
}


// A flag indicating PWM status.
static volatile bool ready_flag;            

// PWM callback function
void pwm_ready_callback(uint32_t pwm_id)    
{
    ready_flag = true;
}

// enable motor A - PWM for speed control
uint32_t pinENA = 1;
// direction control for motor A
uint32_t pinIN1 = 2;
uint32_t pinIN2 = 3;
// enable motor B - PWM for speed control
uint32_t pinENB = 4;
// direction control for motor B
uint32_t pinIN3 = 5;
uint32_t pinIN4 = 6;

// Function for initializing services that will be used by the application.
void services_init()
{
    uint32_t       err_code;
    ble_nus_init_t nus_init;
    
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;
    
    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}

// Application main function.
int main(void)
{
    uint32_t err_code;

    // set up timers
    APP_TIMER_INIT(0, 4, 4, false);

    // initlialize BLE
    ble_stack_init();
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();

    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);

    // init GPIOTE
    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    // init PPI
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    // intialize UART
    uart_init();

    // prints to serial port
    printf("starting...\n");

    // set up GPIOs
    nrf_gpio_cfg_output(pinIN1);
    nrf_gpio_cfg_output(pinIN2);
    nrf_gpio_cfg_output(pinIN3);
    nrf_gpio_cfg_output(pinIN4);
    
    // set direction A
    nrf_gpio_pin_set(pinIN1);
    nrf_gpio_pin_clear(pinIN2);
    
    // set direction B
    nrf_gpio_pin_set(pinIN3);
    nrf_gpio_pin_clear(pinIN4);


    // Create the instance "PWM1" using TIMER1.
    APP_PWM_INSTANCE(PWM1,1);                   
   
    // 2-channel PWM
    app_pwm_config_t pwm1_cfg = 
      APP_PWM_DEFAULT_CONFIG_2CH(1000L, pinENB, pinENA);

    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&PWM1,&pwm1_cfg,pwm_ready_callback);
    APP_ERROR_CHECK(err_code);
    app_pwm_enable(&PWM1);

    bool motorsStopped = false;
    stopMotors = true;
    changed = true;

    // set speed
    while (app_pwm_channel_duty_set(&PWM1, 0, speedA) == NRF_ERROR_BUSY);
    while (app_pwm_channel_duty_set(&PWM1, 1, speedB) == NRF_ERROR_BUSY);

    while(1) {

      // change things only if flags set
      if (changed) {

        // start/stop - but only when needed
        if(stopMotors) {
          if(!motorsStopped) {
            motorsStopped = true;

            app_pwm_disable(&PWM1);
            // This is required becauase app_pwm_disable()
            // has a bug. 
            // See: 
            // https://devzone.nordicsemi.com/question/41179/how-to-stop-pwm-and-set-pin-to-clear/
            nrf_drv_gpiote_out_task_disable(pinENA);
            nrf_gpio_cfg_output(pinENA);
            nrf_gpio_pin_clear(pinENA);
            nrf_drv_gpiote_out_task_disable(pinENB);
            nrf_gpio_cfg_output(pinENB);
            nrf_gpio_pin_clear(pinENB);
          }
        }
        else {
          if(motorsStopped) {
            motorsStopped = false;

            nrf_drv_gpiote_out_task_enable(pinENA);
            nrf_drv_gpiote_out_task_enable(pinENB);
            app_pwm_enable(&PWM1);
          }
        }

        // set speed
        while (app_pwm_channel_duty_set(&PWM1, 0, speedA) == NRF_ERROR_BUSY);
        while (app_pwm_channel_duty_set(&PWM1, 1, speedB) == NRF_ERROR_BUSY);
        
        if(flipA) {
          // set direction A
          nrf_gpio_pin_set(pinIN2);
          nrf_gpio_pin_clear(pinIN1);
        }
        else {
          // set direction A
          nrf_gpio_pin_set(pinIN1);
          nrf_gpio_pin_clear(pinIN2);
        }
        
        if(flipB) {
          // set direction A
          nrf_gpio_pin_set(pinIN3);
          nrf_gpio_pin_clear(pinIN4);
        }
        else {
          // set direction A
          nrf_gpio_pin_set(pinIN4);
          nrf_gpio_pin_clear(pinIN3);
        }

        // reset flag
        changed = false;
      }

      // delay
      nrf_delay_ms(100);
    }
}
