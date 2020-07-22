//Vprocess5 - Uchile: 13 mayo 2020. Felipe Hooper
#include "Arduino.h"
#include <Wire.h>
#include "Adafruit_ADS1015.h"
Adafruit_ADS1115 ads1(0x48);  //en placa sensores el de arriba es 49(address esta conectado a vdd en el ads de arriba en la placa) y el de abajo es 48 (address conectado a nada)
//Adafruit_ADS1115 ads1(0x49);
#include "rgb_lcd.h"
rgb_lcd lcd;

const int colorR = 255;
const int colorG = 0;
const int colorB = 0;

#define rtd1 101
#define rtd2 102

#define  INT(x)   (x-48)  //ascii convertion
#define iINT(x)   (x+48)  //inverse ascii convertion

#define SPEED_MIN 2.0
#define SPEED_MAX 150     //[RPM]
#define TEMP_MAX  60      //[ºC]

//INA169 + ADS1115: Factores para normalizar mediciones.
#define PGA1 0.125F
#define PGA2 0.0625F
const int VOLTAGE_REF  = 5;    // before: 5  // Reference voltage for analog read
const int RS = 10;             // Shunt resistor value (in ohms)
#define mA 1000.0
#define K 1.0 / (10.0 * RS )
#define alpha 0.35F
//INA169 + ADS1115: Factores para normalizar mediciones.


//#define REMONTAJE_PIN  A0 //bomba remontaje
#define AGUA_FRIA      A1 //D10 = rele 1 (cable rojo)
#define AGUA_CALIENTE  A2 //D11 = rele 2 (cable amarillo)
#define VENTILADOR     A3 //ventilador

#define Gap_temp0 0.5
#define Gap_temp1 1.0    //1C
#define Gap_temp2 2.0
#define Gap_temp3 3.0
#define Gap_temp4 4.0
#define Gap_temp5 5.0
#define Gap_temp6 7.0    //6.0
#define Gap_temp7 9.0    //7.0
#define Gap_temp8 11.0   //8.0
#define Gap_temp9 13.0   //9.0

#define Gap_pH_0  0.05
#define Gap_pH_1  0.10     // 0.1 (pH)
#define Gap_pH_2  0.50
#define Gap_pH_3  0.75
#define Gap_pH_4  1.00
#define Gap_pH_5  2.00


String message     = "";
String new_write   = "b000f000000m0000tn000r111111111\n"; //"x100f100100m1500tx100r11111111\n";
String temp_calibrar = "nada";

boolean stringComplete = false;  // whether the string is complete

//RESET SETUP
uint8_t rst1 = 1;  uint8_t rst2 = 1;  uint8_t rst3 = 1;  uint8_t rst4 = 1;  uint8_t rst5 = 1;

//DIRECTION SETUP
uint8_t dir1 = 1;  uint8_t dir2 = 1;  uint8_t dir3 = 1; uint8_t dir4 = 1;

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
float umbral_a = SPEED_MAX;
float umbral_b = SPEED_MAX;
float umbral_temp = SPEED_MAX;

//******//******//******//******//******//******/
//VARIABLES DE CONTROL pH y Temperatura
float Temp1 = 0;
float Temp2 = 0;

float myph_set   = 0;
int myfeed_set   = 0;
int myunload_set = 0;
int mymix_set    = 0;

uint8_t u_temp_save = 0;
float mytemp_set = 25;
float dTemp  = 0;
float Temp_  = 0;
int u_temp = 0;

String ph_select   = "n";
String temp_select = "n";
String svar = "";
float u_ph = 0;
float dpH  = 0;

float Iph = 0;
float Iod = 0;

float m = 0;
float n = 0;

//pH=:(m0,n0)
float m0 = +0.87;//+0.864553;//+0.75;
float n0 = -3.39;//-3.634006;//-3.5;

//oD=:(m1,n1)
float m1 = +6.02;
float n1 = -20.42;

float pH    = m0 * Iph + n0;
float oD    = m1 * Iod + n1;

//fin variables control
//******//******//******//******//******//******/

byte received_from_computer = 0; //we need to know how many characters have been received.
byte serial_event = 0;           //a flag to signal when data has been received from the pc/mac/other.
byte code = 0;                   //used to hold the I2C response code.

char RTD_data[20];
char RTD_data1[20];
char RTD_data2[20];

byte in_char = 0;                //used as a 1 byte buffer to store in bound bytes from the RTD Circuit.
byte i = 0;                      //counter used for RTD_data array.

int time_ = 600;                 //used to change the delay needed depending on the command sent to the EZO Class RTD Circuit.


void calibrate_sensor_atlas() {
  // comunicacion a sensor rtd2
  Wire.beginTransmission(rtd2);       //FH, OJO: se esta midiendo con la rtd2!
  //temp_calibrar = "cal," + temp_calibrar;
  //Wire.write(temp_calibrar.c_str());
  Wire.write("cal,19");
  Wire.endTransmission();                                                              //end the I2C data transmission.

  if ( strcmp("cal,19", "sleep" ) != 0) {
    delay(time_);                                                                     //wait the correct amount of time for the circuit to complete its instruction.
    Wire.requestFrom(rtd2, 20, 1);     //FH, OJO: se esta midiendo con la rtd2!       //call the circuit and request 20 bytes (this may be more than we need)
    code = Wire.read();                                                               //the first byte is the response code, we read this separately.

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
  serial_event = false;                   //reset the serial event flag.
  return;
}


void rtd2_sensor() {
  Wire.beginTransmission(rtd2);                                                 //call the circuit by its ID number.
  String read = "r";
  Wire.write(read.c_str());
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


rtds_sensors(){
  //rtd1_sensor();
  rtd2_sensor();
  return Temp_ = Temp2; //0.5 * (Temp1 + Temp2);
}

//c0+00.91-03.86e //calibrar pH
//c0+00.75-03.50e //calibrar pH
//c2+00.75-03.50e // c: (0=>ph) (1=>od) (2=>temp)
void sensor_calibrate(){
  //calibrate function for "message"
  var = message[1];
  m   = message.substring(2,8 ).toFloat();
  n   = message.substring(8,14).toFloat();

  switch (var) {
    case '0': //pH case for calibration
      m0 = m;
      n0 = n;
      break;

    case '1': //Oxigen disolve case for calibration
      m1 = m;
      n1 = n;
      break;


    default:
      break;
  }

  Serial.println("Sensor calibrated: " + String(m) + " " + String(n));
  return;
}




void hamilton_pH_sensor(){
  //Iph = ads1.readADC_SingleEnded(0);
  //Iph = alpha * (PGA1 * K ) * ads1.readADC_SingleEnded(0) + (1 - alpha) * Iph;
  Iph = (PGA1 * K ) * ads1.readADC_SingleEnded(0);
  pH = m0 * Iph + n0;
  delay(200);
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


// Validate and crumble SETPOINT from serial in raspberry (5 julio 2020. Esquema I2C UdeChile:
int validate() {
    //message format write values from raspberry-pi(serial): wph07.0f017u010m0001t030r111111d000111
    if ( message[0] == 'w' ) {
          myph_set     = message.substring(3,7).toFloat();
          myfeed_set   = message.substring(8,11).toInt();
          myunload_set = message.substring(12,15).toInt();
          mymix_set    = message.substring(16,20).toInt();
          mytemp_set   = message.substring(21,24).toFloat();

          rst1 = int(INT(message[25]));  //rst_feed    (bomba1)
          rst2 = int(INT(message[26]));  //rst_mix     (mezclador)
          rst3 = int(INT(message[27]));  //rst_pH      (bombas pH)
          rst4 = int(INT(message[28]));  //rest_unload (bomba2)
          rst5 = int(INT(message[29]));  //rst_temp    (bomba temperatura)

          dir1 = int(INT(message[31]));  //dir_feed
          dir2 = int(INT(message[32]));  //dir_pH
          dir3 = int(INT(message[33]));  //dir_temp
          dir4 = int(INT(message[34]));  //dir_unload

          return 1;
    }
    // Validate CALIBRATE
    else if ( message[0]  == 'c' )
          return 1;

    else if ( message[0]  == 't') {
          temp_calibrar = message.substring(1);
          return 1;
    }

    //Validete umbral actuador temp: u2t003e
    else if ( message[0] == 'u' && message[1] == '2' &&
              message[2] == 't' && message[6] == 'e'
            )
          return 1;

    // NOT VALIDATE
    else  return 0;
}


//function for transform numbers to string format of message
String format_message(int var, char type) {
  //reset to svar string
  svar = "";
  if (type != 'm') {
    if      (var < 10)   svar = "00" + String(var);
    else if (var < 100)  svar =  "0" + String(var);
    else                 svar = String(var);
  }
  else {
    if      (var < 10)   svar = "000" + String(var);
    else if (var < 100)  svar =  "00" + String(var);
    else if (var < 1000) svar =   "0" + String(var);
    else                 svar = String(var);
  }

  return svar;
}


void daqmx() {
  //data adquisition measures
  Byte0 = Temp_;
  Byte1 = Iph; //Iph vprocess5;
  Byte2 = pH;  //pH vprocess5;

  Byte3 = m0;
  Byte4 = n0;//Iod;
  //Byte5 = temp_calibrar;
  //Byte6 = 0;//Itemp2;
  //Byte7 = 0;//flujo;

  dtostrf(Byte0, 7, 2, cByte0);
  dtostrf(Byte1, 7, 2, cByte1);
  dtostrf(Byte2, 7, 2, cByte2);
  dtostrf(Byte3, 7, 2, cByte3);
  dtostrf(Byte4, 7, 2, cByte4);
  //dtostrf(Byte5, 7, 2, cByte5);
  //dtostrf(Byte6, 7, 2, cByte6);
  //dtostrf(Byte7, 7, 2, cByte7);

  Serial.print(cByte0);  Serial.print("\t");
  Serial.print(cByte1);  Serial.print("\t");
  Serial.print(cByte2);  Serial.print("\t m0: ");
  Serial.print(cByte3);  Serial.print("\t n0: ");
  Serial.print(cByte4);  Serial.print("\t temp_calibrar: ");
  //Serial.print(cByte5);  Serial.print("\t");
  Serial.print(temp_calibrar);  Serial.print("\t");
  //Serial.print(cByte6);  Serial.print("\t");
  //Serial.print(cByte7);  Serial.print("\t");

  //Serial.print(message.substring(0,34));     Serial.print("\t");
  Serial.print(new_write.substring(0,31));   Serial.print("\t");
  //Serial.print(format_message(u_ph,'x'));    Serial.print("\t");
  //Serial.print(String(u_ph));                Serial.print("\t");
  //Serial.print(String(myph_set));
  Serial.println("\n");

  return;
}



//Re-transmition commands to slave micro controller
void broadcast_setpoint() {
  new_write = "";
  new_write = ph_select + format_message(u_ph,'x') + "f" + format_message(myfeed_set,'x') + format_message(myunload_set,'x') + "m" + format_message(mymix_set,'m') + "t" + temp_select + format_message(u_temp,'x') + "r" + rst1 + rst2 + rst3 + rst4 + rst5 + dir1 + dir2 + dir3 + dir4 + "\n"; //+  String(dir2) +  String(dir3);
  i2c_send_command(new_write, 2); //va hacia uc_slave

  return;
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


//Control temperatura para agua fria y caliente
void control_temp() {
  if (rst5 == 0) {
    //touch my delta temp
    dTemp = mytemp_set - Temp_;

    //CASO: necesito calentar por que setpoint es inferior a la medicion
    if ( dTemp >= -0.1 ) {
      temp_select = "c"; //calentar
      delay(1);
      digitalWrite(AGUA_FRIA, HIGH);
      delay(1);
      digitalWrite(AGUA_CALIENTE, LOW);
    }
    //CASO: necesito enfriar por que medicion es mayor a setpoint
    else if ( dTemp < 0.2 ) {
      temp_select = "e"; //enfriar
      delay(1);
      digitalWrite(AGUA_FRIA, LOW);
      delay(1);
      digitalWrite(AGUA_CALIENTE, HIGH);
      dTemp = (-1)*dTemp;
    }

    if      ( dTemp <= Gap_temp0 ) u_temp = 90;
    else if ( dTemp <= Gap_temp1 ) u_temp = 95;
    else if ( dTemp <= Gap_temp2 ) u_temp = 100;
    else if ( dTemp <= Gap_temp3 ) u_temp = 110;
    else if ( dTemp <= Gap_temp4 ) u_temp = 120;
    else if ( dTemp <= Gap_temp5 ) u_temp = 130;
    else if ( dTemp <= Gap_temp6 ) u_temp = 135;
    else if ( dTemp <= Gap_temp7 ) u_temp = 140;
    else if ( dTemp <= Gap_temp8 ) u_temp = 145;
    else if ( dTemp  > Gap_temp9 ) u_temp = SPEED_MAX;
  }

  else {
    temp_select = "n";
    //el sistema se deja stanby
    digitalWrite(AGUA_CALIENTE, HIGH);
    digitalWrite(AGUA_FRIA, HIGH);
  }

  u_temp_save = int(u_temp);
  return;
}


void control_ph() {
  if (rst3 == 0) {
    //for debug //myphset = 7.0; //touch my delta ph
    dpH = myph_set - pH;

    if (myph_set >= 7) dpH = 3;
    else dpH = -1;

    // Escenario en que se debe aplicar acido.
    if ( dpH > 0.0 ) {
      if ( dpH <= Gap_pH_0 ) //5% ó OFF según sí el 5% de umbral_a sea < 1
        u_ph = 0.05 * umbral_b;
      else if ( dpH <= Gap_pH_1 )
        u_ph = 0.1 * umbral_b;  //10%
      else if ( dpH <= Gap_pH_2 )
        u_ph = 0.2 * umbral_b;  //20%
      else if ( dpH <= Gap_pH_3 )
        u_ph = 0.3 * umbral_b;  //30%
      else if ( dpH <= Gap_pH_4 )
        u_ph = 0.5 * umbral_b; //50%
      else if ( dpH <= Gap_pH_5 )
        u_ph = 0.75 * umbral_b;//75%
      else if ( dpH > Gap_pH_5 )
        u_ph = umbral_b;       //100%

      ph_select = "b";  //=> Acido
      }
    // Escenario en que se debe aplicar base.
    else if ( dpH <= 0.0 ) {
      if ( dpH >= -Gap_pH_0 )
      u_ph = 0.05 * umbral_a;   //5%
      else if ( dpH >= -Gap_pH_1 )
        u_ph = 0.1 * umbral_a;  //10%
      else if ( dpH >= -Gap_pH_2 )
        u_ph = 0.2 * umbral_a;  //20%
      else if ( dpH >= -Gap_pH_3 )
        u_ph = 0.3 * umbral_a;  //30%
      else if ( dpH >= -Gap_pH_4 )
        u_ph = 0.5 * umbral_a;  //50%
      else if ( dpH >= -Gap_pH_5 )
        u_ph = 0.75 * umbral_a; //75%
      else if ( dpH < -Gap_pH_5 )
        u_ph = umbral_a;        //100%

      ph_select = "a";  //=> Básico
    }
    else {
      u_ph = 0;
      ph_select = "N";  //no hacer nada
    }

    u_ph = u_ph;
  }
  else u_ph = 0;

  return;
}


void clean_strings() {
  //clean strings
  serial_event = false;
  message   = "";
}

//wph07.2f017u010m0001t030r11110d000


/*
void lcd_i2c_grove() {
  lcd.clear();
  lcd.setCursor(0, 0);
  //lcd.print(new_write);
  //String test = "x100f100100m1500tx100r11111111\n";
  //lcd.print(test[10]);
}
*/
