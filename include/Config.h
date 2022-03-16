//Config.h

#pragma once
#ifndef _CONFIG_h
#define _CONFIG_h

#include "WiFiCredentials.h"

#define UNI_FRAME_VERSION "2.2.0"

const WiFiInfo LEADER_INFO("Universal-Framework-"UNI_FRAME_VERSION, "7kAtZZm9ak", "Leader");

const String SETUP_PASSWORD = "zJ2f5xSX";

const String MDNS_ID = "UniFrame";

const int SETUP_PORT = 80;
// const int NORMAL_PORT = 1200;
#define PORT 1200

const int GPIO_OUTPUT_PIN = 0;
const int GPIO_INPUT_PIN = 2;

const String CLOUD_IP = "ec2-100-25-194-250.compute-1.amazonaws.com";
const int CLOUD_PORT = 5000;

#endif