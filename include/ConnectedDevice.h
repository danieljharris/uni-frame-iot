// ConnectedDevice.h

#ifndef _CONNECTEDDEVICE_h
#define _CONNECTEDDEVICE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "ConnectedInfo.h"

class ConnectedDevice {
public:
	void save(String id, String method,  String path, String data);
	ConnectedInfo load();
};

#endif

