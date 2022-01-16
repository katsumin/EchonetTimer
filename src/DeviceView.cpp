#include "DeviceView.h"
#include "Node.h"

DeviceView *DeviceView::createView(Device *device, TFT_eSPI *lcd)
{
    uint16_t type = device->getClassType();
    if (type == 0x0130)
    {
        return new AirconView((Aircon *)device, lcd);
    }
    else if (type == 0x027d)
    {
        return new BatteryView((Battery *)device, lcd);
    }
    return nullptr;
}

//
DeviceView::DeviceView(Device *device, TFT_eSPI *lcd) : View(lcd)
{
    setDevice(device);
}
void DeviceView::init()
{
    getLcd()->setFont(&fonts::lgfxJapanGothic_12);
    int16_t fh = getLcd()->fontHeight();
    int32_t y = 13;
    _baseY = y + fh + 4;
    getLcd()->fillRect(0, y, getLcd()->width(), getLcd()->height() - y * 2 + 1, TFT_BLACK); // ヘッダ以外を消去
    getLcd()->drawFastHLine(0, _baseY - 2, getLcd()->width());
}

//
int AirconView::_count = 0;
AirconView::AirconView(Aircon *device, TFT_eSPI *lcd) : DeviceView(device, lcd)
{
    const int len = 3 + 1 + 1; // "Air" + "x" + '\0'
    char *buf = (char *)malloc(len);
    snprintf(buf, len, "Air%01d", _count++);
    setName(buf);
}
void AirconView::init()
{
    DeviceView::init();
    getLcd()->setFont(&fonts::lgfxJapanGothic_12);
    getLcd()->setTextColor(TFT_WHITE);

    int x = 2;
    int y = 14;
    getLcd()->setTextDatum(TL_DATUM);
    getLcd()->drawString("エアコン", x, y);

    getLcd()->setFont(&fonts::lgfxJapanGothic_12);
    _fontHeight = getLcd()->fontHeight();
    getLcd()->setTextDatum(TL_DATUM);
    x = 2;
    y = _baseY;
    getLcd()->drawString("動作状態", x, y);
    y += _fontHeight;
    getLcd()->drawString("動作モード", x, y);
    _fontWidth = getLcd()->textWidth("0");
}
void AirconView::update()
{
    char buf[64];
    Aircon *ac = (Aircon *)getDevice();

    getLcd()->setFont(&fonts::lgfxJapanGothic_12);
    getLcd()->setTextColor(TFT_YELLOW);
    getLcd()->setTextDatum(TR_DATUM);

    int x = getLcd()->width() - 2;

    int backColor = TFT_BLACK;
    int w = _fontWidth * 17 * 2;

    int y = 14;
    String id = "                " + ac->getAlias();
    int pos = (id.length() > 16) ? id.length() - 16 : 0; // 16文字以下
    id = id.substring(pos);
    w = getLcd()->textWidth(id);
    getLcd()->fillRect(x - w, y, w, _fontHeight, backColor);
    getLcd()->drawString(id, x, y);

    y = _baseY;
    w = getLcd()->textWidth(ac->getOn());
    getLcd()->fillRect(x - w, y, w, _fontHeight, backColor);
    getLcd()->drawString(ac->getOn(), x, y);

    w = getLcd()->textWidth(ac->getMode());
    y += _fontHeight;
    getLcd()->fillRect(x - w, y, w, _fontHeight, backColor);
    getLcd()->drawString(ac->getMode(), x, y);
}

//
BatteryView::BatteryView(Battery *device, TFT_eSPI *lcd) : DeviceView(device, lcd)
{
    setName("Battery");
}
void BatteryView::init()
{
    DeviceView::init();
    getLcd()->setFont(&fonts::lgfxJapanGothic_12);
    getLcd()->setTextColor(TFT_WHITE);

    int x = 2;
    int y = 14;
    getLcd()->setTextDatum(TL_DATUM);
    getLcd()->drawString("蓄電池", x, y);

    getLcd()->setFont(&fonts::lgfxJapanGothic_12);
    _fontHeight = getLcd()->fontHeight();
    getLcd()->setTextDatum(TL_DATUM);
    x = 2;
    y = _baseY;
    getLcd()->drawString("動作状態", x, y);
    y += _fontHeight;
    getLcd()->drawString("動作モード", x, y);
    y += _fontHeight;
    getLcd()->drawString("蓄電残量", x, y);

    _fontWidth = getLcd()->textWidth("0");
}
void BatteryView::update()
{
    char buf[64];
    Battery *batt = (Battery *)getDevice();

    getLcd()->setFont(&fonts::lgfxJapanGothic_12);
    getLcd()->setTextColor(TFT_YELLOW);
    getLcd()->setTextDatum(TR_DATUM);

    int x = getLcd()->width() - 2;

    int backColor = TFT_BLACK;
    int w = _fontWidth * 17 * 2;
    int y = 14;
    String id = "                    " + batt->getAlias();
    int pos = (id.length() > 20) ? id.length() - 20 : 0; // 20文字以下
    id = id.substring(pos);
    w = getLcd()->textWidth(id);
    getLcd()->fillRect(x - w, y, w, _fontHeight, backColor);
    getLcd()->drawString(id, x, y);

    y = _baseY;
    w = getLcd()->textWidth(batt->getState());
    getLcd()->fillRect(x - w, y, w, _fontHeight, backColor);
    getLcd()->drawString(batt->getState(), x, y);

    y += _fontHeight;
    w = getLcd()->textWidth(batt->getMode());
    getLcd()->fillRect(x - w, y, w, _fontHeight, backColor);
    getLcd()->drawString(batt->getMode(), x, y);

    y += _fontHeight;
    snprintf(buf, sizeof(buf), "%3d%%", batt->getPercent());
    w = getLcd()->textWidth(buf);
    getLcd()->fillRect(x - w, y, w, _fontHeight, backColor);
    getLcd()->drawString(buf, x, y);
}
