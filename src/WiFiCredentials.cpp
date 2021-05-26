//WiFiCredentials.cpp

#include "WiFiCredentials.h"
#include <EEPROM.h>

void WiFiCredentials::save(String hostname) {
	WiFiInfo oldInfo = load();
	String ssid = oldInfo.ssid;
	String password = oldInfo.password;
	save(ssid, password, hostname);
}

void WiFiCredentials::save(String ssid, String password) {
	WiFiInfo oldInfo = load();
	String hostname = oldInfo.hostname;
	save(ssid, password, hostname);
}

void WiFiCredentials::save(String ssid, String password, String hostname) {
	Serial.println("Entering saveWiFiCredentials");

	char ssidChr[32] = "";
	char passwordChr[32] = "";
	char hostnameChr[32] = "";

	ssid.toCharArray(ssidChr, sizeof(ssidChr) - 1);
	password.toCharArray(passwordChr, sizeof(passwordChr) - 1);
	hostname.toCharArray(hostnameChr, sizeof(hostnameChr) - 1);

	EEPROM.begin(512);
	EEPROM.put(0, ssidChr);
	EEPROM.put(0 + sizeof(ssidChr), passwordChr);
	EEPROM.put(0 + sizeof(ssidChr) + sizeof(passwordChr), hostnameChr);
	char ok[2 + 1] = "OK";
	EEPROM.put(0 + sizeof(ssidChr) + sizeof(passwordChr) + sizeof(hostnameChr), ok);

	EEPROM.commit();
	EEPROM.end();
}

WiFiInfo WiFiCredentials::load() {
	char ssidChr[32] = "";
	char passwordChr[32] = "";
	char hostnameChr[32] = "";

	EEPROM.begin(512);
	EEPROM.get(0, ssidChr);
	EEPROM.get(0 + sizeof(ssidChr), passwordChr);
	EEPROM.get(0 + sizeof(ssidChr) + sizeof(passwordChr), hostnameChr);
	char ok[2 + 1];
	EEPROM.get(0 + sizeof(ssidChr) + sizeof(passwordChr) + sizeof(hostnameChr), ok);
	EEPROM.end();

	if (String(ok) != String("OK")) {
		ssidChr[0] = 0;
		passwordChr[0] = 0;
		hostnameChr[0] = 0;
	}

	WiFiInfo info(ssidChr, passwordChr, hostnameChr);
	return info;
}
