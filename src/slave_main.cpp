#include <Arduino.h>
#include "slave.h"

BusSlave slave(TYPE_PVE, 0xAABBCCDD); 

void setup() {
	Serial.begin(115200);
	slave.begin();
	Serial.println("Slave started");
}

void loop() {
	slave.listen();
}