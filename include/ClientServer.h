// ClientServer.h

#ifndef _CLIENTSERVER_h
#define _CLIENTSERVER_h

#include "arduino.h"

#include "FrameworkServer.h"
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <TinyUPnP.h>

class ClientServer : public FrameworkServer {
private:
	// Client endpoint handleing
	std::function<void()> handleClientGetInfo();
	std::function<void()> handleClientPostDevice();
	std::function<void()> handleClientPostName();
	std::function<void()> handleClientPostWiFiCreds();
	std::function<void()> handleClientPostRestart();
	std::function<void()> handleClientPostNewLeader();

	// Handle configurable connected device
	std::function<void()> handleClientGetConnected();
	std::function<void()> handleClientPostConnected();

	// Client creation
	bool findAndConnectToLeader();
	bool findLeader();
	bool getAndSaveMainWiFiInfo();

	// Client to leader transition handleing
	String leaderIP = "";
	void checkinWithLeader();
	bool updateLeaderIP();
	void electNewLeader();
	void becomeLeader(std::vector<String> clientIPs);

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
		
		Endpoint("/device", 	 HTTP_GET,  handleClientGetInfo()),
		Endpoint("/device", 	 HTTP_POST, handleClientPostDevice()),
		Endpoint("/name", 		 HTTP_POST, handleClientPostName()),
		Endpoint("/credentials", HTTP_POST, handleClientPostWiFiCreds()),
		Endpoint("/restart", 	 HTTP_POST, handleClientPostRestart()),
		Endpoint("/leader", 	 HTTP_POST, handleClientPostNewLeader()),

		// Handle configurable connected device
		Endpoint("/connected", HTTP_GET,  handleClientGetConnected()),
		Endpoint("/connected", HTTP_POST, handleClientPostConnected()),

		// Light switch example
		// Endpoint("/light/switch", HTTP_POST, handleClientPostLightSwitch()),
	};

	// General reusable functions for client & leader servers
	void startServer(std::vector<Endpoint> endpoints);
	String getDeviceInfo();
	bool connectToWiFi(WiFiInfo info);
	void enableMDNS();
	void enableOTA();
	void enableUPnP(int port, String hostname);
	TinyUPnP tinyUPnP = TinyUPnP(20000);

	// Cloud integration
	bool selfRegister();
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

