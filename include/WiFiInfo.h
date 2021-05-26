// WiFiInfo.h

#ifndef _WIFIINFO_h
#define _WIFIINFO_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class WiFiInfo {
public:
	char ssid[32] = { '\0' };
	char password[32] = { '\0' };
	char hostname[32] = { 'U','n','k','n','o','w','n','\0' };

	WiFiInfo() {};
	WiFiInfo(char ssid[32], char password[32], char hostname[32]) {
		strcpy(this->ssid, ssid);
		strcpy(this->password, password);
		strcpy(this->hostname, hostname);
	}
};

#endif