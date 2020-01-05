#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>


const char* ssid = "fieldsOfGold";
const char* password = "blueWifi148";
const char* mqtt_server = "192.168.1.147";
const char* programId = "Irrigation v1";
const char* loggingTopic = "Logging/Irrigation";
static long lastTime = 0;
static long shutdownAllAfter = 10000;

#define OFF 0x1
#define ON  0x0

WiFiClient espClient;
PubSubClient mqttClient(espClient);
Adafruit_MCP23017 mcp;

void setup()
{
	setupDebug();
	setupWifi();
	setupMqtt();
	setupDiode();
	setupMcp();
	lastTime = millis();
}

void loop()
{
	if (!mqttClient.connected())
		MQTT_reconnect();
	mqttClient.loop();
	setOffIfNotActive();
}

void setupDebug()
{
	Serial.begin(9600);
	Serial.println(programId);
	Serial.println();
}

void setupWifi()
{
	// Connect to your WiFi network
	WiFi.begin(ssid, password);
	Serial.print("Connecting");

	// Wait for successful connection
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println();
	Serial.print("Connected to: ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	Serial.println();
}

void setupMqtt()
{
	mqttClient.setServer(mqtt_server, 1883);
	mqttClient.setCallback(MQTT_callback);
}

void setupDiode()
{
	pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

void setupMcp()
{
	mcp.begin();      // use default address 0
	mcp.pinMode(0, OUTPUT);
	mcp.pinMode(1, OUTPUT);
	mcp.pinMode(2, OUTPUT);
	mcp.pinMode(3, OUTPUT);
	mcp.pinMode(4, OUTPUT);
	mcp.pinMode(5, OUTPUT);
	mcp.pinMode(6, OUTPUT);
	mcp.pinMode(7, OUTPUT);
	mcp.pinMode(8, OUTPUT);
	setAllToOff();
}

void setAllToOff() {
	Log("Setting all to off");
	for (size_t i = 0; i < 8; i++)
		mcp.digitalWrite(i, OFF);
}

void setOffIfNotActive()
{
	if (millis() - lastTime > shutdownAllAfter)
	{
		//TODO do not proceed if all is already off
		Log("Emergency shotdown will proceed because of inactivity.");
		setAllToOff();
		lastTime = millis();
	}
}

void MQTT_callback(char* _topic, byte* _payload, unsigned int length)
{
	std::string topic(_topic);
	std::string payload((char*)_payload, (size_t)length);
	std::string log = "MQTT message arrived [" + topic + "]:[" + payload + "]";
	Log(log.c_str());
	Blink(100, 100);
	int command = payload == "on" ? ON : OFF; 

	if(topic.substr(0,10) == "Irrigation")// message arrived it means activity
		lastTime = millis();
		
	//TODO other ones, maybe better code
	// also all off needed
	//also logic that dont start motor without at least ventil open
	if (topic == "Irrigation/Studna") {
		Log("Setting studna", command == ON ? "on" : "off");
		mcp.digitalWrite(0, command);
	}
	if (topic == "Irrigation/JV") {
		Log("Setting JV", command == ON ? "on" : "off");
		mcp.digitalWrite(1, command);
	}
}

void MQTT_reconnect()
{
	// Loop until we're reconnected
	while (!mqttClient.connected())
	{
		Serial.print("Attempting MQTT connection...");
		// Attempt to connect
		if (mqttClient.connect("ESP8266Client"))
		{
			Log("Irrrigation connected");
			mqttClient.subscribe("Irrigation/#");
		}
		else
		{
			Serial.print("failed, rc=");
			Serial.print(mqttClient.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

void Log(const char* logMsg) {
	Serial.println(logMsg);
	mqttClient.publish(loggingTopic, logMsg);
}

void Log(const char* logMsg, const char* logPar) {
	std::string msg(logMsg);
	msg += " ";
	msg += logPar;
	Serial.println(msg.c_str());
	mqttClient.publish(loggingTopic, msg.c_str());
}

void Blink(int nDelayOn, int nDelayOff)
{
	digitalWrite(LED_BUILTIN, OFF);  
	delay(nDelayOn);
	digitalWrite(LED_BUILTIN, ON);
	delay(nDelayOff);
}