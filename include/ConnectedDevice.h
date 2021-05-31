// ConnectedDevice.h

#ifndef _CONNECTEDDEVICE_h
#define _CONNECTEDDEVICE_h

#include "arduino.h"

#include "ConnectedInfo.h"

class ConnectedDevice {
public:
	void save(String id, String method,  String path, String data);
	ConnectedInfo load();
};

#endif

