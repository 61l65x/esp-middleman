#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

const byte	DNS_PORT = 53;
IPAddress	apIP(192, 168, 1, 1); // Static IP for the AP
DNSServer	dnsServer;
WebServer	server(80);

void		setupWebServer(void);
void		scanAndSetupAP(void);

void	setup(void)
{
	Serial.begin(115200);
	Serial.println("Serial initialized.");
	scanAndSetupAP();
	dnsServer.start(DNS_PORT, "*", apIP);
	setupWebServer();
	Serial.println("AP Configured. Ready for connections...");
}

void	loop(void)
{
	dnsServer.processNextRequest();
	server.handleClient();
}

void	scanAndSetupAP(void)
{
	int		n;
	int		maxRSSI;
	String	maxSSID;

	Serial.println("Scanning for WiFi networks...");
	n = WiFi.scanNetworks(false, true);
	Serial.println("Scan complete.");
	if (n == 0)
	{
		Serial.println("No networks found.");
	}
	else
	{
		maxRSSI = -1000;
		maxSSID = "";
		for (int i = 0; i < n; ++i)
		{
			if (WiFi.RSSI(i) > maxRSSI)
			{
				maxRSSI = WiFi.RSSI(i);
				maxSSID = WiFi.SSID(i);
			}
		}
		Serial.print("Strongest SSID: ");
		Serial.print(maxSSID);
		Serial.print(" with RSSI: ");
		Serial.println(maxRSSI);
		WiFi.disconnect();
		WiFi.softAP(maxSSID.c_str());
		WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
		Serial.println("AP Configured. Ready for connections...");
	}
}

void	setupWebServer(void)
{
	server.on("/", HTTP_GET, []() { server.send(200, "text/html",
			"<h1>Welcome to ESP32-C3 AP!</h1><p>Please log in!</p>"); });
	server.onNotFound([]() {
		server.sendHeader("Location", "/", true);
		// Redirect to the root directory
		server.send(302, "text/plain", "");
		// Use HTTP code 302 for redirection
	});
	server.begin();
	Serial.println("Web server started, captive portal active.");
}
