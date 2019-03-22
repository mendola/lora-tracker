/**
 * \file sio2host.c
 *
 * \brief Handles Serial I/O  Functionalities For the Host Device
 *
 * Copyright (c) 2013-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 */

/* === INCLUDES ============================================================ */

#include "asf.h"
#include "sio2host.h"
#include "conf_sio2host.h"

/* === TYPES =============================================================== */

/* === MACROS ============================================================== */

/* === PROTOTYPES ========================================================== */
void gps_usart_read_callback(struct usart_module *const usart_module);
void gps_usart_write_callback(struct usart_module *const usart_module);

/* === GLOBALS ========================================================== */
static struct usart_module usb_uart_module;
static struct usart_module gps_uart_module;

/**
 * Receive buffer
 * The buffer size is defined in sio2host.h
 */
static uint8_t usb_serial_rx_buf_[USB_SERIAL_RX_BUF_SIZE_HOST];
static uint8_t gps_serial_rx_buf_[GPS_SERIAL_RX_BUF_SIZE_HOST];
static bool gps_fresh_data_buffer = false;


/**
 * Receive buffer head
 */
static uint8_t usb_serial_rx_buf_head;
static uint8_t gps_serial_rx_buf_head;

/**
 * Receive buffer tail
 */
static uint8_t usb_serial_rx_buf_tail;
static uint8_t gps_serial_rx_buf_tail;

/**
 * Number of bytes in receive buffer
 */
static uint8_t usb_serial_rx_count;
static uint8_t gps_serial_rx_count;



/* === IMPLEMENTATION ====================================================== */
void uart_init(void) {
	uart_usb_init();
	uart_gps_init();
}

void uart_usb_init(void) {
	struct usart_config usb_uart_config;
	/* Configure USART for unit test output */
	usart_get_config_defaults(&usb_uart_config);
	usb_uart_config.mux_setting = USB_UART_SERCOM_MUX_SETTING;

	usb_uart_config.pinmux_pad0 = USB_UART_SERCOM_PINMUX_PAD0;
	usb_uart_config.pinmux_pad1 = USB_UART_SERCOM_PINMUX_PAD1;
	usb_uart_config.pinmux_pad2 = USB_UART_SERCOM_PINMUX_PAD2;
	usb_uart_config.pinmux_pad3 = USB_UART_SERCOM_PINMUX_PAD3;
	usb_uart_config.baudrate    = USB_UART_BAUDRATE;
	stdio_serial_init(&usb_uart_module, USB_UART_SERCOM, &usb_uart_config);
	usart_enable(&usb_uart_module);
	/* Enable transceivers */
	usart_enable_transceiver(&usb_uart_module, USART_TRANSCEIVER_TX);
	usart_enable_transceiver(&usb_uart_module, USART_TRANSCEIVER_RX);
	USB_UART_HOST_RX_ISR_ENABLE();
}

void uart_gps_init(void) {
	struct usart_config gps_uart_config;
	/* Configure USART for unit test output */
	usart_get_config_defaults(&gps_uart_config);
	gps_uart_config.mux_setting = GPS_UART_SERCOM_MUX_SETTING;

	gps_uart_config.pinmux_pad0 = GPS_UART_SERCOM_PINMUX_PAD0;
	gps_uart_config.pinmux_pad1 = GPS_UART_SERCOM_PINMUX_PAD1;
	gps_uart_config.pinmux_pad2 = GPS_UART_SERCOM_PINMUX_PAD2;
	gps_uart_config.pinmux_pad3 = GPS_UART_SERCOM_PINMUX_PAD3;
	gps_uart_config.baudrate    = GPS_UART_BAUDRATE;
	
	while (usart_init(&gps_uart_module, GPS_UART_SERCOM, &gps_uart_config) != STATUS_OK) {
	}
	
	usart_enable(&gps_uart_module);
	
	
	usart_register_callback(&gps_uart_module, gps_usart_write_callback, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_register_callback(&gps_uart_module, gps_usart_read_callback, USART_CALLBACK_BUFFER_RECEIVED);

	usart_enable_callback(&gps_uart_module, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_enable_callback(&gps_uart_module, USART_CALLBACK_BUFFER_RECEIVED);
		
	//stdio_serial_init(&gps_uart_module, GPS_UART_SERCOM, &gps_uart_config);
	//usart_enable(&gps_uart_module);
	/* Enable transceivers */
	//usart_enable_transceiver(&gps_uart_module, USART_TRANSCEIVER_TX);
	//usart_enable_transceiver(&gps_uart_module, USART_TRANSCEIVER_RX);
	// GPS_UART_HOST_RX_ISR_ENABLE();
}

void uart_usb_deinit(void) { //sio2host_deinit
	usart_disable(&usb_uart_module);

	/* Disable transceivers */
	usart_disable_transceiver(&usb_uart_module, USART_TRANSCEIVER_TX);
	usart_disable_transceiver(&usb_uart_module, USART_TRANSCEIVER_RX);
}

void uart_gps_deinit(void) { //sio2host_deinit
	usart_disable(&gps_uart_module);

	/* Disable transceivers */
	usart_disable_transceiver(&gps_uart_module, USART_TRANSCEIVER_TX);
	usart_disable_transceiver(&gps_uart_module, USART_TRANSCEIVER_RX);
}

void gps_uart_request_rx(void) {
	usart_read_buffer_job(&gps_uart_module, (uint8_t *)gps_serial_rx_buf_, GPS_SERIAL_RX_BUF_SIZE_HOST);
}

status_code_genare_t gps_uart_blocking_read(uint16_t *const rx_data) {
	return usart_read_wait(&gps_uart_module, rx_data);
}

void gps_uart_copy_data(uint8_t* destination, int8_t maxlen) {
	int8_t copycount;
	if (maxlen < USB_SERIAL_RX_BUF_SIZE_HOST){
		copycount = maxlen;
	} else {
		copycount = USB_SERIAL_RX_BUF_SIZE_HOST;
	}
	memcpy((void*)destination, (void*)gps_serial_rx_buf_, maxlen * sizeof(uint8_t));
}

uint8_t usb_uart_tx(uint8_t *data, uint8_t length) {
	status_code_genare_t status;
	do {
		status = usart_serial_write_packet(&usb_uart_module,
				(const uint8_t *)data, length);
	} while (status != STATUS_OK);
	return length;
}

uint8_t gps_uart_tx(uint8_t *data, uint8_t length) {
	status_code_genare_t status;
	do {
		status = usart_serial_write_packet(&gps_uart_module,
				(const uint8_t *)data, length);
	} while (status != STATUS_OK);
	return length;
}

uint8_t usb_uart_rx(uint8_t *data, uint8_t max_length) {
	uint8_t data_received = 0;
	if(usb_serial_rx_buf_tail >= usb_serial_rx_buf_head) {
		usb_serial_rx_count = usb_serial_rx_buf_tail - usb_serial_rx_buf_head;
	} else {
		usb_serial_rx_count = usb_serial_rx_buf_tail + (USB_SERIAL_RX_BUF_SIZE_HOST - usb_serial_rx_buf_head);
	}
	
	if (0 == usb_serial_rx_count) {
		return 0;
	}

	if (USB_SERIAL_RX_BUF_SIZE_HOST <= usb_serial_rx_count) {
		/*
		 * Bytes between head and tail are overwritten by new data.
		 * The oldest data in buffer is the one to which the tail is
		 * pointing. So reading operation should start from the tail.
		 */
		usb_serial_rx_buf_head = usb_serial_rx_buf_tail;

		/*
		 * This is a buffer overflow case. But still only the number of
		 * bytes equivalent to
		 * full buffer size are useful.
		 */
		usb_serial_rx_count = USB_SERIAL_RX_BUF_SIZE_HOST;

		/* Bytes received is more than or equal to buffer. */
		if (USB_SERIAL_RX_BUF_SIZE_HOST <= max_length) {
			/*
			 * Requested receive length (max_length) is more than
			 * the
			 * max size of receive buffer, but at max the full
			 * buffer can be read.
			 */
			max_length = USB_SERIAL_RX_BUF_SIZE_HOST;
		}
	} else {
		/* Bytes received is less than receive buffer maximum length. */
		if (max_length > usb_serial_rx_count) {
			/*
			 * Requested receive length (max_length) is more than
			 * the data
			 * present in receive buffer. Hence only the number of
			 * bytes
			 * present in receive buffer are read.
			 */
			max_length = usb_serial_rx_count;
		}
	}

	data_received = max_length;
	while (max_length > 0) {
		/* Start to copy from head. */
		*data = usb_serial_rx_buf_[usb_serial_rx_buf_head];
		data++;
		max_length--;
		if ((USB_SERIAL_RX_BUF_SIZE_HOST - 1) == usb_serial_rx_buf_head) {
			usb_serial_rx_buf_head = 0;
		} else {
			usb_serial_rx_buf_head++;
		}
	}
	return data_received;
}

uint8_t gps_uart_rx(uint8_t *data, uint8_t max_length) {
	uint8_t data_received = 0;
	if(gps_serial_rx_buf_tail >= gps_serial_rx_buf_head) {
		gps_serial_rx_count = gps_serial_rx_buf_tail - gps_serial_rx_buf_head;
	} else {
		gps_serial_rx_count = gps_serial_rx_buf_tail + (GPS_SERIAL_RX_BUF_SIZE_HOST - gps_serial_rx_buf_head);
	}
	
	if (0 == gps_serial_rx_count) {
		return 0;
	}

	if (GPS_SERIAL_RX_BUF_SIZE_HOST <= gps_serial_rx_count) {
		/*
		 * Bytes between head and tail are overwritten by new data.
		 * The oldest data in buffer is the one to which the tail is
		 * pointing. So reading operation should start from the tail.
		 */
		gps_serial_rx_buf_head = gps_serial_rx_buf_tail;

		/*
		 * This is a buffer overflow case. But still only the number of
		 * bytes equivalent to
		 * full buffer size are useful.
		 */
		gps_serial_rx_count = GPS_SERIAL_RX_BUF_SIZE_HOST;

		/* Bytes received is more than or equal to buffer. */
		if (GPS_SERIAL_RX_BUF_SIZE_HOST <= max_length) {
			/*
			 * Requested receive length (max_length) is more than
			 * the
			 * max size of receive buffer, but at max the full
			 * buffer can be read.
			 */
			max_length = GPS_SERIAL_RX_BUF_SIZE_HOST;
		}
	} else {
		/* Bytes received is less than receive buffer maximum length. */
		if (max_length > gps_serial_rx_count) {
			/*
			 * Requested receive length (max_length) is more than
			 * the data
			 * present in receive buffer. Hence only the number of
			 * bytes
			 * present in receive buffer are read.
			 */
			max_length = gps_serial_rx_count;
		}
	}

	data_received = max_length;
	while (max_length > 0) {
		/* Start to copy from head. */
		*data = gps_serial_rx_buf_[gps_serial_rx_buf_head];
		data++;
		max_length--;
		if ((GPS_SERIAL_RX_BUF_SIZE_HOST - 1) == gps_serial_rx_buf_head) {
			gps_serial_rx_buf_head = 0;
		} else {
			gps_serial_rx_buf_head++;
		}
	}
	return data_received;
}

uint8_t usb_uart_getchar(void) {
	uint8_t c;
	while (0 == usb_uart_rx(&c, 1)) {
	}
	return c;
}

uint8_t gps_uart_getchar(void) {
	uint8_t c;
	while (0 == gps_uart_rx(&c, 1)) {
	}
	return c;
}

void usb_uart_putchar(uint8_t ch) {
	usb_uart_tx(&ch, 1);
}

void gps_uart_putchar(uint8_t ch) {
	gps_uart_tx(&ch, 1);
}

int usb_uart_getchar_nowait(void) {
	uint8_t c;
	int back = usb_uart_rx(&c, 1);
	if (back >= 1) {
		return c;
	} else {
		return (-1);
	}
}

int gps_uart_getchar_nowait(void) {
	uint8_t c;
	int back = gps_uart_rx(&c, 1);
	if (back >= 1) {
		return c;
	} else {
		return (-1);
	}
}

void USB_USART_ISR(uint8_t instance) {
	uint8_t temp;
	usart_serial_read_packet(&usb_uart_module, &temp, 1);

	/* Introducing critical section to avoid buffer corruption. */
	cpu_irq_disable();

	/* The number of data in the receive buffer is incremented and the
	 * buffer is updated. */

	usb_serial_rx_buf_[usb_serial_rx_buf_tail] = temp;

	if ((USB_SERIAL_RX_BUF_SIZE_HOST - 1) == usb_serial_rx_buf_tail) {
		/* Reached the end of buffer, revert back to beginning of
		 * buffer. */
		usb_serial_rx_buf_tail = 0x00;
	} else {
		usb_serial_rx_buf_tail++;
	}

	cpu_irq_enable();
}

void GPS_USART_ISR(uint8_t instance) {  // Needs rewriting
	uint8_t temp;
	usart_serial_read_packet(&gps_uart_module, &temp, 1);

	/* Introducing critical section to avoid buffer corruption. */
	cpu_irq_disable();

	/* The number of data in the receive buffer is incremented and the
	 * buffer is updated. */

	gps_serial_rx_buf_[gps_serial_rx_buf_tail] = temp;

	if ((GPS_SERIAL_RX_BUF_SIZE_HOST - 1) == gps_serial_rx_buf_tail) {
		/* Reached the end of buffer, revert back to beginning of
		 * buffer. */
		gps_serial_rx_buf_tail = 0x00;
	} else {
		gps_serial_rx_buf_tail++;
	}

	cpu_irq_enable();
}

void usb_uart_disable(void) {
	usart_disable(&usb_uart_module); // was host_uart_module
}

void usb_uart_enable(void) {
	usart_enable(&usb_uart_module);
}

void gps_uart_disable(void) {
	usart_disable(&gps_uart_module); // was host_uart_module
}

void gps_uart_enable(void) {
	usart_enable(&gps_uart_module);
}

void gps_usart_read_callback(struct usart_module *const usart_module)
{
	//usart_write_buffer_job(&usart_instance, (uint8_t *)rx_buffer, MAX_RX_BUFFER_LENGTH);
	gps_fresh_data_buffer = true;
}

bool gpsUartHasData(void) {
	return gps_fresh_data_buffer;
}

void gps_usart_write_callback(struct usart_module *const usart_module)
{
}

/* EOF */
