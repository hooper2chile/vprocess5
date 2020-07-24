// MODIFICADO PARA Vprocess5 - UdeChile 5 Julio 2020
#include "Arduino.h"
#include "Wire.h"
#include "SoftwareSerial.h"
//#include "rgb_lcd.h"

SoftwareSerial mySerial(2, 3);  //RX(Digital2), TX(Digital3) Software serial port.
SoftwareSerial mixer(4, 5); //for control in mezclador


//rgb_lcd lcd;

const int colorR = 255;
const int colorG = 0;
const int colorB = 0;

#define  INT(x)   (x-48)  //ascii convertion
#define iINT(x)   (x+48)  //inverse ascii convertion

#define SPEED_MAX 150.0 //MIN_to_us/(STEPS*TIME_MIN)
#define SPEED_MIN 1
#define TEMP_MAX  60

//#define REMONTAJE_PIN  A0 //bomba remontaje
#define AGUA_FRIA      A0
#define AGUA_CALIENTE  A1

                     //wf000u000t000r111d111
                     //wph07.0f014u007m0001t025r111111d000111
String  new_write   = "a000f012010m0700tn000r111110000\n";//"wf000u000t000r111d111\n";
String  new_write0  = "";

String message = "";
String state   = "";

boolean stringComplete = false;  // whether the string is complete

//Re-formatting
String  uset_temp = "";
String  svar      = "";

unsigned int mymix_setup = 0;

// RST setup
uint8_t rst1 = 1;       uint8_t rst2 = 1;       uint8_t rst3 = 1;
uint8_t rst1_save = 1;  uint8_t rst2_save = 1;  uint8_t rst3_save = 1;
//DIRECTION SETUP
uint8_t dir1 = 1;  uint8_t dir2 = 1;  uint8_t dir3 = 1;

uint8_t pump_enable = 0;
char relay_temp = 0;

float mytemp_set = 0;
float mytemp_set_save = 0;

uint8_t unload = 0;
uint8_t unload_save = 0;

uint8_t feed = 0;
uint8_t feed_save = 0;

uint8_t u_temp_save = 0;


int i = 0;
int data = 0;
int data_cero = 0;
uint16_t s_rpm_save = 115;



//for communication i2c
void receiveEvent() {
  while ( Wire.available() > 0 ) {
    byte x = Wire.read();
     message += (char) x;
     if ( (char) x == '\n' ) {
      stringComplete = true;
     }
  }  //Serial.println(message);         // print the character
}

//for hardware serial
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    message += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}



void cooler(int rst1, int rst2, int rst3) {
  if (rst1 == 0 || rst2 == 0 || rst3 == 0) digitalWrite(A3, LOW );
  else                                     digitalWrite(A3, HIGH);

}


void broadcast_setpoint(uint8_t select) {

  switch (select) {
    case 0: //only re-tx and update pid uset's.
      new_write0 = "";
      //new_write0 = "probando case 0";
      new_write0 = new_write + "\n"; //"wf" + new_write.substring(2,5) + 'u' + new_write.substring(6,9) + 't' + uset_temp + 'r' + new_write.substring(14,17) + 'd' + "111\n";
      mySerial.print(new_write0);
      break;

    case 1: //update command and re-tx.
      new_write  = "";
      //new_write = "probando case 1";
      new_write  = message + "\n"; //"wf" + message.substring(2,5)  + 'u' + message.substring(6,9)    + 't' + uset_temp + 'r' + message.substring(14,17)   + 'd' + "111\n";
      mySerial.print(new_write);
      break;

    default:
      break;
  }

  return;
}


void reles_temp(){
  if (relay_temp == 'e') {
    digitalWrite(AGUA_CALIENTE, HIGH);
    delay(1);
    digitalWrite(AGUA_FRIA,      LOW);
    delay(1);
  }
  else if (relay_temp == 'c') {
    digitalWrite(AGUA_CALIENTE,  LOW);
    delay(1);
    digitalWrite(AGUA_FRIA,     HIGH);
    delay(1);
  }
  else {
    digitalWrite(AGUA_CALIENTE, HIGH);
    delay(1);
    digitalWrite(AGUA_FRIA,     HIGH);
    delay(1);
  }

  return;
}


void clean_strings() {
  //clean strings
  stringComplete = false;
  message   = "";
}


int validate_write() {
  if ( message[0] == 'n' || message[0] == 'a'|| message[0] == 'b' ) {
    //se "desmenuza" el command de setpoints
    mymix_setup = message.substring(12,16).toInt();
    rst2 = message.substring(23,24).toInt();

    relay_temp = message[17];

    Serial.println("echo: " + message + "t: " + relay_temp);

    return 1;
  }
}


void Motor_set_RPM(int high, int low)
{
  int checksum = (177 + high + low) & 0xff;

  mixer.write(254);
  delay(100);
  mixer.write(177);
  delay(100);
  mixer.write(high);
  delay(100);
  mixer.write(low);
  delay(100);
  mixer.write(data_cero);
  delay(100);
  mixer.write(checksum);
}
//254 160 0 0 0 160       254 160 0 0 0 160     254 177 0 0 0 177       254 177 0 0 0 177      254 177 0 d 0 21      254 177
void Motor_conectar()
{
  delay(100);
  mixer.write(254);
  delay(100);
  mixer.write(160);
  delay(100);
  data = 0;
  mixer.write(data);  // data = 0
  delay(100);
  mixer.write(data);
  delay(100);
  mixer.write(data);
  delay(100);
  mixer.write(160);
}

void agitador(uint16_t s_rpm, uint8_t rst) {
    if( !rst ) {
      if ( s_rpm_save != s_rpm ) {
        s_rpm_save = s_rpm;
        int rpm_h = (s_rpm >> 8) & 0xff;
        int rpm_l = s_rpm & 0xff;

        while ( i <= 1 ) {
           Motor_conectar();
           Motor_set_RPM(rpm_h, rpm_l);
          i++;
        }
        i = 0;
      }
    }
    else if ( rst ) {
      data_cero = 0;
      s_rpm_save = data_cero;
      int rpm_h = (data_cero >> 8) & 0xff;
      int rpm_l = data_cero & 0xff;

      while ( i <= 1 ) {
         Motor_conectar();
         Motor_set_RPM(rpm_h, rpm_l);
        i++;
      }
      i = 0;
    }
}


/*
void lcd_i2c_grove() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(mymix_setup);

}
*/
