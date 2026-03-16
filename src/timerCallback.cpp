#include <Arduino.h>
#include <timerCallback.h>

timerCallback::timerCallback(void (*callbackFunc)(), unsigned long interval)
{
    this->interval = interval;
    this->callback = callbackFunc;
    this->previousTime = millis();
}

void timerCallback::loop()
{
    if (callback == nullptr) return;
        
    unsigned long currentTime = millis();
    
    if (currentTime - previousTime >= interval) 
    {
        previousTime = currentTime;
        callback();
    }
}