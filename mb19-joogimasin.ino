#include <Ethernet.h>
#include <EthernetUdp.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10, 0, 0, 51);

unsigned int localPort = 8888;      // local port to listen on

// buffers for receiving and sending data
int packetSize = 0;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
String wordCompare = "jook";
String requestGlassSize20 = "20ml";
String requestGlassSize30 = "30ml";
String requestGlassSize40 = "40ml";

struct glassSizes {
  enum {ML20, ML30, ML40};
} glassSize;

// default shot glass size at startup
int useGlassSize = glassSize.ML30;
int lastUsedGlassSize = glassSize.ML30;

struct valveStates {
  enum {OPEN, CLOSE};
} valveState;

struct valveTypes {
  enum {SHOT_GLASS, TANK};
} valveType;

const int stepPin = 6; 
const int dirPin = 5; 
const int enPin = 7;

struct motorImpulseAmount {
  enum {ML20 = 2000, ML30 = 3000, ML40 = 4000};
} motorImpulseAmount;

int motorTurnsValue = motorImpulseAmount.ML30;

const int motorSensorPin = A0;
const int cupSensorPin = A1;

const int valveRelay = 8;

const int motorSensorAdcLimit = 10;
const int cupSensorDetectLimit = 300;
const int cupSensorRemoveLimit = 150;

int motorSensorAdcRead = 0;
int cupSensorAdcRead = 0;

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(enPin,OUTPUT);

  pinMode(valveRelay, OUTPUT);
  digitalWrite(valveRelay, HIGH);

  startEthernet();
  calibrateSyringes(motorTurnsValue);
}

void loop() {
  // if there's data available, read a packet
  packetSize = Udp.parsePacket();

  if (packetSize) {
    memset(packetBuffer, 0, sizeof(packetBuffer));
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    String packetToString(packetBuffer);

    packetToString.trim();

    if (packetToString == "ping") {
      messageToEthernet("pong");
    }

    if (packetToString == requestGlassSize20) {
      useGlassSize = glassSize.ML20;
      motorTurnsValue = motorImpulseAmount.ML20;
      messageToEthernet("Changing shot size to 2cl");
    } else if (packetToString == requestGlassSize30) {
      useGlassSize = glassSize.ML30;
      motorTurnsValue = motorImpulseAmount.ML30;
      messageToEthernet("Changing shot size to 3cl");
    } else if (packetToString == requestGlassSize40) {
      useGlassSize = glassSize.ML40;
      motorTurnsValue = motorImpulseAmount.ML40;
      messageToEthernet("Changing shot size to 4cl");
    }

    if (useGlassSize != lastUsedGlassSize) {
      calibrateSyringes(motorTurnsValue);
      lastUsedGlassSize = useGlassSize;
      messageToEthernet("Change done!");
    }

    debugEthernet();

    if(packetToString == wordCompare) {
      messageToEthernet("Starting dispensing. Waiting for cup...");
      while (!cupAddDetected()) {
        // Wait until cup (or accidentally something else...) detected
      }
      messageToEthernet("Cup detected. Now dispensing.");
      delay(2000);
      dispenseDrink(motorTurnsValue);
      messageToEthernet("Dispense done! Remove cup...");

      while (!cupRemoveDetected()) {
        // Wait for cup to be removed
      }
      messageToEthernet("Cup removed!");
      delay(5000);
      messageToEthernet("Ready for next!");
    }

    packetToString = "";
  }
  delay(10);
}

void startEthernet() {
    // start the Ethernet
  Ethernet.begin(mac, ip);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start UDP
  Udp.begin(localPort);
}

void debugEthernet() {
  Serial.print("Received packet of size ");
  Serial.println(packetSize);
  Serial.print("From ");
  IPAddress remote = Udp.remoteIP();
  for (int i = 0; i < 4; i++) {
    Serial.print(remote[i], DEC);
    if (i < 3) {
      Serial.print(".");
    }
  }
  Serial.print(", port ");
  Serial.println(Udp.remotePort());

  Serial.println("Contents:");
  Serial.println(packetBuffer);
}

void messageToEthernet(const char *message) {
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.write(message);
  Udp.endPacket();
}

/*!
 * Initializes the syringes on first startup.
 * Pulls both syringes up until first syringe reaches
 * end-state detection sensor. Second syringe has to be
 * manually calibrated to the same level as the first one for it to work.
 */
void calibrateSyringes(int motorTurnAmount) {
  digitalWrite(enPin,LOW);
  digitalWrite(dirPin,HIGH);
  bool endReached = false;

  while (!endReached) {
    motorSensorAdcRead = analogRead(motorSensorPin);
    digitalWrite(stepPin,HIGH); 
    delayMicroseconds(500); 
    digitalWrite(stepPin,LOW); 
    delayMicroseconds(500);
    if (motorSensorAdcRead < motorSensorAdcLimit) {
      endReached = true;
    }
  }
  digitalWrite(valveRelay,LOW);
  delay(500);
  digitalWrite(dirPin,LOW);
  for (int x = 0; x < 5600; x++) {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin,LOW);
    delayMicroseconds(500);
  }
  digitalWrite(valveRelay,HIGH);
  delay(500);
  digitalWrite(dirPin,HIGH);
  for (int x = 0; x < motorTurnAmount; x++) {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin,LOW);
    delayMicroseconds(500);
  }
  digitalWrite(enPin,HIGH);
}

void dispenseDrink(int motorTurnAmount) {
  digitalWrite(enPin,LOW);
  digitalWrite(dirPin,LOW);
  for (int x = 0; x < motorTurnAmount; x++) {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin,LOW);
    delayMicroseconds(500);
  }
  digitalWrite(valveRelay,LOW);
  delay(500);
  digitalWrite(dirPin,HIGH);
  for (int x = 0; x < motorTurnAmount; x++) {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin,LOW);
    delayMicroseconds(500);
  }
  digitalWrite(valveRelay,HIGH);
  delay(500);
  digitalWrite(enPin,HIGH);
}

bool cupAddDetected() {
  cupSensorAdcRead = analogRead(cupSensorPin);
  Serial.print("detect add: ");
  Serial.println(cupSensorAdcRead);
  return cupSensorAdcRead > cupSensorDetectLimit;
}

bool cupRemoveDetected() {
  cupSensorAdcRead = analogRead(cupSensorPin);
  Serial.print("detect remove: ");
  Serial.println(cupSensorAdcRead);
  return cupSensorAdcRead < cupSensorRemoveLimit;
}
