//ClientServer.cpp

#include "ClientServer.h"

bool ClientServer::start() {
	Serial.println("Entering startClient");

	WiFi.mode(WIFI_STA);

	if (!findAndConnectToMaster()) {
		Serial.println("Failed to become client");
		return false;
	}

	Serial.println("I am a client!");

	Serial.println("Letting master know I exist...");
	checkinWithMaster();

	startServer(clientEndpoints, CLIENT_PORT, CLIENT_MDNS_ID);
	Serial.println("Ready!");

	return true;
}

void ClientServer::startServer(std::vector<Endpoint> endpoints, int port, String mdnsId) {
	Serial.println("Adding endpoints...");
	addEndpoints(endpoints);

	Serial.println("Enabeling SSL encryption...");
	enableSSL();

	Serial.println("Starting server...");
	server.begin(port);

	Serial.println("Starting MDNS...");
	enableMDNS(mdnsId);

	Serial.println("Enableing OTA updates...");
	enableOTA();

	Serial.println("Configuring UPnP...");
	enableUPnP(port, mdnsId);

	Serial.println("Registering on cloud...");
	selfRegister();

	Serial.println("Updating config from cloud...");
	configUpdate();
}

void ClientServer::update() { checkinWithMaster(); }

//Client endpoints
std::function<void()> ClientServer::handleClientGetInfo() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientGetInfo");
		server.send(HTTP_CODE_OK, "application/json", getDeviceInfo());
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostDevice() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostDevice");

		if (!server.hasArg("plain")) { server.send(HTTP_CODE_BAD_REQUEST); return; }

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));

		if (!json.success()) { server.send(HTTP_CODE_BAD_REQUEST); return; }
		if (!json.containsKey("action")) {
			server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"action_field_missing\"}");
			return;
		}

		bool lastState = gpioPinState;

		if (json["action"] == "toggle") {
			power_toggle();
		}
		else if (json["action"] == "pulse") {
			if (!json.containsKey("direction")) {
				server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"direction_field_missing\"}");
				return;
			}
			else if (json["direction"] == "off_on_off") {
				power_on();
				delay(1000);
				power_off();
			}
			else if (json["direction"] == "on_off_on") {
				power_off();
				delay(1000);
				power_on();
			}
			else {
				server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"direction_field_invalid\"}");
				return;
			}
		}
		else if (json["action"] == "set") {
			if (!json.containsKey("power")) {
				server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"power_field_missing\"}");
				return;
			}
			else if (!json["power"].is<bool>()) {
				server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"power_field_invalid\"}");
				return;
			}
			else if (json["power"] == true) {
				power_on();
			}
			else if (json["power"] == false) {
				power_off();
			}
			else {
				server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"power_field_invalid\"}");
				return;
			}
		}
		else {
			server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"action_field_invalid\"}");
			return;
		}

		//Creates return json object
		DynamicJsonBuffer jsonBuffer2;
		JsonObject& output = jsonBuffer2.createObject();

		output["power"] = gpioPinState;
		output["last_state"] = lastState;

		String result;
		output.printTo(result);

		server.send(HTTP_CODE_OK, "application/json", result);
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostName() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostName");

		if (!server.hasArg("plain")) { server.send(HTTP_CODE_BAD_REQUEST); return; }

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));

		if (!json.success()) { server.send(HTTP_CODE_BAD_REQUEST); return; }
		if (!json.containsKey("new_name")) {
			server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"new_name_field_missing\"}");
			return;
		}

		String new_name = json["new_name"].asString();
		creds.save(new_name);
		WiFi.hostname(new_name);

		//Creates the return json object
		DynamicJsonBuffer jsonBuffer2;
		JsonObject& output = jsonBuffer2.createObject();

		output["new_name"] = new_name;

		String outputStr;
		output.printTo(outputStr);

		server.send(HTTP_CODE_OK, "application/json", outputStr);
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostWiFiCreds() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostWiFiCreds");

		if (!server.hasArg("plain")) { server.send(HTTP_CODE_BAD_REQUEST); return;}

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));

		if (!json.success()) { server.send(HTTP_CODE_BAD_REQUEST); return; }
		if (!json.containsKey("ssid") && !json.containsKey("password")) {
			server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"ssid_and_password_fields_missing\"}");
			return;
		}

		WiFiInfo oldInfo = creds.load();

		String ssid = oldInfo.ssid;
		String password = oldInfo.password;

		if (json.containsKey("ssid")) ssid = json["ssid"].asString();
		if (json.containsKey("password")) password = json["password"].asString();

		creds.save(ssid, password);

		//Creates the return json object
		DynamicJsonBuffer jsonBuffer2;
		JsonObject& output = jsonBuffer2.createObject();

		if (json.containsKey("ssid")) output["ssid"] = json["ssid"];
		if (json.containsKey("password")) output["password"] = json["password"];

		String outputStr;
		output.printTo(outputStr);

		server.send(HTTP_CODE_OK, "application/json", outputStr);
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostRestart() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostRestart");

		//mDNS should not run during sleep
		MDNS.close();

		//Default sleep for 1 second
		int delaySeconds = 1;

		if (server.hasArg("plain")) {
			DynamicJsonBuffer jsonBuffer;
			JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));

			if (json.success() && json.containsKey("delay_seconds") && json["delay_seconds"].is<int>()) {
				delaySeconds = json["delay_seconds"].as<int>();
				if (delaySeconds < 1) delaySeconds = 1; //Min of 1 second
				if (delaySeconds > 30) delaySeconds = 30; //Max of 30 seconds
			}
		}

		//Creates the return json object
		DynamicJsonBuffer jsonBuffer2;
		JsonObject& output = jsonBuffer2.createObject();

		output["delay_seconds"] = delaySeconds;

		String outputStr;
		output.printTo(outputStr);

		//Default sleep used allow server to return this send line:
		server.send(HTTP_CODE_OK, "application/json", outputStr);

		Serial.print("Sleeping for seconds: ");
		Serial.println(delaySeconds);

		delay(1000 * delaySeconds); //1000 = 1 second

		ESP.restart();
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostNewMaster() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostNewMaster");

		if (!server.hasArg("plain")) { server.send(HTTP_CODE_BAD_REQUEST); return; }

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));

		if (!json.success()) { server.send(HTTP_CODE_BAD_REQUEST); return; }
		if (!json.containsKey("ip")) {
			server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"ip_field_missing\"}");
			return;
		}
		server.send(HTTP_CODE_OK);

		String newMasterIP = json["ip"].asString();
		masterIP = newMasterIP;

		if (json.containsKey("delay_seconds")) {
			int delaySeconds = json["delay_seconds"].as<int>();
			if (delaySeconds > 10) delaySeconds = 10; //Max of 10 seconds

			Serial.print("Sleeping for seconds: ");
			Serial.println(delaySeconds);
			delay(1000 * delaySeconds); //1000 = 1 second
		}
	};
	return lambda;
}

//Configurable connected device
std::function<void()> ClientServer::handleClientGetConnected() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientGetConnected");
		server.send(HTTP_CODE_OK, "application/json", getConnected());
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostConnected() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostConnected");

		if (!server.hasArg("plain")) { server.send(HTTP_CODE_BAD_REQUEST); return; }

		String inputString = server.arg("plain");

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(inputString);

		if (!json.success()) { server.send(HTTP_CODE_BAD_REQUEST); return; }

		if (!json.containsKey("dest_id") || !json.containsKey("method") || !json.containsKey("path") || !json.containsKey("data")) {
			server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"missing_required_field\"}");
			return;
		}

		String id     = json["dest_id"].asString();
		String method = json["method"].asString();
		String path   = json["path"].asString();
		String data   = json["data"].asString();

		connected.save(id, method, path, data);
		server.send(HTTP_CODE_OK, "application/json");
	};
	return lambda;
}

//Client creation
bool ClientServer::findAndConnectToMaster() {
	//Can the master access point be found?
	if (findMaster()) {
		//Can I connect to the master access point?
		if (connectToWiFi(MASTER_INFO)) {
			//Can I get and save WiFi credentials from the master?
			if (getAndSaveMainWiFiInfo()) {
				//Can I connect to the WiFi router using the credentials?
				return connectToWiFi(creds.load());
			}
		}
	}
	return false;
}
bool ClientServer::findMaster() {
	Serial.println("Entering findMaster");

	String lookingFor = MASTER_INFO.ssid;
	int foundNetworks = WiFi.scanNetworks();
	for (int i = 0; i < foundNetworks; i++) {
		String current_ssid = WiFi.SSID(i);

		if (current_ssid.equals(lookingFor)) {
			return true;
		}
	}
	return false;
}
bool ClientServer::getAndSaveMainWiFiInfo() {
	Serial.println("Entering getAndSaveMainWiFiInfo");

	WiFiClientSecure client;
	String host = WiFi.gatewayIP().toString();

	if (client.connect(host, MASTER_PORT)) {
		client.print(String("GET /wifi_info") + " HTTP/1.1\r\n" +
			"Host: " + host + "\r\n" +
			"Connection: close\r\n" +
			"\r\n"
		);

		String payload;

		// Gets the last line of the message
		while (client.connected() || client.available()) {
			if (client.available()) payload = client.readStringUntil('\n');
		}
		client.stop();

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(payload);

		if (!json.success()) return false;
		if (!json.containsKey("ssid") && !json.containsKey("password") && !json.containsKey("master_ip")) {
			return false;
		}

		masterIP = json["master_ip"].asString();
		String ssid = json["ssid"].asString();
		String password = json["password"].asString();

		creds.save(ssid, password);

		return true;
	}
	else {
		client.stop();
		Serial.println("Failed to get WiFi credentials from master");
		return false;
	}
}

//Client to master handleing
void ClientServer::checkinWithMaster() {
	Serial.println("Checking up on master");

	HTTPClient http;
	http.begin("http://" + masterIP + ":" + String(MASTER_PORT) + "/checkin");
	int httpCode = http.sendRequest("POST", getDeviceInfo());
	http.end();

	if (httpCode != HTTP_CODE_OK && !updateMasterIP()) {
		electNewMaster();
	}
}
bool ClientServer::updateMasterIP() {
	Serial.println("Entering updateMasterIP");

	if (MDNS.queryService(MASTER_MDNS_ID, "tcp") < 1) {
		return false;
	}
	else {
		if (!MDNS.answerIP(0).isSet()) {
			return false;
		}
		else {
			masterIP = MDNS.answerIP(0).toString();
			return true;
		}
	}
}
void ClientServer::electNewMaster() {
	Serial.println("Entering electNewMaster");

	//Initalises the chosen id to the id of the current device
	String myId = String(ESP.getChipId());
	String chosenId = myId;

	std::vector<String> clientIPs;

	//Query for client devices
	int devicesFound = MDNS.queryService(CLIENT_MDNS_ID, "tcp");
	for (int i = 0; i < devicesFound; ++i) {
		//Saves IPs for use when becoming master later
		clientIPs.push_back(MDNS.answerIP(i).toString());

		String reply = MDNS.answerHostname(i);

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(reply);

		if (json.success() && json.containsKey("id")) {
			String currentId = json["id"].asString();

			if (currentId < chosenId) {
				chosenId = currentId;
			}
		}
	}

	if (myId.equals(chosenId)) {
		Serial.println("I've been chosen as the new master");
		becomeMaster(clientIPs);
	}
	else {
		Serial.println("I've been chosen to stay as a client");
	}
}
void ClientServer::becomeMaster(std::vector<String> clientIPs) {
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["ip"] = WiFi.localIP().toString();
	json["delay_seconds"] = 10;

	String result;
	json.printTo(result);

	//Notifies all clients that this device will be the new master
	for (std::vector<String>::iterator it = clientIPs.begin(); it != clientIPs.end(); ++it) {
		Serial.println("Letting know I'm about to be master: " + *it);

		HTTPClient http;
		//Increase the timeout from 5000 to allow other clients to go through the electNewMaster stages
		http.setTimeout(7000);
		http.begin("http://" + *it + ":" + CLIENT_PORT + "/master");
		http.sendRequest("POST", result);
		http.end();
	}

	MDNS.close();
	ESP.restart();
}

//General reusable functions for client & master servers
String ClientServer::getDeviceInfo() {
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["id"] = ESP.getChipId();
	json["ip"] = WiFi.localIP().toString();
	json["name"] = creds.load().hostname;

	String result;
	json.printTo(result);

	return result;
}
bool ClientServer::connectToWiFi(WiFiInfo info) {
	if (info.ssid == "" || info.password == "") {
		Serial.println("Invalid WiFi credentials");
		return false;
	}

	Serial.println("Connecting to:");
	Serial.print("SSID: ");
	Serial.println(info.ssid);
	Serial.print("Password: ");
	Serial.println(info.password);
	Serial.print("Name: ");
	Serial.println(info.hostname);

	WiFi.hostname(info.hostname);
	WiFi.begin(info.ssid, info.password);

	// Wait for connection
	for (int i = 0; WiFi.status() != WL_CONNECTED && i < 10; i++) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");

	if (WiFi.status() == WL_CONNECTED) {
		Serial.print("Connected!\nIP address: ");
		Serial.println(WiFi.localIP().toString());
	}
	else Serial.println("Could not connect!");
	return (WiFi.status() == WL_CONNECTED);
}
void ClientServer::enableMDNS(String mdnsId) {
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["id"] = ESP.getChipId();
	json["name"] = creds.load().hostname;
	
	String jsonName;
	json.printTo(jsonName);

	MDNS.close();
	MDNS.begin(jsonName.c_str());
	MDNS.addService(mdnsId, "tcp", 80); //Broadcasts IP so can be seen by other devices
}
void ClientServer::enableOTA() {
	//Enabled "Over The Air" updates so that the ESPs can be updated remotely 
	ArduinoOTA.setPort(8266);
	ArduinoOTA.setHostname(getDeviceHostName());

	ArduinoOTA.onStart([]() {
		MDNS.close(); //Close to not interrupt other devices
		Serial.println("OTA Update Started...");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nOTA Update Ended!");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
	});

	ArduinoOTA.begin();
}
void ClientServer::enableUPnP(int port, String mdnsId) {
	boolean portMappingAdded = false;
	tinyUPnP.addPortMappingConfig(WiFi.localIP(), port, "TCP", 36000, mdnsId);
	while (!portMappingAdded) {
		portMappingAdded = tinyUPnP.commitPortMappings();
		if (!portMappingAdded) {
			tinyUPnP.printAllPortMappings();
			delay(30000);  // 30 seconds before trying again
		}
	}
}

//Power controls
void ClientServer::power_toggle() {
	Serial.println("Entering power_toggle");

	if (gpioPinState == false) power_on();
	else power_off();
}
void ClientServer::power_on() {
	Serial.println("Entering power_on");

	gpioPinState = true;
	digitalWrite(GPIO_OUTPUT_PIN, LOW);
}
void ClientServer::power_off() {
	Serial.println("Entering power_off");

	gpioPinState = false;
	digitalWrite(GPIO_OUTPUT_PIN, HIGH);
}

// Cloud integration
void ClientServer::selfRegister() {
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["id"] = ESP.getChipId();
	json["name"] = creds.load().hostname;

	String body;
	json.printTo(body);

	String ip = CLOUD_IP + ":" + CLOUD_PORT;
	String url = "http://" + ip + "/device";

	HTTPClient http;
	http.begin(url);
	http.POST(body);
	http.end();
}
void ClientServer::configUpdate() {
	WiFiClientSecure client;
	if (!client.connect(CLOUD_IP, CLOUD_PORT)) return;
	client.print(String("GET /config?id=") + ESP.getChipId() + " HTTP/1.1\r\n" +
		"Host: " + CLOUD_IP + "\r\n" +
		"Connection: close\r\n" +
		"\r\n"
	);

	String payload;

	// Gets the last line of the message
	while (client.connected() || client.available()) {
		if (client.available()) payload = client.readStringUntil('\n');
	}
	client.stop();

	if (payload == "") return;

	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.parseObject(payload);

	if (!json.success()
		|| !json.containsKey("dest_id")
		|| !json.containsKey("method")
		|| !json.containsKey("path")
		|| !json.containsKey("data")) return;

	String id     = json["dest_id"].asString();
	String method = json["method"].asString();
	String path   = json["path"].asString();

	String data = "";
	DynamicJsonBuffer().parseObject(json["data"].asString()).printTo(data);

	connected.save(id, method, path, data);
}

//Light switch example

//std::function<void()> ClientServer::handleClientPostLightSwitch() {
//	std::function<void()> lambda = [=]() {
//		Serial.println("Entering handleClientPostLightSwitch");
//
//		if (!server.hasArg("plain")) { server.send(HTTP_CODE_BAD_REQUEST); return; }
//
//		DynamicJsonBuffer jsonBuffer;
//		JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));
//
//		if (!json.success()) { server.send(HTTP_CODE_BAD_REQUEST); return; }
//		if (!json.containsKey("power")) {
//			server.send(HTTP_CODE_BAD_REQUEST, "application/json", "{\"error\":\"power_field_missing\"}");
//			return;
//		}
//		server.send(HTTP_CODE_OK);
//
//		String ip = masterIP + ":" + MASTER_PORT;
//		String url = "http://" + ip + "/device";
//
//		DynamicJsonBuffer jsonBuffer2;
//		JsonObject& toClient = jsonBuffer2.createObject();
//
//		toClient["name"] = "LightBulb";
//		toClient["action"] = "set";
//		toClient["power"] = json["power"];
//
//		String body;
//		toClient.printTo(body);
//
//		HTTPClient http;
//		http.begin(url);
//		http.POST(body);
//		http.end();
//	};
//	return lambda;
//}


String ClientServer::getConnected() {
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	ConnectedInfo device = connected.load();

	String id     = device.id;
	String method = device.method;
	String path   = device.path;
	String data   = device.data;

	if (!id.isEmpty())     json["dest_id"] = id;
	if (!method.isEmpty()) json["method"] = method;
	if (!path.isEmpty())   json["path"] = path;
	if (!data.isEmpty()) {
		StaticJsonBuffer<200> jsonDataBuffer;
		JsonObject& jsonData = jsonDataBuffer.parseObject(data);
		if (!jsonData.success()) { Serial.println("JSON Read Error"); }
		json["data"] = jsonData;
	}

	String result;
	json.printTo(result);

	return result;
}

void ClientServer::handle() {
	server.handleClient();
	checkInputChange();
	tinyUPnP.updatePortMappings(600000);
};
void ClientServer::checkInputChange() {
	bool currentInputValue = (LOW == digitalRead(GPIO_INPUT_PIN));
	if (lastInputValue != currentInputValue) {
		Serial.println("Input changed!");
		lastInputValue = currentInputValue;

		ConnectedInfo conDevice = connected.load();

		String id = conDevice.id;
		String method = conDevice.method;
		String path = conDevice.path;
		String data = conDevice.data;

		if (!id.isEmpty() && !method.isEmpty() && !path.isEmpty() && !data.isEmpty()) {
			DynamicJsonBuffer jsonBuffer;
			JsonObject& json = jsonBuffer.parseObject(data);

			if (!json.success()) {
				Serial.print("Error reading json from memory: ");
				Serial.println(data);
				return;
			}

			String dataWithID;
			json.printTo(dataWithID);

			String ip = masterIP + ":" + MASTER_PORT;
			String url = "http://" + ip + "/" + path + "?id=" + id;

			// Send http command to connected device
			HTTPClient http;
			http.begin(url);

			if (method == "PUT")  http.PUT(dataWithID);
			if (method == "POST") http.POST(dataWithID);

			http.end();
		}
	}
}