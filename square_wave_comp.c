/*
    Copyright (C) 2022 Roberto Cazzaro <https://github.com/robcazzaro>
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/
#include <stdint.h>
#include "boards.h"
#include "bsp.h"

#include "nrf_delay.h"
#include "app_error.h"

#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"

#include "nrf_drv_comp.h"
#include "nrfx_comp.h"

#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define PHASE1 200  // delay in clock cycles between timer start and first up transition for square wave
#define PHASE2 PHASE1 + 800   

static const nrf_drv_timer_t m_timer_square = NRF_DRV_TIMER_INSTANCE(3);

nrf_ppi_channel_t ppi_channel_0, ppi_channel_1, ppi_channel_2;

void timer_handler_square(nrf_timer_event_t event_type, void * p_context)
{
}

static void comp_event_handler(nrf_comp_event_t event)
{
}

void COMP_init(void)
{
    ret_code_t err_code;

    nrf_drv_comp_config_t comp_config = NRF_DRV_COMP_DEFAULT_CONFIG(NRF_COMP_INPUT_2);
    // Configure COMP
    comp_config.reference = NRF_COMP_REF_Int2V4;
    comp_config.main_mode = NRF_COMP_MAIN_MODE_SE;
    comp_config.speed_mode = NRF_COMP_SP_MODE_High;
    err_code = nrf_drv_comp_init(&comp_config, comp_event_handler);
    NRF_COMP->TASKS_START=1;
    while(NRF_COMP->EVENTS_READY==0);
}

void ppi_init()
{
    ret_code_t err_code;

    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_0);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_2);
    APP_ERROR_CHECK(err_code);

    uint32_t timer_square_compare_event_addr0  = nrf_drv_timer_event_address_get(&m_timer_square, NRF_TIMER_EVENT_COMPARE0);
    uint32_t timer_square_compare_event_addr1  = nrf_drv_timer_event_address_get(&m_timer_square, NRF_TIMER_EVENT_COMPARE1);

    uint32_t gpiote_task_addr7 = nrf_drv_gpiote_out_task_addr_get(ARDUINO_7_PIN);

    uint32_t comp_evt_addr_up               = (uint32_t)nrf_comp_event_address_get(NRF_COMP_EVENT_UP);
    uint32_t timer_count_capture_task_addr  = nrf_drv_timer_task_address_get(&m_timer_square, NRF_TIMER_TASK_CAPTURE5);

    err_code = nrf_drv_ppi_channel_assign(ppi_channel_0,
                                          timer_square_compare_event_addr0,
                                          gpiote_task_addr7); // Timer cc0 toggles square wave pin.
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(ppi_channel_1,
                                          timer_square_compare_event_addr1,
                                          gpiote_task_addr7); // Timer cc1 toggles square wave pin.
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(ppi_channel_2,
                                          comp_evt_addr_up,
                                          timer_count_capture_task_addr); // COMP event stores timer value in CC5
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_enable(ppi_channel_0);
    APP_ERROR_CHECK(err_code);  
    err_code = nrf_drv_ppi_channel_enable(ppi_channel_1);
    APP_ERROR_CHECK(err_code);    
    err_code = nrf_drv_ppi_channel_enable(ppi_channel_2);
    APP_ERROR_CHECK(err_code); 
}

void square_setup(void)
{
    ret_code_t err_code;

    // Configure TIMER3 
    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_cfg.frequency = NRF_TIMER_FREQ_16MHz;
    timer_cfg.mode = NRF_TIMER_MODE_TIMER;
    timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;

    err_code = nrf_drv_timer_init(&m_timer_square, &timer_cfg, timer_handler_square);
    APP_ERROR_CHECK(err_code);
    nrf_drv_timer_compare(&m_timer_square, NRF_TIMER_CC_CHANNEL0, PHASE1, false);
    nrf_drv_timer_compare(&m_timer_square, NRF_TIMER_CC_CHANNEL1, PHASE2, false);
    nrf_drv_timer_extended_compare(&m_timer_square, NRF_TIMER_CC_CHANNEL2, 1600, NRF_TIMER_SHORT_COMPARE2_CLEAR_MASK, false);

    // GPIOTE init
    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_out_config_t config = GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);
    err_code = nrf_drv_gpiote_out_init(ARDUINO_7_PIN, &config);
    APP_ERROR_CHECK(err_code);  
    nrf_drv_gpiote_out_task_enable(ARDUINO_7_PIN);
}

int main(void)
{
    uint32_t err_code;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();

    COMP_init();
    square_setup();
    ppi_init();
    nrf_drv_timer_enable(&m_timer_square);
   
    while (true)
    {
        while(NRF_LOG_PROCESS() == true);
        uint32_t counter5 = nrf_timer_cc_read(NRF_TIMER3, NRF_TIMER_CC_CHANNEL5) - PHASE1;
        uint32_t integer = (1.0 / 16.0) * (float)counter5;
        NRF_LOG_INFO("Timer = %d.%d us", integer, ((1.0 / 0.016) * (float)counter5) - (integer * 1000));
    }
}

/** @} */
