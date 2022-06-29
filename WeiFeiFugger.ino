#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESP32Ping.h>

const char* soft_ap_ssid = "WeiFeiFugger";
const char* soft_ap_password = "1234567890";
DynamicJsonDocument output(1024);
DynamicJsonDocument hosts(2048);
String ap_config;
String pass_config;
const char* temp_ap;
const char* temp_pass;
AsyncWebServer server(80);

bool portScan = false;
bool runPing = false;
String scanIP;
String scanportIP;
String scanIPJson;
String scanIPJson2;
String scanportJson;
String scanportJson2;
int foundCount = 0;
void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {
    case SYSTEM_EVENT_WIFI_READY:
      Serial.println("WiFi interface ready");
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      Serial.println("Completed scan for access points");
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println("WiFi client started");
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("WiFi clients stopped");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("Connected to access point");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Disconnected from WiFi access point");
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      Serial.println("Authentication mode of access point has changed");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println("Lost IP address and IP address is reset to 0");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case SYSTEM_EVENT_AP_START:
      Serial.println("WiFi access point started");
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println("WiFi access point  stopped");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("Client connected");
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("Client disconnected");
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      Serial.println("Assigned IP address to client");
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      Serial.println("Received probe request");
      break;
    case SYSTEM_EVENT_GOT_IP6:
      Serial.println("IPv6 is preferred");
      break;
    case SYSTEM_EVENT_ETH_START:
      Serial.println("Ethernet started");
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("Ethernet stopped");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet connected");
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet disconnected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.println("Obtained IP address");
      break;
    default: break;
  }
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
}


void setup() {

  Serial.begin(115200);

  WiFi.onEvent(WiFiEvent);

  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/index.html", String(), false);
  });
  // Route for root / web page
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/favicon.ico", "image/x-icon");
  });
  // Route to load bootstrap.css file
  server.on("/weifei.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/weifei.png", "image/png");
  });
  // Route to load bootstrap.css file
  server.on("/bootstrap.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/bootstrap.css", "text/css");
  });
  // Route to load bootstrap.js file
  server.on("/bootstrap.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/bootstrap.js", "text/javascript");
  });
  // Route to load jquery.js file
  server.on("/jquery.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/jquery.js", "text/javascript");
  });
  // Route to load styles.css file
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });

  server.on("/connectionInfo", HTTP_GET, [](AsyncWebServerRequest * request) {
    String connectionInfo;
    connectionInfo = WiFi.status();
    output["connectionstatus"] = connectionInfo;
    output["ipaddress"] = WiFi.localIP();
    String jsonOut;
    serializeJson(output, jsonOut);
    request->send(200, "text/plain", jsonOut);
  });
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", "Rebooting");
    ESP.restart();
  });
  server.on("/pingResults", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (runPing == false) {
      if (scanIPJson2.length() > 1) {
        Serial.println("OUTPUT: " + scanIPJson2);
        request->send(200, "text/plain", scanIPJson2);
      }
    } else {
      request->send(400, "text/plain", "Scan is running");
    }
  });
  server.on("/portResults", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (portScan == false) {
      if (scanportJson2.length() > 1) {
        Serial.println("OUTPUT: " + scanportJson2);
        request->send(200, "text/plain", scanportJson2);
        scanportJson2 = "";
      }
    } else {
      request->send(400, "text/plain", "Scan is running");
    }
  });
  server.on("/pingStart", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (runPing == false) {
      runPing = true;
      scanIP = request->getParam("ip")->value();
      request->send(200, "text/plain", "Ping started");
    } else {
      request->send(400, "text/plain", "Ping already running");
    }
  });
  server.on("/portStart", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (portScan == false) {
      portScan = true;
      scanportIP = request->getParam("ip")->value();
      request->send(200, "text/plain", "Port scan started");
    } else {
      request->send(400, "text/plain", "Port scan already running");
    }
  });
  // Route to set scan Networks
  server.on("/scanNetwork", HTTP_GET, [](AsyncWebServerRequest * request) {
    String jsonOut = "{'accesspoints': [";
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [UNSECURED] " : " [SECURED] ");
        jsonOut += "{'ssid':'" + WiFi.SSID(i) + "', 'rssi':'" + WiFi.RSSI(i) + "', 'encryption':'" + ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "UNSECURED" : "SECURED") + "'},";
        delay(10);

      }
    }
    jsonOut += "]}";
    Serial.println(jsonOut);
    request->send(200, "text/plain", jsonOut);
  });

  // Route to connect to AP and scan network
  server.on("/connectAP", HTTP_GET, [](AsyncWebServerRequest * request) {
    int paramsNr = request->params();
    Serial.println(paramsNr);

    String ssid_param = request->getParam("ssid")->value();
    String encryption_param = request->getParam("encryption")->value();
    String password_param = request->getParam("password")->value();
    ssid_param.remove(0, 1);
    Serial.println(ssid_param);
    Serial.println(encryption_param);
    Serial.println(password_param);
    // Connect to Wi-Fi
    if (password_param == NULL) {
      WiFi.begin(ssid_param.c_str());
    } else {
      WiFi.begin(ssid_param.c_str(), password_param.c_str());
    }
    request->send(200, "text/plain", "OK"); //String(WiFi.localIP()
    //delay(2000);
    //ESP.restart();
  });


  // Start server
  server.begin();
}

void loop() {
  if (runPing) {
    String jsonPing = "{'clients': [";
    for (int i = 0; i < 254; i++) {
      String s_ip = scanIP + String(i);
      const char* ip = s_ip.c_str();
      bool ret = Ping.ping(ip, 1);
      if (ret) {
        foundCount++;
        String clientName = "client_" + String(foundCount);
        Serial.println(clientName);
        //hosts[clientName] = scanIP + String(i);

        jsonPing += "{'clientip':'" + s_ip + "'},";

      }
      Serial.println(String(ip) + " " + ret);
    }
    runPing = false;
    //serializeJson(hosts, scanIPJson);
    foundCount = 0;
    //hosts.clear();
    scanIPJson2 = jsonPing;
    scanIPJson2 += "]}";
  }
  if (portScan) {
    Serial.println("Portscan started");
    String jsonPort = "{'ports': [";
    const int ports2scan[] = {1, 3, 7, 9, 13, 17, 19, 21, 23, 25, 26, 37, 53, 79, 80, 81, 82, 88, 100, 106, 110, 111, 113, 119, 135, 139, 143, 144, 179, 199, 254, 255, 280, 311, 389, 427, 443, 445, 464, 465, 497, 513, 514, 515, 543, 544, 548, 554, 587, 593, 625, 631, 636, 646, 787, 808, 873, 902, 990, 993, 995, 1000, 1022, 1024, 1033, 1035, 1041, 1044, 1048, 1050, 1053, 1054, 1056, 1058, 1059, 1064, 1066, 1069, 1071, 1074, 1080, 1110, 1234, 1433, 1494, 1521, 1720, 1723, 1755, 1761, 1801, 1900, 1935, 1998, 2000, 2003, 2005, 2049, 2103, 2105, 2107, 2121, 2161, 2301, 2383, 2401, 2601, 2717, 2869, 2967, 3000, 3001, 3128, 3268, 3306, 3389, 3689, 3690, 3703, 3986, 4000, 4001, 4045, 4899, 5000, 5001, 5003, 5009, 5050, 5051, 5060, 5101, 5120, 5190, 5357, 5432, 5555, 5631, 5666, 5800, 5900, 5901, 6000, 6002, 6004, 6112, 6646, 6666, 7000, 7070, 7937, 7938, 8000, 8002, 8006, 8008, 8010, 8031, 8080, 8081, 8443, 8888, 9000, 9001, 9090, 9100, 9102, 9999, 10001, 10010, 32768, 32771, 49152, 49157, 50000};
    Serial.println(String(sizeof(ports2scan)));
    for(int i = 0; i < 174; i++) {
      WiFiClient clientscan;
      clientscan.setTimeout(1);
      if (clientscan.connect(scanportIP.c_str(), ports2scan[i])) {
        Serial.println("Port: " + String(ports2scan[i]) + " is open!");
        clientscan.stop();
        jsonPort += "{'port':'" + String(ports2scan[i]) + "'},";
      } else {
        Serial.println("Port: " + String(ports2scan[i]) + " is closed!");
      }
    }
    portScan = false;
    scanportJson2 = jsonPort;
    scanportJson2 = scanportJson2 + "]}";
  }
}
