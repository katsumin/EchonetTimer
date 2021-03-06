#ifndef _HEADVIEW_H_
#define _HEADVIEW_H_
#include <NTPClient.h>

class HeadView
{
private:
    TFT_eSPI *_lcd;
    // IPアドレス
    IPAddress _ipaddr;
    // NW type
    String _nwType;
    // NTPClient
    NTPClient *_ntp;
    // font1高
    int16_t _font1Height;
    char _buf[32];
    void _printNwInfo()
    {
        getLcd()->setTextDatum(TL_DATUM);
        getLcd()->setTextFont(1);
        getLcd()->setTextSize(1);
        getLcd()->setTextColor(TFT_ORANGE);
        // snprintf(_buf, sizeof(_buf), "%s(%s)", _ipaddr.toString().c_str(), _nwType.c_str());
        snprintf(_buf, sizeof(_buf), "%s", _ipaddr.toString().c_str());
        int w = getLcd()->textWidth(_buf);
        int x_pos = 1;
        int y_pos = 1;
        // int y_pos = getLcd()->height() - _font1Height;
        getLcd()->fillRect(x_pos - 1, y_pos - 1, w, _font1Height + 2, TFT_BLACK);
        getLcd()->drawString(_buf, x_pos, y_pos);
    }
    void _printDate()
    {
        getLcd()->setTextDatum(TR_DATUM);
        getLcd()->setTextFont(1);
        getLcd()->setTextSize(1);
        getLcd()->setTextColor(TFT_ORANGE);
        time_t epoch = _ntp->getEpochTime();
        tm *t = localtime(&epoch);
        snprintf(_buf, sizeof(_buf), "%02d/%02d/%02d %02d:%02d:%02d", t->tm_year % 100, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
        int w = getLcd()->textWidth(_buf);
        int x_pos = getLcd()->width();
        int y_pos = getLcd()->height() - _font1Height;
        getLcd()->fillRect(x_pos - w, y_pos - 1, w, _font1Height + 2, TFT_BLACK);
        getLcd()->drawString(_buf, x_pos, y_pos);
    }

public:
    HeadView(TFT_eSPI *lcd)
    {
        _lcd = lcd;
    }
    inline TFT_eSPI *getLcd() { return _lcd; }
    inline void setIpAddress(IPAddress addr)
    {
        _ipaddr = addr;
        _printNwInfo();
    };
    inline void setNwType(const char *type)
    {
        _nwType = String(type);
        _printNwInfo();
    };
    inline void setNtp(NTPClient *ntp) { _ntp = ntp; };
    void init()
    {
        getLcd()->setTextFont(1);
        getLcd()->setTextSize(1);
        _font1Height = getLcd()->fontHeight();
    };
    void update()
    {
        _printNwInfo();
        _printDate();
    };
};

#endif