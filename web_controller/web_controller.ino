#include <WiFi.h>

#define MAC "24:6F:28:51:F4:55"
#define WIFI_SSID "YummyWater"
#define WIFI_PASSWORD "iswaterwet"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

WiFiServer server(80);
bool ran_once = false;
long long unsigned int start_time = 0;
//std::hashmap<int,int> active_processes;
//std::hashmap<int,int> flow_readings;
const int pump_pin_x = 23;
const int relay_pin_a = 4;
const int flow_pin_data_a = 5;
const int relay_pin_b = 27;
const int flow_pin_data_b = 34;

const int pump_pin_y = 22;
const int relay_pin_c = 21;
const int flow_pin_data_c = 15;
int state[5]; // {A, B, pumpX, C, pumpY}

void print_ip();
void led_number_print(int pin, int num, int num_digits);

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(pump_pin_x, OUTPUT);
    pinMode(relay_pin_a, OUTPUT);
    pinMode(relay_pin_b, OUTPUT);
    pinMode(flow_pin_data_a, INPUT);
    pinMode(flow_pin_data_b, INPUT);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(pump_pin_x, LOW);
    digitalWrite(relay_pin_a, LOW);
    digitalWrite(relay_pin_b, LOW);

    pinMode(pump_pin_y, OUTPUT);
    pinMode(relay_pin_c, OUTPUT);
    pinMode(flow_pin_data_c, INPUT);
    digitalWrite(pump_pin_y, LOW);
    digitalWrite(relay_pin_c, LOW);
    state[0] = 0;
    state[1] = 0;
    state[2] = 0;
    state[3] = 0;
    state[4] = 0;
    delay(200);
    WiFi.mode(WIFI_AP);
    WiFi.softAPsetHostname(WIFI_SSID);
    while (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("failed to start server");
        delay(1000);
    }
    delay(100);
    IPAddress local_ip(192, 168, 1, 1);
    IPAddress gateway_ip(192, 168, 1, 1);
    IPAddress net_mask(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway_ip, net_mask);
    Serial.println("starting server");
    server.begin();
}

void loop() {
    if (!ran_once && millis() > start_time + 1000) {
        ran_once = true;
        print_ip();
        start_time = millis();
    }

    WiFiClient client = server.available();
    if (client) {
        Serial.print("client connected: ");
        Serial.println(client.remoteIP());
        for (int i = 0; i < 3; ++i) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
        }

        char c;
        String oldline = "";
        String line = "";
        while (client.connected()) {
            if (client.available()) {
                c = client.read();
                Serial.print(c);
                if (c == '\n') {
                    if (line.length() == 0) {
                        break;
                    }
                } else if (c == '\r') {
                    oldline = line;
                    line = "";
                } else {
                    line += c;
                }
            }
        }

        String message = "";
        if (oldline.indexOf("GET / ") >= 0) {
            Serial.println(">> GET / ");
            message = "hello";
        } else if (oldline.indexOf("GET /starta") >= 0) {
            Serial.println(">> GET /starta");
            message = "start a";
            state[0] = 1;
            digitalWrite(relay_pin_a, HIGH);
        } else if (oldline.indexOf("GET /stopa") >= 0) {
            Serial.println(">> GET /stopa");
            message = "stop a";
            state[0] = 0;
            digitalWrite(relay_pin_a, LOW);
        } else if (oldline.indexOf("GET /startb") >= 0) {
            Serial.println(">> GET /startb");
            message = "start b";
            state[1] = 1;
            digitalWrite(relay_pin_b, HIGH);
        } else if (oldline.indexOf("GET /stopb") >= 0) {
            Serial.println(">> GET /stopb");
            message = "stop b";
            state[1] = 0;
            digitalWrite(relay_pin_b, LOW);
        } else if (oldline.indexOf("GET /startpumpx") >= 0) {
            Serial.println(">> GET /startpumpx");
            message = "start pump x";
            state[2] = 1;
            digitalWrite(pump_pin_x, HIGH);
        } else if (oldline.indexOf("GET /stoppumpx") >= 0) {
            Serial.println(">> GET /stoppumpx");
            message = "stop pump x";
            state[2] = 0;
            digitalWrite(pump_pin_x, LOW);
        ///
        } else if (oldline.indexOf("GET /startc") >= 0) {
            Serial.println(">> GET /startc");
            message = "start c";
            state[3] = 1;
            digitalWrite(relay_pin_c, HIGH);
        } else if (oldline.indexOf("GET /stopc") >= 0) {
            Serial.println(">> GET /stopc");
            message = "stop c";
            state[3] = 0;
            digitalWrite(relay_pin_c, LOW);
        } else if (oldline.indexOf("GET /startpumpy") >= 0) {
            Serial.println(">> GET /startpumpy");
            message = "start pump y";
            state[4] = 1;
            digitalWrite(pump_pin_y, HIGH);
        } else if (oldline.indexOf("GET /stoppumpy") >= 0) {
            Serial.println(">> GET /stoppumpy");
            message = "stop pump y";
            state[4] = 0;
            digitalWrite(pump_pin_y, LOW);
        }

        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println("Connection: close");
        client.println();
        client.println("<!doctype html>");
        client.println("<html>");
        client.println("<body>");
        client.printf("<h1>%s</h1>", message.c_str());
        client.println("<pre>");
        client.println("                                      | |            ");
        client.println("                              valve b |_|            ");
        client.println("                                      | |            ");
        client.println("  |--------| valve c  |--------|      | | valve a    ");
        client.println("  | pump y |====|_|===| pump x |======| |====|_|==== ");
        client.println("  |--------|          |--------|                     ");
        client.println("</pre>");
        client.println("<br/>");
        client.println("<hr/>");
        client.println("<br/>");
        client.println("<a href=\"/startpumpx\">start pump x</a><br/>");
        client.println("<a href=\"/stoppumpx\">stop pump x</a><br/>");
        client.printf("<p>pump x state: %s</p>", (state[2] == 0) ? "OFF" : "ON");
        client.println("<a href=\"/starta\">start a</a><br/>");
        client.println("<a href=\"/stopa\">stop a</a><br/>");
        client.printf("<p>A state: %s</p>", (state[0] == 0) ? "CLOSED" : "OPEN");
        client.println("<a href=\"/startb\">open b</a><br/>");
        client.println("<a href=\"/stopb\">close b</a><br/>");
        client.printf("<p>B state: %s</p>", (state[1] == 0) ? "CLOSED" : "OPEN");
        client.println("<br/>");
        client.println("<hr/>");
        client.println("<br/>");
        client.println("<a href=\"/startpumpy\">start pump y</a><br/>");
        client.println("<a href=\"/stoppumpy\">stop pump y</a><br/>");
        client.printf("<p>pump y state: %s</p>", (state[4] == 0) ? "OFF" : "ON");
        client.println("<a href=\"/startc\">open c</a><br/>");
        client.println("<a href=\"/stopc\">close c</a><br/>");
        client.printf("<p>C state: %s</p>", (state[3] == 0) ? "CLOSED" : "OPEN");
        client.println("<body>");
        client.println("</html>");
        client.stop();
    }

//    for (i in range len active_processes) {
//        if (active_processes[j] > 0) {
//            solenoid_pin[j].digitalWrite(HIGH);
//        } else {
//            active_processes.remove(process);
//            solenoid_pin[j].digitalWrite(LOW);
//        }
//    }
//
//    for (i in range len flow_readings) {
//        int ml = read_ml(flow_readings[i]);
//        active_processes[i] -= ml;
//    }
}

void print_ip() {
    IPAddress ip = WiFi.softAPIP();
    led_number_print(LED_BUILTIN, ip[0], 3);
    delay(500);
    led_number_print(LED_BUILTIN, ip[1], 3);
    delay(500);
    led_number_print(LED_BUILTIN, ip[2], 3);
    delay(500);
    led_number_print(LED_BUILTIN, ip[3], 3);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("IP: ");
    Serial.println(ip);
    Serial.print("SSID: ");
    Serial.println(WiFi.softAPSSID());
    Serial.print("Hostname: ");
    Serial.println(WiFi.softAPgetHostname());
    Serial.print("MAC: ");
    Serial.println(WiFi.softAPmacAddress());
}


void led_number_print(int pin, int num, int num_digits) {
    int j, k;
    int digits[10];
    if (num_digits > 8) {
        num_digits = 8;
    }

    j = 10;
    while (num > 0 && j > 0) {
        digits[--j] = num % 10;
        num /= 10;
    }

    for (; num_digits && j<10; ++j, --num_digits) {
        for (k=0; k<digits[j]; ++k) {
            digitalWrite(pin, LOW);
            delay(150);
            digitalWrite(pin, HIGH);
            delay(150);
        }
        delay(800);
    }
}
