/*
* uc_step_motor
* 01/05/2019
* ======================
* write by: Felipe Hooper
* Electronic Engineer
*/

#include <avr/wdt.h>
#include <TimerOne.h>
#include <slibrary.h>


void setup() {
  wdt_disable();

  Serial.begin(9600);

  DDRB = DDRB | (1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB3) | (1<<PB4) | (1<<PB5);
  DDRC = DDRC | (1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5);
  DDRD = DDRD | (1<<PD3) | (1<<PD4) | (1<<PD5);



  Timer1.initialize(TIME_T);
  Timer1.attachInterrupt(motor_control);

  message.reserve(65);
  wdt_enable(WDTO_8S);
}


void loop() {
  if ( stringComplete ) {
    if ( validate_write() ) {                 //a/b 000-100                 rst3                               dir2
      Serial.println("Good message: " +  message.substring(0,4) + " " + message.substring(24,25) + " " +  message.substring(28,29) + "  message: " + message);

      //se "desmenuza" el command de setpoints
      crumble();

      //Time setup for counters:
      if ( myfeed != myfeed_save ) {
        time_setup(myfeed, &count_m3_set, &count_m3);
        myfeed_save = myfeed;
      }

      if ( myunload != myunload_save ) {
        time_setup(myunload, &count_m4_set, &count_m4);
        myunload_save = myunload;
      }

      if ( mytemp != mytemp_save ) {
        time_setup(mytemp, &count_m5_set, &count_m5);  //setear en otra función que reciba este mensaje desde un lazo de control
        mytemp_save = mytemp;
      }

      if ( myph_a != myph_a_save ) {
        time_setup(myph_a, &count_m1_set, &count_m1);  //setear en otra función que reciba este mensaje desde un lazo de control
        myph_a_save = myph_a;
      }

      if ( myph_b != myph_b_save ) {
        time_setup(myph_b, &count_m2_set, &count_m2);  //setear en otra función que reciba este mensaje desde un lazo de control
        myph_b_save = myph_b;
      }



      //RST and DIR SETTING:
      //feed: rst1=0 (enable); dir1=1 (cw), else ccw.
      setup_dir_rst( _BV(RST_FEED), _BV(DIR_FEED), &myfeed, &rst1, &dir1, &PORTC, &PORTC );

      //unload: rst4, dir4=null, PORT_2 = NULL.
      setup_dir_rst( _BV(RST_UNLOAD), _BV(NULL), &myunload, &rst4, &dir4, &PORTC, &PORTC );

      //temp: rst5, dir3: PORT_1 distinto a PORT_2.
      setup_dir_rst( _BV(RST_TEMP), _BV(DIR_TEMP), &mytemp, &rst5, &dir5, &PORTC, &PORTB );

       //PH
       //setup_dir_rst( _BV(RST_PHa), _BV(DIR_PH),   &myph_a, &rsta, &dir2,  &PORTD, &PORTC );
       //setup_dir_rst( _BV(RST_PHb), _BV(DIR_PH),   &myph_b, &rstb, &dir2,  &PORTD, &PORTC );
       setup_dir_rst( _BV(RST_PHa), _BV(DIR_PH), &myph_a, &rsta, &dir2, &PORTD, &PORTC );
       setup_dir_rst( _BV(RST_PHb), _BV(DIR_PH), &myph_b, &rstb, &dir2, &PORTD, &PORTC );


      clean_strings();
      wdt_reset();
    }

    else {
      Serial.println("BAD message: " + message);
      clean_strings();
    }
  }
}
