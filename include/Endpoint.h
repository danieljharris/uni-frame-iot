// Endpoint.h

#ifndef _ENDPOINT_h
#define _ENDPOINT_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <ESP8266WebServer.h>

class Endpoint {
public:
	String path;
	HTTPMethod method;
	std::function<void()> function;

	Endpoint(String path, HTTPMethod method, std::function<void()> function) {
		this->path = path;
		this->method = method;
		this->function = function;
	}
};

#endif