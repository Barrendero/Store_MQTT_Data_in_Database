//**MONITOREO OXÍGENO MQTT**
// Código destinado al sensado del oxígeno en base a la señal digital
// del sensor de nivel, así como para el envío de datos vía MQTT

// Comunicacion librerias

#include "WiFi.h"

// OX
#include <SoftwareSerial.h>
#include <PubSubClient.h>

#define rx 33                                          //define what pin rx is going to be yellow
#define tx 32                                          //define what pin tx is going to be orange
SoftwareSerial myserial(rx, tx);                      //define how the soft serial port is going to work
#define CON 2

// Global Variables
String inputstring = "";                              //a string to hold incoming data from the PC
String sensorstring = "";                             //a string to hold the data from the Atlas Scientific product
boolean input_string_complete = false;                //have we received all the data from the PC
boolean sensor_string_complete = false;               //have we received all the data from the Atlas Scientific product
float DO;                                             //used to hold a floating point number that is the DO
int ox;
int OxCopia = 0;

// ud
static const int dc_pin = 26;
int val = 0;
static byte TheState = 1;
int ud = 0;

// Comunicacion definiciones

const char* ssid = "INFINITUMC47D";
const char* password = "CRYPTELMEX1";

char* server = "inserta tu broker";
int port = 1883;

float var = 0;
char datosOx[40];
char datosUd[40];

WiFiClient Cliente;
PubSubClient mqttClient(Cliente);

//Iniciar MQTT
void wifiInit(){
  Serial.print("Conectandose a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status () != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("Conectado a WiFi:");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

// Prepare OD EMBEDDED
void serialEvent() {                                  //if the hardware serial port_0 receives a char
  inputstring = Serial.readStringUntil(13);           //read the string until we see a <CR>
  input_string_complete = true;                       //set the flag used to tell if we have received a completed string from the PC
}

// Task GETOX
void GetOx(void *parameters) {
  while(1){
  // GetOx
  if (TheState == 1){                                 // If humidity level sensor is into the water
        ud = 0;                                       // Then ud is equal to 0 and do the measurement process
    if (input_string_complete == true) {                //if a string from the PC has been received in its entirety
    myserial.print(inputstring);                      //send that string to the Atlas Scientific product
    myserial.print('\r');                             //add a <CR> to the end of the string
    inputstring = "";                                 //clear the string
    input_string_complete = false;                    //reset the flag used to tell if we have received a completed string from the PC
  }

  if (myserial.available() > 0) {                     //if we see that the Atlas Scientific product has sent a character
    char inchar = (char)myserial.read();              //get the char we just received
    sensorstring += inchar;                           //add the char to the var called sensorstring
    if (inchar == '\r') {                             //if the incoming character is a <CR>
      sensor_string_complete = true;                  //set the flag
    }
  }


  if (sensor_string_complete == true) {               //if a string from the Atlas Scientific product has been received in its entirety
    //Serial.println(sensorstring);                   // Print sensor string
    ox = sensorstring.toInt(); 
    OxCopia = ox;
       if(OxCopia > 2){
       Serial.println("El oxígeno es: " + String(OxCopia));
        //sensor_string_complete = false;+
       sprintf(datosOx, "Oxigeno: %d ", OxCopia);    // Prepare data to put in on Char variable
       sprintf(datosUd, "Mstate: %d ", ud);          // Same idea of the row above
       mqttClient.publish("Test/Oxigeno", datosOx);  // Publish data on MQTT broker using datosOx Char variable
       mqttClient.publish("Test/Oxigeno", datosUd);  // Same idea of the row above
       Serial.println("Datos enviados al broker");   // Tell us data has benn succesfully sended
       vTaskDelay(10000 / portTICK_PERIOD_MS);
       }

    //listo_para_enviar = true;
    sensorstring = "";                                //clear the string
    sensor_string_complete = false;                   //reset the flag used to tell if we have received a completed string from the Atlas Scientific product
    
  }
 }
  else{                                               // If humidity sensor is out
    ud = 1;                                           // Then communicate to broker system is not measuring
   vTaskDelay(1000 / portTICK_PERIOD_MS);
   sprintf(datosUd, "Mstate: %d ", ud);               // Prepare data to put it into char variable
   mqttClient.publish("Test/Oxigeno", datosUd);       // Publish data on MQTT broker using datosUs as char variable
   Serial.println("Datos enviados al broker");        // Tell us data has benn succesfully sended
  }
 }
}


void setup() {
  // Initialize Serial and settings
  Serial.begin(9600);
  wifiInit();
  mqttClient.setServer(server, port);
  myserial.begin(9600);                               //set baud rate for the software serial port to 9600
  inputstring.reserve(10);                            //set aside some bytes for receiving data from the PC
  sensorstring.reserve(30);                           //set aside some bytes for receiving data from Atlas Scientific product
  pinMode(dc_pin, INPUT);
  pinMode(CON, OUTPUT);
  digitalWrite(CON, LOW);

  // Wait a moment to start
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---CONTROL DEL MOTOR TEST---");

  // TASKS
 // Task Send data low priority
 xTaskCreatePinnedToCore(GetOx,
                         "Send Data",
                         2048,
                         NULL,
                         1,
                         NULL,
                         1);




}

void loop(){
   val = digitalRead(dc_pin);
   if(val == 0){
     TheState = 1;
     Serial.println("\t\t\tEl valor es: " + String(val));
     Serial.println("\t\t\tEl sensor está dentro!");
      vTaskDelay(2000 / portTICK_PERIOD_MS);
   }if(val == 1){
    TheState = 2;
    Serial.println("\t\t\tEl valor es: " + String(val));
    Serial.println("\t\t\tEl sensor está afuera!");
     vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
 }
