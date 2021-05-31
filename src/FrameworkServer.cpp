//FrameworkServer.cpp

#include "FrameworkServer.h"

FrameworkServer::~FrameworkServer() { stop(); }

void FrameworkServer::stop() {
	Serial.println("Entering stop");

	server.close();
	server.stop();
}

char* FrameworkServer::getDeviceHostName() {
	char* newName = "ESP_";
	char hostName[10];
	sprintf(hostName, "%d", ESP.getChipId());
	strcat(newName, hostName);

	return newName;
}

std::function<void()> FrameworkServer::handleUnknownEndpoint() {
	std::function<void()> lambda = [=]() {
		Serial.println("Entering handleClientUnknown");

		Serial.println("");
		Serial.print("Unknown command: ");
		Serial.println(server.uri());

		Serial.print("Method: ");
		Serial.println(server.method());

		server.send(HTTP_CODE_NOT_FOUND);
	};
	
	return lambda;
}

void FrameworkServer::addEndpoints(std::vector<Endpoint> endpoints) {
	for (std::vector<Endpoint>::iterator it = endpoints.begin(); it != endpoints.end(); ++it) {
		if (it->path.isEmpty()) server.onNotFound(it->function);
		else server.on(it->path, it->method, it->function);
	}
}

void FrameworkServer::enableSSL() {
static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIC6jCCAlOgAwIBAgIULjQilGiurQVOEJQ4G2QQ0EDNAscwDQYJKoZIhvcNAQEL
BQAwejELMAkGA1UEBhMCUk8xCjAIBgNVBAgMAUIxEjAQBgNVBAcMCUJ1Y2hhcmVz
dDEbMBkGA1UECgwST25lVHJhbnNpc3RvciBbUk9dMRYwFAYDVQQLDA1PbmVUcmFu
c2lzdG9yMRYwFAYDVQQDDA1lc3A4MjY2LmxvY2FsMB4XDTIxMDUzMTAwMTk0M1oX
DTIyMDUzMTAwMTk0M1owejELMAkGA1UEBhMCUk8xCjAIBgNVBAgMAUIxEjAQBgNV
BAcMCUJ1Y2hhcmVzdDEbMBkGA1UECgwST25lVHJhbnNpc3RvciBbUk9dMRYwFAYD
VQQLDA1PbmVUcmFuc2lzdG9yMRYwFAYDVQQDDA1lc3A4MjY2LmxvY2FsMIGfMA0G
CSqGSIb3DQEBAQUAA4GNADCBiQKBgQCpVOchsNAP5/u5boXR7cMDtMskBAX56nkl
ZTMHlfmzdKGeUyCZqOFkZmVT88ym9w/Mc4lZkH5mL6Nw04vcjKxfFXeMcqqzXFzM
qv7+mYZm+lyRQliMpmAeLTcsnpVztXZ3UmS7kIcxuXh4EqWmL9E4olp4PDA4hUAZ
6v7reQRPRwIDAQABo20wazAdBgNVHQ4EFgQUgE8HRwTThIAAS6zqOzA+LXgE96ow
HwYDVR0jBBgwFoAUgE8HRwTThIAAS6zqOzA+LXgE96owDwYDVR0TAQH/BAUwAwEB
/zAYBgNVHREEETAPgg1lc3A4MjY2LmxvY2FsMA0GCSqGSIb3DQEBCwUAA4GBAFXR
zf8X1rlUsm3Cx+XRUIn3avm3Z+ogvFgjK30xaqYRlvJB/VyEMJTHb056Uv7X4sna
lxrzzrq59uaYXyiPEIoquK8xGy6SkQ8zqwKnDIeq/dGYoyrF+ZDVqS007CSIA/Cy
tm1cExXz6sH4rakv8mFnmOZvCv2mYKLNA6E1KgIR
-----END CERTIFICATE-----
)EOF";

static const char serverKey[] PROGMEM =  R"EOF(
-----BEGIN PRIVATE KEY-----
MIICdgIBADANBgkqhkiG9w0BAQEFAASCAmAwggJcAgEAAoGBAKlU5yGw0A/n+7lu
hdHtwwO0yyQEBfnqeSVlMweV+bN0oZ5TIJmo4WRmZVPzzKb3D8xziVmQfmYvo3DT
i9yMrF8Vd4xyqrNcXMyq/v6Zhmb6XJFCWIymYB4tNyyelXO1dndSZLuQhzG5eHgS
paYv0TiiWng8MDiFQBnq/ut5BE9HAgMBAAECgYBB/ThlxMYQrNNInG3CNeo904Mm
8fpyPpIfpKSSXDwHV3h0fujBeTL9MXpjkSs8FiQuBQiNwuW/ZOlI2ugydw/lC+MW
9PIE5e3hHRmylbQCvpSkZdPiU7h2BKDDNUPO/cCdTFBs+yTcT0FizDgEh55NCB6J
CRh0wsqoTrzQu2GNwQJBANTmEQu8D+bKfGdT2wYig9/qYQhcY7MUSeVUb3tz32Wg
Bvyhe3JL5NuzU/CFGVt5vNk+q+8mTuetu+MtSMjHFs8CQQDLnOFcVmxe4/6aZ1fd
QeII7l6hbLPCeVGGAOQg2w0t9lKyPgFV0tIjMiLVjz334HAiUm8/HHUcJ3xeIUR3
M94JAkAuZhGy+AKTLvAb6NekJ6OMCl2pX9FOtw4/z74YLrGySUUci+kGiOnQw+14
Ttmu6QIyaok4LqYlseRv52+kaldbAkEAupJiHk5CtyCLZ8hSRrfb+vsRUzFb9lNc
VEH0x/ZwuTEAzbrrVkz7qKyEJtQ+oCfUGF8Y+OeGl+nGmCo7pk6soQJAAnmySRFz
P8+siA6o9GnrlN+YKrXJTbw6lFBSqeUyAGO1sME/19q5SzF91oJMJvljdfae2yXU
K0x4kJR5SCrwYQ==
-----END PRIVATE KEY-----
)EOF";

	// server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
}