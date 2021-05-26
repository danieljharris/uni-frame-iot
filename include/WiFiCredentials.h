// WiFiCredentials.h

#ifndef _WIFICREDENTIALS_h
#define _WIFICREDENTIALS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "WiFiInfo.h"

class WiFiCredentials {
public:
	void save(String hostname);
	void save(String ssid, String password);
	void save(String ssid, String password, String hostname);
	WiFiInfo load();
};

#endif

