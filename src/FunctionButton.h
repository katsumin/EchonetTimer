#ifndef _FUNC_BTN_H_
#define _FUNC_BTN_H_
#define LGFX_M5STACK
#include <LGFX_TFT_eSPI.hpp>
#include <utility/Button.h>

#define TEXT_HEIGHT (12)
#define WIDTH (60 / 2)
#define POS_A_X (36 / 2)
#define POS_B_X (90)
#define POS_C_X (160 - WIDTH)

#define BUTTON_A_PIN 37
#define BUTTON_B_PIN 39

class FunctionButton
{
private:
    TFT_eSPI *_lcd;
    Button *_button;
    char *_label;
    void _set(const char *label, int color);
    boolean _enable = false;
    uint16_t _xpos;

public:
    FunctionButton(Button *button, TFT_eSPI *lcd, uint16_t xpos);
    FunctionButton(uint8_t pin, TFT_eSPI *lcd, uint16_t xpos);
    inline TFT_eSPI *getLcd() { return _lcd; }
    inline char *getLabel() { return _label; };
    inline Button *getButton()
    {
        return _button;
    };
    inline void setButton(Button *button)
    {
        _button = button;
    }
    inline void enable(const char *label)
    {
        _set(label, TFT_WHITE);
        _enable = true;
    };
    inline void disable(const char *label)
    {
        _set(label, TFT_MAGENTA);
        _enable = false;
    };
    inline boolean isEnable() { return _enable; };
    uint8_t wasPressed() { return _button->wasPressed(); };
    uint8_t wasReleased() { return _button->wasReleased(); };
    uint8_t isPressed() { return _button->isPressed(); };
    uint8_t isReleased() { return _button->isReleased(); };
    inline uint8_t read() { return _button->read(); };
};

#endif
