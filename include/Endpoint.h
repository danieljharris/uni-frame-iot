// Endpoint.h

#ifndef _ENDPOINT_h
#define _ENDPOINT_h

#include "arduino.h"

#include <ESP8266WebServerSecure.h>

class Endpoint {
public:
	String path;
	HTTPMethod method;
	std::function<void()> function;

	Endpoint(std::function<void()> function) {
		this->function = function;
	}

	Endpoint(String path, HTTPMethod method, std::function<void()> function) {
		this->path = path;
		this->method = method;
		this->function = function;
	}
};

#endif