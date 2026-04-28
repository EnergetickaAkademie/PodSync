#pragma once
#include <Arduino.h>

#define CLK PC1
#define DAT PC2
#define LED_PIN PC0

enum BusCommand {
	CMD_DISCOVER = 0x01,
	CMD_ASSIGN_ID = 0x02,
	CMD_POLL_DATA = 0x03,
	CMD_SET_STATE = 0x04,
	CMD_PING = 0x05,
	CMD_LED_BLINK = 0x06,
	CMD_RAPID_BLINK = 0x07
};

enum DeviceType {
	TYPE_UNKNOWN = 0,
	TYPE_NPP = 1,
	TYPE_GAS = 2,
	TYPE_BATTERY = 3,
	TYPE_COAL = 4,
	TYPE_WIND = 5,
	TYPE_HYDRO = 6,
	TYPE_HYDRO_PUMPED = 7
};

const int DELAY_BIT_US = 200;
const int DELAY_BREAK_MS = 5;
const int DELAY_PACKET_GAP_MS = 1;
const uint32_t TIMEOUT_DISCONNECT_MS = 3000;

inline uint8_t calculateCRC(const uint8_t* data, uint8_t len) {
	uint8_t crc = 0;
	for (uint8_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint8_t j = 0; j < 8; j++) {
			if (crc & 0x80) crc = (crc << 1) ^ 0x07;
			else crc <<= 1;
		}
	}
	return crc;
}