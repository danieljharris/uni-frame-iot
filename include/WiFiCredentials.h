// WiFiCredentials.h

#ifndef _WIFICREDENTIALS_h
#define _WIFICREDENTIALS_h

#include "arduino.h"

#include "WiFiInfo.h"

class WiFiCredentials {
public:
	void save(String hostname);
	void save(String ssid, String password);
	void save(String ssid, String password, String hostname);
	WiFiInfo load();
};

#endif

