#pragma once
#include <Arduino.h>
#include "bitbus.h"

struct SlaveDevice {
	uint8_t id;
	uint8_t type;
	uint32_t uid;
	bool active;
	uint32_t last_seen_ms;
};

class BusMaster {
private:
	SlaveDevice slaves[12];

	void sendByte(uint8_t b) {
		for (int8_t i = 7; i >= 0; i--) {
			digitalWrite(CLK, LOW);
			if ((b >> i) & 1) pinMode(DAT, INPUT);
			else { pinMode(DAT, OUTPUT); digitalWrite(DAT, LOW); }
			delayMicroseconds(DELAY_BIT_US);
			digitalWrite(CLK, HIGH);
			delayMicroseconds(DELAY_BIT_US);
		}
		pinMode(DAT, INPUT);
	}

	uint8_t readByte() {
		uint8_t b = 0;
		for (int8_t i = 7; i >= 0; i--) {
			digitalWrite(CLK, LOW);
			delayMicroseconds(DELAY_BIT_US);
			b |= (digitalRead(DAT) << i);
			digitalWrite(CLK, HIGH);
			delayMicroseconds(DELAY_BIT_US);
		}
		return b;
	}

	bool sendPacket(uint8_t target_id, uint8_t cmd, uint8_t* payload, uint8_t len) {
		digitalWrite(CLK, LOW);
		delay(DELAY_BREAK_MS);
		digitalWrite(CLK, HIGH);
		delay(DELAY_PACKET_GAP_MS);

		uint8_t header = (target_id << 4) | (cmd & 0x0F);
		sendByte(header);
		sendByte(len);
		for (uint8_t i = 0; i < len; i++) sendByte(payload[i]);

		uint8_t crc_data[18];
		crc_data[0] = header;
		crc_data[1] = len;
		if (len > 0 && payload != nullptr) memcpy(&crc_data[2], payload, len);
		sendByte(calculateCRC(crc_data, len + 2));

		return true;
	}

	bool readPacket(uint8_t* buffer, uint8_t* len) {
		delay(DELAY_PACKET_GAP_MS);
		*len = readByte();
		if (*len > 16) return false;

		for (uint8_t i = 0; i < *len; i++) buffer[i] = readByte();
		uint8_t crc = readByte();

		uint8_t crc_data[18];
		crc_data[0] = *len;
		if (*len > 0) memcpy(&crc_data[1], buffer, *len);
		
		return calculateCRC(crc_data, *len + 1) == crc;
	}

	void pollDiscovery() {
		sendPacket(0, CMD_DISCOVER, nullptr, 0);
		
		uint8_t buf[16];
		uint8_t len = 0;
		
		if (readPacket(buf, &len) && len >= 5) {
			delay(5); 
			uint8_t type = buf[0];
			uint32_t uid;
			memcpy(&uid, &buf[1], 4);
			
			uint8_t target_id = 0;
			
			for (int i = 0; i < 12; i++) {
				if (slaves[i].active && slaves[i].uid == uid) {
					target_id = slaves[i].id;
					break;
				}
			}
			
			if (target_id == 0) {
				for (int i = 0; i < 12; i++) {
					if (!slaves[i].active) {
						target_id = slaves[i].id;
						slaves[i].active = true;
						slaves[i].type = type;
						slaves[i].uid = uid;
						slaves[i].last_seen_ms = millis();
						break;
					}
				}
			}
			
			if (target_id != 0) {
				uint8_t payload[5];
				payload[0] = target_id;
				memcpy(&payload[1], &uid, 4);
				
				sendPacket(0, CMD_ASSIGN_ID, payload, 5);
				
				Serial.print("[MASTER] Assigned ID ");
				Serial.print(target_id);
				Serial.print(" to Type ");
				Serial.print(type);
				Serial.print(" (UID: 0x");
				Serial.print(uid, HEX);
				Serial.println(")");
			}
		}
	}

	void pingDevices() {
		uint32_t now = millis();
		for (int i = 0; i < 12; i++) {
			if (slaves[i].active) {
				sendPacket(slaves[i].id, CMD_PING, nullptr, 0);
				
				uint8_t buf[16];
				uint8_t len = 0;
				
				if (readPacket(buf, &len)) {
					slaves[i].last_seen_ms = now;
				} else if (now - slaves[i].last_seen_ms > TIMEOUT_DISCONNECT_MS) {
					slaves[i].active = false;
					Serial.print("[MASTER] Device disconnected: ID ");
					Serial.println(slaves[i].id);
				}
			}
		}
	}

public:
	void begin() {
		pinMode(CLK, OUTPUT);
		digitalWrite(CLK, HIGH);
		pinMode(DAT, INPUT);
		for (int i = 0; i < 12; i++) {
			slaves[i].active = false;
			slaves[i].id = i + 1;
		}
	}

	void loop() {
		pollDiscovery();
		delay(50);
		pingDevices();
		delay(50);
	}
};