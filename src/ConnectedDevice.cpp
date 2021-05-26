// ConnectedDevice.cpp

#include "ConnectedDevice.h"
#include <EEPROM.h>

void ConnectedDevice::save(String id, String method, String path, String data) {
	Serial.println("Entering saveConnectedDevice");

	char idChr[32] = "";
	char methodChr[32] = "";
	char pathChr[32] = "";
	char dataChr[32] = "";

	id.toCharArray(idChr, sizeof(idChr) - 1);
	method.toCharArray(methodChr, sizeof(methodChr) - 1);
	path.toCharArray(pathChr, sizeof(pathChr) - 1);
	data.toCharArray(dataChr, sizeof(dataChr) - 1);

	unsigned int offset = 32U + 32U + 32U + 3U;

	EEPROM.begin(512);
	EEPROM.put(offset, idChr);
	EEPROM.put(offset + sizeof(idChr), methodChr);
	EEPROM.put(offset + sizeof(idChr) + sizeof(methodChr), pathChr);
	EEPROM.put(offset + sizeof(idChr) + sizeof(methodChr) + sizeof(pathChr), dataChr);
	char ok[2 + 1] = "OK";
	EEPROM.put(offset + sizeof(idChr) + sizeof(methodChr) + sizeof(pathChr) + sizeof(dataChr), ok);

	EEPROM.commit();
	EEPROM.end();
}

ConnectedInfo ConnectedDevice::load() {
	char idChr[32] = "";
	char methodChr[32] = "";
	char pathChr[32] = "";
	char dataChr[32] = "";

	unsigned int offset = 32U + 32U + 32U + 3U;

	EEPROM.begin(512);
	EEPROM.get(offset, idChr);
	EEPROM.get(offset + sizeof(idChr), methodChr);
	EEPROM.get(offset + sizeof(idChr) + sizeof(methodChr), pathChr);
	EEPROM.get(offset + sizeof(idChr) + sizeof(methodChr) + sizeof(pathChr), dataChr);
	char ok[2 + 1];
	EEPROM.get(offset + sizeof(idChr) + sizeof(methodChr) + sizeof(pathChr) + sizeof(dataChr), ok);
	EEPROM.end();

	if (String(ok) != String("OK")) {
		idChr[0] = 0;
		methodChr[0] = 0;
		pathChr[0] = 0;
		dataChr[0] = 0;
	}

	ConnectedInfo info(idChr, methodChr, pathChr, dataChr);
	return info;
}
