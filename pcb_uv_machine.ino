#include <LiquidCrystal.h>

#define BLINK_UP_MS 500
#define BLINK_DOWN_MS 350
#define SELECT_DELAY_MS 680
#define PLUS_BUTTON_DELAY_MS 200
#define SELECTION_BLINK_PAUSE_MS 100
#define FAST_CHANGE_DELAY_MS 1000
#define FAST_CHANGE_PERIOD_MS 120
#define DELAY_DEBOUNCE_MS 150
#define SERIAL_DEBUG
#define START_DELAY_MS 1000
#define UV_RELAY_PIN 13

#define FIX_NEG(A, MAX) (((A) < 0)?MAX-1:A)

// LCD pin config
const int d4 = 2, d5 = 3, d6 = 4, d7 = 5;
const int rs = 6, e = 7;
LiquidCrystal lcd(rs, e, d4, d5, d6, d7);

int pinStartStop = 8;
int pinPlus = 9;
int pinMinus = 10;
int pinSelect = 11;

int8_t h = 0, m = 0, s = 0;
int select = 0;

bool buttonPress = true;

unsigned long lastSelect = 0;
unsigned long lastPressedSelect = 0;
unsigned long lastPressedPlus = 0;
unsigned long lastPressedMinus = 0;
unsigned long lastPressedStartStop = 0;

bool lastPlusValue = false;
bool lastMinusValue = false;
bool lastSelectValue = false;
bool lastStartStopValue = false;

bool exposureStarted = false;

String generateHMSString(){
    char output[] = "00:00:00";
    sprintf(output, "%02d:%02d:%02d", h, m, s);
    return String(output);
}

void increaseValue(){
    switch(select){
        case 0: h++; h %= 100; break;
        case 1: m++; m %=  60; break;
        case 2: s++; s %=  60; break;
    }
    return;
}

void decreaseValue(){
    switch(select){
        case 0: h--; h = FIX_NEG(h, 100); break;
        case 1: m--; m = FIX_NEG(m, 60 ); break;
        case 2: s--; s = FIX_NEG(s, 60 ); break;
    }    
}

void changeSegment(){
    select = (select + 1) % 3;
}

String generateHMSStringBlink(int segment){
    char output[] = "00:00:00";
    switch(segment){
        case 0: sprintf(output, "  :%02d:%02d", m, s); break;
        case 1: sprintf(output, "%02d:  :%02d", h, s); break;
        case 2: sprintf(output, "%02d:%02d:  ", h, m); break;
    }
    return String(output);
}

void refreshTime(){
    lcd.setCursor(6, 1);
    lcd.print(generateHMSString());  
}

void refreshTimeBlink(int segment){
    segment = segment % 3;
    lcd.setCursor(6, 1);
    lcd.print(generateHMSStringBlink(segment));
}

bool isInStopwatchMode(){
    return (h == 0 && m == 0 && s == 0);
}

String hms(unsigned long time_seconds){
    int h = (time_seconds / 3600) % 100;
    int remaining = time_seconds % 3600;
    int m = remaining / 60;
    int s = remaining % 60;

    char format[] = "00:00:00";
    sprintf(format, "%02d:%02d:%02d", h, m, s);
    
    return String(format);
}

void displayMenu(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("set exposure");
    lcd.setCursor(0, 1);
    lcd.print("time: ");
}

void turnON(){
    digitalWrite(UV_RELAY_PIN, HIGH);
}

void turnOFF(){
    digitalWrite(UV_RELAY_PIN, LOW);
}

void stopwatchMode(){
    unsigned long stopwatchStartTime = millis();
    int row = 0;
    int col = 0;

    turnON();
    
    lcd.clear();
    lcd.setCursor(col, row);
    lcd.print("[stopwatch mode]");
    row = 1;
    col = 4;
    lcd.setCursor(col, row);

    lcd.print(hms((millis()/1000) - (stopwatchStartTime/1000)));
    delay(START_DELAY_MS);

    row = 0;
    col = 0;
    lcd.setCursor(col, row);
    lcd.print(" elapsed time:  ");
    
    row = 1;
    col = 4;
    lcd.setCursor(col, row);

    String lastTime = hms((millis()/1000) - (stopwatchStartTime/1000));
    
    while(!detectStopButton()){
        String now;
        if((now = hms((millis()/1000) - (stopwatchStartTime/1000))) != lastTime){
            Serial.println("Time changed: " + now);
            lcd.print(now);
            lastTime = now;
            lcd.setCursor(col, row);
        }
    }
    turnOFF();
    return;
}

unsigned long getSecondsFromHMS(){
    return (unsigned long) (h * 3600 + m * 60 + s);
}

void countdownMode(){
    unsigned long stopwatchStartTime = millis();
    int row = 0;
    int col = 0;

    turnON();
    
    lcd.clear();
    lcd.setCursor(col, row);
    lcd.print("[countdown mode]");
    row = 1;
    col = 4;
    lcd.setCursor(col, row);

    lcd.print(hms(getSecondsFromHMS()));
    delay(START_DELAY_MS);

    row = 0;
    col = 0;
    lcd.setCursor(col, row);
    lcd.print("remaining time: ");
    
    row = 1;
    col = 4;
    lcd.setCursor(col, row);

    String lastTime = hms((millis()/1000) - (stopwatchStartTime/1000));
    
    while(!detectStopButton() && ((millis() - stopwatchStartTime) <= (getSecondsFromHMS()*1000))){
        String now;
        if((now = hms((millis()/1000) - (stopwatchStartTime/1000))) != lastTime){
            Serial.println("Time changed: " + now);

            unsigned long elapsedMillis = millis() - stopwatchStartTime;
            unsigned long elapsedSeconds = elapsedMillis / 1000;
            unsigned long startSeconds = getSecondsFromHMS();
            int remaining = startSeconds - elapsedSeconds;
            
            lcd.print(hms(remaining));
            lastTime = now;
            lcd.setCursor(col, row);
        }
    }
    turnOFF();
    return;
}

void startExposure(){
    Serial.println("exposure started");

    if(isInStopwatchMode()){
        Serial.println("stopwatch mode started");
        stopwatchMode();
        Serial.println("stopwatch mode stopped");
    }
    else{
        Serial.println("timer mode stared");
        countdownMode();
        Serial.println("timer mode ended");
    }

    displayMenu();
    return;    
}

bool detectStopButton(){
    bool startStopButton = (digitalRead(pinStartStop) == LOW);
    
    if(startStopButton && (lastStartStopValue == false)){
        Serial.println("stop pressed!");
        lastStartStopValue = true;
        delay(DELAY_DEBOUNCE_MS);
        return false;
    }
    if((startStopButton == false) && (lastStartStopValue == true)){
        Serial.println("start released");
        lastStartStopValue = false;
        return true;
    }
    lastStartStopValue = startStopButton;
    return false;
}

bool detectButtonPress(){
    
    bool startStopButton = (digitalRead(pinStartStop) == LOW);
    bool plusButton = (digitalRead(pinPlus) == LOW);
    bool minusButton = (digitalRead(pinMinus) == LOW);
    bool selectButton = (digitalRead(pinSelect) == LOW);

    ///////////////////////////////////
    //////// select button ////////////
    ///////////////////////////////////
    
    if(selectButton && (lastSelectValue == false)){
        Serial.println("select pressed!");
        select++;
        select %= 3;
        lastSelectValue = true;
        delay(DELAY_DEBOUNCE_MS);
        return true;
    }
    if((selectButton == false) && (lastSelectValue == true)) Serial.println("select released");
    lastSelectValue = selectButton;
    
    ///////////////////////////////////
    ////////  plus button  ////////////
    ///////////////////////////////////
    
    if(plusButton && (lastPlusValue == false)){
        Serial.println("plus pressed!");
        lastPressedPlus = millis();
        
        increaseValue();
        refreshTime();
        lastPlusValue = true;
        delay(DELAY_DEBOUNCE_MS);
        return true;
    }
    else if(plusButton && (lastPlusValue == true) && ((millis() - lastPressedPlus) > FAST_CHANGE_DELAY_MS)){
        Serial.println("plus hold");
        int iter = 0;
        while(true){
            bool plusButton = (digitalRead(pinPlus) == LOW);
            Serial.println(plusButton);
            increaseValue();
            refreshTime();
            delay(FAST_CHANGE_PERIOD_MS);
            if(plusButton == false) break;
            iter++;
            }  
        lastPressedPlus = millis();
        Serial.print("plus released...[");Serial.print(iter);Serial.println("]");
        lastPlusValue = false;
        return true;
    }
    
    if((plusButton == false) && (lastPlusValue == true)) Serial.println("plus released");
    lastPlusValue = plusButton;

    ///////////////////////////////////
    ////////  minus button  ///////////
    ///////////////////////////////////
    
    if(minusButton && (lastMinusValue == false)){
        Serial.println("minus pressed!");
        lastPressedMinus = millis();
        decreaseValue();
        refreshTime();
        lastMinusValue = true;
        delay(DELAY_DEBOUNCE_MS);
        return true;
    }
    else if(minusButton && (lastMinusValue == true) && ((millis() - lastPressedMinus) > FAST_CHANGE_DELAY_MS)){
        Serial.println("minus hold");
        int iter = 0;
        while(true){
            bool minusButton = (digitalRead(pinMinus) == LOW);
            decreaseValue();
            refreshTime();
            delay(FAST_CHANGE_PERIOD_MS);
            if(minusButton == false) break;
            iter++;
            }  
        lastPressedMinus = millis();
        Serial.print("minus released...[");Serial.print(iter);Serial.println("]");
        lastMinusValue = false;
        return true;
    }
    
    if((minusButton == false) && (lastMinusValue == true)) Serial.println("minus released");
    lastMinusValue = minusButton;


    ///////////////////////////////////
    //////// start/stop button ////////
    ///////////////////////////////////
    
    if(startStopButton && (lastStartStopValue == false)){
        Serial.println("start pressed!");
        lastStartStopValue = true;
        delay(DELAY_DEBOUNCE_MS);
        return true;
    }
    if((startStopButton == false) && (lastStartStopValue == true)){
        Serial.println("start released");
        lastStartStopValue = false;
        startExposure();
    }
    lastStartStopValue = startStopButton;
    
    return false;
}

void setup() {
    pinMode(UV_RELAY_PIN, OUTPUT);
    digitalWrite(UV_RELAY_PIN, LOW);
    
    #ifdef SERIAL_DEBUG
    Serial.begin(115200);
    #endif
    
    lcd.begin(16, 2);
    displayMenu();
    
    digitalWrite(pinStartStop, HIGH);
    digitalWrite(pinPlus, HIGH);
    digitalWrite(pinMinus, HIGH);
    digitalWrite(pinSelect, HIGH);
    
    pinMode(pinStartStop, INPUT_PULLUP);
    pinMode(pinPlus, INPUT_PULLUP);
    pinMode(pinMinus, INPUT_PULLUP);
    pinMode(pinSelect, INPUT_PULLUP);
    
    digitalWrite(pinStartStop, HIGH);
    digitalWrite(pinPlus, HIGH);
    digitalWrite(pinMinus, HIGH);
    digitalWrite(pinSelect, HIGH);
}


void loop() {
    
    refreshTime();
    
    unsigned long current_time = millis();
    while((millis() - current_time) < BLINK_UP_MS){
        buttonPress = detectButtonPress();
        if(buttonPress){
            break;
        }
    }

    if(buttonPress){
        lastSelect = millis();
        refreshTime();
    }
    else{
        if((millis() - lastSelect) > SELECTION_BLINK_PAUSE_MS){
            refreshTimeBlink(select);
        }
        else{
            refreshTime();
        }
    }
    
    current_time = millis();
    while((millis() - current_time) < BLINK_DOWN_MS){
        buttonPress = detectButtonPress();
        if(buttonPress) break;
    }
}
