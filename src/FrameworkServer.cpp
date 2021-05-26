//FrameworkServer.cpp

#include "FrameworkServer.h"

FrameworkServer::~FrameworkServer() { stop(); }

void FrameworkServer::stop() {
	Serial.println("Entering stop");

	server.close();
	server.stop();
}

char* FrameworkServer::getDeviceHostName() {
	char* newName = "ESP_";
	char hostName[10];
	sprintf(hostName, "%d", ESP.getChipId());
	strcat(newName, hostName);

	return newName;
}

void FrameworkServer::addUnknownEndpoint() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientUnknown");

		Serial.println("");
		Serial.print("Unknown command: ");
		Serial.println(server.uri());

		Serial.print("Method: ");
		Serial.println(server.method());

		server.send(HTTP_CODE_NOT_FOUND);
	};
	server.onNotFound(lambda);
}

void FrameworkServer::addEndpoints(std::vector<Endpoint> endpoints) {
	for (std::vector<Endpoint>::iterator it = endpoints.begin(); it != endpoints.end(); ++it) {
		server.on(it->path, it->method, it->function);
	}
}