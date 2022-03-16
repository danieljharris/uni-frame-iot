//ClientServer.cpp

#include "ClientServer.h"

bool ClientServer::start() {
	Serial.println("Entering startClient");

	WiFi.mode(WIFI_STA);

	if (!findAndConnectToLeader()) {
		Serial.println("Failed to become client");
		return false;
	}

	delay(1000);

	Serial.println("I am a client!");

	Serial.println("Letting leader know I exist...");
	checkinWithLeader();

	startServer(clientEndpoints);
	Serial.println("Ready!");

	return true;
}

void ClientServer::startServer(std::vector<Endpoint> endpoints) {
	Serial.println("Adding endpoints...");
	addEndpoints(endpoints);

	Serial.println("Enabeling SSL encryption...");
	enableSSL();

	Serial.println("Starting server...");
	server.begin(PORT);

	Serial.println("Starting MDNS...");
	enableMDNS();

	Serial.println("Enableing OTA updates...");
	enableOTA();

	// TODO: Fix UPnP
	// Serial.println("Configuring UPnP...");
	// enableUPnP(PORT, MDNS_ID);

	Serial.println("Registering on cloud...");
	if (selfRegister()) {
		Serial.println("Updating config from cloud...");
		configUpdate();
	}
	else
		Serial.println("Warning: Unable to contact cloud");
}

void ClientServer::update() { checkinWithLeader(); }

//Client endpoints
std::function<void()> ClientServer::handleClientGetInfo() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientGetInfo");
		return Response(HTTP_CODE_OK, getDeviceInfo());
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostDevice() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostDevice");

		if (!server.hasArg("plain")) return ErrorResponse(HTTP_CODE_BAD_REQUEST);

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));

		if (!json.success())             return ErrorResponse(HTTP_CODE_BAD_REQUEST);
		if (!json.containsKey("action")) return ErrorResponse(HTTP_CODE_BAD_REQUEST, "action_field_missing");

		bool lastState = gpioPinState;

		if (json["action"] == "toggle") {
			power_toggle();
		}
		else if (json["action"] == "pulse") {
			if (!json.containsKey("direction")) {
				return ErrorResponse(HTTP_CODE_BAD_REQUEST, "direction_field_missing");
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
				return ErrorResponse(HTTP_CODE_BAD_REQUEST, "direction_field_invalid");
			}
		}
		else if (json["action"] == "set") {
			if (!json.containsKey("power")) {
				return ErrorResponse(HTTP_CODE_BAD_REQUEST, "power_field_missing");
			}
			else if (!json["power"].is<bool>()) {
				return ErrorResponse(HTTP_CODE_BAD_REQUEST, "power_field_invalid");
			}
			else if (json["power"] == true) {
				power_on();
			}
			else if (json["power"] == false) {
				power_off();
			}
			else {
				return ErrorResponse(HTTP_CODE_BAD_REQUEST, "power_field_invalid");
			}
		}
		else {
			return ErrorResponse(HTTP_CODE_BAD_REQUEST, "action_field_invalid");
		}

		//Creates return json object
		DynamicJsonBuffer jsonBuffer2;
		JsonObject& output = jsonBuffer2.createObject();

		output["power"] = gpioPinState;
		output["last_state"] = lastState;

		return Response(HTTP_CODE_OK, output);
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostName() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostName");

		if (!server.hasArg("plain")) return ErrorResponse(HTTP_CODE_BAD_REQUEST);

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));

		if (!json.success()) return ErrorResponse(HTTP_CODE_BAD_REQUEST);
		if (!json.containsKey("new_name")) {
			return ErrorResponse(HTTP_CODE_BAD_REQUEST, "new_name_field_missing");
		}

		String new_name = json["new_name"].asString();
		creds.save(new_name);
		WiFi.hostname(new_name);

		//Creates the return json object
		DynamicJsonBuffer jsonBuffer2;
		JsonObject& output = jsonBuffer2.createObject();
		output["new_name"] = new_name;

		return Response(HTTP_CODE_OK, output);
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostWiFiCreds() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostWiFiCreds");

		if (!server.hasArg("plain")) return ErrorResponse(HTTP_CODE_BAD_REQUEST);

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));

		if (!json.success()) return ErrorResponse(HTTP_CODE_BAD_REQUEST);
		if (!json.containsKey("ssid") && !json.containsKey("password")) {
			return ErrorResponse(HTTP_CODE_BAD_REQUEST, "ssid_and_password_fields_missing");
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

		return Response(HTTP_CODE_OK, output);
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

		//Default sleep used allow server to return this send line:
		output["delay_seconds"] = delaySeconds;
		Response(HTTP_CODE_OK, output);

		Serial.print("Sleeping for seconds: ");
		Serial.println(delaySeconds);

		delay(1000 * delaySeconds); //1000 = 1 second

		ESP.restart();
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostNewLeader() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostNewleader");

		if (!server.hasArg("plain")) return ErrorResponse(HTTP_CODE_BAD_REQUEST);

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(server.arg("plain"));

		if (!json.success())         return ErrorResponse(HTTP_CODE_BAD_REQUEST);
		if (!json.containsKey("ip")) return ErrorResponse(HTTP_CODE_BAD_REQUEST, "ip_field_missing");

		Response(HTTP_CODE_OK);

		String newleaderIP = json["ip"].asString();
		leaderIP = newleaderIP;

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
		Response(HTTP_CODE_OK, getConnected());
	};
	return lambda;
}
std::function<void()> ClientServer::handleClientPostConnected() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientPostConnected");

		if (!server.hasArg("plain")) return ErrorResponse(HTTP_CODE_BAD_REQUEST);

		String inputString = server.arg("plain");

		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.parseObject(inputString);

		if (!json.success()) return ErrorResponse(HTTP_CODE_BAD_REQUEST);

		if (!json.containsKey("dest_id") || !json.containsKey("method") || !json.containsKey("path") || !json.containsKey("data")) {
			return ErrorResponse(HTTP_CODE_BAD_REQUEST, "missing_required_field");
		}

		String id     = json["dest_id"].asString();
		String method = json["method"].asString();
		String path   = json["path"].asString();
		String data   = json["data"].asString();

		connected.save(id, method, path, data);
		return Response(HTTP_CODE_OK);
	};
	return lambda;
}

//Client creation
bool ClientServer::findAndConnectToLeader() {
	//Can the leader access point be found?
	if (findLeader()) {
		//Can I connect to the leader access point?
		if (connectToWiFi(LEADER_INFO)) {
			//Can I get and save WiFi credentials from the leader?
			if (getAndSaveMainWiFiInfo()) {
				//Can I connect to the WiFi router using the credentials?
				return connectToWiFi(creds.load());
			}
		}
	}
	return false;
}
bool ClientServer::findLeader() {
	Serial.println("Entering findleader");

	String lookingFor = LEADER_INFO.ssid;
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

	// Serial.println("1..............................1");
	// // Serial.println("Host: " + host);
	// Serial.println("2..............................2");
	// Serial.println("Port: " + PORT);
	// Serial.println("Port: " + PORT);
	// Serial.println("Port: " + PORT);
	// Serial.println("Port: " + 1200);
	// Serial.println("Port: 1200");
	// Serial.println("3..............................3");
	// Serial.println("TEST..............................");

	if (client.connect(host, PORT)) {
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
		if (!json.containsKey("ssid") && !json.containsKey("password") && !json.containsKey("leader_ip")) {
			return false;
		}

		leaderIP = json["leader_ip"].asString();
		String ssid = json["ssid"].asString();
		String password = json["password"].asString();

		creds.save(ssid, password);

		return true;
	}
	else {
		client.stop();
		Serial.println("Failed to get WiFi credentials from leader");
		return false;
	}
}

//Client to leader handleing
void ClientServer::checkinWithLeader() {
	Serial.println("Checking up on leader");

	HTTPClient http;
	http.begin("http://" + leaderIP + ":" + String(PORT) + "/checkin");
	int httpCode = http.sendRequest("POST", getDeviceInfo());
	http.end();

	if (httpCode != HTTP_CODE_OK && !updateLeaderIP()) {
		electNewLeader();
	}
}
bool ClientServer::updateLeaderIP() {
	Serial.println("Entering updateLeaderIP");

	if (MDNS.queryService(MDNS_ID, "tcp") < 1) {
		return false;
	}
	else {
		if (!MDNS.answerIP(0).isSet()) {
			return false;
		}
		else {
			leaderIP = MDNS.answerIP(0).toString();
			return true;
		}
	}
}
void ClientServer::electNewLeader() {
	Serial.println("Entering electNewleader");

	//Initalises the chosen id to the id of the current device
	String myId = String(ESP.getChipId());
	String chosenId = myId;

	std::vector<String> clientIPs;

	//Query for client devices
	int devicesFound = MDNS.queryService(MDNS_ID, "tcp");
	for (int i = 0; i < devicesFound; ++i) {
		//Saves IPs for use when becoming leader later
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
		Serial.println("I've been chosen as the new leader");
		becomeLeader(clientIPs);
	}
	else {
		Serial.println("I've been chosen to stay as a client");
	}
}
void ClientServer::becomeLeader(std::vector<String> clientIPs) {
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["ip"] = WiFi.localIP().toString();
	json["delay_seconds"] = 10;

	String result;
	json.printTo(result);

	//Notifies all clients that this device will be the new leader
	for (std::vector<String>::iterator it = clientIPs.begin(); it != clientIPs.end(); ++it) {
		Serial.println("Letting know I'm about to be leader: " + *it);

		HTTPClient http;
		//Increase the timeout from 5000 to allow other clients to go through the electNewleader stages
		http.setTimeout(7000);
		http.begin("http://" + *it + ":" + PORT + "/leader");
		http.sendRequest("POST", result);
		http.end();
	}

	MDNS.close();
	ESP.restart();
}

//General reusable functions for client & leader servers
String ClientServer::getDeviceInfo() {
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["version"] = UNI_FRAME_VERSION;
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
void ClientServer::enableMDNS() {
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	int id = ESP.getChipId();
	String hostname = creds.load().hostname;
	String serviceName = hostname + "-" + String(id);

	json["id"] = id;
	json["name"] = hostname;
	
	String jsonName;
	json.printTo(jsonName);

	MDNS.close();
	MDNS.begin(serviceName);


	MDNSResponder::hMDNSService service = MDNS.addService(serviceName.c_str(), MDNS_ID.c_str(), "tcp", PORT);
	MDNS.addServiceTxt(service, "Version", UNI_FRAME_VERSION);
	MDNS.addServiceTxt(service, "ID", id);
	MDNS.addServiceTxt(service, "Name", hostname.c_str()); //TODO: Make dynamic

	MDNS.addServiceTxt(service, "Data", jsonName.c_str()); //Replace with reading directly form MDNS TXT
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
void ClientServer::enableUPnP(int port, String serviceName) {
	boolean portMappingAdded = false;
	tinyUPnP.addPortMappingConfig(WiFi.localIP(), port, "TCP", 36000, serviceName);
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
bool ClientServer::selfRegister() {
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["version"] = UNI_FRAME_VERSION;
	json["id"] = ESP.getChipId();
	json["name"] = creds.load().hostname;

	String body;
	json.printTo(body);

	String ip = CLOUD_IP + ":" + CLOUD_PORT;
	String url = "http://" + ip + "/device";

	HTTPClient http;
	http.begin(url);
	int httpCode = http.POST(body);
	http.end();

	if (httpCode != HTTP_CODE_OK)
		return false;
	else
		return true;
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
//		String ip = leaderIP + ":" + LEADER_PORT;
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

			String ip = leaderIP + ":" + PORT;
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