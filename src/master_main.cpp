#include <Arduino.h>
#include "master.h"

BusMaster master;

void setup() {
	Serial.begin(115200);
	master.begin();
	Serial.println("Master started");
}

void loop() {
	master.loop();
}