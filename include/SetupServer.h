// SetupServer.h

#ifndef _SETUPSERVER_h
#define _SETUPSERVER_h

#include "arduino.h"

#include "FrameworkServer.h"
#include <DNSServer.h>

class SetupServer : public FrameworkServer {
private:
	DNSServer dnsServer;

	const char* SETUP_SSID = getDeviceHostName();

	//Setup endpoint handleing
	std::function<void()> handleSetupConfig();
	std::function<void()> handleSetupConnect();

	//Setup endpoints
	std::vector<Endpoint> setupEndpoints{
		Endpoint("/", HTTP_GET, handleSetupConfig()),
		Endpoint("/connect", HTTP_ANY, handleSetupConnect())
	};

	void checkForMaster();

public:
	bool start();
	void update();
	void handle();
};

#endif