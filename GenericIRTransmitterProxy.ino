/*******************************************************************************
***
*** Copyright 2012 sivamgr@gmail.com
***
*** This file is part of Arduino-Generic-IR-Transmitter-Proxy.
***
*** Arduino-Generic-IR-Transmitter-Proxy is free software: you can redistribute it
*** and/or modify it under the terms of the GNU General Public License as 
***  published by the Free Software Foundation, either version 3 of the License, 
***  or (at your option) any later version.
***
***    Arduino-Generic-IR-Transmitter-Proxy is distributed in the hope that it will 
***  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of 
***  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General 
***  Public License for more details.
***
*** You should have received a copy of the GNU General Public License along with 
*** Arduino-Generic-IR-Transmitter-Proxy. If not, see http://www.gnu.org/licenses/.
***
*** FILE NAME : GenericIRTransmitterProxy.ino
***
*** DESCRIPTION : Implementation of J1939 Stack.
***
*** COMPILER : avr-gcc
***
*** HISTORY :
*** 		Apr 16,2012 First Draft - Sivakumar PS[sivamgr@gmail.com] 
***
*******************************************************************************/

/*******************************************************************************
***                           Includes
********************************************************************************/

#include <avr/interrupt.h>
#include <Arduino.h>


/*******************************************************************************
***                           Defines
********************************************************************************/

#define TIMER_PWM_PIN       3 
#define TIMER_ENABLE_PWM    (TCCR2A |= _BV(COM2B1))
#define TIMER_DISABLE_PWM   (TCCR2A &= ~(_BV(COM2B1)))
#define TIMER_ENABLE_INTR   (TIMSK2 = _BV(OCIE2A))
#define TIMER_DISABLE_INTR  (TIMSK2 - 0)
#define SYSCLOCK 			16000000 

/*******************************************************************************
***                           Define Macro Functions
********************************************************************************/

// defines for setting and clearing register bits
#ifndef cbi
	#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
	#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define TIMER_CONFIG_KHZ(val) ({ \
  const uint8_t pwmval = SYSCLOCK / 2000 / (val); \
  TCCR2A = _BV(WGM20); \
  TCCR2B = _BV(WGM22) | _BV(CS20); \
  OCR2A = pwmval; \
  OCR2B = pwmval / 3; \
})

/*******************************************************************************
***                           Global Variables
********************************************************************************/

int sig_arr[256]; /* A Maximum of 256 Pulse/Space(s)*/
int sig_len;	  /* Number of Pulses + Spaces*/
int sig_khz=38;   /* Frequency of IR-LED*/
unsigned char inByte;

/*******************************************************************************
***                           Functions
********************************************************************************/

/*******************************************************************************
***  FUNCTION NAME:  ir_mark
***
***  DESCRIPTION:
***   Sends an IR mark for the specified number of microseconds.
***	  The mark output is modulated at the PWM frequency.
********************************************************************************/

void ir_mark(int time) {
  TIMER_ENABLE_PWM; // Enable pin 3 PWM output
  delayMicroseconds(time);
}

/*******************************************************************************
***  FUNCTION NAME:  ir_space
***
***  DESCRIPTION:
***   Leave pin off for time (given in microseconds)
***	  Sends an IR space for the specified number of microseconds.
***	  A space is no output, so the PWM output is disabled.
********************************************************************************/

void ir_space(int time) {
  TIMER_DISABLE_PWM; // Disable pin 3 PWM output
  delayMicroseconds(time);
}


/*******************************************************************************
***  FUNCTION NAME:  set_ir_freq
***
***  DESCRIPTION:
***   Configures the PWM Pin as per the frequency configured in the global var,
***	  sig_khz
********************************************************************************/

void set_ir_freq()
{
  // Disable the Timer2 Interrupt (which is used for receiving IR)
  TIMER_DISABLE_INTR; //Timer2 Overflow Interrupt
  
  pinMode(TIMER_PWM_PIN, OUTPUT);
  digitalWrite(TIMER_PWM_PIN, LOW); // When not sending PWM, we want it low
  
  TIMER_CONFIG_KHZ(sig_khz);
}

/*******************************************************************************
***  FUNCTION NAME:  transmit_sig
***
***  DESCRIPTION:
***   Transmits the Signal as coded in the global array, sig_arr
***	  
********************************************************************************/

void transmit_sig()
{
  int i;
       for(i=0;i<=sig_len;i+=2)
       {
         ir_mark(sig_arr[i]);
         ir_space(sig_arr[i+1]);
       }      
}

/*******************************************************************************
***  FUNCTION NAME:  print_sig
***
***  DESCRIPTION:
***   Prints the Signal as coded in the global array, sig_arr
***	  
********************************************************************************/

void print_sig()
{
  int val,i;
       for(i=0;i<=sig_len;i++)
       {
         val = sig_arr[i];
         while(val >0)
         {
           Serial.write('0' + (val%10));
           val/=10;
         }
         Serial.write(',');
       }        
       Serial.write(';');
}

/*******************************************************************************
***  FUNCTION NAME:  setup
***
***  DESCRIPTION:
***   Arduino Setup Call : Initializes the global vars, Configures Serial Port
***	  
********************************************************************************/

void setup()
{
  Serial.begin(9600);
  sig_khz=38;
  sig_len=0;
  set_ir_freq();
  Serial.println("Generic IR Transmitter Proxy v0.9");   
}

/*******************************************************************************
***  FUNCTION NAME:  loop
***
***  DESCRIPTION:
***   Arduino Looop Call : Receives the Serial Data and Process
***	  
********************************************************************************/

void loop() 
{
  inByte = Serial.read();
  if(inByte == '[')
  {  
      sig_len=0;
      sig_arr[0]=0;
  }
  else if( (inByte >= '0') && (inByte <='9') )
  {
     sig_arr[sig_len] *=10;
     sig_arr[sig_len] += (inByte - '0');
  }
  else if(inByte == ',')
  {
     sig_len++; 
     sig_arr[sig_len]=0;
  }
  else if(inByte == ']')
  {
    transmit_sig();    
  }
  else if(inByte == '?')
  {
    print_sig();        
  }
  
}
