// ConnectedInfo.h

#ifndef _CONNECTEDINFO_h
#define _CONNECTEDINFO_h

#include "arduino.h"

class ConnectedInfo {
public:
	char id[32]     = { '\0' };
	char method[32] = { '\0' };
	char path[32]   = { '\0' };
	char data[32]   = { '\0' };

	ConnectedInfo() {};
	ConnectedInfo(char id[32], char method[32], char path[32], char data[32]) {
		strcpy(this->id, id);
		strcpy(this->method, method);
		strcpy(this->path, path);
		strcpy(this->data, data);
	}
};

#endif