// ClientServer.h

#ifndef _CLIENTSERVER_h
#define _CLIENTSERVER_h

#include "arduino.h"

#include "FrameworkServer.h"
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <TinyUPnP.h>

class ClientServer : public FrameworkServer {
private:
	// Client endpoint handleing
	std::function<void()> handleClientGetInfo();
	std::function<void()> handleClientPostDevice();
	std::function<void()> handleClientPostName();
	std::function<void()> handleClientPostWiFiCreds();
	std::function<void()> handleClientPostRestart();
	std::function<void()> handleClientPostNewMaster();

	// Handle configurable connected device
	std::function<void()> handleClientGetConnected();
	std::function<void()> handleClientPostConnected();

	// Client creation
	bool findAndConnectToMaster();
	bool findMaster();
	bool getAndSaveMainWiFiInfo();

	// Client to master transition handleing
	String masterIP = "";
	void checkinWithMaster();
	bool updateMasterIP();
	void electNewMaster();
	void becomeMaster(std::vector<String> clientIPs);

	// Power control
	void power_toggle();
	void power_on();
	void power_off();
	bool gpioPinState = false;

	// Light switch example
	// std::function<void()> handleClientPostLightSwitch();

protected:
	// Client endpoints
	std::vector<Endpoint> clientEndpoints{
		Endpoint(handleUnknownEndpoint()),
		
		Endpoint("/device", HTTP_GET, handleClientGetInfo()),
		Endpoint("/device", HTTP_POST, handleClientPostDevice()),
		Endpoint("/name", HTTP_POST, handleClientPostName()),
		Endpoint("/credentials", HTTP_POST, handleClientPostWiFiCreds()),
		Endpoint("/restart", HTTP_POST, handleClientPostRestart()),
		Endpoint("/master", HTTP_POST, handleClientPostNewMaster()),

		// Handle configurable connected device
		Endpoint("/connected", HTTP_GET, handleClientGetConnected()),
		Endpoint("/connected", HTTP_POST, handleClientPostConnected()),

		// Light switch example
		// Endpoint("/light/switch", HTTP_POST, handleClientPostLightSwitch()),
	};

	// General reusable functions for client & master servers
	void startServer(std::vector<Endpoint> endpoints, int port, String mdnsId);
	String getDeviceInfo();
	bool connectToWiFi(WiFiInfo info);
	void enableMDNS(String mdnsId);
	void enableOTA();
	void enableUPnP(int port, String mdnsId);
	TinyUPnP tinyUPnP = TinyUPnP(20000);

	// Cloud integration
	void selfRegister();
	void configUpdate();

	String getConnected();

	bool lastInputValue = false;
	void checkInputChange();

public:
	bool start();
	void update();
	void handle();
};

#endif

