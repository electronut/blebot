#include "nordic_common.h"
#include "nrf.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf51_bitfields.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "app_pwm.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"

bool getDistance(float* dist);
void init_dist_measurement(void);


