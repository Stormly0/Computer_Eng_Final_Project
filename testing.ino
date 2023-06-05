//Imports 
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>

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

/*  
    Notes: 
    1. Need to remake the Timer function as it is too dependent on the single variables 

*/

// Object Initializations 
LiquidCrystal_I2C CONTROL_LCD(CONTROL_LCD_ADDRESS, 16, 2); // System status LCD where you can see the modes and the current status of the system
LiquidCrystal_I2C STATUS_LCD(STATUS_LCD_ADDRESS, 16, 2); // Status LCD where you can see the current temperature and humidity
Adafruit_NeoPixel NEO_PIXEL(24, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800); // RGB LED

// Variables 

// --------------System Parameters------------- \\ 
// Predefined behaviours for the system 

// Lighting Parameters
const unsigned int Max_NEOPIXEL_Brightness = 100; // Maximum brightness of the LEDs [0 - 255]
const unsigned int Max_RGBLED_Brightness = 100; // Maximum brightness of the RGB LED [0 - 255]

const int Neo_pixel_color_pattern[][3] = {
    {255,0,0}, // Red
    {0,255,0}, // Green
    {0,0,255}, // Blue
    {255,255,0}, // Yellow
    {255,0,255}, // Purple
    {0,255,255}, // Cyan
    {255,255,255}, // White
}; 

// Buzzer tones 
const unsigned int Buzzer_Tone = 100; // Frequency of the buzzer tone 
const unsigned int Buzzer_Duration = 250; // Duration of the buzzer tone

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
        bool Check_Time_Millis(double Time){
            // Checks if the time is negative
            if(Time < 0){
                Serial.println("Error: Time cannot be negative");
                return false; // Returns false if the time is negative 
            }

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
        bool Check_Time_Micros(double Time){
            // Checks if the time is negative 
            if(Time < 0){
                Serial.println("Error: Time cannot be negative");
                return false; // Returns false if the time is negative 
            }

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
};

// --------------System Status Variables------------- \\ 
// Displays the current status of the system to the LCD screens

// System Status 
// Char = 1 byte [8x bits]
// String = 1 byte per character + 1 byte for null terminator [8x + 8 bits]
// Use char is more efficient than using string
char System_Mode[] = "Manual"; // Mode that the system is currently in [Manual, Auto]

// Fan Status 
bool Fan_State = false; // Determines if the fan is on or off
int Fan_Speed = 0; // Determines the speed of the fan in percentage [0 - 255] 

bool Auto_Fan_State = false; // Determines if the fan is on or off in auto mode 
int Auto_Fan_Speed = 0; // Determines the speed of the fan in percentage [0 - 255] in auto mode

// Environment Status 
float Temperature = 0; // Temperature in degrees celsius 
float Humidity = 0; // Humidity in percentage [0 - 100]

float Set_Temperature = 0; // Temperature that the user wants the system to be at

// --------------System Control Variables------------- \\ 
// Lighting 
unsigned int Pixel_Index = 0; 
unsigned int Pixel_Pattern_Index = 0; 
int Array_Size = 0; // size of the pattern array 

// Timers 

// NEO_PIXEL TIMER 
Timer Neo_Pixel_Timer; // Timer for the Neo Pixel 

// RGB LED TIMERS
Timer R_LED_TIMER; 
Timer G_LED_TIMER; 
Timer B_LED_TIMER;

unsigned long System_Timer = 0; // Timer for the system 
bool System_Timer_Started = false; // Determines if the system timer has started

// Repeat value cache 
bool Button_State_Cache[3] = {false, false, false}; // Stores the previous state of the buttons

// Displays 
bool Control_Display_Changed = false; // Determines if the control display has changed
bool Status_Display_Changed = false; // Determines if the status display has changed




// Sets the color of the RGB LED with the selected brightness without using a PWM pin
// Brightness [0 - 255] 
void Set_RGB_Color(bool R, bool G, bool B, unsigned int Brightness){
    // Checks what color that the user wants to set and sets the pin to the correct brightness  

    // Calculates the on duty cycle for the one pin (Simulates a PWM pin)
    const double PWM_Frequency = 2040; // Frequency of the PWM signal
    double On_Duty_Cycle = PWM_Frequency - (((255.0d - (double)Brightness) / 255) * PWM_Frequency); // Calculates the on duty cycle of the PWM signal
    double Off_Duty_Cycle = PWM_Frequency - On_Duty_Cycle; // Calculates the off duty cycle of the PWM signal

    // Debugging
    Serial.print("On Duty Cycle: ");
    Serial.println(On_Duty_Cycle);
    Serial.print("Off Duty Cycle: ");
    Serial.println(Off_Duty_Cycle);

    // RED LED 
    if(R){
        // Checks if the on duty cycle is just equal to 0 which means that the LED is just on 
        if(Brightness == 0){
            Serial.println("JUST ON"); 
            digitalWrite(RED_LED_PIN, HIGH); // Turns on the LED
        }

        // Checks if the off duty cycle is just equal to 0 which means that the LED is just off
        if(Brightness == 255){
            Serial.println("JUST OFF"); 
            digitalWrite(RED_LED_PIN, LOW); // Turns off the LED
        }

        // Waits for the timer to elapse 
        if(Brightness != 0 && R_LED_TIMER.Check_Time_Micros(On_Duty_Cycle)){
            Serial.println("TIMED ON"); 
            digitalWrite(RED_LED_PIN, HIGH); // Turns on the LED
        }else if(Brightness != 0){
            digitalWrite(RED_LED_PIN, LOW); // Turns off the LED
            Serial.println("TIMED OFF");
        }        
    }

    // GREEN LED
    if(G){
        // Checks if the on duty cycle is just equal to 0 which means that the LED is just on
        if(Brightness == 0){
            digitalWrite(GREEN_LED_PIN, HIGH); // Turns on the LED
        }

        // Checks if the off duty cycle is just equal to 0 which means that the LED is just off
        if(Brightness == 255){
            digitalWrite(GREEN_LED_PIN, LOW); // Turns off the LED
        }

        // Waits for the timer to elapse 
        if(Brightness != 0 && G_LED_TIMER.Check_Time_Micros(On_Duty_Cycle)){
            digitalWrite(GREEN_LED_PIN, HIGH); // Turns on the LED
        }else if(Brightness != 0){
            digitalWrite(GREEN_LED_PIN, LOW); // Turns off the LED
        }        
    }

    // BLUE LED
    if(B){
        // Checks if the on duty cycle is just equal to 0 which means that the LED is just on
        if(Brightness == 0){
            digitalWrite(BLUE_LED_PIN, HIGH); // Turns on the LED
        }

        // Checks if the off duty cycle is just equal to 0 which means that the LED is just off
        if(Brightness == 255){
            digitalWrite(BLUE_LED_PIN, LOW); // Turns off the LED
        }

        // Waits for the timer to elapse 
        if(Brightness != 0 && B_LED_TIMER.Check_Time_Micros(On_Duty_Cycle)){
            digitalWrite(BLUE_LED_PIN, HIGH); // Turns on the LED
        }else if(Brightness != 0){
            digitalWrite(BLUE_LED_PIN, LOW); // Turns off the LED
        }        
    }
}


// System start
void setup(){

    //System Initialization 
    Serial.begin(9600); 
    CONTROL_LCD.init(); 
    STATUS_LCD.init();

    // Starts the LCD's 
    CONTROL_LCD.clear(); 
    STATUS_LCD.clear();
    CONTROL_LCD.backlight(); 
    STATUS_LCD.backlight();

    // Starts the IR receiver 
    IrReceiver.begin(IR_RECEIVER); 

    // Starts the NeoPixel 
    NEO_PIXEL.begin();

    // Pin Modes

    // Inputs (DIGITAL)
    pinMode(AUTO_BUTTON, INPUT); // Automatically controls the fan 
    pinMode(OFF_BUTTON, INPUT); // Turns off the fan 
    pinMode(ON_BUTTON, INPUT); // Turns on the fan 

    // Outputs (DIGITAL) 
    pinMode(GREEN_LED, OUTPUT); // On Light 
    pinMode(RED_LED, OUTPUT); // Off Light 
    pinMode(PIEZO, OUTPUT); // Buzzer 
    pinMode(DC_MOTOR, OUTPUT); // Motor 
    
    // RGB LED 
    pinMode(BLUE_LED_PIN, OUTPUT); // Determines the status of the fan 
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);

    // UPDATE ON STARTUP

}



void loop(){
    Set_RGB_Color(true,false,false,100); 
}