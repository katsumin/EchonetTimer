#ifndef _NODE_H_
#define _NODE_H_
#include <Arduino.h>
#include <map>
#include "EL.h"
#include "DeviceView.h"

class DataStore;

class Node
{
private:
    EL *_echo;
    String _id = "";
    IPAddress _addr;
    DataStore *_dataStore;
    std::map<uint32_t, Device *> _devices;

private:
    inline uint32_t deviceId(byte eoj0, byte eoj1, byte eoj2) { return eoj0 << 16 | eoj1 << 8 | eoj2; }
    inline uint16_t classType(byte eoj0, byte eoj1) { return eoj0 << 8 | eoj1; }
    char toCode(byte d)
    {
        if (d > 9)
            return d - 10 + 'a';
        else
            return d + '0';
    }
    String hexString(const byte *edt, int len)
    {
        char *buf = new char[len * 2 + 1];
        buf[len * 2] = '\0';
        for (int i = 0; i < len; i++)
        {
            buf[i * 2 + 0] = toCode((edt[i] >> 4) & 0x0f);
            buf[i * 2 + 1] = toCode(edt[i] & 0x0f);
        }
        return String(buf);
    }

public:
    Node(IPAddress addr, DataStore *ds);
    inline String getId() { return _id; }
    inline void setId(String id) { _id = id; }
    inline IPAddress getAddress() { return _addr; }
    inline std::map<uint32_t, Device *> *getDevices() { return &_devices; }
    inline void setEchonet(EL *el) { _echo = el; }
    void deviceRequest(Device *device);
    void request();
    void parse(const byte *props, const byte *seoj);
    void reflectSettings(String settings);
    void initializeSettings();
};

class TimeTableValue
{
private:
    uint8_t _epc;
    uint8_t _state;
    uint8_t _upper;

public:
    TimeTableValue(uint16_t state, uint8_t upper)
    {
        setEpc((state >> 8) & 0xff);
        setState(state & 0xff);
        setChargeUpper(upper);
    }
    inline uint8_t getEpc() const { return _epc; }
    inline void setEpc(uint8_t epc) { _epc = epc; }
    inline uint8_t getState() const { return _state; }
    inline void setState(uint8_t state) { _state = state; }
    inline uint8_t getChargeUpper() const { return _upper; }
    inline void setChargeUpper(uint8_t uppser) { _upper = uppser; }
};

#endif