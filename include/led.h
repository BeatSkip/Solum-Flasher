#ifndef RP2040_WS2812B_H
#define RP2040_WS2812B_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>


enum Color {
        RED,
        GREEN,
        BLUE,
        YELLOW,
        MAGENTA,
        CYAN,
        WHITE,
        BLACK
    };

class StatusLed {
public:
    

    StatusLed(uint16_t n, uint8_t pin) : pixels(n, pin, NEO_GRB + NEO_KHZ800) {}

    void begin() {
        pixels.begin();
    }

    void set(Color color) {
		int pixel = 0;
        currentColor = color;
        switch (color) {
            case RED:     pixels.setPixelColor(pixel, pixels.Color(255, 0, 0)); break;
            case GREEN:   pixels.setPixelColor(pixel, pixels.Color(0, 255, 0)); break;
            case BLUE:    pixels.setPixelColor(pixel, pixels.Color(0, 0, 255)); break;
            case YELLOW:  pixels.setPixelColor(pixel, pixels.Color(255, 255, 0)); break;
            case MAGENTA: pixels.setPixelColor(pixel, pixels.Color(255, 0, 255)); break;
            case CYAN:    pixels.setPixelColor(pixel, pixels.Color(0, 255, 255)); break;
            case WHITE:   pixels.setPixelColor(pixel, pixels.Color(255, 255, 255)); break;
            case BLACK:   pixels.setPixelColor(pixel, pixels.Color(0, 0, 0)); break;
        }
		pixels.show();
    }

    void setBrightness(uint8_t brightness) {
        pixels.setBrightness(brightness);
    }

    void Toggle() {
        if (isOn) {
            set(BLACK);
            isOn = false;
        } else {
            set(currentColor);
            isOn = true;
        }
    }

    void setSilent(Color color){
        currentColor = color;
    }

private:
    Adafruit_NeoPixel pixels;
    bool isOn = false;
    Color currentColor = BLACK;
};

#endif // RP2040_WS2812B_H