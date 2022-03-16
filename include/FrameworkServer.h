// FrameworkServer.h

#ifndef _FRAMEWORKSERVER_h
#define _FRAMEWORKSERVER_h

#include "arduino.h"

#include "WiFiCredentials.h"
#include "ConnectedDevice.h"
#include "Config.h"
#include "Endpoint.h"

#include <ESP8266WebServerSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

class FrameworkServer {
protected:
	// ESP8266WebServerSecure server;
	ESP8266WebServer server;
	WiFiCredentials creds;
	ConnectedDevice connected;

	void Response(t_http_codes code);
	void Response(t_http_codes code, JsonObject& message);
	void Response(t_http_codes code, String message);
	void ErrorResponse(t_http_codes code);
	void ErrorResponse(t_http_codes code, String message);

	char* getDeviceHostName();
	std::function<void()> handleUnknownEndpoint();
	void addEndpoints(std::vector<Endpoint> endpoints);
	void enableSSL();

public:
	virtual bool start() {};
	virtual void update() {};
	virtual void handle() { server.handleClient(); };
	~FrameworkServer();
	void stop();
};

#endif