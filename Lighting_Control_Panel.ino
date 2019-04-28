/*  
 *   Lighting Control Panel
 *
 *   DCS World Version 2.5.4.xxxx
 *   DCS BIOS version 0.7.1
 *   GSS Rain in the DCS Forums
 *   Joe Sim on Youtube
 *   https://youtu.be/EGqNWMJZ_dk
 */

#define DCSBIOS_IRQ_SERIAL
#include "DcsBios.h"

int Anticollision_Coil = 8;    // Magnetic switch
int LIGHT_BUS_CONSOLE = 9;     // Relay Module
int LIGHT_BUS_AUX_INST = 10;   // Relay Module
int LIGHT_BUS_ENG_INST = 11;   // Relay Module
int LIGHT_BUS_FLT_INST = 12;   // Relay Module
int LIGHT_BUS_FLOOD_LITE = 13; // Relay Module
int LASTE_LIGHT = A1;          // Thrustmaster HOTAS LED chain (5V)
int THROTTLE_LIGHT = A0;       // Thrustmaster HOTAS LED chain (5V)
unsigned int LASTE_Brightness;
unsigned int THROTTLE_Brightness;

// We need to monitor the Electrical Panel to see the state of the Power Bus switches
DcsBios::IntegerBuffer eppBatteryPwrBuffer(0x1110, 0x0002, 1, NULL); // Battery Sw
DcsBios::IntegerBuffer eppAcGenPwrLBuffer(0x110c, 0x8000, 15, NULL); // AC Gen L Sw
DcsBios::IntegerBuffer eppAcGenPwrRBuffer(0x1110, 0x0001, 0, NULL);  // AC Gen R Sw
DcsBios::IntegerBuffer eppApuGenPwrBuffer(0x110c, 0x1000, 12, NULL); // APU Gen Pwr Sw
DcsBios::IntegerBuffer eppInverterBuffer(0x110c, 0x6000, 13, NULL);  // Inverter Sw

// We need to monitor the Engine Instrument Panel to see who is able to provide electrical power to the busses
DcsBios::IntegerBuffer apuRpmBuffer(0x10be, 0xffff, 0, NULL);   // APU rpm
DcsBios::IntegerBuffer lEngCoreBuffer(0x10a8, 0xffff, 0, NULL); // Left Engine Core Speed
DcsBios::IntegerBuffer rEngCoreBuffer(0x10ac, 0xffff, 0, NULL); // Right Engine Core Speed

// Position Lights FLASH/OFF/STEADY
DcsBios::Switch3Pos lcpPosition("LCP_POSITION", 2, 3);

// Formation Lights
DcsBios::Potentiometer lcpFormation("LCP_FORMATION", A7);

// Anticollision Lights (Magnetically Held Toggle Switch)
DcsBios::Switch2Pos lcpAnticollision("LCP_ANTICOLLISION", 4);

// Anticollision Lights Coil
void onLcpAnticollisionChange(unsigned int newCoilValue) {
    switch (newCoilValue){
      case 0:
      digitalWrite(Anticollision_Coil, LOW);
      break;
      case 1:
      digitalWrite(Anticollision_Coil, HIGH);
      break;
    }  
}
DcsBios::IntegerBuffer lcpAnticollisionBuffer(0x1144, 0x0080, 7, onLcpAnticollisionChange);

// Engine Instrument Lights
DcsBios::Potentiometer lcpEngInst("LCP_ENG_INST", A6);

// Nose Illumination
DcsBios::Switch2Pos lcpNoseIllum("LCP_NOSE_ILLUM", 5);

// Aux Instrument Lights
DcsBios::Potentiometer lcpAuxInst("LCP_AUX_INST", A5);

// Signal Lights
DcsBios::Switch2Pos lcpSignalLights("LCP_SIGNAL_LIGHTS", 6);

// Flight Instrument Lights
DcsBios::Potentiometer lcpFlightInst("LCP_FLIGHT_INST", A4);

// Accelerometer and Compass Lights
DcsBios::Switch2Pos lcpAccelComp("LCP_ACCEL_COMP", 7);

// Flood Lights
DcsBios::Potentiometer lcpFlood("LCP_FLOOD", A3);

// Console Lights
DcsBios::Potentiometer lcpConsole("LCP_CONSOLE", A2);

// I modified the Thrustmaster Warthog HOTAS.
// LASTE Panel Lighting is now brought out on its own connector. Two wires labeled Laste dimming. Just LEDs and 220 ohm current limiting resisor.
// Throttle Panel Lighting is now brought out on its own connector. Two wires labeled Throttle dimming. Just LEDs and 220 ohm current limiting resisor.

// Aux Instrument Lights (LASTE Panel Brightness depends on Aux Instr Light knob position)
void onLcpAuxInstChange(unsigned int AuxPot_newValue) {
  unsigned int AuxPot_Value = (AuxPot_newValue & 0xffff) >> 0;
  LASTE_Brightness = map(AuxPot_Value, 0, 65535, 0, 100);

  if (LASTE_Brightness > 25 && (eppAcGenPwrLBuffer.getData() == 1 || eppAcGenPwrRBuffer.getData() == 1 || eppBatteryPwrBuffer.getData() == 1))
  {
     digitalWrite(LASTE_LIGHT, HIGH); 
  }
  else
  {
     digitalWrite(LASTE_LIGHT, LOW);     
  }
}
DcsBios::IntegerBuffer lcpAuxInstBuffer(0x114c, 0xffff, 0, onLcpAuxInstChange);

// Console Lights (THROTTLE Panel Brightness depends on Console Light knob position)
void onLcpConsoleChange(unsigned int ConsolePot_newValue) {
  unsigned int ConsolePot_Value = (ConsolePot_newValue & 0xffff) >> 0;
  THROTTLE_Brightness = map(ConsolePot_Value, 0, 65535, 0, 100);
  if (THROTTLE_Brightness > 25 && (eppAcGenPwrLBuffer.getData() == 1 || eppAcGenPwrRBuffer.getData() == 1 || eppBatteryPwrBuffer.getData() == 1))
  {
     digitalWrite(THROTTLE_LIGHT, HIGH);     
  }
  else
  {
     digitalWrite(THROTTLE_LIGHT, LOW);      
  }  
}
DcsBios::IntegerBuffer lcpConsoleBuffer(0x1150, 0xffff, 0, onLcpConsoleChange);

void setup() {
  DcsBios::setup();
  pinMode (Anticollision_Coil, OUTPUT);
  pinMode (LIGHT_BUS_CONSOLE, OUTPUT);
  pinMode (LIGHT_BUS_AUX_INST, OUTPUT);
  pinMode (LIGHT_BUS_ENG_INST, OUTPUT);
  pinMode (LIGHT_BUS_FLT_INST, OUTPUT);
  pinMode (LIGHT_BUS_FLOOD_LITE, OUTPUT);
  pinMode (LASTE_LIGHT, OUTPUT);
  pinMode (THROTTLE_LIGHT, OUTPUT); 
}

void loop() {
  DcsBios::loop();
  
// The switches are Baettery On/Off, LH Gen On/Off, RH Gen On/Off, and APU Gen Pwr On/Off.
// If the switches have changed, see if any are in the ON position.
// If any of the switches are On, then the appropriate lighting bus is on, else lighting bus is off. 
// Flood Light is not really modeled.  We will add that code later when we install a real flood light.
    
// 5V Omron solid state relays 240V/2A, output with resistive fuse 240V/2A.
// (0-2.5V low state relays ON)
// (3-5V state high relay OFF)
// Power draw 5V 0mA 12.5mA    3.3-5V 2mA

  if (eppBatteryPwrBuffer.hasUpdatedData() || eppAcGenPwrLBuffer.hasUpdatedData() || eppAcGenPwrRBuffer.hasUpdatedData() || eppApuGenPwrBuffer.hasUpdatedData() || eppInverterBuffer.hasUpdatedData() || apuRpmBuffer.hasUpdatedData())  {
        
    if (eppAcGenPwrLBuffer.getData() == 1 && lEngCoreBuffer.getData() > 32000) 
    {
   // Left Gen Pwr Switch is On, and Left Engine is running so turn on listed relays and HOTAS LEDs
      digitalWrite(LIGHT_BUS_CONSOLE, LOW);    // On
      digitalWrite(LIGHT_BUS_AUX_INST, LOW);   // On
      digitalWrite(LIGHT_BUS_FLOOD_LITE, LOW); // On

   // Monitor Status of Inverter Switch
      if (eppInverterBuffer.getData() == 2)
          {
          digitalWrite(LIGHT_BUS_ENG_INST, LOW);   // On
          digitalWrite(LIGHT_BUS_FLT_INST, LOW);   // On
          }
      if (eppInverterBuffer.getData() < 2)
          {
          digitalWrite(LIGHT_BUS_ENG_INST, HIGH);   // Off
          digitalWrite(LIGHT_BUS_FLT_INST, HIGH);   // Off
          }
          
      if (LASTE_Brightness > 25 )
      {
        digitalWrite(LASTE_LIGHT, HIGH); 
      }
      if (THROTTLE_Brightness > 25 )
      {
        digitalWrite(THROTTLE_LIGHT, HIGH); 
      }
    }
    else if (eppAcGenPwrRBuffer.getData() == 1 && rEngCoreBuffer.getData() > 32000) 
    {
   // Right Gen Pwr Switch is On, and right Engine is running so turn on all relays and HOTAS LEDs
      digitalWrite(LIGHT_BUS_CONSOLE, LOW);    // On
      digitalWrite(LIGHT_BUS_AUX_INST, LOW);   // On
      digitalWrite(LIGHT_BUS_FLOOD_LITE, LOW); // On

   // Monitor Status of Inverter Switch
      if (eppInverterBuffer.getData() == 2)
          {
          digitalWrite(LIGHT_BUS_ENG_INST, LOW);   // On
          digitalWrite(LIGHT_BUS_FLT_INST, LOW);   // On
          }
      if (eppInverterBuffer.getData() < 2)
          {
          digitalWrite(LIGHT_BUS_ENG_INST, HIGH);   // Off
          digitalWrite(LIGHT_BUS_FLT_INST, HIGH);   // Off
          }   

      if (LASTE_Brightness > 25 )
      {
        digitalWrite(LASTE_LIGHT, HIGH); 
      }
      if (THROTTLE_Brightness > 25 )
      {
        digitalWrite(THROTTLE_LIGHT, HIGH); 
      }
    }
    else if (eppApuGenPwrBuffer.getData() == 1 && apuRpmBuffer.getData() > 50000)
    {
   // APU Gen Pwr Switch is On and APU RPM is above 90%, so turn on Console and Aux Instruments relays
      digitalWrite(LIGHT_BUS_CONSOLE, LOW);   // On 
      digitalWrite(LIGHT_BUS_AUX_INST, LOW);  // On
   
   // Monitor Status of Inverter Switch
      if (eppInverterBuffer.getData() == 2)
          {
          digitalWrite(LIGHT_BUS_ENG_INST, LOW);   // On
          digitalWrite(LIGHT_BUS_FLT_INST, LOW);   // On
          }
      if (eppInverterBuffer.getData() < 2)
          {
          digitalWrite(LIGHT_BUS_ENG_INST, HIGH);   // Off
          digitalWrite(LIGHT_BUS_FLT_INST, HIGH);   // Off
          }
      if (LASTE_Brightness > 25 )
      {
        digitalWrite(LASTE_LIGHT, HIGH); 
      }
      if (THROTTLE_Brightness > 25 )
      {
        digitalWrite(THROTTLE_LIGHT, HIGH); 
      }        
    }    
    else if (eppBatteryPwrBuffer.getData() == 1 && eppInverterBuffer.getData() == 2)
    {
   // Battery is On and Inverter is On, so only turn on Eng Instruments, Flight Instruments, and Flood Light relays
      digitalWrite(LIGHT_BUS_CONSOLE, HIGH);   // Off 
      digitalWrite(LIGHT_BUS_AUX_INST, HIGH);  // Off 
      digitalWrite(LIGHT_BUS_ENG_INST, LOW);   // On
      digitalWrite(LIGHT_BUS_FLT_INST, LOW);   // On
      digitalWrite(LIGHT_BUS_FLOOD_LITE, LOW); // On
      digitalWrite(LASTE_LIGHT, LOW);          // Off
      digitalWrite(THROTTLE_LIGHT, LOW);       // Off                       
    }
    else
    {
   // Battery and Both Generators are off, so turn off all relays and LEDs
      digitalWrite(LIGHT_BUS_CONSOLE, HIGH);    // Off
      digitalWrite(LIGHT_BUS_AUX_INST, HIGH);   // Off
      digitalWrite(LIGHT_BUS_ENG_INST, HIGH);   // Off
      digitalWrite(LIGHT_BUS_FLT_INST, HIGH);   // Off
      digitalWrite(LIGHT_BUS_FLOOD_LITE, HIGH); // Off
      digitalWrite(LASTE_LIGHT, LOW);           // Off
      digitalWrite(THROTTLE_LIGHT, LOW);        // Off
    }
  }
}
