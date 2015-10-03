/* 
   
   blebot/main.c

   A phone controlled / autonomous robot.

   Author: Mahesh Venkitachalam
   Website: electronut.in


   Reference:

   http://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk51.v9.0.0%2Findex.html

 */

#include "ble_init.h"

extern ble_nus_t m_nus;                                  

// Create the instance "PWM1" using TIMER2.
APP_PWM_INSTANCE(PWM1,2);                   

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

void set_speed(int motor, uint8_t speed);
void turn(bool left, int tms);
void set_dir(bool forward);

// current motors speeds
int motor_speeds[] = {0, 0};
// current direction
int curr_dir = true;

// events
typedef enum _BBEventType {
  eBBEvent_Start,
  eBBEvent_Stop,
  eBBEvent_Reverse,
  eBBEvent_Left,
  eBBEvent_Right,
} BBEventType;

// structure handle pending events
typedef struct _BBEvent
{
  bool pending;
  BBEventType event;
  int data;
} BBEvent;

BBEvent bbEvent;

// handle event
void handle_bbevent(BBEvent* bbEvent)
{
  switch(bbEvent->event) {

  case eBBEvent_Start:
    {
      set_speed(0, bbEvent->data);
      set_speed(1, bbEvent->data);
    }
    break;

  case eBBEvent_Stop:
    {
      set_speed(0, 0);
      set_speed(1, 0);
    }
    break;

  case eBBEvent_Left:
    {
      turn(true, 500);
    }
    break;

  case eBBEvent_Right:
    {
      turn(false, 500);
    }
    break;

  case eBBEvent_Reverse:
    {
      set_dir(!curr_dir);
    }
    break;

  default:
    break;
  }

  // clear 
  bbEvent->pending = false;
}

// Function for handling the data from the Nordic UART Service.
static void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, 
                             uint16_t length)
{
  // clear events
  bbEvent.pending = false;

  if (strstr((char*)(p_data), REWIND)) {
    bbEvent.pending = true;
    bbEvent.event = eBBEvent_Left;
  }
  else if (strstr((char*)(p_data), FORWARD)) {
    bbEvent.pending = true;
    bbEvent.event = eBBEvent_Right;
  }
  else if (strstr((char*)(p_data), STOP)) {
    bbEvent.pending = true;
    bbEvent.event = eBBEvent_Stop;
  }
  else if (strstr((char*)(p_data), PLAY)) {
    bbEvent.pending = true;
    bbEvent.event = eBBEvent_Start;
    bbEvent.data = 80;
  }
  else if (strstr((char*)(p_data), SHUFFLE)) {
    bbEvent.pending = true;
    bbEvent.event = eBBEvent_Reverse;
  }
}


// A flag indicating PWM status.
static volatile bool pwmReady = false;            

// PWM callback function
void pwm_ready_callback(uint32_t pwm_id)    
{
    pwmReady = true;
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

/* brake: sudden stop of both motors */
void brake()
{
  // set direction A
  nrf_gpio_pin_set(pinIN1);
  nrf_gpio_pin_set(pinIN2);
    
  // set direction B
  nrf_gpio_pin_set(pinIN3);
  nrf_gpio_pin_set(pinIN4);
}

/* set_speed: set speed for motor 0/1 */
void set_speed(int motor, uint8_t speed)
{
  // error check
  if (motor < 0 || motor > 1)
    return;

  // set speed
  while (app_pwm_channel_duty_set(&PWM1, motor, speed) == NRF_ERROR_BUSY);
  motor_speeds[motor] = speed; 
}


/* turn: turn in given direction for x milliseconds */
void turn(bool left, int tms)
{
  if(left) {
    // stop motor 0
    int tmp = motor_speeds[0];
    set_speed(0, 0);
    set_speed(1, 50);
    // wait
    nrf_delay_ms(tms);
    // reset 
    set_speed(0, tmp);
    set_speed(1, tmp);
  }
  else {
    // stop motor 1
    int tmp = motor_speeds[1];
    set_speed(1, 0);
    set_speed(0, 50);
    // wait
    nrf_delay_ms(tms);
    // reset 
    set_speed(0, tmp);
    set_speed(1, tmp);
  }
}

/* direction: change motor direction */
void set_dir(bool forward)
{
  if(forward) {
    // set direction A
    nrf_gpio_pin_set(pinIN1);
    nrf_gpio_pin_clear(pinIN2);
    // set direction B
    nrf_gpio_pin_set(pinIN3);
    nrf_gpio_pin_clear(pinIN4);
  }
  else {
     // set direction A
    nrf_gpio_pin_clear(pinIN1);
    nrf_gpio_pin_set(pinIN2);
    // set direction B
    nrf_gpio_pin_clear(pinIN3);
    nrf_gpio_pin_set(pinIN4);
  }
  curr_dir = forward;
}

#define APP_TIMER_PRESCALER  0    /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS 6    /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE 4  /**< Size of timer operation queues. */

// Application main function.
int main(void)
{
    uint32_t err_code;

    // set up timers
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, 
                   APP_TIMER_OP_QUEUE_SIZE, false);
   
    // initlialize BLE
    ble_stack_init();
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();
    // start BLE advertizing
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
    
    // set direction
    set_dir(true);

    // 2-channel PWM
    app_pwm_config_t pwm1_cfg = 
      APP_PWM_DEFAULT_CONFIG_2CH(1000L, pinENA, pinENB);

    pwm1_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;
    pwm1_cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_HIGH;

    printf("before pwm init\n");
    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&PWM1,&pwm1_cfg,pwm_ready_callback);
    APP_ERROR_CHECK(err_code);
    printf("after pwm init\n");

    app_pwm_enable(&PWM1);

    // wait till PWM is ready
    //while(!pwmReady);

    // set motor speeds
    //set_speed(0, 80);
    //set_speed(1, 80);

    // set up LED
    uint32_t pinLED = 21;
    nrf_gpio_pin_dir_set(pinLED, NRF_GPIO_PIN_DIR_OUTPUT);

    printf("entering loop\n");
    while(1) {

      // execute command if any 
      if(bbEvent.pending) {
        handle_bbevent(&bbEvent);
      }

      // flash LED once
      nrf_gpio_pin_set(pinLED);
      nrf_delay_ms(200);
      nrf_gpio_pin_clear(pinLED);
      nrf_delay_ms(200);
    }
}
