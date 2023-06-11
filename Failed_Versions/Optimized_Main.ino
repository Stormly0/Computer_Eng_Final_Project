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

// ---------------- [TOP LEVEL SYSTEM VARIABLES] ----------------
// System State 
bool Integrity = true; // Indicates whether the system is able to continue program execution 

// System States 

// System is on or off 
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

// ------------ [Variables] ------------

// System Configurations 

// Lighting Parameters 

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

// ------------ [Enums] ------------ 

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

// Display class that handles all the display states as the code can request the handler to refresh the LCD's
class Display : private Timer{
    // Constructor 
    public: 
        Display(Display_Name Name, int Rows, int Columns) : Timer("Internal LCD Display Timer1"){
            this->LCD_NAME = Name; // Sets the LCD type and name to determine behaviour 
            this->ROWS = Rows; // Indicates how many rows are on the LCD
            this->COLUMNS = Columns; // Indicates how many columns are on the LCD
        }

    // Private 
    private: 
        // Display Specification 
        // The number of rows and columns of the LCD and the name of it 
        Display_Name LCD_NAME; // The function that is called to display the LCD
        int ROWS = 0;
        int COLUMNS = 0;

        // Display States 
        // On and off states of the LCD 
        bool Initialized = false; // Indicates whether the LCD has been initialized
        bool Display_Active = false; // Indicates whether the display is active [Backlight]

        // Debounce checks
        
        int Max_Refresh_Rate = 500; // Time that the LCD has to wait before refreshing again in milliseconds
        int Too_Many_Clear_Calls = 0; // Keeps track of how many clear calls are made to the LCD to detect if the LCD is being refreshed too much by uncontrolled function call requests
        int Max_Clear_Call_Threashold = 100; // The maximum number of clear calls that can be made to the LCD before the LCD is considered to be refreshing too much

        // LCD STATUS
        // The current LCD status  
        bool REFRESHING = false; // Indicates whether the LCD is currently refreshing 
        bool REFRESH_REQUEST = false; // Indicates whether the LCD has been requested to refresh

    // Public
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

        // ---- [Utilities] ---- 
        
        // Turns on the backlight on the LCD 
        void Backlight(bool State){
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

        // Sets the LCD data onto the display 
        void Write(String Data,int X_Pos = 0, int Y_Pos = 0){
            // Checks if the LCD has been initialized 
            if(!this->Initialized){
                // LCD has not been initialized 
                Serial.println((this->LCD_NAME == TOP ? "Top LCD" : "Bottom LCD") + String(" has not been initialized yet")); 
                return; 
            }

            // Checks if the timer time has elapsed other wise it keeps the refreshing to false 
            if((this->REFRESHING) && Milliseconds(Max_Refresh_Rate)){
                // Sets the refreshing to false 
                this->REFRESHING = false; 
            }

            // Checks the Environment 
            #if VIRTUAL_ENVIRONMENT 
                // Virtual Environment 

                // Checks if there is a clear request in the LCD state 
                if(this->REFRESH_REQUEST && !this->REFRESHING){
                    // Sets the refreshing to true 
                    this->REFRESHING = true;

                    // Checks which LCD to clear 
                    if(this->LCD_NAME == TOP){
                        TOPLCD.clear(); // Clears the LCD
                    }else if(this->LCD_NAME == BOTTOM){
                        BOTTOMLCD.clear(); // Clears the LCD
                    }

                    // Sets the refresh request to false 
                    this->REFRESH_REQUEST = false;
                }else if(this->REFRESH_REQUEST){
                    Serial.println("LCD is already refreshing"); 
                    this->Too_Many_Clear_Calls++; // Increments the clear calls
                }

                if(this->LCD_NAME == TOP){
                    TOPLCD.setCursor(X_Pos,Y_Pos); // Sets the cursor position 
                    TOPLCD.print(Data); // Prints the data onto the LCD 
                }else if(this->LCD_NAME == BOTTOM){
                    BOTTOMLCD.setCursor(X_Pos,Y_Pos); // Sets the cursor position 
                    BOTTOMLCD.print(Data); // Prints the data onto the LCD 
                }
            #else
                // Real Environment 

                // Checks if there is a clear request in the LCD state
                if(this->REFRESH_REQUEST && !this->REFRESHING){
                    // Sets the refreshing to true 
                    this->REFRESHING = true;

                    // Clears the LCD 
                    LCD.clear(); 

                    // Sets the refresh request to false 
                    this->REFRESH_REQUEST = false;
                }else if(this->REFRESH_REQUEST){
                    Serial.println("LCD is already refreshing"); 
                    this->Too_Many_Clear_Calls++; // Increments the clear calls
                }

                LCD.setCursor(X_Pos,Y_Pos); // Sets the cursor position 
                LCD.print(Data); // Prints the data onto the LCD
            #endif
        }

        // Writes a special character 
        void Write(int Data, int X_Pos = 0, int Y_Pos = 0){
            // Checks if the LCD has been initialized 
            if(!this->Initialized){
                // LCD has not been initialized 
                Serial.println((this->LCD_NAME == TOP ? "Top LCD" : "Bottom LCD") + String(" has not been initialized yet")); 
                return; 
            }

            // Checks if the timer time has elapsed other wise it keeps the refreshing to false 
            if((this->REFRESHING) && Milliseconds(Max_Refresh_Rate)){
                // Sets the refreshing to false 
                this->REFRESHING = false; 
            }

            // Checks the Environment 
            #if VIRTUAL_ENVIRONMENT 
                // Virtual Environment 

                // Checks if there is a clear request in the LCD state 
                if(this->REFRESH_REQUEST && !this->REFRESHING){
                    // Sets the refreshing to true 
                    this->REFRESHING = true;

                    // Checks which LCD to clear 
                    if(this->LCD_NAME == TOP){
                        TOPLCD.clear(); // Clears the LCD
                    }else if(this->LCD_NAME == BOTTOM){
                        BOTTOMLCD.clear(); // Clears the LCD
                    }

                    // Sets the refresh request to false 
                    this->REFRESH_REQUEST = false;
                }else if(this->REFRESH_REQUEST){
                    Serial.println("LCD is already refreshing"); 
                    this->Too_Many_Clear_Calls++; // Increments the clear calls
                }

                if(this->LCD_NAME == TOP){
                    TOPLCD.setCursor(X_Pos,Y_Pos); // Sets the cursor position 
                    TOPLCD.write(Data); // Prints the data onto the LCD 
                }else if(this->LCD_NAME == BOTTOM){
                    BOTTOMLCD.setCursor(X_Pos,Y_Pos); // Sets the cursor position 
                    BOTTOMLCD.write(Data); // Prints the data onto the LCD 
                }
            #else
                // Real Environment 

                // Checks if there is a clear request in the LCD state
                if(this->REFRESH_REQUEST && !this->REFRESHING){
                    // Sets the refreshing to true 
                    this->REFRESHING = true;

                    // Clears the LCD 
                    LCD.clear(); 

                    // Sets the refresh request to false 
                    this->REFRESH_REQUEST = false;
                }else if(this->REFRESH_REQUEST){
                    Serial.println("LCD is already refreshing"); 
                    this->Too_Many_Clear_Calls++; // Increments the clear calls
                }

                LCD.setCursor( X_Pos, Y_Pos); // Sets the cursor position
                LCD.write(Data); // Prints the data onto the LCD
            #endif
        }

        // Clears a specific row on the LCD 
        void Clear_Row(int Y_Pos = 0){
            // Checks the current runtime environment 
            #if VIRTUAL_ENVIRONMENT 
            Serial.println("Clearing Row");
                // Virtual Environment 
                // Checks which LCD to clear 
                if(this->LCD_NAME == TOP){
                    // Loops through the row and clears the entire row 
                    for(int i = 0; i < this->ROWS; i++){
                        TOPLCD.setCursor(i,Y_Pos); // Sets the cursor position 
                        TOPLCD.print(" "); // Prints the data onto the LCD 
                    }
                }else if(this->LCD_NAME == BOTTOM){
                    // Loops through the row and clears the entire row 
                    for(int i = 0; i < this->ROWS; i++){
                        BOTTOMLCD.setCursor(i,Y_Pos); // Sets the cursor position 
                        BOTTOMLCD.print(" "); // Prints the data onto the LCD 
                    }
                }
            #else 
                // Real Environment 
                // Loops through the row and clears the entire row 
                for(int i = 0; i < this->ROWS; i++){
                    LCD.setCursor(i,Y_Pos); // Sets the cursor position 
                    LCD.print(" "); // Prints the data onto the LCD 
                }
            #endif
        }

        // Clears the LCD
        inline void Clear(){
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
};

// Better class display [Rewrite]
class Display_V2{

    // Constructors 
    public: 
        Display_V2(::Display_Name Name, unsigned int Max_X, unsigned int Max_Y){
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
        ::Timer LCD_Timer_1 = ::Timer("LCD Refresh Timer 1"); // Timer that keeps track of the refresh rate of the LCD
        ::Timer LCD_Timer_2 = ::Timer("LCD Refresh Timer 2"); // Timer that keeps track of the refresh rate of the LCD
        ::Display_Name LCD_NAME; // Name of the LCD 
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
       
        ::Screen_Name Current_Screen = ::Screen_Name::MAIN; // The current screen that the LCD is displaying
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
            
            // Checks the max refresh rate before setting the refreshing state to false 
            if(this->REFRESHING && this->LCD_Timer_2.Milliseconds(this->Max_Refresh_Rate)){
                // Max refresh rate has been reached 
                this->REFRESHING = false; 
            }

            // Checks if the entire screen needs to be cleared and the lcd is not already refreshing  
            if(this->REFRESH_REQUEST && !this->REFRESHING){
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

            // Checks if there is a single row refresh request and the lcd is not already refreshing
            if(this->SINGLE_ROW_REFRESH_REQUEST && !this->REFRESHING){
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

            

            // Checks the LCD timer before updating the display back to the main display 
            if(this->Current_Screen != MAIN && this->LCD_Timer_1.Milliseconds(LCD_Switch_Screen_Delay)){
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
                        TOPLCD.print("Mode: " + ::System_Mode); // Prints the system mode 

                        TOPLCD.setCursor(0,1); // Sets the cursor to the bottom left corner of the LCD 
                        TOPLCD.print("Fan State: " + (String)((::Fan_State) ? "On" : "Off")); // Prints the fan state
                    #else
                        LCD.setCursor(0,0); // Sets the cursor to the top left corner of the LCD 
                        LCD.print("Mode: " + ::System_Mode); // Prints the system mode

                        LCD.setCursor(0,1); // Sets the cursor to the bottom left corner of the LCD
                        LCD.print("Fan State: " + (String)((::Fan_State) ? "On" : "Off")); // Prints the fan state
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
                        Fan_Speed_Percentage = ((float)::Fan_Speed/255.0) * 100; 
                        
                        TOPLCD.print("Fan Speed: " + String(Fan_Speed_Percentage) + "%"); // Prints the fan state
                    #else 
                        // Real Environment 
                        LCD.setCursor(0,0); // Sets the cursor to the top left corner of the LCD 
                        LCD.print("Set Fan Speed"); // Prints the fan speed 

                        LCD.setCursor(0,1); // Sets the cursor to the bottom left corner of the LCD 

                        // Converts the set fan speed into a percentage 
                        Fan_Speed_Percentage = (::Fan_Speed/255) * 100; 

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
                        
                        TOPLCD.print("Temperature: " + String(::Set_Temperature) + "C"); // Prints the fan state
                    #else
                        // Real Environment 
                        LCD.setCursor(0,0); // Sets the cursor to the top left corner of the LCD 
                        LCD.print("Set Temperature"); // Prints the fan speed 

                        LCD.setCursor(0,1); // Sets the cursor to the bottom left corner of the LCD 

                        LCD.print("Temperature: " + String(::Set_Temperature) + "C"); // Prints the fan state
                    #endif
            }
 
        }

};



// ---------------- [Objects] ----------------

// System Class Objects 
#if VIRTUAL_ENVIRONMENT 
    // Virtual Environment 
    Display Display_Top(TOP,16,2); // Top LCD 
    Display Display_Bottom(BOTTOM,16,2); // Bottom LCD
    Display_V2 DT_Test(TOP,16,2); // Top LCD
#else
    // Real Environment 
    Display Main_Display(SINGLE,20,4);
#endif

// Instead of using our previous method on changing displays 
// we can use enumerations to then change the display depending on whether it has changed 
// using in class private states to determine whether a change has occured in the first place 

// NEO Pixel is kind of boring and doesn't go fast enough so we need to make it go faster by 
// reducing the amount of processes that the arduino is doing before the neopixel is called 
// this way we can make the neopixel go faster due to the arduino having a faster execution time 

// Timers 

Timer LCD_Timer("LCD TIMER"); // The timer that switches the LCD screens
Timer LCD_Clear_Timer("LCD CLEAR TIMER"); // The timer that clears the LCD 
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
        // Checks the runtime environment 
        #if VIRTUAL_ENVIRONMENT
            // Virtual Environment 
            Display_Bottom.Clear_Row(1); // Clears the bottom display
        #else
            // Real Environment 
            Main_Display.Clear_Row(3); // Clears the bottom display
        #endif

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
            // Updates the display 
            Display_Top.Clear_Row(0); // Clears the first top status row 
        }

        // Checks if the on button was pressed 
        if(!Active && Check_Button(ON_BUTTON)){
            Serial.println("On Button Pressed!");
            // Buzzes the buzzer 
            Buzz(); 

            // Sets the system to active 
            Active = true; 

            // Updates the display 
            Display_Top.Clear_Row(1); // Clears the first top status row 
        }

        // Checks if the off button was pressed
        if(Active && Check_Button(OFF_BUTTON)){
            Serial.println("Off Button Pressed!");
            // Buzzes the buzzer 
            Buzz(); 

            // Sets the system to inactive 
            Active = false; 

            // Updates the display 
            Display_Top.Clear_Row(1); // Clears the first top status row 
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

            // Indicates that to the display to display the set fan speed screen for a few seconds
            Display_FanSpeed = true; 
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

        // Indicates that to the display to display the set temperature screen for a few seconds
        Display_SetTemp = true; 
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
    
            // Updates the display 
            Main_Display.Clear_Row(0); // Clears the first top status row 
        }
    
        // Checks if the on button was pressed 
        if(!Active && Check_Button(ON_BUTTON)){
            Serial.println("On Button Pressed!");
            // Buzzes the buzzer 
            Buzz(); 
    
            // Sets the system to active 
            Active = true; 
    
            // Updates the display 
            Main_Display.Clear_Row(1); // Clears the first top status row 
        }
    
        // Checks if the off button was pressed
        if(Active && Check_Button(OFF_BUTTON)){
            Serial.println("Off Button Pressed!");
            // Buzzes the buzzer 
            Buzz(); 
    
            // Sets the system to inactive 
            Active = false; 
    
            // Updates the display 
            Main_Display.Clear_Row(1); // Clears the first top status row 
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
    
            // Indicates that to the display to display the set fan speed screen for a few seconds
            Display_FanSpeed = true; 
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
    
            // Indicates that to the display to display the set temperature screen for a few seconds
            Display_SetTemp = true; 
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
            Display_Top.Backlight(true); 
            Display_Bottom.Backlight(true);
        #else
            // Real Environment 
            Main_Display.Backlight(true);
        #endif

        // Turns on the Green LED 
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED,LOW);  
    }else{
        // Turns off the LCD backlights 
        #if VIRTUAL_ENVIRONMENT 
            // Virtual Environment 
            Display_Top.Backlight(false); 
            Display_Bottom.Backlight(false);
        #else
            // Real Environment 
            Main_Display.Backlight(false);
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

    // Checks wihat display to update depending on the current environment 
    #if VIRTUAL_ENVIRONMENT 
        // Virtual Environment 
        // Checks if the display should be updated 
        if(Display_SetTemp){
            // Updates the display 
           
            // Checks if the display was cleared 
            if(!Display_SetTemp_Cleared){
                // Clears the display 
                Display_Top.Clear_Row(0); 
                Display_Top.Clear_Row(1); 
                // Indicates that the display was cleared
                Display_SetTemp_Cleared = true;
            }else if(LCD_Timer.Milliseconds(LCD_Switch_Screen_Delay)){
                Display_SetTemp_Cleared = false; // Indicates that the display can be cleared again 
            }

            // Displays the set temperature 
            Display_Top.Write("Set Temperature",0,0); 
            Display_Top.Write("Set Temp: " + String(Set_Temperature),0,1); // Displays the set temperature 

            // Waits a few seconds before switching back to the main screen 
            if(LCD_Timer.Milliseconds(LCD_Switch_Screen_Delay)){
                // Switches back to the main screen 
                Display_SetTemp = false; 
                // Clears the display row once it is finished 
                Display_Top.Clear_Row(0);
                Display_Top.Clear_Row(1); 
            }

            // Returns 
            return; 
        }

        // Checks if the set fan speed should be displayed 
        if(Display_FanSpeed){

            // Updates the display 

            // Checks if the display was cleared
            if(!Display_FanSpeed_Cleared){
                Display_Top.Clear_Row(0); // Clears the first top status row 
                Display_Top.Clear_Row(1); // Clears the second status 
                Display_FanSpeed_Cleared = true;
            }else if(LCD_Clear_Timer.Milliseconds(Max_LCD_Refresh)){
                Display_FanSpeed_Cleared = false; // Indicates that the display can be cleared again 
            }

            // Converts the set fan speed into a percentage 
            int Fan_Speed_Percentage = (Fan_Speed/255) * 100; 

            // Displays the set fan speed 
            Display_Top.Write("Set Fan Speed",0,0);
            Display_Top.Write("Fan Speed: " + String(Fan_Speed_Percentage) + "%",0,1); // Displays the set fan speed 
            // Display_Top.Write(0,Position,0); // Displays the degrees symbol and C
            // Display_Top.Write("%",Position + 1,0); // Displays the degrees symbol and C

            // Waits a few seconds before switching back to the main screen 
            if(LCD_Timer.Milliseconds(LCD_Switch_Screen_Delay)){
                // Switches back to the main screen 
                Display_FanSpeed = false; 
                Display_FanSpeed_Cleared = false; // Resets the display cleared variable
                // Clears the display row once it is finished 
                Display_Top.Clear_Row(0); 
                Display_Top.Clear_Row(1);  
            }

            // Returns 
            return; 
        }

        // Displays the main screen
        Display_Top.Write("Mode: " + System_Mode,0,0); // Displays the system mode
        Display_Top.Write("Fan State: " + (String)((Fan_State == 1) ? "On" : "Off"),0,1); // Displays the fan state

        // Displays the temperature 
        Display_Bottom.Write("Temperature",0,0);
        Display_Bottom.Write(String(Temperature) + "C",0,1); 
    #else
        // Real Environment 
        // Checks if the display should be updated 
        if(Display_SetTemp){
            // Updates the display 
           
            // Checks if the display was cleared 
            if(!Display_SetTemp_Cleared){
                // Clears the display 
                Main_Display.Clear_Row(0); 
                Main_Display.Clear_Row(1); 
                // Indicates that the display was cleared
                Display_SetTemp_Cleared = true;
            }else if(LCD_Timer.Milliseconds(LCD_Switch_Screen_Delay)){
                Display_SetTemp_Cleared = false; // Indicates that the display can be cleared again 
            }

            // Gets the temperature as a string
            String Set_Temp_String = String(Set_Temperature);
            int Length = Set_Temp_String.length();// Gets the length 

            // Calculates the postion of where the cursor needs to be to properly place the degrees symbol and C 
            int Position = 16 - Length - 2; 

            // Displays the set temperature 
            Main_Display.Write("Set Temperature",0,0); 
            Main_Display.Write("Set Temp: " + String(Set_Temperature),0,1); // Displays the set temperature 
            // Main_Display.Write(0,Position,0); // Displays the degrees symbol and C
            // Main_Display.Write("C",Position + 1,0); // Displays the degrees symbol and C

            // Waits a few seconds before switching back to the main screen 
            if(LCD_Timer.Milliseconds(LCD_Switch_Screen_Delay)){
                // Switches back to the main screen 
                Display_SetTemp = false; 
                Main_Display.Clear_Row(0); // Clears the first top status row
                Main_Display.Clear_Row(1); 
            }

            // Returns 
            return; 
        }

        // Checks if the set fan speed should be displayed 
        if(Display_FanSpeed){

            // Updates the display 

            // Checks if the display was cleared
            if(!Display_FanSpeed_Cleared){
                Main_Display.Clear_Row(0); // Clears the first top status row 
                Main_Display.Clear_Row(1); 
                Display_FanSpeed_Cleared = true;
            }else if(LCD_Clear_Timer.Milliseconds(Max_LCD_Refresh)){
                Display_FanSpeed_Cleared = false; // Indicates that the display can be cleared again 
            }
           
            // Converts the set fan speed into a percentage 
            int Fan_Speed_Percentage = (Fan_Speed/255) * 100; 
            
            // Displays the set fan speed 
            Main_Display.Write("Set Fan Speed",0,0);
            Main_Display.Write("Fan Speed: " + String(Fan_Speed_Percentage) + "%",0,1); // Displays the set fan speed 
            // Main_Display.Write(0,Position,0); // Displays the degrees symbol and C
            // Main_Display.Write("%",Position + 1,0); // Displays the degrees symbol and C

            // Waits a few seconds before switching back to the main screen 
            if(LCD_Timer.Milliseconds(LCD_Switch_Screen_Delay)){
                // Switches back to the main screen 
                Display_FanSpeed = false; 
                Display_FanSpeed_Cleared = false; // Resets the display cleared variable

                Main_Display.Clear_Row(0); 
                Main_Display.Clear_Row(1); 
            }

            // Returns 
            return; 
        }

        // Displays the main screen
        Main_Display.Write("Mode: " + System_Mode,0,0); // Displays the system mode
        Main_Display.Write("Fan State: " + (String)((Fan_State == 1) ? "On" : "Off"),0,1); // Displays the fan state

        // Displays the temperature 
        Main_Display.Write("Temperature",0,2); 
        Main_Display.Write(String(Temperature) + " C",0,3); 
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

    // Checks if the delay time has not passed
    // if(!Neo_Pixel_Timer.Milliseconds(Neo_Pixel_Delay_Between_Lights)){
    //     return; // Returns as the delay time has not passed 
    // }

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
        Display_Top.Init(); // Initializes the LCD
        Display_Bottom.Init(); // Initializes the LCD
    #else
        // Real Environment 
        Main_Display.Init(); // Initializes the LCD
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
        Display_Top.Backlight(true); // Turns on the backlight on the LCD
        Display_Bottom.Backlight(true); // Turns on the backlight on the LCD
    #else
        // Real Environment 
        Main_Display.Backlight(true); // Turns on the backlight on the LCD
    #endif

    //Gets the set temperature and set fan speed 
    Fan_Speed = Get_Set_Fan_Speed(); 
    Set_Temperature = Get_Set_Temperature();   

}


void loop(){
  Serial.println(Active); 
    Update_Environment_Data(); 
    Update_System_On_Input(); 
    Update_System_Components();
    Update_Neo_Pixel(); 
}
