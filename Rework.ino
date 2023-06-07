//Imports 
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.hpp>

// -- Important Rules 
// -- No Loops! 
// -- Delay is not allowed to be used! 

// Notes 
// Static variables cannot be initialized in a class unless the inline keyword is present 

// Defines whether the system is in a virtual environment or not 
#define VIRTUAL_ENVIRONMENT true 

// Pin State Variables 

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
#define HUMIDITY_SENSOR A3

// Potentionmeters 
#define FAN_SPEED_CONTROL A1
#define TEMPERATURE_CONTROL A0

// Neo Pixel Pin 
#define NEO_PIXEL_PIN 8 

// LCD ADDRESSES 

#if VIRTUAL_ENVIRONMENT
    #define CONTROL_LCD_ADDRESS 33 
    #define STATUS_LCD_ADDRESS 32
#else 
    #define CONTROL_LCD_ADDRESS 0x27
#endif

// Object Initializations 
#if VIRTUAL_ENVIRONMENT
    LiquidCrystal_I2C CONTROL_LCD(CONTROL_LCD_ADDRESS, 16, 2); // System status LCD where you can see the modes and the current status of the system
    LiquidCrystal_I2C STATUS_LCD(STATUS_LCD_ADDRESS, 16, 2); // Status LCD where you can see the current temperature and humidity
#else
    LiquidCrystal_I2C CONTROL_LCD(CONTROL_LCD_ADDRESS, 16, 2); // System status LCD where you can see the modes and the current status of the system
#endif

Adafruit_NeoPixel NEO_PIXEL(24, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800); // RGB LED

// ------------ [CLASSES] ------------ \\

// A timer that allows for independent timing of different events 
class Timer{

    // Constructor 
    public: 
        Timer(){
            Timer_Count++; // Increments the number of timers that have been created 
        }


    // Private Variables
    private: 
        // Counters 
        inline static unsigned int Timer_Count = 0; // Indicates how many timers have been created

        // State 
        bool Running = false; // Indicates whether the timer is running or not

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
                this->Running = true; // Indicates that the timer is running
            }

            // Checks if the time has elapsed and resets the timer 
            if(millis() - this->Timer_Start_Millis >= Time && this->Timer_Started_Millis){
                // Resets the timer 
                this->Timer_Start_Millis = millis();
                this->Timer_Started_Millis = false;
                this->Running = false; // Indicates that the timer is not running

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
                this->Running = true; // Indicates that the timer is running
            }

            // Checks if the time has elapsed and resets the timer 
            if(micros() - this->Timer_Start_Micros >= Time && this->Timer_Started_Micros){
                // Resets the timer 
                this->Timer_Start_Micros = micros();
                this->Timer_Started_Micros = false;
                this->Running = false; // Indicates that the timer is not running

                return true;  // Returns true if the time has elapsed
            }else{
                return false; // Returns false if the time has not elapsed 
            }
        }

        // Checks whether the current timer is currently running 
        bool Get_State(){
            return this->Running;
        }

        // Gets the number of timers that have been created 
        unsigned int Get_Timer_Count(){
            return this->Timer_Count;
        }
};

// A event handler that handles all function calls and events that happen in the system through a callback system 
class Event_Handler{
    
    private: 
        // Private Variables 
        inline static unsigned long Void_Event_Calls = 0; // Indicates how many events were called 
        inline static unsigned long Typed_Event_Calls = 0; // Indicates how many events were called with an argument
        inline static unsigned long Total_Event_Calls = 0; // Total event calls that have occured

    public: 
        // Public Functions 
        
        // Calls the function given 
        void Call(void (*Function)(void)){
            Function(); // Calls the function 
            this->Void_Event_Calls++; // Increments the number of void events that have occured
            this->Total_Event_Calls++; // Increments the total number of events that have occured
        }

        // Calls the function given a parameters
        
        template<typename String> void Call(void (*Function)(void), String Name){
            Function(); // Calls the function 
            this->Void_Event_Calls++; // Increments the number of void events that have occured
            // Outputs the name of the function 
            Serial.print("Event: "); 
            Serial.print(Name);
            Serial.println(" Called");
            this->Total_Event_Calls++; // Increments the total number of events that have occured
        }

        // Calls the function given a parameter
        template<typename ParamNum, typename String>void Call(void (*Function)(A Number), String Name){
            Function(Number); // Calls the function 

            Serial.print("Event: "); 
            Serial.print(Name);
            Serial.println(" Called");

            this->Typed_Event_Calls++; // Increments the number of typed events that have occured
            this->Total_Event_Calls++; // Increments the total number of events that have occured
        }

        // Calls the function given a parameter


        // Gets the number of void events that have occured 
        unsigned long Get_Void_Event_Calls(){
            return this->Void_Event_Calls;
        }

        // Gets the number of typed events that have occured
        unsigned long Get_Typed_Event_Calls(){
            return this->Typed_Event_Calls;
        }

        // Gets the total number of events that have occured
        unsigned long Get_Total_Event_Calls(){
            return this->Total_Event_Calls;
        }
};

// ------------ [Variables] ------------ \\

// --- [System Configurations] --- \\ 

// Lighting Parameters 
const unsigned int Max_Neo_Pixel_Brightness = 20; // Max Brightness of the NeoPixel [0 - 255]
const unsigned int Neo_Pixel_Delay_Between_Lights = 500; // Delay between each light in the NeoPixel [ms]
const unsigned int Max_LED_Brightness = 100; // Max Brightness of the LEDs [0 - 255]

// LCD parameters
const unsigned int Max_LCD_Idle_Time = 5000; // Max time that the LCD can be idle before it turns off [ms]

// Buzzer Tones 
const unsigned int Buzzer_Frequency = 100; // Frequency of the buzzer tone [Hz]
const unsigned int Buzzer_Duration = 250; // Duration of the buzzer tone [ms]

// Color Pattern 
const int Pattern_Length = 7; // Length of the color pattern
const int Neo_Pixel_Color_Pattern[][3] = {
    {255,0,0}, // Red
    {0,255,0}, // Green
    {0,0,255}, // Blue
    {255,255,0}, // Yellow
    {255,0,255}, // Purple
    {0,255,255}, // Cyan
    {255,255,255}, // White
};

// --- [System Status Variables] --- \\ 
String System_Mode = "Manual"; // Mode that the system is currently in [Auto,Manual]

// System State 
bool Active = false; // Indicates whether the on or off button was pressed and activates the system 

// LCD states 
bool LCD_Active = false; // Indicates whether to turn off or on the backlight on the LCD's

// IR SENSOR  
IRrecv irrecv(IR_RECEIVER); // Creates a new IR Receiver 
decode_results results; // Creates a new decode results object

// Auto Fan States 
//bool Auto_Fan_State = false; // Indicates whether the auto fan is on or off 
int Auto_Fan_Speed = 0; // Determines the speed of the fan in a range of[0 - 255]

// Fan State 
bool Fan_State = false; // Indicates whether the fan is on or off 
int Fan_Speed = 0; // Determines the speed of the fan in a range of[0 - 255] 

// Buzzer State 
bool Buzzing = false; // Indicates whether the buzzer is buzzing or not

// Environment Status 
float Set_Temperature = 0; // Temperature that the user sets the system to [C]
float Temperature = 0; // Temperature of the environment [C] 
float Humidity = 0; // Humidity of the environment [%]

// --- [System Control Variables] --- \\

// Lighting Control 
bool Neo_Pixel_State = false; // Indicates whether the NeoPixel is on or off
unsigned int Pixel_Light_Index = 0; 
unsigned int Pixel_Pattern_Index = 0; 
unsigned int Pixel_Pattern_Size = 0; // Size of the color pattern array 

// Timers 

// System Timer 
Timer System_Timer; 
// Buzzer Timer 
Timer Buzzer_Timer; 
// NeoPixel Timer 
Timer Neo_Pixel_Timer; 
// PWM Timers
Timer PWM_Timer;
// LCD Timer 
Timer LCD_Timer; 
// PWM emulation Timer 
Timer LED_TIMER; 

// Repeat value Cache for buttons 
bool Button_State_Cache[3] = {false,false,false}; // Stores the previous state of the buttons

// Display states 
#ifdef VIRTUAL_ENVIRONMENT 
    bool Control_Display_Changed = false; // Indicates whether the control display has changed 
    bool Status_Display_Changed = false; // Indicates whether the status display has changed
#else 
    bool Display_Changed = false; // Indicates whether the display has changed
#endif

bool Display_Active = false; // Indicates whether the display should be active or not and turns on the backlight

// Functions 

// ------------ [SETTERS] ------------ \\

// Controls the Motor Speed [0 - 255]
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

// Sets the brightness of any RGB LED on any pin 
void Set_LED(unsigned int LED_NUMBER, unsigned int BRIGHTNESS){

    // Calculates the Duty cycle for the pin (Simulated PWM)
    const double PWM_Frequency = 2040; 
    double On_Duty_Cycle = PWM_Frequency - (((255.0 - (double)BRIGHTNESS) / 255) * PWM_Frequency); // Calculates the on duty cycle of the PWM signal
    double Off_Duty_Cycle = PWM_Frequency - On_Duty_Cycle; // Calculates the off duty cycle of the PWM signal

    // Checks if the brightness is 255 and if so just turn the LED on constantly 
    if(BRIGHTNESS == 255){
        digitalWrite(LED_NUMBER, HIGH);
        return;
    }else if(BRIGHTNESS == 0){
        digitalWrite(LED_NUMBER, LOW);
        return;
    }

    // otherwise we need to simulate the PWM signal 
    if(LED_TIMER.Check_Time_Micros(On_Duty_Cycle)){
        digitalWrite(LED_NUMBER, HIGH);
    }else{
        digitalWrite(LED_NUMBER, LOW);
    }
}


// ------------ [GETTERS] ------------ \\

// Reads the data from the temperature sensor [LM35] [C]
float Get_Temperature(){
    // Voltage [0 - 10mv]
    float Voltage = analogRead(TEMPERATURE_CONTROL); 

    // Checks whether we are in a virtual environment or not 
    #ifdef VIRTUAL_ENVIRONMENT
        // Virtual environment (TPM36)
        return (Voltage * 5) / 1024.0; // Returns the temperature in celsius
    #else
        // Real environment (LM35) 
        return (Voltage /10); 
    #endif
}

// Gets the humidity from the humidity sensor [%]
float Get_Humidity(){
    // Gets the humidity reading from the sensor
    float Voltage = analogRead(HUMIDITY_SENSOR);

    // Checks whether we are in a virtual environment or not 
    #ifdef VIRTUAL_ENVIRONMENT
        // Virtual environment (Soil Moisture Sensor)
        // Max 876
        return (Voltage / 876.0) * 100; 
    #else 
        // In a real environment (Unknown sensor)
        Serial.println("Unknown humidity sensor! Please refer to pinout"); 
        return Voltage; 
    #endif
}

// Gets the set fan speed on the potentiometer and returns it in a range of [0 - 255]
int Get_Set_Fan_Speed(){
    // Gets the potentiometer
    float Voltage = analogRead(FAN_SPEED_CONTROL);

    // Converts the potentiometer voltage to a speed value within range 
    float Fan_Speed = (Voltage/1024.0) * 255.0;
    return (int)Fan_Speed; 
}

// Gets the set temperature setting and returns it in a range of [0-40]
int Get_Set_Temperature(){
    // Gets the potentiometer 
    float Voltage = analogRead(TEMPERATURE_CONTROL);

    // Converts the potentiometer voltage to a temperature value within range 
    float Set_Temperature = (Voltage/1024.0) * 40.0;
    return (int)Set_Temperature;
}


// ------------ [UTILITIES] ------------ \\

// Buzzes the buzzer for a certain duration 
void Buzz(){

    // Checks if the buzzer duration has passed since the last buzz to prevent the buzzer from constantly buzzing if called multiple times
    if(Buzzer_Timer.Check_Time_Millis(Buzzer_Duration)){
        // Sets the buzzer to not buzzing 
        Buzzing = false;
    }


    // Checks if the current buzzer is buzzing 
    if(Buzzing){
        Serial.println("Buzzer is already buzzing!"); 
        return; // Returns as the buzzer is already buzzing
    }

    // Sets the buzzer to buzzing
    Buzzing = true;

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

// ------------ [DISPLAYS] ------------ \\ 

// Displays the top data on the LCD
void Display_Top_Basic(){
    // Displays the basic information such as fan mode and fan state 

    // Checks whether we are in a virtual environment or a real environment 
    #ifdef VIRTUAL_ENVIRONMENT 
        // Virtual environment display control 

        // Checks if the display has changed 
        if(!Control_Display_Changed){
            CONTROL_LCD.clear(); // Clears the top display 
            Control_Display_Changed = false; // Sets the display changed to false as we have just cleared the display
        }

        // Displays the current system mode and fan state 
        CONTROL_LCD.setCursor(0,0); // Sets the cursor to the top left of the display 
        CONTROL_LCD.print("Mode: ");
        CONTROL_LCD.print(Fan_Mode);

        CONTROL_LCD.setCursor(0,1); // Sets the cursor to the bottom left of the display
        CONTROL_LCD.print(" State: ");
        CONTROL_LCD.print(Fan_State);
    #else
        // Real Environment 

        // Checks if the display has changed 
        if(Display_Changed){
            CONTROL_LCD.clear(); // Clears the top display 
            Display_Changed = false; // Sets the display changed to false as we have just cleared the display
        }   

        // Displays the current system mode and fan state 
        CONTROL_LCD.setCursor(0,0); // Sets the cursor to the top left of the display
        CONTROL_LCD.print("Mode: ");
        CONTROL_LCD.print(Fan_Mode);

        CONTROL_LCD.setCursor(0,1); // Sets the cursor to the bottom left of the display
        CONTROL_LCD.print(" State: ");
        CONTROL_LCD.print(Fan_State);
    #endif
    
}

// Displays the top data of the current set fan speed  
void Display_Fan_Speed_Top(){
    // Checks if we are in the virtual environment or not 
    #ifdef VIRTUAL_ENVIRONMENT
        // Virtual environment display control 

        // Checks if the display has changed 
        if(!Control_Display_Changed){
            CONTROL_LCD.clear(); // Clears the top display 
            Control_Display_Changed = false; // Sets the display changed to false as we have just cleared the display
        }

        // Displays the current system mode and fan state 
        CONTROL_LCD.setCursor(0,0); // Sets the cursor to the top left of the display 
        CONTROL_LCD.print("Set Fan Speed"); 

        CONTROL_LCD.setCursor(0,1); // Sets the cursor to the bottom left of the display
        CONTROL_LCD.print(" Set Speed: ");
        CONTROL_LCD.print(Set_Fan_Speed);
    #else
        // Real environment display control 

        // Checks if the display has changed 
        if(Display_Changed){
            CONTROL_LCD.clear(); // Clears the top display 
            Display_Changed = false; // Sets the display changed to false as we have just cleared the display
        }

        // Displays the current system mode and fan state 
        CONTROL_LCD.setCursor(0,0); // Sets the cursor to the top left of the display 
        CONTROL_LCD.print("Set Fan Speed"); 

        CONTROL_LCD.setCursor(0,1); // Sets the cursor to the bottom left of the display
        CONTROL_LCD.print(" Set Speed: ");
        CONTROL_LCD.print(Set_Fan_Speed);
    #endif
}

// Displays the set temperature on the LCD 
void Display_Set_Temperature_Top(){
    // Checks if we are in the virtual environment or not 
    #ifdef VIRTUAL_ENVIRONMENT
        // Virtual environment display control 

        // Checks if the display has changed 
        if(!Control_Display_Changed){
            CONTROL_LCD.clear(); // Clears the top display 
            Control_Display_Changed = false; // Sets the display changed to false as we have just cleared the display
        }

        // Displays the current system mode and fan state 
        CONTROL_LCD.setCursor(0,0); // Sets the cursor to the top left of the display 
        CONTROL_LCD.print("Set Temperature"); 

        CONTROL_LCD.setCursor(0,1); // Sets the cursor to the bottom left of the display
        CONTROL_LCD.print(" Set Temp: ");
        CONTROL_LCD.print(Set_Temperature);
    #else
        // Real environment display control 

        // Checks if the display has changed 
        if(Display_Changed){
            CONTROL_LCD.clear(); // Clears the top display 
            Display_Changed = false; // Sets the display changed to false as we have just cleared the display
        }

        // Displays the current system mode and fan state 
        CONTROL_LCD.setCursor(0,0); // Sets the cursor to the top left of the display 
        CONTROL_LCD.print("Set Temperature"); 

        CONTROL_LCD.setCursor(0,1); // Sets the cursor to the bottom left of the display
        CONTROL_LCD.print(" Set Temp: ");
        CONTROL_LCD.print(Set_Temperature);
    #endif
}

// Displays the bottom data on the LCD [Temperature | Humidity]
void Display_Bottom_Basic(){
    // Displays the basic information such as temperature and humidity 

    // Checks whether we are in a virtual environment or a real environment 
    #ifdef VIRTUAL_ENVIRONMENT 
        // Virtual environment display control 

        // Checks if the display has changed 
        if(!Control_Display_Changed){
            CONTROL_LCD.clear(); // Clears the top display 
            Control_Display_Changed = false; // Sets the display changed to false as we have just cleared the display
        }

        // Displays the current system mode and fan state 
        CONTROL_LCD.setCursor(0,0); // Sets the cursor to the top left of the display 
        CONTROL_LCD.print("Temp: ");
        CONTROL_LCD.print(Temperature);

        CONTROL_LCD.setCursor(0,1); // Sets the cursor to the bottom left of the display
        CONTROL_LCD.print(" Humidity: ");
        CONTROL_LCD.print(Humidity);
    #else
        // Real Environment 

        // Checks if the display has changed 
        if(Display_Changed){
            CONTROL_LCD.clear(); // Clears the top display 
            Display_Changed = false; // Sets the display changed to false as we have just cleared the display
        }   

        // Displays the current system mode and fan state 
        CONTROL_LCD.setCursor(0,3); // Sets the cursor to the top left of the display
        CONTROL_LCD.print("Temperature: ");
        CONTROL_LCD.print(Temperature);

        CONTROL_LCD.setCursor(0,4); // Sets the cursor to the bottom left of the display
        CONTROL_LCD.print(" Humidity: ");
        CONTROL_LCD.print(Humidity);
    #endif
    
}

// ------------ [SYSTEM] ------------ \\

// Updates the temperature and humidity data 
void Update_Environment_Data(){
    // Gets the current temperature and humidity data
    float Current_Temperature = Get_Temperature();
    float Current_Humidity = Get_Humidity();

    // Compares whether the current temperature is different to the previous temperature 
    // If it is a differency of 0.5 then we update the temperature 
    bool Temperature_Changed = abs(Current_Temperature - Temperature) > 0.5;
    bool Humidity_Changed = abs(Current_Humidity - Humidity) > 0.5;

    // Updates the temperature and humidity 
    if(Temperature_Changed){
        // Checks whether we are in a virtual environment or a real environment 
        #ifdef VIRTUAL_ENVIRONMENT 
            // Virtual environment display control 

            // Indicates the the LCD needs to be cleared
            Status_Display_Changed = true;
        #else 
            // Real environment display control 

            // Indicates the the LCD needs to be cleared
            Display_Changed = true;
        #endif

        // Updates the temperature value
        Temperature = Current_Temperature;
    }

    if(Humidity_Changed){
        // Checks whether we are in a virtual environment or a real environment
        #ifdef VIRTUAL_ENVIRONMENT 
            // Virtual environment display control 

            // Indicates the the LCD needs to be cleared
            Status_Display_Changed = true;
        #else
            // Real environment display control 

            // Indicates the the LCD needs to be cleared
            Display_Changed = true;
        #endif

        Humidity = Current_Humidity;
    }
}

// Updates the system based on the user input 
void Update_System_On_Input(){
    // Checks if we are in a virtual environment or a real environment
    #ifdef VIRTUAL_ENVIRONMENT 
        // Virtual environment 

        // --- [IR RECEIVER] --- \\
        // Checks if the IR receiver has received a signal 
        if(iirecv.decode(&results)){
            unsigned int value = results.value; // Gets the value of the IR signal 
            Serial.print("IR Signal Received: "); // Prints the IR signal value
            Serial.println(value);
            Serial.println("---------------------------");
            iirecv.resume(); // Resumes the IR receiver
        }


        // --- [BUTTONS] --- \\

        // Check if the AUTO button is pressed
        if(Active && Check_Button(AUTO_BUTTON)){
            Serial.println("Auto Button was pressed!"); 

            // Indicates that the display needs to be cleared
            Control_Display_Changed = true;

            // Indicates that the LCD backlight needs to be turned on
            LCD_Active = true; 

            // Checks if the system is in auto mode 
            if(System_Mode == AUTO){
                // Sets the system mode to manual 
                System_Mode = "Manual";
            }else{
                // Sets the system mode to auto 
                System_Mode = "Auto";
            }

            // Buzzes the buzzer once 
            Buzz(); 
        }

        // Checks if the OFF button is pressed
        if(Active && Check_Button(OFF_BUTTON)){
            Serial.println("Off button was pressed!"); 

            // Indicates that the display needs to be cleared 
            Control_Display_Changed = true;

            // Indicates that the LCD backlight needs to be turned on
            LCD_Active = true; 

            // Sets the system state to false 
            Active = false; // System shuts down  
            Fan_State = false; // Fan shuts down 
        }

        // Checks if the ON button is pressed 
        if(!Active && Check_Button(ON_BUTTON)){
            Serial.println("On button was pressed!"); 

            // Indicates that the display needs to be cleared 
            Control_Display_Changed = true;

            // Indicates that the LCD backlight needs to be turned on
            LCD_Active = true; 

            // Sets the system state to true 
            Active = true; // System starts up 
            Fan_State = true; // Fan starts up 
        }

        // --- [POTENTIOMETERS] --- \\

        // Checks if the user wants to change the fan speed 
        if(Active && (Get_Set_Fan_Speed() != Fan_Speed)){
            // Indicates that the display needs to be cleared 
            Control_Display_Changed = true;

            // Indicates that the LCD backlight needs to be turned on
            LCD_Active = true; 

            // Checks if the current mode is auto and if so
            // this change indicates that the user wants to go into manual mode 
            if(System_Mode == "Auto"){
                // Sets the system mode to manual 
                System_Mode = "Manual";
            }

            // Gets the new fan speed 
            Fan_Speed = Get_Set_Fan_Speed();
        }

        // Checks if the user wants to change the set temperature 
        if(Active && (Get_Set_Temperature() != Set_Temperature)){
            // Indicates that the display needs to be cleared 
            Control_Display_Changed = true;

            // Indicates that the LCD backlight needs to be turned on
            LCD_Active = true; 

            // Checks if the current mode is manual and if so
            // this change indicates that the user wants to go into auto mode 
            if(System_Mode == "Manual"){
                // Sets the system mode to manual 
                System_Mode = "Auto";
            }

            // Gets the new set temperature 
            Set_Temperature = Get_Set_Temperature();
        }

    #else
        // Real environment 

        // --- [IR RECEIVER] --- \\
        // Checks if the IR receiver has received a signal 
        if(iirecv.decode(&results)){
            unsigned int value = results.value; // Gets the value of the IR signal 
            Serial.print("IR Signal Received: "); // Prints the IR signal value
            Serial.println(value);
            Serial.println("---------------------------");
            iirecv.resume(); // Resumes the IR receiver
        }

        // --- [BUTTONS] --- \\

        // Check if the AUTO button is pressed
        if(Active && Check_Button(AUTO_BUTTON)){
            Serial.println("Auto Button was pressed!"); 

            // Indicates that the display needs to be cleared
            Display_Changed = true;

            // Indicates that the LCD backlight needs to be turned on
            LCD_Active = true; 

            // Checks if the system is in auto mode 
            if(System_Mode == AUTO){
                // Sets the system mode to manual 
                System_Mode = "Manual";
            }else{
                // Sets the system mode to auto 
                System_Mode = "Auto";
            }

            // Buzzes the buzzer once 
            Buzz(); 
        }

        // Checks if the OFF button is pressed
        if(Active && Check_Button(OFF_BUTTON)){
            Serial.println("Off button was pressed!"); 

            // Indicates that the display needs to be cleared 
            Display_Changed = true;
            
            // Indicates that the LCD backlight needs to be turned on
            LCD_Active = true; 

            // Sets the system state to false 
            Active = false; // System shuts down  
            Fan_State = false; // Fan shuts down 
        }

        // Checks if the ON button is pressed
        if(!Active && Check_Button(ON_BUTTON)){
            Serial.println("On button was pressed!"); 

            // Indicates that the display needs to be cleared 
            Display_Changed = true;

           // Indicates that the LCD backlight needs to be turned on
            LCD_Active = true; 

            // Sets the system state to true 
            Active = true; // System starts up 
            Fan_State = true; // Fan starts up 
        }

        // --- [POTENTIOMETERS] --- \\

        // Checks if the user wants to change the fan speed
        if(Active && (Get_Set_Fan_Speed() != Fan_Speed)){
            // Indicates that the display needs to be cleared 
            Display_Changed = true;

            // Indicates that the LCD backlight needs to be turned on
            LCD_Active = true; 

            // Checks if the current mode is auto and if so
            // this change indicates that the user wants to go into manual mode 
            if(System_Mode == "Auto"){
                // Sets the system mode to manual 
                System_Mode = "Manual";
            }

            // Gets the new fan speed 
            Fan_Speed = Get_Set_Fan_Speed();
        }

        // Checks if the user wants to change the set temperature
        if(Active && (Get_Set_Temperature() != Set_Temperature)){
            // Indicates that the display needs to be cleared 
            Display_Changed = true;

            // Indicates that the LCD backlight needs to be turned on
            LCD_Active = true; 

            // Checks if the current mode is manual and if so
            // this change indicates that the user wants to go into auto mode 
            if(System_Mode == "Manual"){
                // Sets the system mode to manual 
                System_Mode = "Auto";
            }

            // Gets the new set temperature 
            Set_Temperature = Get_Set_Temperature();
        }
    #endif

}

// Updates the actual system components based on the current system state 
void Update_System_Components(){
   
    // Checks whether the LCD backlight needs to be turned on or off after a certain amount of time 
    if(LCD_Active){ // LCD active

        // Checks current runtime environment 
        #ifdef VIRTUAL_ENVIRONMENT 
            // Virtual environment 

            // Turns the LCD on 
            CONTROL_LCD.backlight(); 
            STATUS_LCD.backlight();

            // waits the max idle time then turns off the backlight 
            if(LCD_Timer.Check_Time_Millis(Max_LCD_Idle_Time)){
                // Turns off the LCD backlight 
                CONTROL_LCD.noBacklight();
                STATUS_LCD.noBacklight();
                LCD_Active = false; 
            }
        #else
            // Real environment 

            // Turns the LCD on
            CONTROL_LCD.backlight();
            
            // Waits the max idle time and turns off the backlight 
            if(LCD_Timer.Check_Time_Millis(Max_LCD_Idle_Time)){
                // Turns off the LCD backlight 
                CONTROL_LCD.noBacklight();
                LCD_Active = false; 
            }

        #endif
    }
   
    // Checks whether the current system state is not active 
    if(!Active){
        // Turns off the motor 
        Set_Fan_Speed(0); 

        // Turns off the LEDs 
        Set_LED(RED_LED,Max_LED_Brightness); // Red LED is on
        Set_LED(GREEN_LED,0); // Green LED is off
        return; 
    }else{
        // Turns on the LEDs
        Set_LED(RED_LED,0); // Red LED is off
        Set_LED(GREEN_LED,Max_LED_Brightness); // Green LED is on
    }


    // --- [AUTO MODE] --- \\ 

    // Checks if the fan state on and if auto mode is enabled 
    if(Fan_State && System_Mode == "Auto"){
        
        // Checks if the current temperature is greater than the set temperature
        if(Current_Temperature > Set_Temperature){
            // Turns on the fan to cool down the room 
            
            // Calculates the auto fan speed 
            Auto_Fan_Speed = constrain(map(Temperature, Set_Temperature, 40, 0, 255),0,255);

            Set_Fan_Speed(Auto_Fan_Speed);
        }else{
            // Turns off the fan as the room is cooled to the set temperature
            Set_Fan_Speed(0);
        }
        return; 
    }

    // --- [MANUAL MODE] --- \\

    // Checks if the fan state is on and if manual mode is enabled
    if(Fan_State && System_Mode == "Manual"){
        // Sets the fan speed to the set fan speed 
        Set_Fan_Speed(Fan_Speed);
        return; 
    }
}

// Displays the NeoPixel lights using the provided pattern 
void Update_Neo_Pixel(){
    // Checks if the fan is off and the neopixel is on  
    if(!Fan_State || !Active){
        // Only turns it off if the neopixel is on 
        if(Neo_Pixel_State){
            // Turns off the Neo_Pixel 
            Neo_Pixel.clear(); 
            Neo_Pixel.show();
        }
        return; 
    }

    // Checks if the delay time has not passed
    if(!Neo_Pixel_Timer.Check_Time_Millis(Neo_Pixel_Delay_Between_Lights)){
        return; // Returns as the delay time has not passed 
    }

    // Sets the Neo_Pixel max brightness 
    Neo_Pixel.setBrightness(Max_Neo_Pixel_Brightness);

    // Gets the individual colors in the array pattern 
    int R = Neo_Pixel_Color_Pattern[Pixel_Pattern_Index][0];
    int G = Neo_Pixel_Color_Pattern[Pixel_Pattern_Index][1];
    int B = Neo_Pixel_Color_Pattern[Pixel_Pattern_Index][2];

    // Sets the Neo_Pixel color 
    Neo_Pixel.setPixelColor(Pixel_Light_Index, R, G, B);
    Neo_Pixel.show();

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
    
    // Initializations
    Serial.begin(9600); // Starts the serial communication
    
    // -- [INPUTS] -- \\

    // DIGITAL 
    pinMode(ON_BUTTON,INPUT); // Sets the ON button as an input
    pinMode(OFF_BUTTON,INPUT); // Sets the OFF button as an input
    pinMode(AUTO_BUTTON,INPUT); // Sets the AUTO button as an input
    irrecv.enableIRIn(); // Starts the IR receiver

    // ANALOG
    pinMode(TEMPERATURE_SENSOR,INPUT); // Sets the temperature sensor as an input 
    pinMode(HUMIDITY_SENSOR,INPUT); // Sets the humidity sensor as an input

    pinMode(FAN_SPEED_CONTROL,INPUT); // Sets the fan speed control as an input 
    pinMode(TEMPERATURE_CONTROL,INPUT); // Sets the temperature control as an input

    // -- [OUTPUTS] -- \\
    
    // DIGITAL 
    pinMode(GREEN_LED,OUTPUT); // Sets the green LED as an output
    pinMode(RED_LED,OUTPUT); // Sets the red LED as an output
    pinMode(WARNING_LED,OUTPUT); // Sets the warning LED as an output

    pinMode(DC_MOTOR,OUTPUT); // Sets the DC motor as an output
    pinMode(PIEZO,OUTPUT);


    // -- [START] -- \\ 

    // Initializes the NeoPixel 
    NEO_PIXEL.begin(); 
   

    // Starts the LCD displays 
    // Checks current execution environment 
    #ifdef VIRTUAL_ENVIRONMENT
        // Virtual environment 
        CONTROL_LCD.init(); 
        STATUS_LCD.init();

        // Turns on the backlight 
        CONTROL_LCD.backlight(); 
        STATUS_LCD.backlight();

        // Turns off the backlight
        CONTROL_LCD.noBacklight();
        STATUS_LCD.noBacklight();
    #else
        // Real environment 
        CONTROL_LCD.init();
        CONTROL_LCD.backlight();

        // Turns off the backlight
        CONTROL_LCD.noBacklight(); 
    #endif

    
    // Gets how many timers have been created 
    unsigned int Timer_Count = Neo_Pixel_Timer.Get_Timer_Count(); 
    Serial.print("Timer Count: ");
    Serial.println(Timer_Count); 
    
    // Outputs to the serial monitor 
    Serial.println("System started correctly!"); 
}


// System Loop 
void loop(){
    // Updates the environment data on the system 
    Update_Environment_Data();

    // Updates the system based on user input 
    Update_System_On_Input(); 

    // Updates the system components based on the current system state 
    Update_System_Components();

    // Updates the NeoPixel 
    Update_Neo_Pixel(); 

}





