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
