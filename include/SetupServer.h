// SetupServer.h

#ifndef _SETUPSERVER_h
#define _SETUPSERVER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "FrameworkServer.h"

class SetupServer : public FrameworkServer {
private:
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
};

#endif

