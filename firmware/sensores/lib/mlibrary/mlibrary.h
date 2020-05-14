//Vprocess5 - Uchile: 13 mayo 2020. Felipe Hooper
#include "Arduino.h"
#include <Wire.h>
#include "Adafruit_ADS1015.h"
Adafruit_ADS1115 ads1(0x49);
//Adafruit_ADS1115 ads2(0x49);


#define rtd1 102
//#define rtd2 103

#define  INT(x)   (x-48)  //ascii convertion
#define iINT(x)   (x+48)  //inverse ascii convertion

#define SPEED_MIN 2.0
#define SPEED_MAX 150     //[RPM]
#define TEMP_MAX  60      //[ºC]


String message     = "";
String new_write   = "";
String new_write_w = "wf000u000t000r111e0f0.0t18.1\n";
//String new_write_t = "20.0\n";

boolean stringComplete = false;  // whether the string is complete

//RESET SETUP
uint8_t rst1 = 1;  uint8_t rst2 = 1;  uint8_t rst3 = 1;

//DIRECTION SETUP
char dir1 = 1;  char dir2 = 1;  char dir3 = 1;

// for incoming serial data
float Byte0 = 0;  char cByte0[15] = "";  //por que no a 16?
float Byte1 = 0;  char cByte1[15] = "";
float Byte2 = 0;  char cByte2[15] = "";
float Byte3 = 0;  char cByte3[15] = "";
float Byte4 = 0;  char cByte4[15] = "";
float Byte5 = 0;  char cByte5[15] = "";
float Byte6 = 0;  char cByte6[15] = "";
float Byte7 = 0;  char cByte7[15] = "";  //for Temp2
//nuevo
float Byte8 = 0;  char cByte8[15] = "";  //for setpont confirmation //no se necesita: 28-9-19

//calibrate function()
char  var = '0';
float umbral_a, umbral_b, umbral_temp;


float m = 0;
float n = 0;

//pH=:(m0,n0)
float m0 = +0.864553;//+0.75;
float n0 = -3.634006;//-3.5;

//oD=:(m1,n1)
float m1 = +6.02;
float n1 = -20.42;

//Temp1=:(m2,n2)
float m2 = +14.95; //vrer= 2.5   //vref=5   8.58;//11.0;//+5.31;
float n2 = -91.67; //vref= 2.5   //vref=5  -68.89;//-106.86;//-42.95;


float Iph = 0;
float Iod = 0;

//   DEFAULT:
float pH    = m0*Iph    + n0;      //   ph = 0.75*IpH   - 3.5
float oD    = m1*Iod    + n1;
float Temp1 = 0;
float Temp_ = Temp1;//0.5 * ( Temp1 + Temp2 );

//float flujo = 0.0;

byte received_from_computer = 0; //we need to know how many characters have been received.
byte serial_event = 0;           //a flag to signal when data has been received from the pc/mac/other.
byte code = 0;                   //used to hold the I2C response code.

char RTD_data[20];
char RTD_data1[20];
char RTD_data2[20];               //we make a 20 byte character array to hold incoming data from the RTD circuit.

byte in_char = 0;                //used as a 1 byte buffer to store in bound bytes from the RTD Circuit.
byte i = 0;                      //counter used for RTD_data array.

int time_ = 600;                 //used to change the delay needed depending on the command sent to the EZO Class RTD Circuit.


void rtd1_sensor() {
  Wire.beginTransmission(rtd1);                                                 //call the circuit by its ID number.
  Wire.write('r');
  Wire.endTransmission();                                                       //end the I2C data transmission.

  if (strcmp('r', "sleep") != 0) {                                              //if the command that has been sent is NOT the sleep command, wait the correct amount of time and request data.
    delay(time_);                                                               //wait the correct amount of time for the circuit to complete its instruction.
    Wire.requestFrom(rtd1, 20, 1);                                              //call the circuit and request 20 bytes (this may be more than we need)
    code = Wire.read();                                                         //the first byte is the response code, we read this separately.

    while (Wire.available()) {
      in_char = Wire.read();
      RTD_data1[i] = in_char;
      i += 1;
      if (in_char == 0) {
        i = 0;
        break;
      }
    }
    Temp1 = atof(RTD_data1);
  }
  serial_event = false;                   //reset the serial event flag.
  return;
}

/*
void rtd2_sensor() {
  Wire.beginTransmission(rtd2);                                                 //call the circuit by its ID number.
  Wire.write('r');
  Wire.endTransmission();                                                       //end the I2C data transmission.

  if (strcmp('r', "sleep") != 0) {                                              //if the command that has been sent is NOT the sleep command, wait the correct amount of time and request data.
    delay(time_);                                                               //wait the correct amount of time for the circuit to complete its instruction.
    Wire.requestFrom(rtd2, 20, 1);                                              //call the circuit and request 20 bytes (this may be more than we need)
    code = Wire.read();                                                         //the first byte is the response code, we read this separately.

    while (Wire.available()) {            //are there bytes to receive.
      in_char = Wire.read();              //receive a byte.
      RTD_data2[i] = in_char;              //load this byte into our array.
      i += 1;                             //incur the counter for the array element.
      if (in_char == 0) {                 //if we see that we have been sent a null command.
        i = 0;                            //reset the counter i to 0.
        break;                            //exit the while loop.
      }
    }
    Temp2 = atof(RTD_data2);
  }
  serial_event = false;                   //reset the serial event flag.
  return;
}
*/
rtds_sensors(){
  rtd1_sensor();
  //rtd2_sensor();
  return Temp_ = Temp1;//Temp_ = 0.5 * (Temp1 + Temp2);
}


void calibrate_sensor() {
  // comunicacion a sensor 1
  Wire.beginTransmission(rtd1);
  Wire.write("cal,25.0");
  Wire.endTransmission();                                                       //end the I2C data transmission.

  if (strcmp("cal,25.0", "sleep") != 0) {                                     //if the command that has been sent is NOT the sleep command, wait the correct amount of time and request data.
    delay(time_);                                                               //wait the correct amount of time for the circuit to complete its instruction.
    Wire.requestFrom(rtd1, 20, 1);                                              //call the circuit and request 20 bytes (this may be more than we need)
    code = Wire.read();                                                         //the first byte is the response code, we read this separately.

    while (Wire.available()) {            //are there bytes to receive.
      in_char = Wire.read();              //receive a byte.
      RTD_data[i] = in_char;              //load this byte into our array.
      i += 1;                             //incur the counter for the array element.
      if (in_char == 0) {                 //if we see that we have been sent a null command.
        i = 0;                            //reset the counter i to 0.
        break;                            //exit the while loop.
      }
    }
  }
/*
  // comunicacion a sensor 2
  Wire.beginTransmission(rtd2);
  Wire.write("cal,25.5");
  Wire.endTransmission();                                                       //end the I2C data transmission.

  if (strcmp("cal,25.5", "sleep") != 0) {                                     //if the command that has been sent is NOT the sleep command, wait the correct amount of time and request data.
    delay(time_);                                                               //wait the correct amount of time for the circuit to complete its instruction.
    Wire.requestFrom(rtd2, 20, 1);                                              //call the circuit and request 20 bytes (this may be more than we need)
    code = Wire.read();                                                         //the first byte is the response code, we read this separately.

    while (Wire.available()) {            //are there bytes to receive.
      in_char = Wire.read();              //receive a byte.
      RTD_data[i] = in_char;              //load this byte into our array.
      i += 1;                             //incur the counter for the array element.
      if (in_char == 0) {                 //if we see that we have been sent a null command.
        i = 0;                            //reset the counter i to 0.
        break;                            //exit the while loop.
      }
    }
  }
  */
  serial_event = false;                   //reset the serial event flag.
  return;
}


//for hardware serial
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    message += inChar;
    if (inChar == '\n') {
      serial_event = true;
    }
  }
}

void i2c_send_command(String command, uint8_t slave) {   //slave = 2: slave tradicional. 3 es el nuevo
  Wire.beginTransmission(slave); // transmit to device #slave: [2,3]
  Wire.write(command.c_str());   // sends value byte
  Wire.endTransmission();        // stop transmitting
}


//modifica los umbrales de cualquiera de los dos actuadores
void actuador_umbral(){
  //setting threshold ph: u1a160b141e
  if ( message[1] == '1' ) {

    umbral_a = 0; umbral_b = 0;
    umbral_a = message.substring(3,6).toFloat();
    umbral_b = message.substring(7,10).toFloat();

    if ( umbral_a <= SPEED_MIN )
      umbral_a = SPEED_MIN;
    else if ( umbral_a >= SPEED_MAX )
      umbral_a = SPEED_MAX;

    if ( umbral_b <= SPEED_MIN )
      umbral_b = SPEED_MIN;
    else if ( umbral_b >= SPEED_MAX )
      umbral_b = SPEED_MAX;
  }
  //setting threshold temp: u2t011e
  else if ( message[1] == '2' ) {
    umbral_temp = 0;
    umbral_temp = message.substring(3,6).toFloat();

    if ( umbral_temp <= SPEED_MIN )
      umbral_temp = SPEED_MIN;
    else if ( umbral_temp >= SPEED_MAX)
      umbral_temp = SPEED_MAX;
    else
      umbral_temp = umbral_temp;
  }
  Serial.println( "Umbral_Temp: " + String(umbral_temp) );
  //Serial.println( String(umbral_a) + '_' + String(umbral_b) + '_' + String(umbral_temp) );
  return;
}



void tx_reply(){
  //tx of measures
  Serial.print(cByte0);  Serial.print("\t");
  Serial.print(cByte1);  Serial.print("\t");
  Serial.print(cByte2);  Serial.print("\t");
  Serial.print(cByte3);  Serial.print("\t");
  Serial.print(cByte4);  Serial.print("\t");
  //Serial.print(cByte5);  Serial.print("\t");
  //Serial.print(cByte6);  Serial.print("\t");
  //Serial.print(cByte7);  Serial.print("\t");
//nuevo
  Serial.println(new_write);    // Serial.print("\t");
  //Serial.print(new_write_w);  Serial.print("\t");
  //Serial.println(message);
  //Serial.print("\n");
}

void daqmx() {
  //data adquisition measures
  Byte0 = Temp_;
  Byte1 = pH;
  Byte2 = oD;
  Byte3 = 0;//Iph;
  Byte4 = 0;//Iod;
  //Byte5 = 0;//Itemp1;
  //Byte6 = 0;//Itemp2;
  //Byte7 = flujo;

  dtostrf(Byte0, 7, 2, cByte0);
  dtostrf(Byte1, 7, 2, cByte1);
  dtostrf(Byte2, 7, 2, cByte2);
  dtostrf(Byte3, 7, 2, cByte3);
  dtostrf(Byte4, 7, 2, cByte4);
  //dtostrf(Byte5, 7, 2, cByte5);
  //dtostrf(Byte6, 7, 2, cByte6);
  //dtostrf(Byte7, 7, 2, cByte7);
  //dtostrf(Byte8, 7, 2, cByte8);

  tx_reply();
  return;
}

//Re-transmition commands to slave micro controller
void broadcast_setpoint(uint8_t select) {
  switch (select) {
    case 0: //only re-tx and update uset's.
      //se actualiza medicion de temperatura para enviarla a uc_slave
      new_write = "";
      //new_write = new_write_w;
      new_write = new_write_w.substring(0,23) + "t" + String(Temp_) + "\n";
      i2c_send_command(new_write, 2); //va hacia uc_slave
      break;

    case 1: //update command and re-tx.
      new_write_w  = "";
      new_write_w  = message.substring(0,23) + "t" + String(Temp_) + "\n";  //message;// + "\n";
      i2c_send_command(new_write_w, 2);
      break;

    default:
      break;
  }
  return;
}

//Esquema I2C Concha y Toro:
//TRAMA-Proceso  : wf000u000t009r000e1f0.2

void clean_strings() {
  //clean strings
  serial_event = false;
  message   = "";
}

// Validate and crumble SETPOINT
int validate() {
    //message format write values: wf100u100t150r111d111
    if ( message[0] == 'w' ) {
            rst1 = int(INT(message[14]));  //rst_feed
            rst2 = int(INT(message[15]));  //rst_unload
            rst3 = int(INT(message[16]));  //rest_temp
            return 1;
    }
    // Validate CALIBRATE
    else if ( message[0]  == 'c' )
            return 1;

    //Validete umbral actuador temp: u2t003e
    else if ( message[0] == 'u' && message[1] == '2' &&
              message[2] == 't' && message[6] == 'e'
            )
          return 1;

   // Validate READING
    else if ( message[0] == 'r' )
          return 1;

    // NOT VALIDATE
    else
          return 0;
}
