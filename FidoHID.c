/*
			 LUFA Library
	 Copyright (C) Dean Camera, 2021.

  dean [at] fourwalledcubicle [dot] com
		   www.lufa-lib.org
*/

/*
  Copyright 2021  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the GenericHID demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "FidoHID.h"
#include "ctap2hid_packet.h"
#include "ctap2hid_message.h"
#include "ctaphid.h"
#include "packet_queue.h"

typedef struct
{
	uint32_t channel_id;

} fido_state_t;

int ms_till_poll = 5;

void led_error(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
}

void led_ready(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_READY);
}

packet_queue_t in_queue;

packet_queue_t out_queue;

void write_packet(ctap2hid_packet_t *data)
{
	pq_push(&in_queue, *data);
}

void write_message(ctap2hid_message_t *message)
{
	write_message_packets(message, write_packet);
}

void handle_error(ctap2hid_packet_t *packet, uint8_t err)
{
	uint8_t payload[1] = {err};
	ctap2hid_message_t response = {
		.channel_id = packet->channel_id,
		.command_id = CTAPHID_ERROR,
		.payload_length = 1,
		.payload = payload,
	};
	write_message(&response);
}

void handle_ping(ctap2hid_message_t *message)
{
	ctap2hid_message_t response = {
		.channel_id = message->channel_id,
		.command_id = CTAPHID_PING,
		.payload_length = message->payload_length,
		// This payload will be freed by read_message.
		.payload = message->payload,
	};

	write_message(&response);
}

uint32_t next_channel_id = 1;

void handle_init(ctap2hid_message_t *message)
{
	uint64_t nonce = *(uint64_t *)(&message->payload[0]);

	uint8_t payload[17];
	memset(payload, 0, 17);

	// *((uint64_t *)(&payload[0])) = nonce;
	for (int i = 0; i < 8; i++)
	{
		payload[i] = nonce >> (8 * i);
	}

	*((uint32_t *)(&payload[8])) = next_channel_id;
	next_channel_id++;

	payload[12] = CTAPHID_PROTOCOL_VERSION;
	payload[13] = 0;
	payload[14] = 0;
	payload[15] = 1;
	payload[16] = CTAPHID_CAPABILITIES;

	ctap2hid_message_t response = {
		.channel_id = message->channel_id,
		.command_id = CTAPHID_INIT,
		.payload_length = 17,
		.payload = payload,
	};

	write_message(&response);
}

void handle_message(ctap2hid_message_t *message)
{
	LEDs_SetAllLEDs((LEDs_GetLEDs() + 1) & 0xf);

	switch (message->command_id)
	{
	case CTAPHID_PING:
		handle_ping(message);
		return;
	case CTAPHID_INIT:
		handle_init(message);
		return;
	}

	uint8_t payload[1] = {CTAPHID_ERR_INVALID_CMD};
	ctap2hid_message_t response = {
		.channel_id = message->channel_id,
		.command_id = CTAPHID_ERROR,
		.payload_length = 1,
		.payload = payload,
	};
	write_message(&response);
}

ctap2hid_packet_t *read_packet(uint8_t n)
{
	return pq_peek_n(&out_queue, n);
}

bool process_messages(void)
{
	while (!pq_is_empty(&out_queue) && !is_init_packet(pq_peek(&out_queue)))
	{
		pq_pop(&out_queue);
	}

	if (pq_is_empty(&out_queue))
		return false;

	ctap2hid_message_t message = {};
	bool err = false;
	uint8_t packet_count = read_message_packets(&message, &err, read_packet, handle_error);
	pq_pop_n(&out_queue, packet_count);

	if (!err)
	{
		handle_message(&message);
	}

	return true;
}

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	in_queue = pq_init();
	out_queue = pq_init();

	for (;;)
	{
		if (ms_till_poll == 0)
		{
			hid_poll_task();
			ms_till_poll = 5;
		}
		USB_USBTask();
		process_messages();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	// AVR8 Initialisation Code. LUFA-provided: IDK what it does.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= Endpoint_ConfigureEndpoint(FIDO_IN_EPADDR, EP_TYPE_INTERRUPT, FIDO_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(FIDO_OUT_EPADDR, EP_TYPE_INTERRUPT, FIDO_EPSIZE, 1);

	// Enables EVENT_USB_Device_StartOfFrame
	USB_Device_EnableSOFEvents();

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	// CTAP2HID has no traffic on control requests. May have to implement anyway to satisfy OS, but hopefully can be avoided.
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	// This event triggers once every millisecond. This allows us to implement polling intervals!
	ms_till_poll--;
}

void hid_poll_task(void)
{
	/* Device must be connected and configured for the task to run */
	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

	Endpoint_SelectEndpoint(FIDO_IN_EPADDR);

	if (!pq_is_empty(&in_queue) && Endpoint_IsINReady() && Endpoint_IsReadWriteAllowed())
	{
		ctap2hid_packet_t *packet = pq_peek(&in_queue);

		Endpoint_Write_Stream_LE(packet, FIDO_REPORT_SIZE, NULL);
		Endpoint_ClearIN();

		pq_pop(&in_queue);
	}

	Endpoint_SelectEndpoint(FIDO_OUT_EPADDR);

	if (!pq_is_full(&out_queue) && Endpoint_IsOUTReceived() && Endpoint_IsReadWriteAllowed())
	{
		ctap2hid_packet_t packet;

		Endpoint_Read_Stream_LE(&packet, FIDO_REPORT_SIZE, NULL);
		Endpoint_ClearOUT();

		pq_push(&out_queue, packet);
	}
}