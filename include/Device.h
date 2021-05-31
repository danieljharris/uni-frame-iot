// Device.h

#ifndef _DEVICE_h
#define _DEVICE_h

#include "arduino.h"

#include <PolledTimeout.h>

using esp8266::polledTimeout::oneShot;

class Device {
private:
	int timeoutSeconds = 12;
public:
	String id = "Unknown";
	String ip = "Unknown";
	String name = "Unknown";

	//Used to expire devices when they don't checkin
	//Each device must checkin once ever 12 seconds
	oneShot *timeout = new oneShot(0);

	Device() {
		*timeout = oneShot(1000 * timeoutSeconds);
	};
	Device(String id, String ip, String name) {
		this->id = id;
		this->ip = ip;
		this->name = name;
		*timeout = oneShot(1000 * timeoutSeconds);
	}

	//Used by unordered_set (clientLookup) to compare devices
	bool operator==(const Device& device) const {
		if (id == device.id) return true;
		else return false;
	}
};

#endif