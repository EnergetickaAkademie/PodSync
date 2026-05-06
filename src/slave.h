#pragma once
#include <Arduino.h>
#include "bitbus.h"

class BusSlave {
private:
	uint8_t local_id = 0;
	uint8_t device_type;
	uint32_t device_uid;
	uint32_t last_comm_ms = 0;
	void (*command_callback)(uint8_t cmd, const uint8_t* payload, uint8_t len) = nullptr;

	uint8_t readByte() {
		uint8_t b = 0;
		for (int8_t i = 7; i >= 0; i--) {
			while (digitalRead(CLK) == HIGH);
			while (digitalRead(CLK) == LOW);
			b |= (digitalRead(DAT) << i);
		}
		return b;
	}

	void sendByte(uint8_t b) {
		for (int8_t i = 7; i >= 0; i--) {
			while (digitalRead(CLK) == HIGH);
			if ((b >> i) & 1) pinMode(DAT, INPUT);
			else { pinMode(DAT, OUTPUT); digitalWrite(DAT, LOW); }
			while (digitalRead(CLK) == LOW);
		}
		pinMode(DAT, INPUT);
	}

	bool waitForBreak() {
		uint32_t waitStart = millis();
		while (digitalRead(CLK) == HIGH) {
			// Exit periodically to check timeouts
			if (millis() - waitStart > 100) return false; 
		}
		
		uint32_t lowStart = millis();
		while (digitalRead(CLK) == LOW);
		return (millis() - lowStart >= 3);
	}

	void respondPacket(uint8_t* payload, uint8_t len) {
		sendByte(len);
		for (uint8_t i = 0; i < len; i++) sendByte(payload[i]);
		
		uint8_t crc_data[18];
		crc_data[0] = len;
		if (len > 0 && payload != nullptr) memcpy(&crc_data[1], payload, len);
		sendByte(calculateCRC(crc_data, len + 1));
	}

public:
	BusSlave(uint8_t type, uint32_t uid) : device_type(type), device_uid(uid) {}

	void begin() {
		pinMode(CLK, INPUT);
		pinMode(DAT, INPUT);
		last_comm_ms = millis();
	}

	void setCommandCallback(void (*callback)(uint8_t cmd, const uint8_t* payload, uint8_t len)) {
		command_callback = callback;
	}

	uint8_t getDeviceType() const {
		return device_type;
	}

	void listen() {
		if (local_id != 0 && (millis() - last_comm_ms > TIMEOUT_DISCONNECT_MS)) {
			Serial.println("[SLAVE] Master lost. Resetting ID.");
			local_id = 0;
			last_comm_ms = millis(); 
		}

		if (waitForBreak()) {
			uint8_t header = readByte();
			uint8_t len = readByte();
			if (len > 16) return;

			uint8_t payload[16];
			for (uint8_t i = 0; i < len; i++) payload[i] = readByte();
			uint8_t crc = readByte();

			uint8_t crc_data[18];
			crc_data[0] = header;
			crc_data[1] = len;
			if (len > 0) memcpy(&crc_data[2], payload, len);

			if (calculateCRC(crc_data, len + 2) != crc) return;

			uint8_t target_id = header >> 4;
			uint8_t cmd = header & 0x0F;

			if (target_id == local_id || target_id == 0) {
				last_comm_ms = millis(); 

				if (cmd == CMD_DISCOVER && local_id == 0) {
					uint8_t resp[5];
					resp[0] = device_type;
					memcpy(&resp[1], &device_uid, 4);
					respondPacket(resp, 5);
				} 
				else if (cmd == CMD_ASSIGN_ID && local_id == 0) {
					uint32_t target_uid;
					memcpy(&target_uid, &payload[1], 4);
					if (target_uid == device_uid) {
						local_id = payload[0];
						Serial.print("[SLAVE] Assigned ID: ");
						Serial.println(local_id);
					}
				}
				else if (cmd == CMD_PING && target_id == local_id) {
					respondPacket(nullptr, 0);
				}
				else if (target_id == local_id) {
					if (command_callback != nullptr) {
						command_callback(cmd, payload, len);
					}
					// No response by default for application-level commands.
				}
			}
		}
	}
};