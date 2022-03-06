#ifndef _DEVICE_H_
#define _DEVICE_H_
#include <Arduino.h>
#include <map>
#include "NTPClient.h"
#include "EL.h"

#define BUF_SIZE 256
typedef struct
{
    u_char epc;    // 1 byte
    u_char pdc;    // 1 byte
    u_char edt[1]; // n byte (n > 1)
} ECHONET_DATA;

class Node;
class TimeTableValue;

class Device
{
private:
    byte _eoj[3];
    Node *_node;
    NTPClient *_ntp;
    String _alias;
    std::map<uint16_t, TimeTableValue *, std::greater<uint16_t>> _time_table;
    byte _cmd_buf[1 + 2 + 2 + 3 + 3 + 1 + 1 + 5 * 2] = {
        sizeof(_cmd_buf) - 1, // len
        0x10, 0x81,           // EHD
        0x00, 0x01,           // TID
        0x05, 0xff, 0x01,     // SEOJ
        0x01, 0x30, 0x01,     // DEOJ
        0x62,                 // ESV
        0x05,                 // OPC
        0x80, 0x00,           // EPC, PDC
        0x84, 0x00,           // EPC, PDC
        0xb0, 0x00,           // EPC, PDC
        0xbe, 0x00,           // EPC, PDC
        0xbb, 0x00,           // EPC, PDC
    };

protected:
    int16_t edtInt16_t(byte *edt) { return (int16_t)(edt[0] << 8 | edt[1]); }
    uint16_t edtUInt16_t(byte *edt) { return (uint16_t)(edt[0] << 8 | edt[1]); }
    int32_t edtInt32_t(byte *edt) { return (int32_t)(edt[0] << 24 | edt[1] << 16 | edt[2] << 8 | edt[3]); }
    uint32_t edtUInt32_t(byte *edt) { return (uint32_t)(edt[0] << 24 | edt[1] << 16 | edt[2] << 8 | edt[3]); }

public:
    Device(byte eoj0, byte eoj1, byte eoj2, Node *node);
    /**
     * byte[0]: length
     * byte[1]-byte[length]: payload
     */
    virtual byte *request() { return nullptr; };
    virtual void parse(const byte *props){};
    virtual byte *generateBuffer(const byte esv, const byte opc, byte *props, int propsLen);
    inline uint16_t getClassType() { return _eoj[0] << 8 | _eoj[1]; }
    inline Node *getNode() { return _node; }
    inline void setEoj(byte *buf, byte eoj0, byte eoj1, byte eoj2)
    {
        buf[0] = eoj0;
        buf[1] = eoj1;
        buf[2] = eoj2;
    }
    inline void setNtp(NTPClient *ntp) { _ntp = ntp; }
    inline NTPClient *getNtp() { return _ntp; }
    inline void setAlias(String alias) { _alias = alias; }
    inline String getAlias() { return _alias; }
    inline void setTimeTable(uint16_t time, TimeTableValue *action)
    {
        _time_table[time] = action;
    }
    inline std::map<uint16_t, TimeTableValue *, std::greater<uint16_t>> getTimeTable() { return _time_table; }
    inline void clearTimeTable() { _time_table.clear(); }
    static Device *createInstance(byte eoj0, byte eoj1, byte eoj2, Node *node);
};

// 家庭用エアコン（0x0130）
class Aircon : public Device
{
private:
    uint16_t _power = 0xffff;
    int8_t _tempRoom = 0x7e;
    int8_t _tempOut = 0x7e;
    uint8_t _on = 0xff;
    uint8_t _mode = 0x40;
    std::map<uint8_t, char *> _map;

public:
    Aircon(byte eoj0, byte eoj1, byte eoj2, Node *node);
    inline char *getOn() { return _on == 0x30 ? (char *)"　ＯＮ" : _on == 0x31 ? (char *)"ＯＦＦ"
                                                                               : (char *)"－－－"; }
    inline uint16_t getPower() { return _power; }
    inline int8_t getTempRoom() { return _tempRoom; }
    inline int8_t getTempOut() { return _tempOut; }
    inline String getMode() { return _map[_mode]; }
    virtual byte *request();
    virtual void parse(const byte *props);
};

// 蓄電池（0x027d）
class Battery : public Device
{
private:
    uint8_t _state = 0x40;
    uint8_t _mode = 0x40;
    uint8_t _percent = 0xff;
    std::map<uint8_t, char *> _map;

public:
    Battery(byte eoj0, byte eoj1, byte eoj2, Node *node);
    inline String getState() { return _map[_state]; }
    inline String getMode() { return _map[_mode]; }
    inline uint8_t getPercent() { return _percent; }
    virtual byte *request();
    virtual void parse(const byte *props);
};

#endif