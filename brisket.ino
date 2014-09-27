/*
 * Brisket Arduino Temperature Publisher
 *
 * Publishes thermistor probe temperatures to a sinatra backend via POST
 * Built for Rugged Circuits Yellow Jacket
 */
#include <math.h>
#include <WiShield.h>
#include <WiServer.h>

#define WIRELESS_MODE_INFRA	1
#define WIRELESS_MODE_ADHOC	2

// Wireless configuration parameters ----------------------------------------
unsigned char local_ip[] = {172,24,79,59};	// IP address of WiShield
unsigned char gateway_ip[] = {172,24,79,1};	// router or gateway IP address
unsigned char subnet_mask[] = {255,255,255,0};	// subnet mask for the local network
const prog_char ssid[] PROGMEM = {""};		// max 32 bytes

unsigned char security_type = 3; // 0 - open; 1 - WEP; 2 - WPA; 3 - WPA2

// WPA/WPA2 passphrase
const prog_char security_passphrase[] PROGMEM = {""}; // max 64 characters

prog_uchar wep_keys[] PROGMEM = {
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,  // Key 0
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Key 1
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Key 2
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // Key 3
};

// setup the wireless mode
// infrastructure - connect to AP
// adhoc - connect to another WiFi device
unsigned char wireless_mode = WIRELESS_MODE_INFRA;

unsigned char ssid_len;
unsigned char security_passphrase_len;
//---------------------------------------------------------------------------
// Variable Declarations

// What pin to connect the sensor to
#define PROBE0 A5
#define PROBE1 A4

// Termistor constants
const float beta = 4198,
            r0   = 100000,
            t0   = 298,
            pad  = r0;

// Time (in millis) when the data should be retrieved
long updateTime = 0;

// Sinatra backend IP
uint8 brisket_ip[] = {172,24,79,103};

// Post request to send
POSTrequest sendInfo(brisket_ip, 4567, "172.24.79.103:4567", "/publish", checkTemp);

//---------------------------------------------------------------------------
// Functions

int calculateTemperature(int RawADC) {
  long Resistance;
  float Temp;
  
  float a = (1/t0)-(1/beta)*log(r0);
  float b = 1/beta;
  float c = 0;
  
  Resistance = ((1024 * pad / RawADC) - pad);
  Temp = log(Resistance);
  Temp = 1 / (a + (b * Temp) + (c * Temp * Temp * Temp));
  Temp = Temp - 273.15;
  Temp = (Temp * 9.0)/ 5.0 + 32.0;
  return (int) Temp;
}

void checkTemp(){
  int probe0 = analogRead(PROBE0);
  int probe1 = analogRead(PROBE1);
  
  int probe0_f = calculateTemperature(probe0);
  int probe1_f = calculateTemperature(probe1);
  
  Serial.print("Probe 0: ");
  Serial.print(probe0_f);
  Serial.print(" deg f \n");
  
  Serial.print("Probe 1: ");
  Serial.print(probe1_f);
  Serial.print(" deg f \n");
  
  String urlencoded = String("probe0=" + String(probe0_f) + "&probe1=" + String(probe1_f));
  
  WiServer.print(urlencoded);
}

void setup() { 
  WiFi.init();
  WiServer.init(NULL);
  Serial.begin(9600);
  WiServer.enableVerboseMode(false);
  sendInfo.setReturnFunc(printData);
}

void loop()
{
  // Check if it's time to get an update
  if (millis() >= updateTime)
  {
    //Serial.println("Start POST");
    sendInfo.submit();
    //Serial.println("End POST");
    updateTime += 1000 * 10;
    //Serial.println(updateTime);
  }
  
  WiServer.server_task();
  WiFi.run();
  delay(10);
}