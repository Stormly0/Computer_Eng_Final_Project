
// VERSION 1.0.5 alpha 
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

bool Integrity = true; // Indicates whether the system is able to continue program execution 


//Neo Pixel
bool Neo_Pixel_State = false; // Indicates whether the NeoPixel is on or off
const unsigned int Neo_Pixel_Delay_Between_Lights = 1; // Delay between each light in the NeoPixel [ms]
const unsigned int Max_Neo_Pixel_Brightness = 5; // Max Brightness of the NeoPixel [0 - 255]
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
const unsigned int LCD_Switch_Screen_Delay = 1000; // The delay between switching screens in milliseconds
const unsigned int Max_LCD_Refresh = 100; // The maximum refresh rate of the LCD in milliseconds

// ----------------------------------

String System_Mode = "Manual"; // The mode that the system is in [Manual, Auto]
bool Active = false; // Whether the system is active or not (Power button is pressed)

float Set_Temperature = 0; // The temperature that the user sets
float Temperature = 0; // The temperature of the system's surrounding 
float Fan_Speed = 0; // The speed that the fan is running at [0-255]
bool Fan_State = false; // Indicates if the fan is spinning or not 

// Displays 
bool Display_SetTemp_Cleared = false; // Indicates whether the set temperature LCD has been cleared or not
bool Display_FanSpeed_Cleared = false; // Indicates whether the fan speed LCD has been cleared or not
bool Display_SetTemp = false; // Indicates whether to display the set temperature status 
bool Display_FanSpeed= false; // Indicates whether to display the fan speed status

bool Status_Changed = false; // Indicates whether any of the system variables has changed (Mode, and active)

// System Cache Variables 
bool Button_State_Cache[3] = {false,false,false}; // The button state cache
unsigned int Pixel_Light_Index = 0; 
unsigned int Pixel_Pattern_Index = 0; 
unsigned int Pixel_Pattern_Size = 7; // Size of the color pattern array 

// Display types for the virtual LCD's and the real LCD 
enum Display_Name{
    TOP,
    BOTTOM,
    SINGLE
}; 

// Indicates what screen to display on the LCD
enum Screen_Name{
    MAIN,
    TEMP_SET,
    FAN_SET,
};

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

Timer LCD_Timer("LCD TIMER"); // The timer that switches the LCD screens
// Better class display [Rewrite]
class Display : private Timer{

    // Constructors 
    public: 
        Display(Display_Name Name, unsigned int Max_X, unsigned int Max_Y) : Timer("Internal LCD Display Timer"){
            // Sets the display name 
            this->LCD_NAME = Name; 

            // Sets the display specifications 
            this->ROWS = Max_Y; 
            this->COLUMNS = Max_X;
            this->Current_Screen = MAIN; // Sets the current screen to the main screen
        }

    // Private 
    private: 
        // Display Specifications
        Display_Name LCD_NAME; // Name of the LCD 
        int ROWS = 0; // Number of rows on the LCD 
        int COLUMNS = 0; // Number of columns on the LCD

        // Display States 
        bool Initialized = false; // Indicates whether the LCD has been initialized 
        bool Display_Active = false; // Indicates whether the LCD is active or not [Backlight]

        // Debounce Checks 
        // Prevents runaway calls to the LCD which cause the LCD to display incorrect data 
        int Max_Refresh_Rate = 500; // Time that the LCD has to wait before refreshing again in milliseconds
        int Too_Many_Clear_Calls = 0; // Keeps track of how many clear calls are made to the LCD to detect if the LCD is being refreshed too much by uncontrolled function call requests
        int Max_Clear_Call_Threashold = 100; // The maximum number of clear calls that can be made to the LCD before the LCD is considered to be refreshing too much

        // LCD Status 
        
        ::Screen_Name Current_Screen; // The current screen that the LCD is displaying
        bool REFRESHING = false; // Indicates whether the LCD is currently refreshing or not 
        bool REFRESH_REQUEST = false; // Indicates whether the LCD has a request to refresh the entire screen 
        bool SINGLE_ROW_REFRESH_REQUEST = false; // Indicates to the LCD that there is a request to refresh a single row 
        unsigned int ROW_REFRESH = 0; // The row that the LCD is requested to refresh 

        // Temp Cache 
        int Fan_Speed_Percentage = 0; // The current fan speed percentage

    // Public functions
    public:
        // ==== [Initialization] ==== 

        // Initializes the LCD 
        void Init(){
            // Checks the Environment 
            #if VIRTUAL_ENVIRONMENT 
                // Virtual Environment 

                // Checks which LCD to initialize 
                if(this->LCD_NAME == TOP){
                    TOPLCD.init(); // Initializes the LCD
                }else if(this->LCD_NAME == BOTTOM){
                    BOTTOMLCD.init(); // Initializes the LCD
                }
            #else
        // Real Environment 
                LCD.init(); // Initializes the LCD
            #endif

            // Sets the LCD to initialized 
            this->Initialized = true; 
        }


        // ---- [UTILITIES] ---- 

        // Turns on the backlight on the LCD 
        void Backlight(bool State){
            // Checks if the system is initialized 
            if(!this->Initialized){
                // System is not initialized 
                Serial.println("LCD has not been initialized"); 
                return; // Exits the function 
            }

            // Checks the Environment 
            #if VIRTUAL_ENVIRONMENT 
                // Virtual Environment 

                // Checks which LCD to turn on the backlight 
                if(this->LCD_NAME == TOP){
                    State ? TOPLCD.backlight() : TOPLCD.noBacklight(); // Turns on the backlight on the LCD
                }else if(this->LCD_NAME == BOTTOM){
                    State ? BOTTOMLCD.backlight() : BOTTOMLCD.noBacklight(); // Turns on the backlight on the LCD
                }
            #else
                // Real Environment 
                State ? LCD.backlight() : LCD.noBacklight(); // Turns on the backlight on the LCD
            #endif

            // Sets the display active state 
            this->Display_Active = State; 
        }

        // Sets a display request on what screen to display 
        void Set_Screen(::Screen_Name Request_Screen){
            // Checks if the system is initialized 
            if(!this->Initialized){
                // System is not initialized 
                Serial.println("LCD has not been initialized"); 
                return; // Exits the function 
            }

            // Checks if the screen has changed 
            if(Request_Screen == this->Current_Screen){
                // Screen has not changed 
                return; // Exits the function 
            }

            // Sets a screen clear request as the current screen was changed 
            this->Clear(); 

            // Sets a set screen update request 
            Current_Screen = Request_Screen; 
        }

        // Clears a specific row on the LCD 
        void Clear_Row(int Y_Pos = 0){
            // Checks if the system is initialized 
            if(!this->Initialized){
                // System is not initialized 
                Serial.println("LCD has not been initialized"); 
                return; // Exits the function 
            }

            // Checks if there are too many clear calls and will shut down the entire arduino system 
            if(this->Too_Many_Clear_Calls >= this->Max_Clear_Call_Threashold){
                // Too many clear calls 
                Serial.println("Too many clear calls have been made to the LCD"); 
                Serial.println("Shutting down the system"); 
                Integrity = false; // System intergrity is lost 
            }

            // Checks if the current row is greater than the max number of rows 
            if(Y_Pos > this->ROWS){
                // Row is greater than the max number of rows 
                Serial.println("The row that is requested to be cleared is greater than the max number of rows"); 
                Serial.println("Shutting down the system"); 
                Integrity = false; // System intergrity is lost 
                return; 
            }

            // Sets a single row refresh request 
            this->SINGLE_ROW_REFRESH_REQUEST = true;
            this->ROW_REFRESH = Y_Pos;
        }

        // Clears the LCD
        void Clear(){
            // Checks if the system is initialized
            if(!this->Initialized){
                // System is not initialized
                Serial.println("LCD has not been initialized");
                return; // Exits the function
            }

            // Checks if there are too many clear calls and shuts down the entire arduino system 
            if(this->Too_Many_Clear_Calls >= this->Max_Clear_Call_Threashold){
                // Too many clear calls 
                Serial.println("Too many clear calls have been made to the LCD"); 
                Serial.println("Shutting down the system"); 
                Integrity = false; // System intergrity is lost 
            }

            // Sets a refresh request
            this->REFRESH_REQUEST = true;
        }

        // ---- [MAIN] ----  
        // Methods that are called every loop iteration in arduino 

        void Update_Display(){
            // Checks if the system is initialized 
            if(!this->Initialized){
                // System is not initialized 
                Serial.println("LCD has not been initialized"); 
                return; // Exits the function 
            }

            // Checks if the system lost integrity 
            if(!Integrity){
                // System lost integrity 
                Serial.println("System lost integrity. Display update Failed"); 
                return; // Exits the function 
            }

            // Checks if the entire screen needs to be cleared  
            if(this->REFRESH_REQUEST){
                // Entire screen needs to be cleared 
                // Checks the runtime environment 
                #if VIRTUAL_ENVIRONMENT
                    // Virtual Environment 
                    // Checks the the lcd name 
                    if(this->LCD_NAME == TOP){
                        TOPLCD.clear(); 
                    }else{
                        BOTTOMLCD.clear();
                    }
                #else
                    // Real Environment 
                    LCD.clear(); // Clears the LCD 
                #endif

                // Resets the refresh request 
                this->REFRESH_REQUEST = false; 
                this->REFRESHING = true; 
            }

            // Checks if there is a single row refresh request 
            if(this->SINGLE_ROW_REFRESH_REQUEST){
                // Single row refresh request 
                // Checks the runtime environment 
                #if VIRTUAL_ENVIRONMENT
                    // Virtual Environment 
                    // Checks the the lcd name 
                    if(this->LCD_NAME == TOP){
                        TOPLCD.setCursor(0,this->ROW_REFRESH); // Sets the cursor to the row that needs to be cleared 
                        TOPLCD.print("                    "); // Prints a blank line to clear the row
                    }else{
                        BOTTOMLCD.setCursor(0,this->ROW_REFRESH); // Sets the cursor to the row that needs to be cleared 
                        BOTTOMLCD.print("                    "); // Prints a blank line to clear the row
                    }
                #else
                    // Real Environment 
                    LCD.setCursor(0,this->ROW_REFRESH); // Sets the cursor to the row that needs to be cleared 
                    LCD.print("                    "); // Prints a blank line to clear the row 
                #endif

                // Resets the single row refresh request 
                this->SINGLE_ROW_REFRESH_REQUEST = false; 
                this->REFRESHING = true; 
            }

            // Waits the max refresh rate before setting the refreshing state to false 
            if(Milliseconds(this->Max_Refresh_Rate)){
                // Max refresh rate has been reached 
                this->REFRESHING = false; 
            }

            // Checks the LCD timer before updating the display back to the main display 
            if(this->Current_Screen != MAIN && LCD_Timer.Milliseconds(LCD_Switch_Screen_Delay)){
                // Switches the display back to the main display 
                this->Current_Screen = MAIN; 
            }

            // Checks what screen to display  
            if(Current_Screen == MAIN){
                    // Displays the main screen 
                    // Checks the runtime environment 
                    #if VIRTUAL_ENVIRONMENT
                        // Virtual Environment 
                        TOPLCD.setCursor(0,0); // Sets the cursor to the top left corner of the LCD 
                        TOPLCD.print("Mode: " + System_Mode); // Prints the system mode 

                        TOPLCD.setCursor(0,1); // Sets the cursor to the bottom left corner of the LCD 
                        TOPLCD.print("Fan State: " + (String)((Fan_State) ? "On" : "Off")); // Prints the fan state
                    #else
                        LCD.setCursor(0,0); // Sets the cursor to the top left corner of the LCD 
                        LCD.print("Mode: " + System_Mode); // Prints the system mode

                        LCD.setCursor(0,1); // Sets the cursor to the bottom left corner of the LCD
                        LCD.print("Fan State: " + (String)((Fan_State) ? "On" : "Off")); // Prints the fan state
                    #endif
            }else if(Current_Screen == FAN_SET){

                    // Displays the set fan speed screen 
                    // Checks the runtime environment
                    #if VIRTUAL_ENVIRONMENT
                        // Virtual Environment 
                        TOPLCD.setCursor(0,0); // Sets the cursor to the top left corner of the LCD 
                        TOPLCD.print("Set Fan Speed"); // Prints the fan speed 

                        TOPLCD.setCursor(0,1); // Sets the cursor to the bottom left corner of the LCD 
                        
                        // Converts the set fan speed into a percentage 
                        Fan_Speed_Percentage = ((float)Fan_Speed/255.0) * 100; 
                        
                        TOPLCD.print("Fan Speed: " + String(Fan_Speed_Percentage) + "%"); // Prints the fan state
                    #else 
                        // Real Environment 
                        LCD.setCursor(0,0); // Sets the cursor to the top left corner of the LCD 
                        LCD.print("Set Fan Speed"); // Prints the fan speed 

                        LCD.setCursor(0,1); // Sets the cursor to the bottom left corner of the LCD 

                        // Converts the set fan speed into a percentage 
                        Fan_Speed_Percentage = (Fan_Speed/255) * 100; 

                        LCD.print("Fan Speed: " + String(Fan_Speed_Percentage) + "%"); // Prints the fan state
                    #endif

            }else if(Current_Screen == TEMP_SET){
                    // Displays the set temperature screen 
                    // Checks the runtime environment
                    #if VIRTUAL_ENVIRONMENT
                        // Virtual Environment 
                        TOPLCD.setCursor(0,0); // Sets the cursor to the top left corner of the LCD 
                        TOPLCD.print("Set Temperature"); // Prints the fan speed 

                        TOPLCD.setCursor(0,1); // Sets the cursor to the bottom left corner of the LCD 
                        
                        TOPLCD.print("Temperature: " + String(Set_Temperature) + "C"); // Prints the fan state
                    #else
                        // Real Environment 
                        LCD.setCursor(0,0); // Sets the cursor to the top left corner of the LCD 
                        LCD.print("Set Temperature"); // Prints the fan speed 

                        LCD.setCursor(0,1); // Sets the cursor to the bottom left corner of the LCD 

                        LCD.print("Temperature: " + String(Set_Temperature) + "C"); // Prints the fan state
                    #endif
            }
 
        }

};


Display NewDisplay(TOP,16,2); 

void setup(){
    NewDisplay.Init(); 
}

void loop(){

    NewDisplay.Set_Screen(Screen_Name::MAIN); 
    NewDisplay.Update_Display();
    delay(1000);

    NewDisplay.Set_Screen(Screen_Name::FAN_SET);
    NewDisplay.Update_Display();
    delay(1000);

    NewDisplay.Set_Screen(Screen_Name::TEMP_SET);
    NewDisplay.Update_Display();
    delay(1000);

}