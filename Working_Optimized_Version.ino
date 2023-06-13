// VERSION 3.0 
// Added: IR sensor support 

// IMPORTS 
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>

// Definitions 
#define VIRTUAL_ENVIRONMENT true

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

// ------------ [Variables] ------------

// Display types for the virtual LCD's and the real LCD 
enum Display_Name{
    TOP,
    BOTTOM,
    SINGLE
}; 

// The different screens that the LCD can display
enum Screen{
    MAIN,
    SET_TEMP, 
    SET_FAN
};

// Different patterns that the neopixel can display 
enum Pixel_Patterns{
    CLOCKWISE, // Normal clockwise pattern 
    SPEED_DEMON, // Super fast pattern that spins in circles with zero delay
};

// System Configurations 
//Neo Pixel

const Pixel_Patterns Neo_Pixel_Mode = SPEED_DEMON; 
const unsigned int Neo_Pixel_Delay_Between_Lights = 0; // Delay between each light in the NeoPixel [ms]
const unsigned int Max_Neo_Pixel_Brightness = 10; // Max Brightness of the NeoPixel [0 - 255]
const int Neo_Pixel_Color_Pattern[][3] = {
    {255,0,0}, // Red
    {0,255,0}, // Green
    {0,0,255}, // Blue
    {255,255,0}, // Yellow
    {255,0,255}, // Purple
    {0,255,255}, // Cyan
};

// Buzzer Configurations 
const unsigned int Buzzer_Frequency = 100; // The frequency of the buzzer in hertz 
const unsigned int Buzzer_Duration = 100; // The duration of the buzzer in milliseconds

// Sensor Configurations 

// Sensor delays 
const float Temperature_Sensor_Refresh_Rate = 5000; // The rate that we read the temperature from the temperature sensor

const float Temperature_Change_Tolerance = 3.5; // The tolerance of the change detection in Celsius
const float Change_In_Fan_Speed_Tolerance = 2; // The tolerance of the change in fan speed 
const float Change_In_Set_Temperature_Tolerance = 2; // The tolerance of the change in set temperature

// LCD Configurations 

// LCD switch screen delay 
Screen Current_Screen = MAIN; // The current screen that the LCD is displaying
const unsigned int LCD_Switch_Screen_Delay = 2000; // The delay between switching screens in milliseconds
const unsigned int Max_TEMP_LCD_Refresh_Rate = 500; // The maximum refresh rate of the LCD in milliseconds

// ==== [SYSTEM STATES] ====

// System States
String System_Mode = "Manual"; // The mode that the system is in [Manual, Auto]
bool Active = false; // Whether the system is active or not (Power button is pressed)

// Fan control and states
float Set_Temperature = 0; // The temperature that the user sets
float Temperature = 0; // The temperature of the system's surrounding 
float Fan_Speed = 0; // The speed that the fan is running at [0-255]
bool Fan_State = false; // Indicates if the fan is spinning or not 

// Displays 
bool LCD_State = false; // Indicates whether the backlight is on or off 

// Status controls
bool Set_Fan_Speed_LCD_Cleared = false; // Indicates whether the fan speed LCD was cleared already
bool Set_Temp_LCD_Cleared = false; // Indicates whether the set temp LCD was cleared already
bool Status_Changed = false; // Indicates whether any of the system variables has changed (Mode, and active)

bool Set_Fan_Speed_Changed = false; // Indicates whether the set fan speed has changed 
bool Set_Temp_Changed = false; // Indicates whether the set temperature has changed

// Temperature sensors 
bool Refreshed = false; // Indicates whether the temperature has been refreshed 

// IR Sensors 
bool Repeat_Code = false; // Indicates whether the button is held down or not
unsigned long Previous_Code = 0x00000000; // The previous code that was received from the IR sensor

// NeoPixel 
bool Neo_Pixel_State = false; // Indicates whether the NeoPixel is on or off
bool Odd = false; // Indicates whether the fill state is currently on odd or even
bool Start = false; // Indicates whether the fill mode has started or not 
int Neo_Pixel_Color_Index = 0; // The index of the color pattern array
int Neo_Pixel_Light_Index = 0; // The index of the light that is currently on

// ------------ [CLASSES] ------------ 

// Timer that allows for a delay without blocking the main thread execution 
class Timer{

    // Private 
    private: 
        // Timer current state 
        bool Running = false; 

        // Milisecond Timers 
        unsigned long _milisecond_elapsed = 0; 

    // Public
    public: 
        // Checks whether a specific time has elapsed in milliseconds 
        inline bool Milliseconds(unsigned long Time){
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
};

// A class that determines whether a button that is pressed  
class Button{
    // Constructor 
    public: 
        Button(unsigned int Button_Pin){
            this->Button_Pin = Button_Pin; // Sets the button pin 
        }

    private: 
        // Button Indentifier 
        unsigned int Button_Pin; // The pin that the button is connected to

        // Private Button States
        bool Button_State = false; // Indicates whether the button has been pressed or not 

    public: 
        // Checks whether a button has been pressed once and drops all repeaded press calls 
        bool Pressed(){
            // Checks if the button is being pressed 
            unsigned int Current_Button_State = digitalRead(this->Button_Pin); // Gets the current button state

            // Checks if the button has already been pressed 
            bool Button_Registered = Current_Button_State == HIGH && this->Button_State == false; 

            // Checks if the button press should be registered or dropped 
            if(Button_Registered){
                Serial.print("Button: "); 
                Serial.print(this->Button_Pin);
                Serial.println(" Pressed");

                // Button has been pressed 
                this->Button_State = true; // Sets the button state to true as the button has been pressed 
            }else if(Current_Button_State == LOW && this->Button_State == true){
                // Button has been released 
                this->Button_State = false; // Sets the button state to false as the button has been released 
            }
            return Button_Registered; // Returns whether the button has been pressed or not
        }
};

// ---------------- [Objects] ----------------

// Buttons 
Button ON_Button(ON_BUTTON); // The on button
Button OFF_Button(OFF_BUTTON); // Off button 
Button AUTO_Button(AUTO_BUTTON); // Auto button

// Timers 
Timer LCD_Switch_Screen_Timer; // Timer that keeps track time elapsed for the switch screen delay 
Timer LCD_Refresh_Rate_Timer; // Timer that keeps track of the time elapsed for the LCD refresh rate
Timer Neo_Pixel_Timer; // How fast the Neo_Pixel changes colors or turns on and off
Timer Update_Environment_Timer; // Timer that keeps track of the time elapsed for the update environment rate


// ------------ [Functions] ------------

// Gets the temperature from the temperature sensor : returns [Celsius]
inline float Get_Temperature(){
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
inline float Get_Set_Fan_Speed(){
    // Gets the reading from the potentiometer
    float Voltage = analogRead(FAN_SPEED_CONTROL);

    // 127.5 -> 255.0 - 127.5 
    // Converts the voltage to a speed value within the range of 50 - 255 
    float Range = ((Voltage/1023.0) * 127.5) + 127.5; 

    return Range; 
}

// Gets the temperature setting and returns it in a range of [0-40]C 
inline float Get_Set_Temperature(){
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

// ------------ [SYSTEM] ------------ 

// Updates the environment sensors (Temperature sensors)
inline void Update_Environment_Data(){
    // Checks if the delay has not passed and returns nothing
    if(!Update_Environment_Timer.Milliseconds(Temperature_Sensor_Refresh_Rate)){
        Refreshed = false; // Indicates that the refresh is not true as we do not refresh the temperature
        return;  // exits code execution
    }else{
        // Indicates that we refresh the temperature 
        Refreshed = true; 
    }

    // Gets the temperature data 
    float Current_Temperature = Get_Temperature();

    // Compares and checks if the difference is high enough to warrant a refresh 
    bool Temperature_Changed = abs(Current_Temperature - Temperature) > Temperature_Change_Tolerance;

    // Checks if the temperature has changed 
    if(Temperature_Changed){
        // Updates the temperature value only if the temperature has changed
        Temperature = Current_Temperature;
    }
}

// Updates the system based on input from the buttons and sensors 
void Update_System_On_Input(){
    // ---- [BUTTONS] ----

    // Checks if the system is active 
    if(!Active){
        // Checks if the on button has been pressed 
        if(ON_Button.Pressed()){
            Serial.println(F("On Button Pressed!"));
            // Buzzes the buzzer
            Buzz();

            // Turns the system on 
            Active = true;

            // Sets the status change to true as the system state has changed 
            Status_Changed = true; 
        }
        
        // Checks for an IR receiver signal 
        if(IrReceiver.decode()){
            // Gets the value of the data received 
            unsigned long Data = IrReceiver.decodedIRData.decodedRawData;

            // Checks the runtime environment 
            #if VIRTUAL_ENVIRONMENT
                // Virtual environment 
                Serial.println(F("Wireless ON PRESSED")); 
                
                // Checks if the button pressed is the on button 
                if(Data == 0xFF00BF00){
                    // Turns on the system 
                    Active = true;

                    // Sets the status change to true as the system state has changed
                    Status_Changed = true;

                    // Buzzes the buzzer
                    Buzz();
                }
            #else 
                // Real environment 
                Serial.println(F("Wireless ON PRESSED"));

                // Checks if the button pressed is the on button 
                if(Data == 0xFFC23D){
                    // Turns on the system 
                    Active = true;

                    // Sets the status change to true as the system state has changed
                    Status_Changed = true;

                    // Buzzes the buzzer
                    Buzz();
                }
            #endif

        }

        // Returns as the system does not need to check for any other input besides the on button
        return;  
    }

    // Checks if the off button has been pressed 
    if(OFF_Button.Pressed()){
        Serial.println(F("Off Button Pressed!"));

        // Buzzes the buzzer 
        Buzz();

        // Turns the system off 
        Active = false; 

        // Sets the status change to true as the system state has changed
        Status_Changed = true;

        return;  // Returns as the system no longer needs to continue all checks in this iteration
    }

    // Checks if the auto button has been pressed
    if(AUTO_Button.Pressed()){
        Serial.println(F("Auto Button Pressed!"));

        // Buzzes the buzzer once 
        Buzz(); 

        // Toggles the auto mode or manual mode
        // Checks the current mode it is in 
        if(System_Mode == "Auto"){
            // Sets the system mode to manual 
            System_Mode = "Manual";
        }else{
            // Sets the system mode to auto 
            System_Mode = "Auto";
        }

        // Sets the status change to true as the system state has changed
        Status_Changed = true;

        return; // Returns as the system no longer needs to continue all checks in this iteration
    }

    // ---- [POTENTIOMETERS] ----

    // Checks if the user has changed the set fan speed
    if(abs(Get_Set_Fan_Speed() - Fan_Speed) > Change_In_Fan_Speed_Tolerance){
        // Outputs to the serial monitor that the fan speed has changed 
        Serial.println(F("Fan Speed Changed!")); 

        // Checks if the current mode is auto and if so it will change it back to manual mode 
        if(System_Mode == "Auto"){
            System_Mode = "Manual"; 
        }

        // Updates the fan speed 
        Fan_Speed = Get_Set_Fan_Speed(); 

        // Sets the current screen to the fan speed screen 
        ::Current_Screen = SET_FAN; 

        // sets the set fan flag to true as the fan speed is currently being changed
        Set_Fan_Speed_Changed = true;
    }

    // Checks if the user has changed the set temperature 
    if(abs(Get_Set_Temperature() - Set_Temperature) > Change_In_Set_Temperature_Tolerance){
        // Outputs to the serial monitor that the set temperature has changed 
        Serial.println(F("Set Temperature Changed!")); 

        // Checks if the current mode is auto and if so it will change it back to manual mode 
        if(System_Mode == "Manual"){
            System_Mode = "Auto"; 
        }

        // Updates the set temperature 
        Set_Temperature = Get_Set_Temperature(); 

        // Sets the current screen to the set temperature screen 
        ::Current_Screen = SET_TEMP; 

        // sets the set temp flag to true as the temperature is currently being changed 
        Set_Temp_Changed = true; 
    }

    // ---- [IR RECEIVER] ----

    // Checks if the IR receiver has received a signal 
    if(IrReceiver.decode()){
        // Gets the value of the data received 
        unsigned long Data = IrReceiver.decodedIRData.decodedRawData;

        // Checks if it is a repeat code
        if(Data == 0 || Data == 0xFFFFFFFF){
            // Sets the repeat code flag to true as the button is being held down 
            Repeat_Code = true;

            // Sets the data to the previous code 
            Data = Previous_Code;
        }else{
            // Sets the previous code 
            Previous_Code = Data;

            // Sets the repeat code flag to false 
            Repeat_Code = false; 
        }
        
        // Outputs the data into the serial monitor for debugging purposes 
        Serial.print("Data Received from IR Sensor: "); 
        Serial.println(Data,HEX);

        // Checks runtime environment 
        #if VIRTUAL_ENVIRONMENT 
            // Virtual Environment
            /*
                Virtual environment IR remote codes [unsigned long] 
                4278238976 || FF00BF00 - [POWER]
                4111122176 || F50ABF00 - [Speed Increase]
                4144545536 || F708BF00 - [Speed Decrease]
            */

           // Checks if the data received is the power button 
        //    switch (Data)
        //    {
        //         case 0xFF00BF00: // On Button
        //             // Checks if the system is active 
        //             if(Active){
        //                 // Turns the system off 
        //                 Active = false; 
        //             }else{
        //                 // Turns the system on 
        //                 Active = true; 
        //             }
        //             // Sets the status change to true as the system state has changed
        //             Status_Changed = true;
        //             break;
        //         case 0xF50ABF00: // Speed Increase Button
        //             // Checks if the current mode is auto and if so it will change it back to manual mode 
        //             if(System_Mode == "Auto"){
        //                 System_Mode = "Manual"; 
        //             }
                    
        //             // Checks if the fan speed is at the maximum speed and will drop the request 
        //             if(Fan_Speed >= 255){
        //                 return; // Fan speed is already at it's maximum 
        //             }

        //             Fan_Speed += 1; // Increases the fan speed

        //             // Sets the current screen to the fan speed screen 
        //             ::Current_Screen = SET_FAN; 
        //             // sets the set fan flag to true as the fan speed is currently being changed
        //             Set_Fan_Speed_Changed = true;
        //             break;
        //         case 0xF708BF00: // Speed Decrease Button
        //             // Checks if the current mode is auto and if so it will change it back to manual mode 
        //             if(System_Mode == "Auto"){
        //                 System_Mode = "Manual"; 
        //             }
                    
        //             // Checks if the fan speed is at the minimum speed and will drop the request
        //             if(Fan_Speed <= 0){
        //                 return; // Fan speed is already at it's minimum 
        //             }

        //             Fan_Speed -= 1; // Decreases the fan speed

        //             // Sets the current screen to the fan speed screen 
        //             ::Current_Screen = SET_FAN; 
        //             // sets the set fan flag to true as the fan speed is currently being changed
        //             Set_Fan_Speed_Changed = true;
        //             break;
        //     default:
        //         Serial.println("Code not recognised!"); 
        //         break;
        //    }

        #else    
            // Real environment 
            /*
                Real environment IR remote codes [unsigned long] 
                16761405 || 0xFFC23D - [POWER]
                16754775 || 0xFFA857 - [Speed Increase]
                16769055 || 0xFFE01F - [Speed Decrease]
            */

            // Checks if the data received is the power button
            switch(Data){
                case 0xFFC23D: // On Button
                    // Checks if the system is active 
                    if(Active){
                        // Turns the system off 
                        Active = false; 
                    }else{
                        // Turns the system on 
                        Active = true; 
                    }
                    // Sets the status change to true as the system state has changed
                    Status_Changed = true;
                    break;
                case 0xFFA857: // Speed Increase Button
                    // Checks if the current mode is auto and if so it will change it back to manual mode 
                    if(System_Mode == "Auto"){
                        System_Mode = "Manual"; 
                    }
                    
                    // Checks if the fan speed is at the maximum speed and will drop the request 
                    if(Fan_Speed >= 255){
                        return; // Fan speed is already at it's maximum 
                    }

                    Fan_Speed += 1; // Increases the fan speed

                    // Sets the current screen to the fan speed screen 
                    ::Current_Screen = SET_FAN; 
                    // sets the set fan flag to true as the fan speed is currently being changed
                    Set_Fan_Speed_Changed = true;
                    break;
                case 0xFFE01F: // Speed Decrease Button
                    // Checks if the current mode is auto and if so it will change it back to manual mode 
                    if(System_Mode == "Auto"){
                        System_Mode = "Manual"; 
                    }
                    
                    // Checks if the fan speed is at the minimum speed and will drop the request
                    if(Fan_Speed <= 0){
                        return; // Fan speed is already at it's minimum 
                    }

                    Fan_Speed -= 1; // Decreases the fan speed

                    // Sets the current screen to the fan speed screen 
                    ::Current_Screen = SET_FAN; 
                    // sets the set fan flag to true as the fan speed is currently being changed
                    Set_Fan_Speed_Changed = true;
                    break;
                default:
                    Serial.println("Code not recognised!"); 
                    break;
            }
        #endif
    }
}


// Updates the system components based on the current state of the system 
void Update_System_Components(){
    // Checks if the system is inactive and will turn off all power intensive components 
    if(!Active){
        // Checks runtime environment 
        #if VIRTUAL_ENVIRONMENT 
            // Virtual Environment 

            // Checks if the lcd is on 
            if(LCD_State){
                // Turns off the LCD's backlight to conserve power 
                TOPLCD.noBacklight(); 
                BOTTOMLCD.noBacklight();
                LCD_State = false; // Sets the LCD state to off
                
                // Updates the LCD
                TOPLCD.clear(); // Clears the LCD so they no longer display anything
                BOTTOMLCD.clear(); // Clears the lcd so they no longer display anything
            }
        #else
            // Real environment 
            // Checks if the lcd is on 
            if(LCD_State){
                // Turns off the LCD's backlight to conserve power
                LCD.noBacklight();
                LCD.clear(); // Clears the LCD so it no longer display anything
                LCD_State = false; // Sets the LCD state to off
            }
            
        #endif

        digitalWrite(RED_LED, HIGH);// Turns on the red LED to indicate that the system is off
        digitalWrite(GREEN_LED, LOW); // Turns off the green LED

        // Checks if the fan is on 
        if(Fan_State){
            // Turns the fan state to off 
            Fan_State = false; 
            // Turns off the fan 
            analogWrite(DC_MOTOR,0);
        }
        
        // Checks if the neopixel is on 
        if(Neo_Pixel_State){
            // Clears the Neo_Pixel NEOPIXEL 
            NEOPIXEL.clear(); 
            NEOPIXEL.show(); 
            Neo_Pixel_State = false; // Sets the Neo_Pixel state to off
        }
        
        return; // Stops the rest of the code from executing as the program is inactive
    }

    // The system is active and will turn on all system components 
    
    // Turns on the LEDs to indicate that the system is on
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH); 

    // Toggles the LCD's backlight
    // Checks runtime environment 
    #if VIRTUAL_ENVIRONMENT 
        // Virtual Environment 
        // Checks if the LCD is off
        if(!LCD_State){
            // Turns on the LCD's backlight
            TOPLCD.backlight();
            BOTTOMLCD.backlight();
            LCD_State = true; // Sets the LCD state to on
        }
    #else
        // Real environment 
        // Checks if the LCD is off
        if(!LCD_State){
            // Turns on the LCD's backlight
            LCD.backlight();
            LCD_State = true; // Sets the LCD state to on
        }
    #endif

    // ---- [AUTO MODE] ----

    // Checks if the system is in auto mode 
    if(System_Mode == "Auto"){
        // Checks if the temperature is above the set temperature 
        if(Temperature > Set_Temperature){
            // Checks if the fan state has changed 
            if(Fan_State != true){
                Status_Changed = true; // Indicates that the status has changed 
            }
            Fan_State = true; // Turns on the fan 
        }else{
            // Checks if the fan state has changed 
            if(Fan_State != false){
                Status_Changed = true; // Indicates that the status has changed 
            }
            Fan_State = false; // Turns off the fan 
        }
    }

    // ---- [MANUAL MODE] ----

    // Checks if the system is in manual mode 
    if(System_Mode == "Manual"){
        // Checks if the fan speed is greater than 50%
        if(Fan_Speed >= 50){
            Fan_State = true; // Turns on the fan 
        }else{
            Fan_State = false; // Turns off the fan 
        }
    }

    // ---- [DC MOTOR] ----

    // Checks if the fan state is on 
    if(Fan_State){
       // Checks if the mode is auto or manual 
       if(System_Mode == "Auto"){
           // Sets the fan speed to the default fan speed 
            analogWrite(DC_MOTOR,255); 
       }else{
            // Sets the fan speed to the fan speed set by the user 
            analogWrite(DC_MOTOR,Fan_Speed); 
       }
    }else{
        analogWrite(DC_MOTOR,0); 
    }

    // ---- [Displays] ----

    // Checks the runtime environment 
    #if VIRTUAL_ENVIRONMENT 
        // Virtual environment 

        // Checks if the status has changed  
        if(Status_Changed){
            Serial.println(F("System Status Changed!")); 
           
            // Sets the status back to unchanged 
            Status_Changed = false;
           
            // Clears the top LCD 
            TOPLCD.clear(); 

            // Sets the temp and fan speed changed flags back to false as this event takes precedence 
            Set_Temp_LCD_Cleared = false; 
            Set_Fan_Speed_LCD_Cleared = false; 

            // Changes the display back to the main display mode 
            ::Current_Screen = MAIN;
        }

        // Checks if the current screen is not equal to main and will switch back after a delay
        if(::Current_Screen != MAIN && LCD_Switch_Screen_Timer.Milliseconds(LCD_Switch_Screen_Delay)){
            // Sets the current screen back to main 
            ::Current_Screen = MAIN; 

            // Clears the top LCD 
            TOPLCD.clear();

            // Sets both the temp and fan speed changed flags back to false as both lcd's have not been cleared due to the screen changing 
            Set_Temp_LCD_Cleared = false;
            Set_Fan_Speed_LCD_Cleared = false;

            Serial.println(F("Switching back to main screen!"));
        }

        // Checks to determine what screen to display currently 
        switch(::Current_Screen){
            case MAIN: 
                // Displays the main screen 
                TOPLCD.setCursor(0,0);
                TOPLCD.print("Mode: " + System_Mode);
                TOPLCD.setCursor(0,1);
                TOPLCD.print("Fan State: " + (String) ((Fan_State) ? "On" : "Off"));
                break;
            case SET_TEMP: 
                // Checks if the set temperature lcd was not cleared yet
                if(!Set_Temp_LCD_Cleared){
                    // Clears the entire screen 
                    TOPLCD.clear(); 
                    Set_Temp_LCD_Cleared = true; 
                }else{
                    // Checks if the set temperature has changed 
                    if(Set_Temp_Changed){
                        // Clears only the bottom row 
                        TOPLCD.setCursor(0,1);
                        TOPLCD.print("                ");
                        Set_Temp_Changed = false; // Sets the set temperature changed flag back to false as the lcd was just cleared
                    }
                   
                }

                // Displays the set temperature screen
                TOPLCD.setCursor(0,0);
                TOPLCD.print("Set Temperature: ");
                TOPLCD.setCursor(0,1);
                TOPLCD.print("Temp: " + (String) Set_Temperature + "C");
                break; 
            case SET_FAN: 
                // Checks if the set fan speed lcd was not cleared yet
                if(!Set_Fan_Speed_LCD_Cleared){
                    // Clears the entire screen 
                    TOPLCD.clear(); 
                    Set_Fan_Speed_LCD_Cleared = true;
                }else{
                    // Checks if the set fan speed has changed 
                    if(Set_Fan_Speed_Changed){
                        // Clears only the bottom row 
                        TOPLCD.setCursor(0,1);
                        TOPLCD.print("                ");
                        Set_Fan_Speed_Changed = false; // Sets the set fan speed changed flag back to false as the lcd was just cleared
                    }
                }

                // Displays the set fan speed screen 
                TOPLCD.setCursor(0,0);
                TOPLCD.print("Set Fan Speed");
                TOPLCD.setCursor(0,1);
                // Converts the fan speed into a percentage and outputs it onto the lcd
                TOPLCD.print("Speed: " + (String) ((int)((Fan_Speed/255.0f) * 100)) + "%");
        }

        // Displays the bottom display temperature in specific intervals 
        if(LCD_Refresh_Rate_Timer.Milliseconds(Max_TEMP_LCD_Refresh_Rate)){
            
            // Checks if the temperature has been refreshed 
            if(Refreshed){
                // Clears the bottom LCD 
                BOTTOMLCD.setCursor(0,1); 
                BOTTOMLCD.print("                ");
            }
           
            
            // Displays the temperature on the bottom LCD 
            BOTTOMLCD.setCursor(0,0); 
            BOTTOMLCD.print("Temperature"); 
            BOTTOMLCD.setCursor(0,1);
            BOTTOMLCD.print("Temp: " + (String) Temperature + "C");
        }
    #else 
        // Real environment

        // Checks if the status has changed  
        if(Status_Changed){
            Serial.println(F("System Status Changed!")); 
           
            // Sets the status back to unchanged 
            Status_Changed = false;
           
            // Clears only the two rows on the lcd as it is a 20 x 4 lcd panel 
            LCD.setCursor(0,0);
            LCD.print("                    ");
            LCD.setCursor(0,1);
            LCD.print("                    ");

            // Sets the temp and fan speed changed flags back to false as this event takes precedence 
            Set_Temp_LCD_Cleared = false; 
            Set_Fan_Speed_LCD_Cleared = false; 

            // Changes the display back to the main display mode 
            Current_Screen = MAIN;
        }

        // Checks if the current screen is not equal to main and will switch back after a delay
        if(Current_Screen != MAIN && LCD_Switch_Screen_Timer.Milliseconds(LCD_Switch_Screen_Delay)){
            // Sets the current screen back to main 
            Current_Screen = MAIN; 

            // Clears the only the two rows on the lcd as it is a 20 x 4 lcd panel
            LCD.setCursor(0,0);
            LCD.print("                    ");
            LCD.setCursor(0,1);
            LCD.print("                    ");

            // Sets both the temp and fan speed changed flags back to false as both lcd's have not been cleared due to the screen changing 
            Set_Temp_LCD_Cleared = false;
            Set_Fan_Speed_LCD_Cleared = false;

            Serial.println(F("Switching back to main screen!"));
        }

        // Checks to determine what screen to display currently 
        switch(Current_Screen){
            case MAIN: 
                // Displays the main screen 
                LCD.setCursor(0,0);
                LCD.print("Mode: " + System_Mode);
                LCD.setCursor(0,1);
                LCD.print("Fan State: " + (String) ((Fan_State) ? "On" : "Off"));
                break;
            case SET_TEMP: 
                // Checks if the set temperature lcd was not cleared yet
                if(!Set_Temp_LCD_Cleared){
                    // Clears the entire screen 
                    LCD.clear(); 
                    Set_Temp_LCD_Cleared = true; 
                }else{
                    // Checks if the set temp has changed 
                    if(Set_Temp_Changed){
                        // Clears only the bottom row 
                        LCD.setCursor(0,1);
                        LCD.print("                ");
                        Set_Temp_Changed = false; // Sets the set temperature changed flag back to false as the lcd was just cleared
                    }
                }

                // Displays the set temperature screen
                LCD.setCursor(0,0);
                LCD.print("Set Temperature: ");
                LCD.setCursor(0,1);
                LCD.print("Temp: " + (String) Set_Temperature + "C");
                break; 
            case SET_FAN: 
                // Checks if the set fan speed lcd was not cleared yet
                if(!Set_Fan_Speed_LCD_Cleared){
                    // Clears the entire screen 
                    LCD.clear(); 
                    Set_Fan_Speed_LCD_Cleared = true;
                }else{
                    // Checks if the set fan speed has changed 
                    if(Set_Fan_Speed_Changed){
                        // Clears only the bottom row 
                        LCD.setCursor(0,1);
                        LCD.print("                ");
                        Set_Fan_Speed_Changed = false; // Sets the set fan speed changed flag back to false as the lcd was just cleared
                    }
                    
                }

                // Displays the set fan speed screen 
                LCD.setCursor(0,0);
                LCD.print("Set Fan Speed");
                LCD.setCursor(0,1);
                // Converts the fan speed into a percentage and outputs it onto the lcd
                LCD.print("Speed: " + (String) ((int)((Fan_Speed/255.0f) * 100)) + "%");
        }

        // Displays the bottom display temperature in specific intervals 
        if(LCD_Refresh_Rate_Timer.Milliseconds(Max_TEMP_LCD_Refresh_Rate)){
            
            // Checks if the temperature has been refreshed 
            if(Refreshed){
                // Clears the bottom part of the 20 x4 lcd panel
                LCD.setCursor(0,3); 
                LCD.print("                ");
            }
            

            // Displays the temperature on the bottom LCD 
            LCD.setCursor(0,2); 
            LCD.print("Temperature"); 
            LCD.setCursor(0,3);
            LCD.print("Temp: " + (String) Temperature + "C");
        }
    #endif
}

// Clock_Wise - Rotates the neopixel lights clockwise 
void Clockwise(){
    // Checks if the current pixel light index is the last light on the neopixel
    if(Neo_Pixel_Light_Index < (NEOPIXEL.numPixels() - 1)){
        // Not the last light on the neopixel 
        // Increments the light index 
        Neo_Pixel_Light_Index++;
    }else{
        // Last light on the neopixel 
        // Resets the light index back to 0 
        Neo_Pixel_Light_Index = 0;
        // Increments the color index
        Neo_Pixel_Color_Index++;
    }


    // Checks if the current color index is the last color in the color pattern 
    if(Neo_Pixel_Color_Index >= (int)(sizeof(Neo_Pixel_Color_Pattern)/sizeof(Neo_Pixel_Color_Pattern[0]))){
        // Last color in the color pattern 
        // Resets the color index back to 0 
        Neo_Pixel_Color_Index = 0;
    }
}


// Updates the neopixel based on the current system state 
void Update_Neo_Pixel(){
    // Checks if the system state is inactive 
    if(!Active){
        return; // Returns as the system is inactive 
    }
    Neo_Pixel_State = true; // Neopixel is on 
    // Checks if the neopixel time has not elapsed
    if(Neo_Pixel_Mode != SPEED_DEMON && !Neo_Pixel_Timer.Milliseconds(Neo_Pixel_Delay_Between_Lights)){
        // Returns as the delay between lights has not elapsed
        return; 
    }

    // Gets the colors in the current pixel color index 
    uint8_t Red = Neo_Pixel_Color_Pattern[Neo_Pixel_Color_Index][0];
    uint8_t Green = Neo_Pixel_Color_Pattern[Neo_Pixel_Color_Index][1];
    uint8_t Blue = Neo_Pixel_Color_Pattern[Neo_Pixel_Color_Index][2];
    
    // Sets the neopixel color 
    NEOPIXEL.setPixelColor(Neo_Pixel_Light_Index, Red, Green, Blue);
    NEOPIXEL.show();

    // Checks if the current neopixel mode is clockwise 
    if(Neo_Pixel_Mode == CLOCKWISE || Neo_Pixel_Mode == SPEED_DEMON){
        // Clockwise mode 
        Clockwise();
    }
    
}

// MAIN  

// System start 
void setup(){
    // Initializations 
    Serial.begin(9600); // Starts the serial communication at 9600 baud rate

    // Initializes the lcds depending on the environment
    #if VIRTUAL_ENVIRONMENT 
        // Virtual environment 
        TOPLCD.init(); 
        TOPLCD.backlight();
        delay(500); 
        TOPLCD.noBacklight(); 

        BOTTOMLCD.init();
        BOTTOMLCD.backlight();
        delay(500);
        BOTTOMLCD.noBacklight();
    #else
        // Real environment 
        LCD.init(); 
        LCD.backlight();
        delay(500); 
        LCD.noBacklight();
    #endif

    // Starts the neopixel 
    NEOPIXEL.begin();
    NEOPIXEL.setBrightness(Max_Neo_Pixel_Brightness); // Sets the max neopixel brightness

    // Loops through all the colors in the neopixel and turns it off 
    for(int i = 0; i < (int)(sizeof(Neo_Pixel_Color_Pattern)/sizeof(Neo_Pixel_Color_Pattern[0])); i++){
        // Gets the individual colors in the current array 
        int Red = Neo_Pixel_Color_Pattern[i][0]; 
        int Green = Neo_Pixel_Color_Pattern[i][1];
        int Blue = Neo_Pixel_Color_Pattern[i][2];

        // Sets the color of the neopixel to the current color
        for(int j = 0; j < NEOPIXEL.numPixels(); j++){
            NEOPIXEL.setPixelColor(j,NEOPIXEL.Color(Red,Green,Blue));
            NEOPIXEL.show(); 
            delay(50); // Delay to show the color
        }
    }

    // Clears the neopixel 
    NEOPIXEL.clear();
    NEOPIXEL.show();

    // Buzzes the Buzzer once 
    Buzz(); 

    // -- [PIN INITIALIZATIONS] --

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

    // Starts the IR sensor 
    IrReceiver.begin(IR_RECEIVER); 

    // START 

    // Sets the temperature and fan speed to their initial values 
    Fan_Speed = Get_Set_Fan_Speed(); 
    Set_Temperature = Get_Set_Temperature();

    // Updates the current temperature with the current temperature in the environment 
    Temperature = Get_Temperature(); 

    Serial.println("System Started Successfully"); // Prints that the system has started successfully
}

// update 
void loop(){

    // Updates the environment data 
    Update_Environment_Data();

    // Updates the system based on input 
    Update_System_On_Input(); 
    
    // Updates the neopixel 
    Update_Neo_Pixel(); 

    // Updates the system components based on the system state 
    Update_System_Components();
}