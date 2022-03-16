// LeaderServer.h

#ifndef _LEADERSERVER_h
#define _LEADERSERVER_h

#include "arduino.h"

#include "ClientServer.h"
#include "Device.h"

#include <ESP8266HTTPClient.h>
#include <unordered_set>

// Used by unordered_set (clientLookup) to make a hash of each device
namespace std {
	template<> struct hash<Device> {
		size_t operator()(const Device& device) const {
			return hash<int>()(device.id.toInt());
		}
	};
};

class LeaderServer : public ClientServer {
private:
	// Leader endpoint handleing
	std::function<void()> handleUnknownEndpoint();
	std::function<void()> handleLeaderGetWiFiInfo();
	std::function<void()> handleLeaderGetDevices();
	std::function<void()> handleLeaderPostCheckin();

	// Leader endpoints
	std::vector<Endpoint> leaderEndpoints{
		Endpoint(handleUnknownEndpoint()),

		Endpoint("/wifi_info", HTTP_GET,  handleLeaderGetWiFiInfo()),
		Endpoint("/devices",   HTTP_GET,  handleLeaderGetDevices()),
		Endpoint("/checkin",   HTTP_POST, handleLeaderPostCheckin()),

		// Light switch example
		// Endpoint("/light/switch", HTTP_POST, handleLeaderPostLightSwitch())
	};

	// Leader creation
	void checkForOtherLeaders();
	void closeOtherLeaders(std::vector<String> clientIPs);

	// REST request routing
	std::unordered_set<Device> clientLookup;
	String getDeviceIPFromIdOrName(String idOrName);
	void reDirect();
	// String reDirect(String ip);
	void expireClientLookup();

	// Helper functions for REST routing
	const char* getMethod();
	bool isForMe();
	bool validIdOrName();

	// Light switch example
	// std::function<void()> handleLeaderPostLightSwitch();

	bool lastInputValue = false;
	void checkInputChange();

public:
	bool start();
	void update();
	void handle();
};

#endif

