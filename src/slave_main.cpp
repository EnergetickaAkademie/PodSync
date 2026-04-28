#include <Arduino.h>
#include <PeripheralFactory.h>
#include "slave.h"

// TYPE_NPP, TYPE_GAS, TYPE_BATTERY, TYPE_COAL, TYPE_WIND, TYPE_HYDRO, TYPE_HYDRO_PUMPED
#ifndef DEVICE_UID
#define DEVICE_UID 0x00000004
#endif

BusSlave slave(TYPE_NPP, DEVICE_UID);
PeripheralFactory factory;
LED* led = nullptr;

const uint32_t BLINK_ON_MS = 120;
const uint32_t BLINK_OFF_MS = 120;
const uint32_t RAPID_BLINK_INTERVAL_MS = 80;

struct BlinkSequence {
	bool active = false;
	bool led_on = false;
	uint8_t remaining = 0;
	uint32_t last_toggle_ms = 0;
} sequence;

bool rapid_mode = false;

uint8_t getBlinkCount(uint8_t device_type) {
	switch (device_type) {
		case TYPE_NPP:          return 1;
		case TYPE_GAS:          return 2;
		case TYPE_BATTERY:      return 3;
		case TYPE_COAL:         return 4;
		case TYPE_WIND:         return 5;
		case TYPE_HYDRO:        return 6;
		case TYPE_HYDRO_PUMPED: return 7;
		default:                return 0;
	}
}

const char* getTypeName(uint8_t device_type) {
	switch (device_type) {
		case TYPE_NPP:          return "NPP";
		case TYPE_GAS:          return "GAS";
		case TYPE_BATTERY:      return "BATTERY";
		case TYPE_COAL:         return "COAL";
		case TYPE_WIND:         return "WIND";
		case TYPE_HYDRO:        return "HYDRO";
		case TYPE_HYDRO_PUMPED: return "HYDRO_PUMPED";
		default:                return "UNKNOWN";
	}
}

void startBlinkSequence(uint8_t count) {
	if (led == nullptr || count == 0) return;
	sequence.active = true;
	sequence.remaining = count;
	sequence.led_on = true;
	sequence.last_toggle_ms = millis();
	led->on();
}

void stopRapidMode() {
	if (rapid_mode && led != nullptr) {
		led->stopBlink();
		led->off();
	}
	rapid_mode = false;
}

void updateBlinkSequence() {
	if (!sequence.active || led == nullptr) return;
	uint32_t interval = sequence.led_on ? BLINK_ON_MS : BLINK_OFF_MS;
	uint32_t now = millis();
	if (now - sequence.last_toggle_ms < interval) return;

	sequence.last_toggle_ms = now;
	if (sequence.led_on) {
		led->off();
		sequence.led_on = false;
		if (sequence.remaining > 0) {
			sequence.remaining--;
		}
		if (sequence.remaining == 0) {
			sequence.active = false;
		}
	} else {
		led->on();
		sequence.led_on = true;
	}
}

void handleCommand(uint8_t cmd, const uint8_t* payload, uint8_t len) {
	(void)payload;
	(void)len;

	if (cmd == CMD_LED_BLINK) {
		stopRapidMode();
		sequence.active = false;
		uint8_t count = getBlinkCount(slave.getDeviceType());
		Serial.print("[SLAVE] LED_BLINK received, type: ");
		Serial.print(getTypeName(slave.getDeviceType()));
		Serial.print(", count: ");
		Serial.println(count);
		startBlinkSequence(count);
	}
	else if (cmd == CMD_RAPID_BLINK) {
		sequence.active = false;
		if (led != nullptr) {
			led->startBlink(RAPID_BLINK_INTERVAL_MS);
		}
		rapid_mode = true;
		Serial.println("[SLAVE] RAPID_BLINK received");
	}
}

void setup() {
	Serial.begin(115200);
	led = factory.createLed(LED_PIN);

	slave.begin();
	slave.setCommandCallback(handleCommand);

	Serial.println("\n=== Slave LED Blink Test Started ===");
	Serial.print("Type: ");
	Serial.println(getTypeName(slave.getDeviceType()));
	Serial.print("UID: 0x");
	Serial.println(DEVICE_UID, HEX);
	Serial.println("Waiting for LED_BLINK/RAPID_BLINK commands from master...");
}

void loop() {
	slave.listen();
	updateBlinkSequence();
	factory.update();
}