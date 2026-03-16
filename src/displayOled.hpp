#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

class DisplayOled
{
private:
    int _pinSda, _pinScl;
    U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2;

public:
    DisplayOled(int pinSda, int pinScl) 
        : _pinSda(pinSda), 
          _pinScl(pinScl),
          u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ pinScl, /* data=*/ pinSda)
    {}

    void begin()
    {
        Wire.begin(_pinSda, _pinScl);

        if (!u8g2.begin())
        {
            Serial.println("OLED не найден! Проверьте подключение I2C.");
            while (1);
        }
    }

    void print(const char *ssid, const char *password, int currentTemp)
    {
        u8g2.clearBuffer();

        u8g2.drawFrame(0, 0, 72, 40);

        u8g2.setFont(u8g2_font_6x10_tf);

        u8g2.setCursor(5, 12);
        u8g2.print(ssid);
        
        u8g2.setCursor(5, 22);
        u8g2.print(password);

        u8g2.setCursor(5, 32);
        u8g2.print("Temp: ");
        u8g2.print(currentTemp);
        u8g2.print("C");

        u8g2.sendBuffer();
    }
};