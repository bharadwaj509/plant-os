// Author: Devtar Singh <devtar@plant-os.com>
// Github: https://github.com/plantOS/plant-os
// License: MIT (Open Source)

//#include <Wire.h>                              // enable I2C.
#include <Bridge.h>
#include <Process.h>                           // Yun Process, to run processes on the Yun's linux 
#include <SoftwareSerial.h>         //Include the software serial library  
SoftwareSerial sSerial(11, 10);     // RX, TX  - Name the software serial library sftSerial (this cannot be omitted)
                                    // assigned to pins 10 and 11 for maximum compatibility

//Core sensor parameters

#define NUM_CIRCUITS 4                         // <-- CHANGE THIS  set how many I2C circuits are attached to the Tentacle

const unsigned int send_readings_every = 2000; // set at what intervals the readings are sent to the computer (NOTE: this is not the frequency of taking the readings!)
unsigned long next_serial_time;

#define baud_circuits 9600         // NOTE: older circuit versions have a fixed baudrate (e.g. 38400. pick a baudrate that all your circuits understand and configure them accordingly)
int s0 = 7;                         // Tentacle uses pin 7 for multiplexer control S0
int s1 = 6;                         // Tentacle uses pin 6 for multiplexer control S1
int enable_1 = 5;              // Tentacle uses pin 5 to control pin E on shield 1
int enable_2 = 4;             // Tentacle uses pin 4 to control pin E on shield 2
char sensordata[30];                          // A 30 byte character array to hold incoming data from the sensors
byte sensor_bytes_received = 0;               // We need to know how many characters bytes have been received
byte code = 0;                                // used to hold the I2C response code.
byte in_char = 0;                             // used as a 1 byte buffer to store in bound bytes from the I2C Circuit.
//stub
//int channel_ids[] = {97, 98, 99, 100};        // <-- CHANGE THIS. A list of I2C ids that you set your circuits to.
String channel_names[] = {"PH", "ORP", "DO", "EC"};   // <-- CHANGE THIS. A list of channel names (must be the same order as in channel_ids[]) - only used to give a name to the "Signals" sent to initialstate.com
String sensor_data[4];                        // an array of strings to hold the readings of each channel
//char *channel_names[] = {"PH", "ORP", "DO", "EC"};   // <-- CHANGE THIS. A list of channel names (this list should have TOTAL_CIRCUITS entries)
//String readings[NUM_CIRCUITS];                // an array of strings to hold the readings of each channel
int channel = 0;                              // INT pointer to hold the current position in the channel_ids/channel_names array

const unsigned int reading_delay = 1000;      // time to wait for the circuit to process a read command. datasheets say 1 second.
unsigned long next_reading_time;              // holds the time when the next reading should be ready from the circuit
boolean request_pending = false;              // wether or not we're waiting for a reading
const unsigned int blink_frequency = 250;     // the frequency of the led blinking, in milliseconds
unsigned long next_blink_time;                // holds the next time the led should change state
boolean led_state = LOW;                      // keeps track of the current led state

const unsigned int cloud_update_interval = 10000; // time to wait for the circuit to process a read command. datasheets say 1 second.
unsigned long next_cloud_update;              // holds the time when the next reading should be ready from the circuit

// Temp parameters
float temp;
char temp_buffer[4];

// Extended EC parameters
char ec_data[48];                             // we make a 48 byte character array to hold incoming data from the EC circuit.
byte i = 0;
char *ec,*tds,*sal,*sg;                       // global tds,sal,sg values

// INITIAL STATE CONFIGURATION
#define ISBucketURL "https://groker.initialstate.com/api/buckets" // bucket creation url. 
#define ISEventURL  "https://groker.initialstate.com/api/events"  // data destination. Thanks to Yun's linux, we can use SSL, yeah :)       
String bucketKey = "bucketKey";                                 // unique identifier for your bucket (unique in the context of your access key)
String bucketName = "plantOS";                  // Bucket name. Will be used to label your Bucket in the initialstate website.
String accessKey = "accessKey";       // <-- CHANGE THIS Access key (copy/paste from your initialstate account settings page)

void setup() {
  pinMode(13, OUTPUT);                                 // set the led output pin
  pinMode(s0, OUTPUT);                        // set the digital output pins for the serial multiplexer
  pinMode(s1, OUTPUT);
  pinMode(enable_1, OUTPUT);
  pinMode(enable_2, OUTPUT);  
  Bridge.begin();
  Serial.begin(9600);                                     // initialize serial communication over network:
  //while (!Console) ;                                   // wait for Console port to connect.
  sSerial.begin(baud_circuits);               // Set the soft serial port to 9600 (change if all your devices use another baudrate)
  Serial.println("Initialising plantOS console...");
  delay(1000);                                         // Time for the ethernet shield to boot
  createBucket();                                      // create a bucket, if it doesn't exist yet
  next_serial_time = millis() + send_readings_every*3; // wait a little longer before sending serial data the first time
  next_cloud_update = millis() + cloud_update_interval;
}
//stub2
void loop() {
  updateSensors();                // read / write to the sensors. returns fast, does not wait for the data to arrive
  updateSerial();                 // write sensor data to the serial port
  updateCloud();                  // send the sensor data to the cloud. returns fast, except when a cloud update is due.
}

void updateCloud() {
  
  if (millis() >= next_cloud_update) {                // is it time for the next serial communication?
    sendData_new();
    next_cloud_update = millis() + cloud_update_interval;
  }
}

// do serial communication in a "asynchronous" way
void updateSerial() {
  if (millis() >= next_serial_time) {              // is it time for the next serial communication?
    Serial.println("---------------");
    for (int i = 0; i < NUM_CIRCUITS; i++) {       // loop through all the sensors
      Serial.print(channel_names[i]);             // print channel name
      Serial.print(":\t");
      Serial.println(sensor_data[i]);             // print the actual reading
    }
    temp = read_temp();
    Serial.print("Temperature: ");
    Serial.println(temp);
    next_serial_time = millis() + send_readings_every;
  }
}

// take sensor readings in a "asynchronous" way
void updateSensors() {
  if (request_pending) {                          // is a request pending?
    if (millis() >= next_reading_time) {          // is it time for the reading to be taken?
      receiveReading();                           // do the actual I2C communication
    }
  } else {                                        // no request is pending,
    channel = (channel + 1) % NUM_CIRCUITS;       // switch to the next channel (increase current channel by 1, and roll over if we're at the last channel using the % modulo operator)
    requestReading();                             // do the actual I2C communication
  }
}

// Request a reading from the current channel
void requestReading() {
  request_pending = true;
  Wire.beginTransmission(channel_ids[channel]); // call the circuit by its ID number.
  Wire.write('r');                    // request a reading by sending 'r'
  Wire.endTransmission();                   // end the I2C data transmission.
  next_reading_time = millis() + reading_delay; // calculate the next time to request a reading
}

// Receive data from the I2C bus
void receiveReading() {
  sensor_bytes_received = 0;                        // reset data counter
  memset(sensordata, 0, sizeof(sensordata));        // clear sensordata array;

  Wire.requestFrom(channel_ids[channel], 48, 1);    // call the circuit and request 48 bytes (this is more then we need).
  code = Wire.read();
  
  while (Wire.available()) {          // are there bytes to receive?
    in_char = Wire.read();            // receive a byte.
    if (channel == 3){
      ec_data[i] = in_char;
      i += 1;
    }
    if (in_char == 0) {               // if we see that we have been sent a null command.
      Wire.endTransmission();         // end the I2C data transmission.
      i = 0;
      break;                          // exit the while loop, we're done here
    }
    else {
      sensordata[sensor_bytes_received] = in_char;  // load this byte into our array.
      sensor_bytes_received++;
    }
  }
  
  char *filtered_sensordata;                     // pointer to hold a modified version of the data
  filtered_sensordata = strtok (sensordata,","); // we split at the first comma - needed for the ec stamp only

  switch (code) {                       // switch case based on what the response code is.
    case 1:                             // decimal 1  means the command was successful.
      sensor_data[channel] = filtered_sensordata;
      break;                              // exits the switch case.

    case 2:                             // decimal 2 means the command has failed.
      Serial.print("channel \"");
      Serial.print( channel_names[channel] );
      Serial.println ("\": command failed");
      break;                              // exits the switch case.

    case 254:                           // decimal 254  means the command has not yet been finished calculating.
      Serial.print("channel \"");
      Serial.print( channel_names[channel] );
      Serial.println ("\": reading not ready");
      break;                              // exits the switch case.

    case 255:                           // decimal 255 means there is no further data to send.
      Serial.print("channel \"");
      Serial.print( channel_names[channel] );
      Serial.println ("\": no answer");
      break;                              // exits the switch case.
  }
  request_pending = false;                  // set pending to false, so we can continue to the next sensor
}

// make a HTTP connection to the server and send create the InitialState Bucket (if it doesn't exist yet)
void createBucket()
{
  // Initialize the process
  Process isbucket;

  isbucket.begin("curl");
  isbucket.addParameter("-k");    // we use SSL, but we bypass certificate verification! 
  isbucket.addParameter("-v");
  isbucket.addParameter("-X");
  isbucket.addParameter("POST");
  isbucket.addParameter("-H");
  isbucket.addParameter("Content-Type:application/json");
  isbucket.addParameter("-H");
  isbucket.addParameter("Accept-Version:0.0.4");

  // IS Access Key Header
  isbucket.addParameter("-H");
  isbucket.addParameter("X-IS-AccessKey:" + accessKey);

  // IS Bucket Key Header
  isbucket.addParameter("-d");
  isbucket.addParameter("{\"bucketKey\": \"" + bucketKey + "\", \"bucketName\": \"" + bucketName + "\"}");
  
  isbucket.addParameter(ISBucketURL);
  
  // Run the process
  isbucket.run();
}

void sendData_new()
{
  Process isstreamer;
  String newjson="https://groker.initialstate.com/api/events?accessKey="+accessKey+"&bucketKey="+bucketKey;
  ec_string_pars();
  newjson+= "&DO="+sensor_data[0];
  newjson+= "&ORP="+sensor_data[1];
  newjson+= "&PH="+sensor_data[2];
  newjson+= "&EC="+sensor_data[3];
  newjson+= "&Temp="+temp_string();
  newjson+= "&TDS="+String(tds);
  newjson+= "&Sal="+String(sal);
  newjson+= "&GRV="+String(sg);
  
  isstreamer.begin("curl");
  //isstreamer.addParameter("Accept-Version:0.0.4");  
  isstreamer.addParameter(newjson);
  // Print posted data for debug
  Serial.print("Sending data: ");
  Serial.println(newjson);
  isstreamer.run();
}

void ec_string_pars() {
 ec = strtok(ec_data, ",");
 tds = strtok(NULL, ",");
 sal = strtok(NULL, ",");
 sg = strtok(NULL, ",");
}

String temp_string() {
  char charVal[10];               //temporarily holds data from vals 
  String stringVal = "";     //data on buff is copied to this string
  
  dtostrf(temp, 4, 2, charVal);  //4 is mininum width, 4 is precision; float value is copied onto buff
  for(int i=0;i<5;i++)
  {
    stringVal+=charVal[i];
  }
  return stringVal;
}

float read_temp(void){
    float v_out;
    float temp;
    digitalWrite(A1, LOW);
    digitalWrite(12, HIGH);
    delay(2);
    v_out = analogRead(1);
    digitalWrite(12, LOW);
    v_out*=.0048;
    v_out*=1000;
    temp= 0.0512 * v_out -20.5128;
    return temp;
}

// Open a channel via the Tentacle serial multiplexer
void open_channel() {

  switch (channel) {

    case 0:                                  // if channel==0 then we open channel 0
      digitalWrite(enable_1, LOW);           // setting enable_1 to low activates primary channels: 0,1,2,3
      digitalWrite(enable_2, HIGH);          // setting enable_2 to high deactivates secondary channels: 4,5,6,7
      digitalWrite(s0, LOW);                 // S0 and S1 control what channel opens
      digitalWrite(s1, LOW);                 // S0 and S1 control what channel opens
      break;

    case 1:
      digitalWrite(enable_1, LOW);
      digitalWrite(enable_2, HIGH);
      digitalWrite(s0, HIGH);
      digitalWrite(s1, LOW);
      break;

    case 2:
      digitalWrite(enable_1, LOW);
      digitalWrite(enable_2, HIGH);
      digitalWrite(s0, LOW);
      digitalWrite(s1, HIGH);
      break;

    case 3:
      digitalWrite(enable_1, LOW);
      digitalWrite(enable_2, HIGH);
      digitalWrite(s0, HIGH);
      digitalWrite(s1, HIGH);
      break;

    case 4:
      digitalWrite(enable_1, HIGH);
      digitalWrite(enable_2, LOW);
      digitalWrite(s0, LOW);
      digitalWrite(s1, LOW);
      break;

    case 5:
      digitalWrite(enable_1, HIGH);
      digitalWrite(enable_2, LOW);
      digitalWrite(s0, HIGH);
      digitalWrite(s1, LOW);
      break;

    case '6':
      digitalWrite(enable_1, HIGH);
      digitalWrite(enable_2, LOW);
      digitalWrite(s0, LOW);
      digitalWrite(s1, HIGH);
      break;

    case 7:
      digitalWrite(enable_1, HIGH);
      digitalWrite(enable_2, LOW);
      digitalWrite(s0, HIGH);
      digitalWrite(s1, HIGH);
      break;
  }
}