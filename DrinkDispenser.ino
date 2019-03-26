#include <Ethernet.h>
#include <EthernetUdp.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 177);

unsigned int localPort = 8888;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged";        // a string to send back
String wordCompare = "jook";
int upperValveRelay = 8;
int lowerValveRelay = 9;

const int stepPin = 6; 
const int dirPin = 5; 
const int enPin = 7;

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  pinMode(upperValveRelay, OUTPUT);
  pinMode(lowerValveRelay, OUTPUT);
  digitalWrite(upperValveRelay, HIGH);
  digitalWrite(lowerValveRelay, HIGH);

  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(enPin,OUTPUT);
  
  startEthernet();
  valveClosed("lower", true);
  initSyringe();
}

void loop() {
  motorEndReached();
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remote = Udp.remoteIP();
    for (int i=0; i < 4; i++) {
      Serial.print(remote[i], DEC);
      if (i < 3) {
        Serial.print(".");
      }
    }
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    String packetToString(packetBuffer);
    Serial.println("Contents:");
    Serial.println(packetBuffer);
    
    if(packetToString == wordCompare) {
      
    }

    /*Commented out, because it crashes router*/
    // send a reply to the IP address and port that sent us the packet we received
    // Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    // Udp.write(ReplyBuffer);
    // Udp.endPacket();
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

bool motorEndReached() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);
  if (sensorValue <= 10) {
    return true;
  } else {
    return false;
  }
}

/*!
 * Controls the valves
 * param valve - choose the valve to use
 * param state - true if valve is closed, false if valve is open
 */
void valveClosed(String valve, bool state) {
  int relay = 0;
  if (valve == "upper") {
    relay = upperValveRelay;
  } else if (valve == "lower") {
    relay = lowerValveRelay;
  }
  
  if (state == true) {
    digitalWrite(relay, HIGH);
  } else if (state == false) {
    digitalWrite(relay, LOW);
  }
}

/*!
 * Initializes the syringes on first startup.
 * Pulls both syringes up until first syringe reaches
 * end-state detection sensor. Second syringe has to be
 * adjusted to the same level as the first one for it to work.
 */
void initSyringe() {
  digitalWrite(dirPin,HIGH);
  bool endReached = false;
  int sensorValue = 0;
  while (!endReached) {
    sensorValue = analogRead(A0);
    digitalWrite(stepPin,HIGH); 
    delayMicroseconds(500); 
    digitalWrite(stepPin,LOW); 
    delayMicroseconds(500);
    if (sensorValue < 10) {
      endReached = true;
    }
  }
  delay(500);
  
  digitalWrite(dirPin,LOW);
  for(int x = 0; x < 10800; x++) {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin,LOW);
    delayMicroseconds(500);
  }
  delay(500);

  digitalWrite(dirPin,HIGH);
  for(int x = 0; x < 6000; x++) {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin,LOW);
    delayMicroseconds(500);
  }
}




/*
  Processing sketch to run with this example
 =====================================================

 // Processing UDP example to send and receive string data from Arduino
 // press any key to send the "Hello Arduino" message


 import hypermedia.net.*;

 UDP udp;  // define the UDP object


 void setup() {
 udp = new UDP( this, 6000 );  // create a new datagram connection on port 6000
 //udp.log( true ); 		// <-- printout the connection activity
 udp.listen( true );           // and wait for incoming message
 }

 void draw()
 {
 }

 void keyPressed() {
 String ip       = "192.168.1.177";	// the remote IP address
 int port        = 8888;		// the destination port

 udp.send("Hello World", ip, port );   // the message to send

 }

 void receive( byte[] data ) { 			// <-- default handler
 //void receive( byte[] data, String ip, int port ) {	// <-- extended handler

 for(int i=0; i < data.length; i++)
 print(char(data[i]));
 println();
 }
 */
