#include "Node.h"
#include "DataStore.h"
#include <ArduinoJson.h>

Node::Node(IPAddress addr, DataStore *ds)
{
    _addr = addr;
    _id = addr.toString();
    _dataStore = ds;
}

void Node::deviceRequest(Device *device)
{
    byte *data = device->request();
    int len = data[0];
    _echo->send(_addr, &data[1], len);
#ifdef DEBUG
    Serial.printf("request packet(%s): ", _addr.toString().c_str());
    for (int i = 1; i <= len; i++)
        Serial.printf("%02x ", data[i]);
    Serial.println(".");
#endif
}

void Node::request()
{
    for (auto itr = _devices.begin(); itr != _devices.end(); itr++)
    {
        deviceRequest(itr->second);
    }
}

extern String preferenceRead(const char *key);
void Node::parse(const byte *props, const byte *seoj)
{
    ECHONET_DATA *ed = (ECHONET_DATA *)props;
    uint16_t ct = classType(seoj[0], seoj[1]);
    if (ct == 0x0ef0)
    {
        switch (ed->epc)
        {
        case 0x83:
            // 識別番号
            {
                int l = ed->pdc;
                setId(hexString(ed->edt, l));
                for (auto itr = _devices.begin(); itr != _devices.end(); itr++)
                {
                    itr->second->setAlias(getId());
                }

                // nvs設定を参照
                String setting = preferenceRead(_id.c_str());
                if (!setting.isEmpty())
                {
                    // nvs設定をデバイスに反映
                    Serial.printf("nvs: %s\n", setting.c_str());
                    reflectSettings(setting);
                }
                else
                {
                    Serial.printf("%s is empty\n", _id.c_str());
                    initializeSettings();
                }
            }
            break;
        case 0xd5:
        case 0xd6:
            // インスタンスリスト
            {
                int c = ed->edt[0];
                for (int i = 0; i < c; i++)
                {
                    uint32_t key = deviceId(ed->edt[i * 3 + 1], ed->edt[i * 3 + 2], ed->edt[i * 3 + 3]);
                    Serial.printf("   %02x%02x%02x", ed->edt[i * 3 + 1], ed->edt[i * 3 + 2], ed->edt[i * 3 + 3]);
                    if (_devices.count(key) == 0)
                    {
                        Device *d = Device::createInstance(ed->edt[i * 3 + 1], ed->edt[i * 3 + 2], ed->edt[i * 3 + 3], this);
                        if (d != nullptr)
                        {
                            Serial.print(" ->created.");
                            _devices[key] = d;
                            ViewController *vc = _dataStore->getViewController();
                            d->setNtp(vc->getNtp());
                            d->setAlias(getId());
                            d->request();
                            DeviceView *dv = DeviceView::createView(d, vc->getLcd());
                            if (dv != nullptr)
                            {
                                Serial.printf(" ->view created. :%s", dv->getName());
                                vc->setView(dv->getName(), dv);
                            }
                        }
                    }
                    Serial.println();
                }
            }
            break;
        default:
            break;
        }
    }
    else
    {
        uint32_t key = deviceId(seoj[0], seoj[1], seoj[2]);
        if (_devices.count(key) != 0)
        {
            _devices[key]->parse(props);
        }
    }
}

void Node::reflectSettings(String settings)
{
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, settings);
    if (!error)
    {
        uint32_t type = strtoul((const char *)doc["type"], NULL, 16);
        if (_devices.count(type) > 0)
        {
            _devices[type]->setAlias(doc["alias"]);
            JsonArray table = doc["table"];
            Serial.printf("type: %04x, doc.size: %d\n", type, table.size());
            _devices[type]->clearTimeTable();
            for (int i = 0; i < table.size(); i++)
            {
                String t = String((const char *)table[i]["time"]);
                int pos = t.indexOf(':');
                uint16_t time = t.substring(0, pos).toInt() * 60 + t.substring(pos + 1).toInt();
                uint16_t value = strtoul((const char *)table[i]["value"], NULL, 16);
                uint8_t upper = 0;
                const char *pUpper = (const char *)table[i]["upper"];
                if (pUpper != NULL && strlen(pUpper) > 0)
                {
                    upper = strtoul(pUpper, NULL, 10);
                }
                TimeTableValue *v = new TimeTableValue(value, upper);
                _devices[type]->setTimeTable(time, v);
                Serial.printf("%04d -> %04x\n", time, value);
            }
        }
    }
}

#define PREFERENCES_KEYS ("keys")
extern void preferenceWrite(const char *key, String value);
void Node::initializeSettings()
{
    String id = getId();
    for (auto itr = _devices.begin(); itr != _devices.end(); itr++)
    {
        uint32_t type = itr->first;
        if (type == 0x27d01 || type == 0x013001)
        {
            String keys = preferenceRead(PREFERENCES_KEYS);
            Serial.printf("before keys: %s\n", keys.c_str());
            if (keys.indexOf(id) < 0)
            {
                // not contain
                if (keys.isEmpty())
                    keys = id;
                else
                    keys += "," + id;
                preferenceWrite(PREFERENCES_KEYS, keys);
            }
            Serial.printf("after keys: %s\n", keys.c_str());

            char buf[256];
            snprintf(buf, sizeof(buf), "{\"id\":\"%s\",\"alias\":\"%s\",\"type\":\"%06x\",\"table\": []}", id.c_str(), id.c_str(), type);
            String setting = String(buf);
            Serial.printf("initialize: %s\n", setting.c_str());
            preferenceWrite(id.c_str(), setting);
        }
    }
}