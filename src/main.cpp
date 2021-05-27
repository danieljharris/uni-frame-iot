// main.cpp

#include "ConnectedDevice.h"
#include "FrameworkServer.h"
#include "Setupserver.h"
#include "ClientServer.h"
#include "MasterServer.h"
#include "Config.h"

#include <ArduinoOTA.h>
#include <PolledTimeout.h>

FrameworkServer* server;

//The frequency to call server update (1000 = 1 second)
esp8266::polledTimeout::periodic period(1000 * 10);

void setup() {
	Serial.begin(115200);
	Serial.println("\nStarting ESP8266...");
	
	pinMode(GPIO_OUTPUT_PIN, OUTPUT);
	digitalWrite(GPIO_OUTPUT_PIN, HIGH);
	pinMode(GPIO_INPUT_PIN, INPUT_PULLUP);

	server = new ClientServer();
	if (server->start()) return;

	server = new MasterServer();
	if (server->start()) return;

	server = new SetupServer();
	if (server->start()) return;

	Serial.println("\nFailed to become client, master, and setup. Restarting...");
	ESP.restart();
}

void loop() {
	server->handle();
	ArduinoOTA.handle();

	if (period) server->update();
}