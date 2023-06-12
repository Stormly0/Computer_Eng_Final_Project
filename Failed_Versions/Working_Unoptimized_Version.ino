// VERSION 1.0.5 alpha 
// IMPORTS 
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>

// Definitions 
#define VIRTUAL_ENVIRONMENT false

// Buttons
#define AUTO_BUTTON 8
#define OFF_BUTTON 7
#define ON_BUTTON 6

// LEDs
#define GREEN_LED 5
#define RED_LED 4
#define WARNING_LED 3

// Buzzer
#define PIEZO 2

// Fan Motor
#define DC_MOTOR 11 

// IR Receiver pin 
#define IR_RECEIVER 9

// Temperature Sensor 
#define TEMPERATURE_SENSOR A2
//#define HUMIDITY_SENSOR A3

// Potentionmeters 
#define FAN_SPEED_CONTROL A1
#define TEMPERATURE_CONTROL A0

// Neo Pixel Pin 
#define NEO_PIXEL_PIN 10 

// LCD Addresses 
#if VIRTUAL_ENVIRONMENT 
    #define TOP_LCD_ADDRESS 33 
    #define BOTTOM_LCD_ADDRESS 32 
#else
    #define LCD_ADDRESS 0x27
#endif

// Object Initializations 

// LCDs 
#if VIRTUAL_ENVIRONMENT 
// Virtual Environment
    LiquidCrystal_I2C TOPLCD(TOP_LCD_ADDRESS, 16, 2);
    LiquidCrystal_I2C BOTTOMLCD(BOTTOM_LCD_ADDRESS, 16, 2);
#else   
    // Real Environment 
    LiquidCrystal_I2C LCD(LCD_ADDRESS, 20, 4);
#endif

// NeoPixel
Adafruit_NeoPixel NEOPIXEL(24,NEO_PIXEL_PIN,NEO_GRB + NEO_KHZ800);

// ---------------- [TOP LEVEL SYSTEM VARIABLES] ----------------
// System State 
bool Integrity = true; // Indicates whether the system is able to continue program execution 

// ==== [SYSTEM STATES] ====

// System is on or off 
String System_Mode = "Manual"; // The mode that the system is in [Manual, Auto]
bool Active = false; // Whether the system is active or not (Power button is pressed)

float Set_Temperature = 0; // The temperature that the user sets
float Temperature = 0; // The temperature of the system's surrounding 
float Fan_Speed = 0; // The speed that the fan is running at [0-255]
bool Fan_State = false; // Indicates if the fan is spinning or not 

// Displays 

bool Set_Fan_Speed_Changed = false; // Indicates whether the set fan speed has changed
bool Set_Temp_Changed = false; // Indicates whether the set temperature has changed 
bool Status_Changed = false; // Indicates whether any of the system variables has changed (Mode, and active)

// System Cache Variables 
bool Button_State_Cache[3] = {false,false,false}; // The button state cache
unsigned int Pixel_Light_Index = 0; 
unsigned int Pixel_Pattern_Index = 0; 
unsigned int Pixel_Pattern_Size = 7; // Size of the color pattern array 

// ------------ [Variables] ------------

// --- [Enums] ---
// Display types for the virtual LCD's and the real LCD 
enum Display_Name{
    TOP,
    BOTTOM,
    SINGLE
}; 

enum Screen{
    MAIN,
    SET_TEMP, 
    SET_FAN
};

// System Configurations 


//Neo Pixel
bool Neo_Pixel_State = false; // Indicates whether the NeoPixel is on or off
const unsigned int Neo_Pixel_Delay_Between_Lights = 100; // Delay between each light in the NeoPixel [ms]
const unsigned int Max_Neo_Pixel_Brightness = 10; // Max Brightness of the NeoPixel [0 - 255]
const int Neo_Pixel_Color_Pattern[][3] = {
    {255,0,0}, // Red
    {0,255,0}, // Green
    {0,0,255}, // Blue
    {255,255,0}, // Yellow
    {255,0,255}, // Purple
    {0,255,255}, // Cyan
    {255,255,255}, // White
};

// Buzzer Configurations 
const unsigned int Buzzer_Frequency = 100; // The frequency of the buzzer in hertz 
const unsigned int Buzzer_Duration = 100; // The duration of the buzzer in milliseconds

// Sensor Configurations 
const float Change_Detect_Tolerance = 2.5; // The tolerance of the change detection in Celsius
const float Change_In_Fan_Speed_Tolerance = 2; // The tolerance of the change in fan speed 
const float Change_In_Set_Temperature_Tolerance = 2; // The tolerance of the change in set temperature

// LCD Configurations 

// LCD switch screen delay 
Screen Current_Screen = MAIN; // The current screen that the LCD is displaying
const unsigned int LCD_Switch_Screen_Delay = 2000; // The delay between switching screens in milliseconds
const unsigned int Max_LCD_Refresh_Rate = 100; // The maximum refresh rate of the LCD in milliseconds


// ------------ [CLASSES] ------------ 

// Timer that allows for a delay without blocking the main thread execution 
class Timer{

    // Constructor 
    public: 
        Timer(char* Clock_Name){
            this->Clock_Name = Clock_Name; 
        }

    // Private 
    private: 
        // Timer current state 
        bool Running = false; 
        char* Clock_Name = ""; 

        // Milisecond Timers 
        unsigned long _milisecond_elapsed = 0; 
        
        // Microsecond timers 
        unsigned long _microsecond_elapsed = 0; 

    // Public
    public: 
        // Checks whether a specific time has elapsed in milliseconds 
        bool Milliseconds(unsigned long Time){
            // Checks the system integrity to determine whether the system has shutdown
            if(!Integrity){
                // System is not able to continue running 
                Serial.println(("Millisecond timer " + String(this->Clock_Name) + " has stopped due to system integrity failure")); 
                return false; 
            }

            // Checks if the timer has started 
            if(this->Running == false){
                this->_milisecond_elapsed = millis(); // Starts the time elapsed 
                this->Running = true; // Sets the timer to running
            }

            // Checks if the time has elapsed and restarts the timer 
            if((millis() - this->_milisecond_elapsed >= Time) && this->Running){
                this->Running = false; // Sets the timer to not running 
                return true; // Returns true 
            }else{
                return false; // Returns false 
            }
        }

        // Checks whether a specific time has elapsed in microseconds 
        bool Microseconds(unsigned long Time){
            // Checks the system integrity to determine whether the system has shutdown 
            if(!Integrity){
                // System is not able to continue running 
                Serial.println(("Microsecond timer " + String(this->Clock_Name) + " has stopped due to system integrity failure")); 
                return false; 
            }

            // Checks if the timer has started 
            if(this->Running == false){
                this->_microsecond_elapsed = micros(); // Starts the time elapsed 
                this->Running = true; // Sets the timer to running
            }else{
                // Cannot run the timer in microseconds and milliseconds at the same time 
                // Crashes the program in arduino 


                return false;
            }

            // Checks if the time has elapsed and restarts the timer 
            if((micros() - this->_microsecond_elapsed >= Time) && this->Running){
                this->Running = false; // Sets the timer to not running 
                return true; // Returns true 
            }else{
                return false; // Returns false 
            }
        }
};

// ---------------- [Objects] ----------------

// Timers 
Timer LCD_Timer_1("LCD TIMER 1");
Timer LCD_Refresh_Rate_Timer("LCD REFRESH RATE TIMER");
Timer Neo_Pixel_Timer("NEO PIXEL TIMER"); 


// ------------ [Functions] ------------

// Gets the temperature from the temperature sensor : returns [Celsius]
float Get_Temperature(){
    float Voltage = analogRead(TEMPERATURE_SENSOR); 

    // Checks whether we are in a virtual environment or not 
    #if VIRTUAL_ENVIRONMENT
        float Converted_Voltage = Voltage * (5.0 / 1023.0); // Converts the voltage to a value between 0 and 5 volts

        // Converts it into a temperature 
        float Temperature = (Converted_Voltage - 0.5) * 100; // Converts the voltage to a temperature in celsius

        // Virtual environment (TPM36)
        return Temperature; 
    #else
        // Real environment (LM35) 

        // Gets the signal and converts it to voltage 
        float Converted_Voltage = Voltage * (5.0 / 1024.0); // Converts the voltage to a value between 0 and 5 volts

        float Temperature = Converted_Voltage * 100; 

        return Temperature; 
    #endif
}

// Gets the set fan speed [127.5 - 255]
float Get_Set_Fan_Speed(){
    // Gets the reading from the potentiometer
    float Voltage = analogRead(FAN_SPEED_CONTROL);

    // 127.5 -> 255.0 - 127.5 
    // Converts the voltage to a speed value within the range of 50 - 255 
    float Range = ((Voltage/1023.0) * 127.5) + 127.5; 

    return Range; 
}

// Gets the temperature setting and returns it in a range of [0-40]C 
float Get_Set_Temperature(){
    // Gets the reading from the potentiometer
    float Voltage = analogRead(TEMPERATURE_CONTROL);

    // Converts the voltage to a temperature value within the range of 0 - 40 
    float Range = (Voltage/1023.0) * 40.0; 

    return Range; 
}

// ------------ [UTILITIES] ------------ 

// Buzzes the buzzer for a certain period of time 
inline void Buzz(){
    tone(PIEZO,Buzzer_Frequency,Buzzer_Duration);
}

// Checks whether a specific button has been pressed 
// IMPORTANT: WILL DROP REPEATED VALUES 
bool Check_Button(int Button){
    // Gets the button state 
    int STATE = digitalRead(Button); 

    // Checks if the button has already been pressed 
    bool Button_Registered = STATE == HIGH && Button_State_Cache[abs(Button - 6)] == false; 

    // Checks if the button has been pressed 
    if(Button_Registered){
        Serial.print("Button: ");
        Serial.print(Button);
        Serial.println(" has been pressed!");
        // Button has been pressed
        Button_State_Cache[abs(Button - 6)] = true; // Sets the button state to true
    }else if(STATE == LOW && Button_State_Cache[abs(Button - 6)] == true){
        // Button has been released
        Button_State_Cache[abs(Button - 6)] = false; // Sets the button state to false
    }

    return Button_Registered;
}

// ------------ [SYSTEM] ------------ 

// Updates the environment sensors (Temperature sensors)
void Update_Environment_Data(){
    // Gets the temperature data 
    float Current_Temperature = Get_Temperature();

    // Compares and checks if the difference is high enough to warrant a refresh 
    bool Temperature_Changed = abs(Current_Temperature - Temperature) > Change_Detect_Tolerance;

    // Checks if the temperature has changed 
    if(Temperature_Changed){
        // Updates the temperature value 
        Temperature = Current_Temperature;
    }
}

// Updates the system based on input from the buttons and sensors 
void Update_System_On_Input(){
    // Checks runtime environment 
    #if VIRTUAL_ENVIRONMENT 
        // Virtual environment 
        // ----- [BUTTONS] -----
        
        //Checks if the auto button was pressed 
        if(Active && Check_Button(AUTO_BUTTON)){
            Serial.println("Auto Button Pressed!");
            // Buzzes the buzzer 
            Buzz(); 

            // Checks if the system is in auto mode 
            if(System_Mode == "Auto"){
                // Switches to manual mode 
                System_Mode = "Manual";
            }else{
                // Switches to auto mode 
                System_Mode = "Auto";
            }

            Status_Changed = true; // Sets the status changed to true as the system has changed modes 
        }

        // Checks if the on button was pressed 
        if(!Active && Check_Button(ON_BUTTON)){
            Serial.println("On Button Pressed!");
            // Buzzes the buzzer 
            Buzz(); 

            // Sets the system to active 
            Active = true; 

            Status_Changed = true; // Sets the status changed to true as the system has gone from inactive to active
        }

        // Checks if the off button was pressed
        if(Active && Check_Button(OFF_BUTTON)){
            Serial.println("Off Button Pressed!");
            // Buzzes the buzzer 
            Buzz(); 

            // Sets the system to inactive 
            Active = false; 

            Status_Changed = true; // Sets the status changed to true as the system has gone from active to inactive
        }


        // ----- [POTENTIOMETERS] -----

        // Checks if the user changed the set fan speed 
        if(Active && abs(Get_Set_Fan_Speed() - Fan_Speed) > Change_In_Fan_Speed_Tolerance){
            Serial.println("Fan Speed Changed!");
            
            // Checks if the current mode is auto and if so it will change it back to manual mode since the user wants to manually change the fan speed 
            if(System_Mode == "Auto"){
                System_Mode = "Manual"; 
            }

            // Updates the fan speed 
            Fan_Speed = Get_Set_Fan_Speed(); 

            // Sets the current screen to the fan speed screen
            Current_Screen = SET_FAN;            
        }

        // Checks if the user changed the set temperature
        if(Active && abs(Get_Set_Temperature() - Set_Temperature) > Change_In_Set_Temperature_Tolerance){
            Serial.println("Set Temperature Changed!");

            // Checks if the current mode is manual and if so it will change it back to auto mode since the user wants to have the system automatically change the fan speed 
            if(System_Mode == "Manual"){
                System_Mode = "Auto"; 
            }
                

            // Updates the set temperature 
            Set_Temperature = Get_Set_Temperature(); 

            // Sets the current screen to the set temperature screen
            Current_Screen = SET_TEMP;

        }
    
    #else 
        // Real environment 
        // ----- [BUTTONS] -----
        
        //Checks if the auto button was pressed 
        if(Active && Check_Button(AUTO_BUTTON)){
            Serial.println("Auto Button Pressed!");
            // Buzzes the buzzer 
            Buzz(); 
    
            // Checks if the system is in auto mode 
            if(System_Mode == "Auto"){
                // Switches to manual mode 
                System_Mode = "Manual";
            }else{
                // Switches to auto mode 
                System_Mode = "Auto";
            }

            // Sets the status changed flag to true as the system has changed modes 
            Status_Changed = true;
    
        }
    
        // Checks if the on button was pressed 
        if(!Active && Check_Button(ON_BUTTON)){
            Serial.println("On Button Pressed!");
            // Buzzes the buzzer 
            Buzz(); 
    
            // Sets the system to active 
            Active = true; 

            Status_Changed = true; // Sets the status changed flag to true as the system has changed from inactive to active
        }
    
        // Checks if the off button was pressed
        if(Active && Check_Button(OFF_BUTTON)){
            Serial.println("Off Button Pressed!");
            // Buzzes the buzzer 
            Buzz(); 
    
            // Sets the system to inactive 
            Active = false; 

            Status_Changed = true; // Sets the status changed flag to true as the system has changed from active to inactive
        }

        // ----- [POTENTIOMETERS] -----

        // Checks if the user changed the set fan speed
        if(Active && abs(Get_Set_Fan_Speed() - Fan_Speed) > Change_In_Fan_Speed_Tolerance){
            Serial.println("Fan Speed Changed!");

            // Checks if the current mode is auto and if so it will change it back to manual mode since the user wants to manually change the fan speed 
            if(System_Mode == "Auto"){
                System_Mode = "Manual"; 
            }
    
            // Updates the fan speed 
            Fan_Speed = Get_Set_Fan_Speed(); 

            // Sets the current screen to the set fan speed screen
            Current_Screen = SET_FAN;

        }

        // Checks if the user changed the set temperature
        if(Active && abs(Get_Set_Temperature() - Set_Temperature) > Change_In_Set_Temperature_Tolerance){
            Serial.println("Set Temperature Changed!"); 

            // Checks if the current mode is manual and if so it will change it back to auto mode since the user wants to have the system automatically change the fan speed 
            if(System_Mode == "Manual"){
                System_Mode = "Auto"; 
            }
            
            // Updates the set temperature 
            Set_Temperature = Get_Set_Temperature(); 

            // Sets the current screen to the set temperature screen
            Current_Screen = SET_TEMP; 
        }
    #endif

}

// Updates the system components based on the current state of the system 
void Update_System_Components(){
    // Checks if the system is active 
    if(Active){
        // Turns on the LCD backlights 
        #if VIRTUAL_ENVIRONMENT 
            // Virtual Environment 
            TOPLCD.backlight();
            BOTTOMLCD.backlight();
        #else
            // Real Environment 
            LCD.backlight();
        #endif

        // Turns on the Green LED 
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED,LOW);  
    }else{
        // Turns off the LCD backlights 
        #if VIRTUAL_ENVIRONMENT 
            // Virtual Environment 
            TOPLCD.noBacklight();
            BOTTOMLCD.noBacklight();
        #else
            // Real Environment 
            LCD.noBacklight();
        #endif

        // Turns off the fan state 
        Fan_State = false; 

        // Clears the NeoPixel 
        NEOPIXEL.clear(); 
        NEOPIXEL.show();

        // Resets the NeoPixel index 
        Pixel_Light_Index = 0; 
        Pixel_Pattern_Index = 0; 

        // Turns off the Green LED 
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED,HIGH);
    }


    // ---- [AUTO MODE] ----

    // Checks if the system is in auto mode
    if(Active && System_Mode == "Auto"){
        // Checks if the temperature is above the set temperature 
        if(Get_Temperature() > Set_Temperature){
            // Turns on the fan 
            Fan_State = true; 
            
        }else{
            // Turns off the fan 
            Fan_State = false; 
        }
    }

    // ---- [MANUAL MODE] ----

    // Checks if the system is in manual mode
    if(Active && System_Mode == "Manual"){
        // Checks if the fan speed is greater than 0 
        if(Fan_Speed >= 50){
            // Turns on the fan 
            Fan_State = true;
        }else if(!Active){
            // Turns off the fan 
            Fan_State = false; 
        }
    }

    // ---- [FAN CONTROL] ----

    // Checks for the fan state 
    if(Active && Fan_State){
        // Turns on the fan 
        
        // Checks if it is in auto mode 
        if(System_Mode == "Auto"){
            analogWrite(DC_MOTOR,255);
        }else{
            analogWrite(DC_MOTOR,Fan_Speed);
        }

    }else{
        // Turns off the fan 
        analogWrite(DC_MOTOR,0);
    }

    // ---- [DISPLAY] ----

    // Checks if the system state has changed 
    #if VIRTUAL_ENVIRONMENT 
        // Virtual Environment 
        if(Status_Changed){
            Serial.println("STATUS CHANGED!"); 
            // Clears the top LCD 
            TOPLCD.clear(); 
            // Sets the status back to unchanged 
            Status_Changed = false;

            // Sets the temp and fan speed flags to false as this takes precedence
            Set_Temp_Changed = false;
            Set_Fan_Speed_Changed = false;

            // Changes the display back to main to display the mode 
            Current_Screen = MAIN;
        }

        // Checks if the current screen is not main and switches it back after a delay 
        if(Current_Screen != MAIN && LCD_Timer_1.Milliseconds(LCD_Switch_Screen_Delay)){
            Current_Screen = MAIN; 

            // Clears the top screen 
            TOPLCD.clear();

            // Sets both the set temperature and set fan speed changed flags to false
            Set_Temp_Changed = false;
            Set_Fan_Speed_Changed = false;

            Serial.println("Switching back to Main Screen"); 
            
            // Clears the top screen 
            TOPLCD.clear();
        }

        // Checks to determine what screen to display 
        switch (Current_Screen)
        {
            case MAIN:
                // Displays the main screen 
                TOPLCD.setCursor(0,0);
                TOPLCD.print("Mode: " + System_Mode);
                TOPLCD.setCursor(0,1);
                TOPLCD.print("Fan State: " + (String) ((Fan_State) ? "On" : "Off"));
                break;
            case SET_TEMP:
                // Clears the screen
                if(!Set_Temp_Changed){
                    TOPLCD.clear(); 
                    Set_Temp_Changed = true; 
                }else{
                    // Clears the bottom line 
                    TOPLCD.setCursor(0,1);
                    TOPLCD.print("                ");
                }

                // Displays the set temperature screen 
                TOPLCD.setCursor(0,0);
                TOPLCD.print("Set Temperature");
                TOPLCD.setCursor(0,1);
                TOPLCD.print("Temp: " + (String) Set_Temperature + "C");
                break;
            case SET_FAN:
                // Clears the screen
                if(!Set_Fan_Speed_Changed){
                    TOPLCD.clear(); 
                    Set_Fan_Speed_Changed = true;
                }else{
                    // Clears the bottom line 
                    TOPLCD.setCursor(0,1);
                    TOPLCD.print("                ");
                }

                // Displays the set fan speed screen 
                TOPLCD.setCursor(0,0);
                TOPLCD.print("Set Fan Speed");
                TOPLCD.setCursor(0,1);

                // Converts fan speed to a percentage and outputs it to the screen

                TOPLCD.print("Speed: " + (String) ((Fan_Speed/255) * 100) + "%");
                break;
        }

        // Displays the bottom display temperature in specific intervals 
        if(LCD_Refresh_Rate_Timer.Milliseconds(Max_LCD_Refresh_Rate)){
            // Clears the bottom LCD 
            BOTTOMLCD.setCursor(0,1); 
            BOTTOMLCD.print("                "); 

            // Displays the temperature on the bottom LCD 
            BOTTOMLCD.setCursor(0,0);
            BOTTOMLCD.print("Temperature");
            BOTTOMLCD.setCursor(0,1);
            BOTTOMLCD.print("Temp: " + (String) Get_Temperature() + "C"); // Gets the temperature and displays it 
        }

    #else 
        // Real Environment
        if(Status_Changed){
            // Clears the main LCD 
            LCD.clear(); 

            // Sets the status back to unchanged
            Status_Changed = false; 

            // Sets the temp and fan speed flags to false as this takes precedence
            Set_Temp_Changed = false;
            Set_Fan_Speed_Changed = false;

            // Changes the display back to main to display the mode 
            Current_Screen = MAIN;
        }

         // Checks if the current screen is not main and switches it back after a delay 
        if(Current_Screen != MAIN && LCD_Timer_1.Milliseconds(LCD_Switch_Screen_Delay)){
            Current_Screen = MAIN; 
            Serial.println("Switching back to Main Screen"); 

            // Resets the set temperature and set fan speed changed flags 
            Set_Temp_Changed = false;
            Set_Fan_Speed_Changed = false;

            // Clears the top screen 
            LCD.clear();
        }

        // Checks to determine what screen to display
        switch (Current_Screen)
        {
            case MAIN:
                // Displays the main screen 
                LCD.setCursor(0,0);
                LCD.print("Mode: " + System_Mode);
                LCD.setCursor(0,1);
                LCD.print("Fan State: " + (String) ((Fan_State) ? "On" : "Off"));
                break;
            case SET_TEMP:
                // Clears the screen
                if(!Set_Temp_Changed){
                    LCD.clear(); 
                    Set_Temp_Changed = true; 
                }else{
                    // Clears the bottom line 
                    LCD.setCursor(0,1);
                    LCD.print("                ");
                }

                // Displays the set temperature screen 
                LCD.setCursor(0,0);
                LCD.print("Set Temperature");
                LCD.setCursor(0,1);
                LCD.print("Temp: " + (String) Set_Temperature + "C");
                break;
            case SET_FAN:
                // Clears the screen
                if(!Set_Fan_Speed_Changed){
                    LCD.clear(); 
                    Set_Fan_Speed_Changed = true;
                }else{
                    // Clears the bottom line 
                    LCD.setCursor(0,1);
                    LCD.print("                ");
                }

                // Displays the set fan speed screen 
                LCD.setCursor(0,0);
                LCD.print("Set Fan Speed");
                LCD.setCursor(0,1);
                LCD.print("Speed: " + (String) ((Fan_Speed/255) * 100) + "%");
                break;
        }

       // Displays the bottom display temperature in specific intervals 
        if(LCD_Refresh_Rate_Timer.Milliseconds(Max_LCD_Refresh_Rate)){
            // Clears the bottom LCD 
            LCD.setCursor(0,3); 
            LCD.print("                ");
        
            // Displays the temperature on the bottom LCD 
            LCD.setCursor(0,2);
            LCD.print("Temperature");
            LCD.setCursor(0,3);
            LCD.print("Temp: " + (String) Get_Temperature() + "C"); // Gets the temperature and displays it 
        }


    #endif
}

// Displays the NeoPixel lights using the provided pattern 
void Update_Neo_Pixel(){
    // Checks if the fan is off and the neopixel is on  
    if(!Active){
        // Only turns it off if the neopixel is on 
        if(Neo_Pixel_State){
            // Turns off the Neo_Pixel 
            NEOPIXEL.clear(); 
            NEOPIXEL.show();
            Neo_Pixel_State = false;
        }
        return; 
    }

    //Checks if the delay time has not passed
    if(!Neo_Pixel_Timer.Milliseconds(Neo_Pixel_Delay_Between_Lights)){
        return; // Returns as the delay time has not passed 
    }

    Neo_Pixel_State = true; // Indicates that the Neo_Pixel is on

    // Gets the individual colors in the array pattern 
    int R = Neo_Pixel_Color_Pattern[Pixel_Pattern_Index][0];
    int G = Neo_Pixel_Color_Pattern[Pixel_Pattern_Index][1];
    int B = Neo_Pixel_Color_Pattern[Pixel_Pattern_Index][2];

    // Sets the Neo_Pixel color 
    NEOPIXEL.setPixelColor(Pixel_Light_Index, R, G, B);
    // Sets the Neo_Pixel max brightness 
    NEOPIXEL.setBrightness(Max_Neo_Pixel_Brightness);
    NEOPIXEL.show();

    // Increments the pixel light index 
    Pixel_Light_Index++;

    // Checks if the pixel light index is greater than the max pixel lights
    if(Pixel_Light_Index >= 24){
        // Resets the pixel light index 
        Pixel_Light_Index = 0;

        // Increments the pixel pattern index 
        Pixel_Pattern_Index++;

        // Checks if the pixel pattern index is greater than the max pixel patterns 
        if(Pixel_Pattern_Index >= 4){
            // Resets the pixel pattern index 
            Pixel_Pattern_Index = 0;
        }
    }
}


// MAIN 

// System Start 
void setup(){
    // Initialization 
    Serial.begin(9600); // Starts the serial communication

    // Initializes the LCD's
    #if VIRTUAL_ENVIRONMENT 
        // Virtual Environment 
        TOPLCD.init(); 
        BOTTOMLCD.init();
    #else
        // Real Environment 
        LCD.init();
    #endif

    // -- [PIN INITIALIZATION] --

    // DIGITAL INPUTS 
    pinMode(ON_BUTTON,INPUT); // On Button
    pinMode(OFF_BUTTON,INPUT); // Off Button
    pinMode(AUTO_BUTTON,INPUT); // Up Button

    // ANALOG INPUTS 
    pinMode(TEMPERATURE_SENSOR,INPUT); // Temperature Sensor
    pinMode(FAN_SPEED_CONTROL,INPUT); // Fan Speed Control
    pinMode(TEMPERATURE_CONTROL,INPUT); // Temperature Control

    // DIGITAL OUTPUTS
    pinMode(GREEN_LED,OUTPUT); // Green LED
    pinMode(RED_LED,OUTPUT); // Red LED
    pinMode(DC_MOTOR,OUTPUT); // DC Motor
    pinMode(PIEZO,OUTPUT); // Piezo

    // START 
    
    // NeoPixel starts 
    NEOPIXEL.begin();

    // Turns on the backlight on the LCD 
    #if VIRTUAL_ENVIRONMENT 
        // Virtual Environment
        TOPLCD.backlight();
        BOTTOMLCD.backlight();
    #else
        // Real Environment 
        LCD.backlight(); 
    #endif

    //Gets the set temperature and set fan speed 
    Fan_Speed = Get_Set_Fan_Speed(); 
    Set_Temperature = Get_Set_Temperature();   

}


void loop(){
    Update_Environment_Data(); 
    Update_System_On_Input(); 
    Update_System_Components();
    Update_Neo_Pixel(); 

}
