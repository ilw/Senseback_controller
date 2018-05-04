//Include SDK files
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "sdk_common.h"
#include "app_uart.h"
#include "app_error.h"
#include "app_util.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_error.h"
#include "nrf_esb.h"
#include "nrf_esb_error_codes.h"
#include "nrf_drv_timer.h"
#include "bsp.h"
#include "boards.h"
#include "nrf_log.h"
#include "SEGGER_RTT.h"

//Define UART buffer sizes
#define UART_TX_BUF_SIZE 2048u      /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 2048u         /**< UART RX buffer size. */

//Declare ESB data variables
static volatile bool esb_xfer_done;
static unsigned char rx_msg;
// static unsigned char* uartrx;
// static unsigned int bytecount = 0;
static uint32_t errcode, payload_w_ptr = 0;
static volatile int rxpacketid = -5, uart_state = 0, tx_fail_count = 0;
static volatile int tx_fail_flag = 0, tx_burst_flag = 0, tx_success_flag = 0, readpackets_flag = 0, uart_flag = 0;
nrf_esb_payload_t rx_payload;
nrf_esb_payload_t tx_payload;
nrf_esb_payload_t dummy_payload = NRF_ESB_CREATE_PAYLOAD(0, 0x61);
nrf_esb_payload_t reset_payload = NRF_ESB_CREATE_PAYLOAD(0, 0x12, 0x35, 0x37);
// static char fail_string[17] = "Transfer Failed\r\n";
// static char success_string[21] = "Transfer Successful: ";
// static char received_string[19] = "Received Payload: ";
// static char newline_string[19] = "\r\n";


//Instantiate timer (for use during recording to repeatedly query the device)
const nrf_drv_timer_t TIMER_TX = NRF_DRV_TIMER_INSTANCE(0);

//Start high freq clocks
void clocks_start( void )
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;

    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

void timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    switch(event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0: {
					if (nrf_esb_is_idle()) {
						errcode = nrf_esb_write_payload(&dummy_payload);
					}

					break;
        }
        default:
            //Do nothing.
            break;
    }
}

//ESB event handler - checking to see packet has been sent successfully
void nrf_esb_event_handler(nrf_esb_evt_t const * p_event)
{
//	int i;
    switch (p_event->evt_id)
    {
				case NRF_ESB_EVENT_RX_RECEIVED: {
					readpackets_flag = 1;
          break;
				}
        case NRF_ESB_EVENT_TX_SUCCESS: {
					tx_success_flag = 1;
          break;
				}
        case NRF_ESB_EVENT_TX_FAILED: {
					tx_fail_flag = 1;
					tx_fail_count++;
					break;
				}
				default: {
					break;
				}

    }
}

//Initialize ESB parameters
uint32_t esb_init( void )
{
    uint32_t err_code;
    uint8_t base_addr_0[4] = {0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t base_addr_1[4] = {0xC2, 0xC2, 0xC2, 0xC2};
    uint8_t addr_prefix[8] = {0xE7, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8 };

		//Configuration params
    nrf_esb_config_t nrf_esb_config         = NRF_ESB_DEFAULT_CONFIG;
		nrf_esb_config.protocol                 = NRF_ESB_PROTOCOL_ESB_DPL;
    nrf_esb_config.payload_length           = tx_payload.length;
    nrf_esb_config.bitrate                  = NRF_ESB_BITRATE_2MBPS;
    nrf_esb_config.mode                     = NRF_ESB_MODE_PTX;			//TX mode
    nrf_esb_config.event_handler            = nrf_esb_event_handler;
		nrf_esb_config.tx_mode                  = NRF_ESB_TXMODE_AUTO;	//automatic send after write
    nrf_esb_config.selective_auto_ack       = false;
		nrf_esb_config.retransmit_count					= 5;
		//nrf_esb_config.retransmit_delay					= 500;

    err_code = nrf_esb_init(&nrf_esb_config);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_0(base_addr_0);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_base_address_1(base_addr_1);
    VERIFY_SUCCESS(err_code);

    err_code = nrf_esb_set_prefixes(addr_prefix, 8);
    VERIFY_SUCCESS(err_code);

    return err_code;
}

//UART event handler
void uart_event_handler(app_uart_evt_t * p_event)
{
	switch (p_event->evt_type) {
		case APP_UART_DATA_READY: { //We have received new data
			uart_flag = 1;
			break;
		}
		case APP_UART_COMMUNICATION_ERROR: {
			APP_ERROR_HANDLER(p_event->data.error_communication);
			break;
		}
		case APP_UART_FIFO_ERROR: {
			APP_ERROR_HANDLER(p_event->data.error_code);
			break;
		}
		default: {
			break;
		}
	}
}



int main(void)
{
	uint32_t time_ms = 100; //Timer interval (query rate during recording)
	uint32_t time_ticks;
	int8_t tmp;
	int i;
	//Can use LED command to indicate state of device
  //note to add this changes to custom_board.h file need to be made, see config in pca10040.h for guidence
	// LEDS_CONFIGURE(LEDS_MASK);
	// LEDS_OFF(LEDS_MASK);
	uint32_t err_code;

	err_code = nrf_drv_timer_init(&TIMER_TX, NULL, timer_event_handler);
    APP_ERROR_CHECK(err_code);
    time_ticks = nrf_drv_timer_ms_to_ticks(&TIMER_TX, time_ms);
    nrf_drv_timer_extended_compare(&TIMER_TX, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);


		//Initialize UART comms
		const app_uart_comm_params_t comm_params =
      {
          RX_PIN_NUMBER,
          TX_PIN_NUMBER,
          RTS_PIN_NUMBER,
          CTS_PIN_NUMBER,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
          UART_BAUDRATE_BAUDRATE_Baud1M
      };

		//Initialize UART FIFO
    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_event_handler,
                         APP_IRQ_PRIORITY_LOW,
                         err_code);
    APP_ERROR_CHECK(err_code);

		//Initialize NRF_LOG (for debug messaging)
    err_code = NRF_LOG_INIT();
    APP_ERROR_CHECK(err_code);

    clocks_start();
    err_code = esb_init(); //Initialize radio

		for (i=0;i<252;i++) {
			tx_payload.data[i] = 0;
		}
	while (true) {
		rxpacketid = -5;
		while (true) { //Verify connection loop
			errcode = nrf_esb_write_payload(&dummy_payload); //Send reset payload to RX
			while ((tx_fail_flag == 0) && (readpackets_flag == 0) && (tx_success_flag == 0)){

				app_uart_put(0x12);
				nrf_delay_ms(2000);

//				app_uart_put(0x3B);
//				nrf_delay_ms(2000);

//				app_uart_put(0x00);
//				nrf_delay_ms(2000);

//				app_uart_put(0x7F);
//				nrf_delay_ms(2000);
			};

			if (tx_fail_flag == 1) {
				tx_success_flag = 0;
				readpackets_flag = 0;
				tx_fail_flag = 0;
				app_uart_put(0x12);
				app_uart_put(0x3B);
				app_uart_put(0x00);
				app_uart_put(0x7F);
				nrf_delay_ms(2000);
			}
			else {
				tx_success_flag = 0;
				readpackets_flag = 0;
				tx_fail_flag = 0;
				nrf_esb_flush_rx();
				app_uart_put(0x12);
				app_uart_put(0x3B);
				app_uart_put(0x00);
				app_uart_put(0x71);
				nrf_delay_ms(100);
				break;
			}
		}
		while (true) { //RX Reset Loop
			errcode = nrf_esb_write_payload(&reset_payload); //Send reset payload to RX
			while ((tx_fail_flag == 0) && (readpackets_flag == 0) && (tx_success_flag == 0)){};
			if (tx_fail_flag == 1) {
				tx_success_flag = 0;
				readpackets_flag = 0;
				tx_fail_flag = 0;
				app_uart_put(0x12);
				app_uart_put(0x3B);
				app_uart_put(0x00);
				app_uart_put(0x7F);
				nrf_delay_ms(5000);
			}
			else {
				tx_success_flag = 0;
				tx_fail_flag = 0;
				readpackets_flag = 0;
				nrf_esb_flush_rx();
				app_uart_put(0x12);
				app_uart_put(0x3B);
				app_uart_put(0x00);
				app_uart_put(0x72);
				rxpacketid = -1;
				nrf_delay_ms(5000);
				break;
			}
		}

		nrf_drv_timer_enable(&TIMER_TX); //Start automatic RX query/heartbeat monitor.
		while ((tx_fail_flag == 0) && (readpackets_flag == 0)){};
		if (tx_fail_flag == 1) {
			app_uart_put(0x12);
			app_uart_put(0x3B);
			app_uart_put(0x00);
			app_uart_put(0x7F);
			nrf_drv_timer_disable(&TIMER_TX);
		}
		else {
			nrf_esb_read_rx_payload(&rx_payload);
			if (rx_payload.data[0] == 0) {
				rxpacketid = 0;
				app_uart_put(0x12);
				app_uart_put(0x3B);
				app_uart_put(0x00);
				app_uart_put(0x7C);
				break;
			}
			else {
				app_uart_put(0x12);
				app_uart_put(0x3B);
				app_uart_put(0x00);
				app_uart_put(0x7F);
				nrf_drv_timer_disable(&TIMER_TX);
				nrf_esb_flush_rx();
			}

		}
	}

	while (true) { //main loop
		if (readpackets_flag == 1) {
			while (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS) {

				if (rx_payload.length >= 250) { //If payload max length seen more data may be available from PRX: immediately send dummy command to get more data
					nrf_drv_timer_clear(&TIMER_TX);
					errcode = nrf_esb_write_payload(&dummy_payload);
					tx_payload.pid++;
					app_uart_put(0xFE);
					app_uart_put(0xED);
					app_uart_put(0xBE);
					app_uart_put(0xAD);
				}

				if (((rxpacketid < 255) && ((int)rx_payload.data[0] != rxpacketid+1)) || ((rxpacketid == 255) && ((int)rx_payload.data[0] != 0))) {
					tmp = (int8_t)(rxpacketid+1-(int)rx_payload.data[0]);
					app_uart_put(0xBA);
					app_uart_put(0xDF);
					app_uart_put(0x00);
					app_uart_put(tmp);
				}
				rxpacketid = rx_payload.data[0];
				if (rx_payload.length == 3 && rx_payload.data[1] == 0xF1 && rx_payload.data[2] == 0xF0) {
					//If receiving FIFO_EMPTY payload from RX, do not print to UART (do nothing)
					//app_uart_put(0xFF);
				}
				else {
					app_uart_put(0xDA);
					app_uart_put(0x7A);
					app_uart_put(rx_payload.length);
					for (i=0;i<rx_payload.length;i++) {app_uart_put(rx_payload.data[i]);}
					//for (i=0;i<2;i++)	{app_uart_put(newline_string[i]);}
				}
			}
			readpackets_flag = 0;
		}
		if (tx_fail_flag == 1) {
			//nrf_drv_timer_disable(&TIMER_TX);
			app_uart_put(0xDE);
			app_uart_put(0xAD);
			app_uart_put(0xBE);
			app_uart_put(0xEF);
			if (tx_fail_count >= 15) {
				nrf_esb_flush_tx();
				tx_fail_count = 0;
			}
			tx_fail_flag = 0;
		}
		if (uart_flag == 1) {
			while (app_uart_get(&rx_msg) == NRF_SUCCESS) {
				switch (uart_state) {
					case 0: {
						switch (rx_msg) {
							case 0x17: { //Command: start heartbeat timer for RX monitoring
								nrf_drv_timer_enable(&TIMER_TX);
								break;
							}
							case 0x23: { //Command: stop heartbeat timer for RX monitoring
								nrf_drv_timer_disable(&TIMER_TX);
								break;
							}
							case 0x85: { //Command: start feeding data for a new packet
								uart_state = 1;
								break;
							}
							case 0x90: {
								nrf_esb_pop_tx();
								break;
							}
							case 0xE1: {
								errcode = nrf_esb_flush_tx();
								break;
							}
							default: {
								break;
							}
						}
						break;
					}
					case 1: { //Receive packet length, determining number of bytes to place in packet
						tx_payload.length = rx_msg;
						uart_state = 2;
						break;
					}
					case 2: {
						tx_payload.data[payload_w_ptr] = rx_msg;
						payload_w_ptr++;
						if (payload_w_ptr == tx_payload.length) { //end of packet reached, send and return uart to default state
							uart_state = 0;
							nrf_drv_timer_clear(&TIMER_TX);
							nrf_esb_write_payload(&tx_payload);
							app_uart_put((uint8_t)payload_w_ptr);
							payload_w_ptr = 0;
							tx_payload.length = 0;
							tx_payload.pid++;

						}
						break;
					}
					default: {
						break;
					}
				}
			}
			uart_flag = 0;
		}
	}
}
