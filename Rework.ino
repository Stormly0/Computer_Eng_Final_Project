//Imports 
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>

// Pin State Variables 

// Buttons
#define AUTO_BUTTON 2
#define OFF_BUTTON 4
#define ON_BUTTON 3

// LEDs
#define GREEN_LED 5
#define RED_LED 6

// RGB LED Pins 
#define BLUE_LED_PIN 11 
#define GREEN_LED_PIN 12
#define RED_LED_PIN 13

// Buzzer
#define PIEZO 7

// Fan Motor
#define DC_MOTOR 10 

// IR Receiver pin 
#define IR_RECEIVER 9

// Temperature Sensor 
#define TEMPERATURE_SENSOR A1 
#define HUMIDITY_SENSOR A2

// Potentionmeters 
#define FAN_SPEED_CONTROL A0
#define TEMPERATURE_CONTROL A3

// Neo Pixel Pin 
#define NEO_PIXEL_PIN 8 

// LCD ADDRESSES 
#define CONTROL_LCD_ADDRESS 33 
#define STATUS_LCD_ADDRESS 32

// Object Initializations 
LiquidCrystal_I2C CONTROL_LCD(CONTROL_LCD_ADDRESS, 16, 2); // System status LCD where you can see the modes and the current status of the system
LiquidCrystal_I2C STATUS_LCD(STATUS_LCD_ADDRESS, 16, 2); // Status LCD where you can see the current temperature and humidity
Adafruit_NeoPixel NEO_PIXEL(24, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800); // RGB LED

// ------------ [CLASSES] ------------ \\

// A timer that allows for independent timing of different events 
class Timer{
    // Private Variables
    private: 
        // Millisecond timers
        unsigned long Timer_Start_Millis = 0; // Stores the time start time  
        bool Timer_Started_Millis = false; // Indicates if the timer is started 

        // Microsecond timers 
        unsigned long Timer_Start_Micros = 0; // Stores the time start time
        bool Timer_Started_Micros = false; // Indicates if the timer is started

    // Public Variables
    public: 
        // Checks whether a specific amount of time has elapsed given time in milliseconds
        bool Check_Time_Millis(unsigned long Time){
            // Checks if the system timer has started 
            if(this->Timer_Started_Millis == false){
                this->Timer_Start_Millis = millis(); // Sets the start time 
                this->Timer_Started_Millis = true; // Indicates that the timer has started
            }

            // Checks if the time has elapsed and resets the timer 
            if(millis() - this->Timer_Start_Millis >= Time && this->Timer_Started_Millis){
                // Resets the timer 
                this->Timer_Start_Millis = millis();
                this->Timer_Started_Millis = false;

                return true;  // Returns true if the time has elapsed
            }else{
                return false; // Returns false if the time has not elapsed 
            }
        }

        // Checks whether a specific amount of time has elapsed given time in microseconds 
        bool Check_Time_Micros(unsigned long Time){
            // Checks if the system timer has started 
            if(this->Timer_Started_Micros == false){
                this->Timer_Start_Micros = micros(); // Sets the start time 
                this->Timer_Started_Micros = true; // Indicates that the timer has started
            }

            // Checks if the time has elapsed and resets the timer 
            if(micros() - this->Timer_Start_Micros >= Time && this->Timer_Started_Micros){
                // Resets the timer 
                this->Timer_Start_Micros = micros();
                this->Timer_Started_Micros = false;

                return true;  // Returns true if the time has elapsed
            }else{
                return false; // Returns false if the time has not elapsed 
            }
        }
}

// A event handler that handles all function calls and events that happen in the system through a callback system 
class Event_Handler{
    
    private: 
        // Private Variables 
        static unsigned long Void_Event_Calls = 0; // Indicates how many events were called 
        static unsigned long Typed_Event_Calls = 0; // Indicates how many events were called with an argument
        static unsigned long Total_Event_Calls = 0; // Total event calls that have occured

    public: 
        // Public Functions 
        
        // Calls the function given 
        void Call(void (*Function)(void)){
            Function(); // Calls the function 
            this->Void_Event_Calls++; // Increments the number of void events that have occured
            this->Total_Event_Calls++; // Increments the total number of events that have occured
        }

        // Calls the function given a parameter
        template<typename A, typename B> void 


}

// ------------ [Variables] ------------ \\

// --- [System Configurations] --- \\ 

// Lighting Parameters 
const unsigned int Max_Neo_Pixel_Brightness = 100; // Max Brightness of the NeoPixel [0 - 255]
const unsigned int Max_RGB_LED_Brightness = 100; // Max Brightness of the RGB LED [0 - 255]

// Buzzer Tones 
const unsigned int Buzzer_Frequency = 100; // Frequency of the buzzer tone [Hz]
const unsigned int Buzzer_Duration = 250; // Duration of the buzzer tone [ms]

// Color Pattern 
const int Neo_Pixel_Color_Pattern[][3] = {
    {255,0,0}, // Red
    {0,255,0}, // Green
    {0,0,255}, // Blue
    {255,255,0}, // Yellow
    {255,0,255}, // Purple
    {0,255,255}, // Cyan
    {255,255,255}, // White
}

// --- [System Status Variables] --- \\ 
String System_Mode = "Manual"; // Mode that the system is currently in [Auto,Manual]


