#include <LiquidCrystal.h>

#define BLINK_UP_MS 500
#define BLINK_DOWN_MS 350
#define SELECT_DELAY_MS 680
#define PLUS_BUTTON_DELAY_MS 200
#define SELECTION_BLINK_PAUSE_MS 100
#define FAST_CHANGE_DELAY_MS 1000
#define FAST_CHANGE_PERIOD_MS 120
#define DELAY_DEBOUNCE_MS 150

#define FIX_NEG(A, MAX) (((A) < 0)?MAX-1:A)

// LCD pin ponfig
const int d4 = 2, d5 = 3, d6 = 4, d7 = 5;
const int rs = 6, e = 7;

LiquidCrystal lcd(rs, e, d4, d5, d6, d7);

int pinStartStop = 8;
int pinPlus = 9;
int pinMinus = 10;
int pinSelect = 11;
int8_t h = 0, m = 10, s = 0;
int select = 0;

void setup() {
    Serial.begin(115200);
    
    lcd.begin(16, 2);
    lcd.print("set exposure");
    lcd.setCursor(0, 1);
    lcd.print("time: ");
    delay(1000);
    
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

String generateHMSString(){
    h = h % 100;
    m = m % 60;
    s = s % 60;
    char output[] = "00:00:00";
    sprintf(output, "%02d:%02d:%02d", h, m, s);
    return String(output);
}

String generateHMSStringBlink(int segment){
    segment = segment % 3;
    
    h = h % 100;
    m = m % 60;
    s = s % 60;
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


unsigned long lastPressedSelect = 0;
unsigned long lastPressedPlus = 0;
unsigned long lastPressedMinus = 0;
unsigned long lastPressedStart = 0;

bool lastPlusValue = false;
bool lastMinusValue = false;
bool lastSelectValue = false;

void startExposure(){
    while(1);
    return;    
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
        //lastPressedSelect = millis();
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
        
        switch(select){
             case 0: h++; h %= 100; break;
             case 1: m++; m %=  60; break;
             case 2: s++; s %=  60; break;
        }
        refreshTime();
        lastPlusValue = true;
        //delay(PLUS_BUTTON_DELAY_MS);
        delay(DELAY_DEBOUNCE_MS);
        return true;
    }
    else if(plusButton && (lastPlusValue == true) && ((millis() - lastPressedPlus) > FAST_CHANGE_DELAY_MS)){
        Serial.println("plus hold");
        int iter = 0;
        while(true){
            bool plusButton = (digitalRead(pinPlus) == LOW);
            Serial.println(plusButton);
            switch(select){
                case 0: h++; h %= 100; break;
                case 1: m++; m %=  60; break;
                case 2: s++; s %=  60; break;
            }
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
        //Serial.println("minus pressed!");
        lastPressedMinus = millis();
        
        switch(select){
                case 0: h--; h = FIX_NEG(h, 100); break;
                case 1: m--; m = FIX_NEG(m, 60 ); break;
                case 2: s--; s = FIX_NEG(s, 60 ); break;
            }
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
            switch(select){
                case 0: h--; h = FIX_NEG(h, 100); break;
                case 1: m--; m = FIX_NEG(m, 60 ); break;
                case 2: s--; s = FIX_NEG(s, 60 ); break;
            }
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
    
    return false;
}

bool buttonPress = true;
unsigned long lastSelect = 0;

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
            //refreshTime();
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

