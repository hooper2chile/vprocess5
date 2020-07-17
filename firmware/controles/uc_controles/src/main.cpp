/*
*  MODIFICADO PARA Vprocess5 - UdeChile 5 Julio 2020
*  uc_controles: control de temperatura, bomba Remontaje
*  y reles de electrovalvulas.
*  01/05/2019
*  Writed by: Felipe Hooper
*  Electronic Engineer
*/

#include <avr/wdt.h>
#include "mlibrary.h"

void setup() {
  wdt_disable();

  pinMode(A0, OUTPUT); //Bomba remontaje
  pinMode(A1, OUTPUT); //Valvula 1
  pinMode(A2, OUTPUT); //Valvula 2
  pinMode(A3, OUTPUT); //Ventilador

  digitalWrite(A0, HIGH);
  digitalWrite(A1, HIGH);
  digitalWrite(A2, HIGH);
  digitalWrite(A3, HIGH);

  Serial.begin(9600);
  mySerial.begin(9600);

  message.reserve(65);

  DDRB = DDRB | (1<<PB0) | (1<<PB5);
  PORTB = (0<<PB0) | (1<<PB5);

  Wire.begin(2);  //se inicia i2c slave con direccion: 2
  Wire.onReceive(receiveEvent); // data slave recieved


  //inicia enviando apagado total al uc_step_motor
  mySerial.print(new_write);


  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);

  wdt_enable(WDTO_8S);
}

void loop() {
  if ( stringComplete ) {
      if ( validate_write() ) {
	      cooler(rst1, rst2, rst3);
        lcd_i2c_grove();
        //funcion para mezclador dragonlab
        if (message[0] == 'n' || message[0] == 'a'|| message[0] == 'b') broadcast_setpoint(1);
        else                                                            broadcast_setpoint(0);

      }
      else Serial.println("BAD command to uc_controles: " + message);

      clean_strings();
      wdt_reset();
  }
}
