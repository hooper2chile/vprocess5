//Vprocess5 - Uchile: Felipe Hooper
/*
*  uc_sensores
*  Writed by: Felipe Hooper
*  Electronic Engineer
*/
#include <avr/wdt.h>
#include "mlibrary.h"

void setup() {
  wdt_disable();

  Serial.begin(9600);
  Wire.begin(); //se inicia i2c master

  ads1.begin();

  //ads2.begin();
  //                                           ADS1015  ADS1115
  //                                           -------  -------
  ads1.setGain(GAIN_ONE);      // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV

  //ads2.setGain(GAIN_ONE);
  //ads.setGain(GAIN_TWO);     // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV

  DDRB = DDRB | (1<<PB0) | (1<<PB5);
  PORTB = (0<<PB0) | (1<<PB5);

  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);

  //pinMode(A0, OUTPUT); //Bomba remontaje
  pinMode(A1, OUTPUT); //Valvula 1
  pinMode(A2, OUTPUT); //Valvula 2
  pinMode(A3, OUTPUT); //Ventilador

  //digitalWrite(A0, HIGH);
  //digitalWrite(A1, HIGH);
  //digitalWrite(A2, HIGH);
  //digitalWrite(A3, HIGH);

  new_write.reserve(65);
  message.reserve(65);
  wdt_enable(WDTO_8S);


  //lcd.begin(16, 2);
  //lcd.setRGB(colorR, colorG, colorB);
  //lcd.print("hello, world!");
  //delay(1000);
}

void loop() {
  if ( serial_event  ) {
      if ( validate() ) {
          //PORTB = 1<<PB0;
          switch ( message[0] ) {
              case 'w':
                rtds_sensors();
                hamilton_pH_sensor();
                control_ph();
                control_temp();
                daqmx();
                broadcast_setpoint();
                //lcd_i2c_grove();
                break;

              case 'c':
                sensor_calibrate();   //command test: c0+00.91-03.86e
                Serial.println("CALIBRADO!");
                break;

              case 'z':
                calibrate_sensor_atlas();
                Serial.println(" ---- Sensor Atlas Scientific Calibrado ---- : " + temp_calibrar );
                break;

              case 'u':
                actuador_umbral();
                break;

              default:
                break;
          }
          //wdt_reset(); //nuevo
          //PORTB = 0<<PB0;
      }
      else {
        Serial.println("bad validate:" + message);
      }
    clean_strings();
    wdt_reset();
  }
}
