#include "FunctionButton.h"
#define DEBOUNCE_MS 10

FunctionButton::FunctionButton(Button *button, TFT_eSPI *lcd, uint16_t xpos)
{
    _button = button;
    _lcd = lcd;
    _xpos = xpos;
}
FunctionButton::FunctionButton(uint8_t pin, TFT_eSPI *lcd, uint16_t xpos)
{
    _button = new Button(pin, true, DEBOUNCE_MS);
    _lcd = lcd;
    _xpos = xpos;
}

void FunctionButton::_set(const char *label, int color)
{
    _label = (char *)label;
    int32_t x_pos = _xpos;
    // int32_t y_pos = getLcd()->height() - TEXT_HEIGHT;
    int32_t y_pos = 0;
    getLcd()->setTextDatum(TL_DATUM);
    getLcd()->setTextFont(1);
    getLcd()->setTextSize(1);
    getLcd()->fillRect(x_pos, y_pos, WIDTH, TEXT_HEIGHT, TFT_BLACK);
    getLcd()->setTextColor(color);
    int32_t x = x_pos + WIDTH / 2 - getLcd()->textWidth(label) / 2;
    int32_t y = y_pos + 3;
    getLcd()->drawString(label, x, y);
    getLcd()->drawRoundRect(x_pos, y_pos, WIDTH, TEXT_HEIGHT, 2, TFT_WHITE);
}
