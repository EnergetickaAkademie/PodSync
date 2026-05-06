#include <Arduino.h>
#include "master.h"

BusMaster master;

const uint32_t LED_BLINK_INTERVAL_MS = 5000;
const uint32_t RAPID_BLINK_INTERVAL_MS = 1000;
uint32_t last_blink_cmd_ms = 0;
bool rapid_mode = false;

void setup() {
	Serial.begin(115200);
	master.begin();
	last_blink_cmd_ms = millis();
}

void loop() {
	master.loop();
	
	uint8_t active = master.getActiveCount();
	bool should_rapid = (active >= 4);
	if (should_rapid != rapid_mode) {
		rapid_mode = should_rapid;
		Serial.print("\n[MASTER] Mode change: ");
		Serial.println(rapid_mode ? "RAPID_BLINK" : "LED_BLINK");
		last_blink_cmd_ms = 0;
	}

	uint32_t now = millis();
	uint32_t interval = rapid_mode ? RAPID_BLINK_INTERVAL_MS : LED_BLINK_INTERVAL_MS;
	if (now - last_blink_cmd_ms >= interval) {
		last_blink_cmd_ms = now;
		if (rapid_mode) {
			Serial.println("\n[MASTER] >>> Sending RAPID_BLINK to all slaves...");
			master.sendCommandToAll(CMD_RAPID_BLINK, nullptr, 0);
		} else {
			Serial.println("\n[MASTER] >>> Sending LED_BLINK to all slaves...");
			master.sendCommandToAll(CMD_LED_BLINK, nullptr, 0);
		}
	}
}