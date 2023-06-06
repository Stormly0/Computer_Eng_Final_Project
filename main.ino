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

/*  
    Notes: 
    1. Need to remake the Timer function as it is too dependent on the single variables [Done]

*/

// Object Initializations 
LiquidCrystal_I2C CONTROL_LCD(CONTROL_LCD_ADDRESS, 16, 2); // System status LCD where you can see the modes and the current status of the system
LiquidCrystal_I2C STATUS_LCD(STATUS_LCD_ADDRESS, 16, 2); // Status LCD where you can see the current temperature and humidity
Adafruit_NeoPixel NEO_PIXEL(24, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800); // RGB LED


// CLASSES 
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
};

// Variables 

// --------------System Parameters------------- \\ 
// Predefined behaviours for the system 

// Lighting Parameters
const unsigned int Max_NEOPIXEL_Brightness = 100; // Maximum brightness of the LEDs [0 - 255]
const unsigned int Max_RGB_LED_Brightness = 100; // Maximum brightness of the RGB LED [0 - 255]

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

// --------------System Status Variables------------- \\ 
// Displays the current status of the system to the LCD screens

// System Status 
// Char = 1 byte [8x bits]
// String = 1 byte per character + 1 byte for null terminator [8x + 8 bits]
// Use char is more efficient than using string
String System_Mode = "Manual"; // Mode that the system is currently in [Manual, Auto]

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

// unsigned long System_Timer = 0; // Timer for the system 
// bool System_Timer_Started = false; // Determines if the system timer has started

// Repeat value cache 
bool Button_State_Cache[3] = {false, false, false}; // Stores the previous state of the buttons

// Displays 
bool Control_Display_Changed = false; // Determines if the control display has changed
bool Status_Display_Changed = false; // Determines if the status display has changed


// Functions 

// ------------ [SETTERS] ------------ \\

// Controls the Motor Speed
void Set_Fan_Speed(unsigned int Speed){

    // Checks if the value is less than
    if(Speed > 255){
        Serial.println("Speed cannot be greater than 255");
        return; 
    } else if(Speed < 0){
        Serial.println("Speed cannot be less than 0");
        return; 
    }

    // Turns on the fan 
    analogWrite(DC_MOTOR, Speed);
}

// Sets the color of the RGB LED with the selected brightness without using a PWM pin
// Use once throw away function (Probably the most inefficient way to program but will be optimized later on)
// Brightness [0 - 255] 
void Set_RGB_Color(bool R, bool G, bool B, unsigned int Brightness){
    // Checks what color that the user wants to set and sets the pin to the correct brightness  

    // Calculates the on duty cycle for the one pin (Simulates a PWM pin)
    const double PWM_Frequency = 2040; // Frequency of the PWM signal
    double On_Duty_Cycle = PWM_Frequency - (((255.0d - (double)Brightness) / 255) * PWM_Frequency); // Calculates the on duty cycle of the PWM signal
    double Off_Duty_Cycle = PWM_Frequency - On_Duty_Cycle; // Calculates the off duty cycle of the PWM signal

    // Debugging
    // Serial.print("On Duty Cycle: ");
    // Serial.println(On_Duty_Cycle);
    // Serial.print("Off Duty Cycle: ");
    // Serial.println(Off_Duty_Cycle);

    // RED LED 
    if(R){
        // Checks if the on duty cycle is just equal to 0 which means that the LED is just on 
        if(Brightness == 255){
            digitalWrite(RED_LED_PIN, HIGH); // Turns on the LED
        }

        // Checks if the off duty cycle is just equal to 0 which means that the LED is just off
        if(Brightness == 0){
            digitalWrite(RED_LED_PIN, LOW); // Turns off the LED
        }

        // Waits for the timer to elapse 
        if(Brightness != 0 && R_LED_TIMER.Check_Time_Micros(On_Duty_Cycle)){
            digitalWrite(RED_LED_PIN, HIGH); // Turns on the LED
        }else if(Brightness != 0){
            digitalWrite(RED_LED_PIN, LOW); // Turns off the LED
        }        
    }

    // GREEN LED
    if(G){
        // Checks if the on duty cycle is just equal to 0 which means that the LED is just on
        if(Brightness == 255){
            digitalWrite(GREEN_LED_PIN, HIGH); // Turns on the LED
        }

        // Checks if the off duty cycle is just equal to 0 which means that the LED is just off
        if(Brightness == 0){
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
        if(Brightness == 255){
            digitalWrite(BLUE_LED_PIN, HIGH); // Turns on the LED
        }

        // Checks if the off duty cycle is just equal to 0 which means that the LED is just off
        if(Brightness == 0){
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


// ------------ [GETTERS] ------------ \\

// Reads the data from the Temperature sensor 
float Get_Temperature(){
    long temperature = analogRead(TEMPERATURE_SENSOR); // Range 0 - 1024

    // Convert the analog reading to voltage 
    float voltage = (temperature * 5.0) / 1024.0;

    // Convert the voltage to temperature in degrees celsius
    float celsius = (voltage - 0.5) * 100; 

    return celsius;
}

// Gets the humidity from the humidity sensor 
float Get_Humidity(){
    // Gets the humidity values fromt he sensor 
    long Humidity = analogRead(HUMIDITY_SENSOR); // Range 0 - 1024 

    // Converts the analog reading to a percentage of humidity 
    float humidity = (Humidity / 1024.0) * 100; 
    
    return humidity;
}

// Reads the potentiometer value and returns it as a fan speed in a custom range [0 - 255]
int Get_Fan_SetSpeed(){
    // Gets the potentiometer value 
    int potentiometer_value = analogRead(FAN_SPEED_CONTROL); // Range 0 - 1024

    // Converts the potentiometer value to a fan speed 
    int fan_speed = ((1024.0 - potentiometer_value) / 1024.0) * 255;

    return fan_speed;
}

// Reads the potentionmeter value and returns it as a temperature in a custom range [0 - 40]
int Get_Temperature_SetValue(){
    // Gets the potentiometer value 
    int potentiometer_value = analogRead(TEMPERATURE_CONTROL); // Range 0 - 1024

    // Converts the potentiometer value to a temperature 
    int temperature = ((1024.0 - potentiometer_value) / 1024.0) * 40;

    return temperature;
}

// ------------ [UTILITIES] ------------ \\

// Checks whether a specific time has elapsed given time in milliseconds 
// bool Check_Time(unsigned long Time){
//     // Checks if the system timer has started 
//     if(System_Timer_Started == false){
//         System_Timer = millis();
//         System_Timer_Started = true;
//     }

//     // Checks if the specific time has elapsed and resets the timer 
//     if(millis() - System_Timer >= Time && System_Timer_Started){
//         // Resets the system timer 
//         System_Timer_Started = false;
//         System_Timer = millis();

//         return true; 
//     } else {
//         return false; 
//     }
// }

// Buzzes the buzzer 
void Buzz(){
    tone(PIEZO, Buzzer_Tone, Buzzer_Duration);
}

// Checks whether the buttons have been pressed and will drop repeated values 
bool Check_Button(int Button){
    // Gets the button state given the pin
    int Button_State = digitalRead(Button); 

    // Checks if the button has already been pressed 
    bool Button_Pressed = Button_State == HIGH && Button_State_Cache[Button - 2] == false;

    // Updates the button state cache to the current button state 
    if(Button_Pressed){
        Button_State_Cache[Button - 2] = true; 
    } else if(Button_State == LOW && Button_State_Cache[Button - 2] == true){ // Only fires when the button is not pressed and the button state cache is true indicating that the button was pressed and is no longer pressed
        Button_State_Cache[Button - 2] = false; 
    } 

    Serial.println("FIREDD!"); 
    return Button_Pressed;
}


// Updates the color of the RGB LED depending on the current system status
void Update_RGB_LED(){
    // Temperature Ranges 
    // 0 - 5 = white (snow)
    // 5 - 10 = blue (cold)
    // 10 - 15 = green (cool)
    // 15 - 20 = yellow (warm)
    // 20 - 25 = orange (hot)
    // 25 - 30 = red (very hot)

    // Checks the current set temperature value 
    if(Set_Temperature < 5){
        // Super cold 
        Set_RGB_Color(true,true,true,Max_RGB_LED_Brightness); // White 
    }else if(Set_Temperature >= 5 && Set_Temperature < 10){
        // Cold 
        Set_RGB_Color(false,false,true,Max_RGB_LED_Brightness); // Blue
    }else if(Set_Temperature >= 10 && Set_Temperature < 15){
        // Cool 
        Set_RGB_Color(false,true,false,Max_RGB_LED_Brightness); // Green
    }else if(Set_Temperature >= 15 && Set_Temperature < 20){
        // Warm 
        Set_RGB_Color(true,true,false,Max_RGB_LED_Brightness); // Yellow
    }else if(Set_Temperature >= 20 && Set_Temperature < 25){
        // Hot 
        Set_RGB_Color(true,false,false,Max_RGB_LED_Brightness); // Orange
    }else if(Set_Temperature >= 25){
        // Super hot 
        Set_RGB_Color(true,false,false,Max_RGB_LED_Brightness); // Red
    }
}

// TOP DISPLAY 
// This displays the basic mode of the current system and whether the fan is on 
void Display_Basic_State_Top(){ 
    // Checks whether the display has changed and if so it will clear the lcd 
    if(Control_Display_Changed){
        CONTROL_LCD.clear();
        Control_Display_Changed = false; // Resets the control display changed variable
    }

    // Displays the system mode 
    CONTROL_LCD.setCursor(0, 0);
    CONTROL_LCD.print("Mode: ");
    CONTROL_LCD.print(System_Mode);

    // Displays the fan state 
    CONTROL_LCD.setCursor(0, 1);
    CONTROL_LCD.print("Fan State: ");

    if(System_Mode == "Auto"){
        CONTROL_LCD.print((Auto_Fan_State) ? "On" : "Off");
    } else {
        CONTROL_LCD.print((Fan_State) ? "On" : "Off");
    }

}

// Displays the fan speed when the user is currently changing the fan speed 
void Display_Fan_Speed_Top(){
    // Checks whether the display has changed and if so it will clear the lcd 
    if(Control_Display_Changed){
        CONTROL_LCD.clear();
        Control_Display_Changed = false; // Resets the control display changed variable
    }

    // Converts the fan speed into a percentage 
    int Fan_Speed_Percentage = (Fan_Speed / 255.0) * 100;

    // Displays the title 
    CONTROL_LCD.setCursor(0,0); 
    CONTROL_LCD.print("Set Fan Speed");

    // Displays the fan speed 
    CONTROL_LCD.setCursor(0, 1);
    CONTROL_LCD.print("Fan Speed: ");
    CONTROL_LCD.print(Fan_Speed_Percentage);
    CONTROL_LCD.print("%");
}

// Displays the current temperature control when the user is currently setting the desired temperature 
void Display_Set_Temperature(){
    // Checks whether the display has changed and if so it will clear the lcd 
    if(Control_Display_Changed){
        CONTROL_LCD.clear();
        Control_Display_Changed = false; // Resets the control display changed variable
    }

    // Displays the title
    CONTROL_LCD.setCursor(0,0);
    CONTROL_LCD.print("Set Temperature");

    // Displays the temperature 
    CONTROL_LCD.setCursor(0, 1);
    CONTROL_LCD.print("Temp: ");
    CONTROL_LCD.print(Set_Temperature);
    CONTROL_LCD.print("C");
}

//BOTTOM DISPLAY 
// This displays the humidity and temperature of the current environment 
void Display_Humidity_Temperature(){

    // Checks whether the display has changed and if so it will clear the lcd
    if(Status_Display_Changed){
        STATUS_LCD.clear();
        Status_Display_Changed = false; // Resets the status display changed variable
    }

    // Displays the temperature 
    STATUS_LCD.setCursor(0, 0);
    STATUS_LCD.print("Temp: ");
    
    STATUS_LCD.print(Temperature);
    STATUS_LCD.print("C");

    // Displays the humidity 
    STATUS_LCD.setCursor(0, 1);
    STATUS_LCD.print("Humidity: ");
    STATUS_LCD.print(Humidity);
    STATUS_LCD.print("%");
}

// NEOPIXEL LIGHTS 
// This will set the neopixel lights to a specific color 

void Basic_Neo_Pixel_Light_Pattern(const int (*Color_Pattern)[7][3], int Pattern_Length, int Delay_Time){
    // Checks if the fan state is on and if so it will turn on the neopixel lights 
    if(!Fan_State && !Auto_Fan_State){
        return; 
    }
  
    NEO_PIXEL.setBrightness(Max_NEOPIXEL_Brightness);
    
    // Checks whether that specific time has elapsed 
    if(!Neo_Pixel_Timer.Check_Time_Millis(Delay_Time)){
        Serial.println("Delay Time Not Elapsed");
        return;
    }

    // Gets the individual colors 
    int Red = (*Color_Pattern)[Pixel_Pattern_Index][0]; 
    int Green = (*Color_Pattern)[Pixel_Pattern_Index][1];
    int Blue = (*Color_Pattern)[Pixel_Pattern_Index][2]; 

    // Sets the neopixel lights to the specific color 
    NEO_PIXEL.setPixelColor(Pixel_Index, Red, Green, Blue); 
    NEO_PIXEL.show();

    // Updates the pixel pattern index 
    Pixel_Index++; 
    
    // Checks if the pixel index is less than 24
    if(Pixel_Index >= 24){
        Pixel_Pattern_Index++; 
    }

    // Checks if the pixel pattern index is greater than the pattern length 
    if(Pixel_Pattern_Index >= Pattern_Length){
        Pixel_Pattern_Index = 0; 
    }
}


// ------------ [SYSTEM] ------------ \\

// Updates the temperature and humidity data
void Update_Environment_Data(){
    // Gets the temperature and humidity 
    float Current_Temperature = Get_Temperature();
    float Current_Humidity = Get_Humidity();

    // Checks if there was any changes in the temperature and humidity 
    bool Temperature_Changed = Current_Temperature != Temperature; 
    bool Humidity_Changed = Current_Humidity != Humidity;

    // Updates the temperature and humidity
    Temperature = Current_Temperature;
    Humidity = Current_Humidity;

    // Sets the display changed variables to true if there was any changes in the temperature and humidity 
    if(Temperature_Changed){
        Status_Display_Changed = true;
    }

    if(Humidity_Changed){
        Status_Display_Changed = true;
    }
}

// Checks for user input and determines what to display in the top display
void Update_System_On_UserInput(){
    // Checks if the system received an IR signal as input 
    // if(IrReceiver.decode()){
    //     Serial.println(IrReceiver.decodedIRData.command);
    //     IrReceiver.resume(); // Resumes the IR receiver to receive more signals 
    // }

    // Checks if the auto button is pressed 
    if((Fan_State || Auto_Fan_State) && Check_Button(AUTO_BUTTON)){
        Serial.println("User changed system mode to auto");
        // Changes the Control display changed variable to true 
        Control_Display_Changed = true;

        // Checks if the current fan is on and if so it will turn on the manual fan 
        if(!Fan_State){
            Fan_State = true; 
        }

        // Checks if the mode is already auto and if so it will change it to manual 
        if(System_Mode == "Auto"){
            Auto_Fan_State = false; 

            Serial.println("User changed system mode to manual");

            // Sets the system mode to manual
            System_Mode = "Manual";
            return; 
        }

        // Buzzes the buzzer once 
        Buzz(); 

        // Sets the system mode to auto 
        System_Mode = "Auto";

        // Sets the fan state to on 
        Auto_Fan_State = true;  
        return; 
    }

    // Checks if the off button is pressed 
    if((Fan_State || Auto_Fan_State) && Check_Button(OFF_BUTTON)){
        Serial.println("User turned off fan");

        // Changes the Control display changed variable to true 
        Control_Display_Changed = true;

        // Buzzes the buzzer once 
        Buzz(); 

        // Sets the fan state to off 
        Fan_State = false;
        Auto_Fan_State = false;
        return; 
    }

    // Checks if the on button is pressed
    if((!Fan_State && !Auto_Fan_State) && Check_Button(ON_BUTTON)){
        Serial.println("User turned on fan");

        // Changes the Control display changed variable to true 
        Control_Display_Changed = true;

        // Buzzes the buzzer once 
        Buzz(); 

        // Sets the fan state to on depending on what the current mode is set at 
        if(System_Mode == "Auto"){
            Auto_Fan_State = true; 
        } else {
            Fan_State = true;
        }
        return; 
    }

    // Checks if the fan speed is being changed 
    if((Fan_State || Auto_Fan_State) && Get_Fan_SetSpeed() != Fan_Speed){
        Serial.println("User changed fan speed"); 

        int Current_Fan_Speed = Get_Fan_SetSpeed();

        // Sets the fan speed to the current fan speed 
        Fan_Speed = Current_Fan_Speed;

        // Changes the Control display changed variable to true 
        Control_Display_Changed = true;

        // Checks if the current mode is auto and if so it will be set to manual 
        if(System_Mode == "Auto"){
            // Sets the system mode to manual and returns control to manual user set values
            System_Mode = "Manual";

            Auto_Fan_State = false; 
        }

        // Changes the display to display the set fan speed in percentage
        Display_Fan_Speed_Top();

        // Changes the Control display changed variable to true 
        Control_Display_Changed = true;

        return; 
    }

    // Checks if the temperature is being changed 
    if((Fan_State || Auto_Fan_State) && Get_Temperature_SetValue() != Set_Temperature){
        Serial.println("User changed temperature");

        int Current_Temperature = Get_Temperature_SetValue();

        // Sets the temperature to the current temperature
        Set_Temperature = Current_Temperature;

        // Changes the Control display changed variable to true 
        Control_Display_Changed = true;

        // Checks if the current mode is manual and if so it will be set to auto 
        if(System_Mode == "Manual"){
            System_Mode = "Auto";
            Auto_Fan_State = true; 
        }

        // Changes the display to display the set temperature 
        Display_Set_Temperature();

        // Changes the Control display changed variable to true 
        Control_Display_Changed = true;

        return; 
    }

    // Returns to display the regular system display 

    Display_Basic_State_Top(); // Displays the basic state of the system after the delay
}

// Updates the system based on environmental changes such as room temperature 
void Update_System_On_External_Input(){
    // Checks if the current temperature is greater than the set temperature and if so it will turn on the fan 
    if(System_Mode == "Auto"){
        // Checks if the temperature is greater than the set temperature 
        if(Temperature > Set_Temperature){
            // Checks if the fan is already on and if so it will return 
            if(Auto_Fan_State){
                return; 
            }

            // Sets the fan state to on 
            Auto_Fan_State = true; 

            // Changes the Control display changed variable to true 
            Control_Display_Changed = true;

            // Buzzes the buzzer once 
            Buzz(); 

            return; 
        }else if(Temperature <= Set_Temperature){
            // Checks if the fan is already off and if so it will return 
            if(!Auto_Fan_State){
                return; 
            }

            // Sets the fan state to off 
            Auto_Fan_State = false; 

            // Changes the Control display changed variable to true 
            Control_Display_Changed = true;

            // Buzzes the buzzer once 
            Buzz(); 

            return; 
        }

    }
}

// Updates the components in the system based on the system status and mode 
void Update_System_Components(){
    // Checks whether the system is in auto mode and it will automatically adjust the fan speed based on the current temperature and the set temperature 
    if(Auto_Fan_State && System_Mode == "Auto" && Temperature > Set_Temperature){
        // Sets the fan speed to the temperature difference within a range of 0 to 255
        //Auto_Fan_Speed = (abs(Temperature - Set_Temperature)/85) * 255; 
        Auto_Fan_Speed = constrain(map(Temperature, Set_Temperature, 40, 0, 255),0,255);

        // Sets the fan speed 
        Set_Fan_Speed(Auto_Fan_Speed);

        return; 
    }else if (!Auto_Fan_State && System_Mode == "Auto" && Temperature <= Set_Temperature){

        Set_Fan_Speed(0); // Turns off the fan 
        return; 
    }
    
    //Updates the fan motor
    if(Fan_State && !Auto_Fan_State){
        // Turns on the green LED and turns off the red LED
        digitalWrite(GREEN_LED, HIGH); 
        digitalWrite(RED_LED, LOW); 

        // Sets the fan speed to the current set fan speed 
        Set_Fan_Speed(Fan_Speed);

    }else if(!Auto_Fan_State){
        // Turns off the green LED and turns on the red LED
        digitalWrite(GREEN_LED, LOW); 
        digitalWrite(RED_LED, HIGH); 

        // Sets the fan speed to 0 
        Set_Fan_Speed(0);
    }
}


// MAIN 

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

    Display_Basic_State_Top(); // Displays the basic state of the system

    // Sets the basic state of the system 
    Temperature = Get_Temperature();
    Humidity = Get_Humidity();
    Fan_Speed = Get_Fan_SetSpeed();
    Set_Temperature = Get_Temperature_SetValue();

    // Neopixel initialization
    Array_Size = sizeof(Neo_pixel_color_pattern) / sizeof(Neo_pixel_color_pattern[0]);
}

// System loop 
void loop(){

    // Updates the system based on external input 
    Update_System_On_External_Input();

    // Updates the system based on user input 
    Update_System_On_UserInput();

    // Updates the system components
    Update_System_Components();

    // Updates the temperatures and humidity data 
    Update_Environment_Data();

    // Displays the current humidity and temperature on the bottom lcd 
    Display_Humidity_Temperature();

    // Updates the NeoPixel 
    Basic_Neo_Pixel_Light_Pattern(&Neo_pixel_color_pattern,Array_Size, 10);

    // Updates the RGB LED 
    Update_RGB_LED();

    // Debugging
    Serial.println("Running"); 

}

