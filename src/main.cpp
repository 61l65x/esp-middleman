#include <DNSServer.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

const byte	DNS_PORT = 53;
IPAddress	apIP(192, 168, 1, 1); // Static IP for the AP
DNSServer	dnsServer;
WebServer	server(80);

void		setupWebServer(void);
void		scanAndSetupAP(void);

void	listFilesInDir(File dir, int numTabs = 1)
{
	File	entry;

	while (true)
	{
		entry = dir.openNextFile();
		if (!entry)
			break ;
		for (int i = 0; i < numTabs; i++)
			Serial.print('\t');
		Serial.print(entry.name());
		if (entry.isDirectory())
		{
			Serial.println("/");
			listFilesInDir(entry, numTabs + 1);
		}
		else
		{
			Serial.print("\t\t");
			Serial.println(entry.size(), DEC);
		}
		entry.close();
	}
}

void	initLittleFS(void)
{
	File	root;

	if (!LittleFS.begin())
	{
		Serial.println("Failed to mount file system. Attempting to format...");
		if (!LittleFS.format())
			Serial.println("Failed to format file system. Please check the hardware.");
		if (!LittleFS.begin())
			Serial.println("Failed to mount file system even after formatting. Exiting...");
	}
	Serial.println("Listing directory contents:");
	root = LittleFS.open("/");
	listFilesInDir(root);
	root.close();
}

void	setup(void)
{
	Serial.begin(115200);
	while (!Serial)
		continue ;
	Serial.println("Serial initialized.");
	initLittleFS();
	Serial.println("File system mounted successfully.");
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

void	serveFile(String path)
{
	File	file;
	String	fileContent;

	Serial.print("Requested path: ");
	Serial.println(path);
	file = LittleFS.open(path, "r");
	if (!file)
	{
		Serial.println("File not found on LittleFS. Sending 404.");
		server.send(404, "text/plain", "File not found");
		return ;
	}
	fileContent = "";
	while (file.available())
	{
		fileContent += (char)file.read();
	}
	file.close();
	Serial.println("File found. Sending content...");
	server.send(200, "text/html", fileContent);
}

void	setupWebServer(void)
{
	server.on("/", HTTP_GET, []() {
		// if (WiFi.SSID().indexOf("Hive") != -1)
		serveFile("/42login.html");
		// else
		//	server.send(200, "text/html",
		//		"<h1>Welcome to ESP32-C3 AP!</h1><p>Please log in!</p>");
	});
	server.onNotFound([]() {
		server.sendHeader("Location", "/", true);
		server.send(302, "text/plain", "");
	});
	server.begin();
	Serial.println("Web server started, captive portal active.");
}
