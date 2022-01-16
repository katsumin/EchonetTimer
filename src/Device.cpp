#include "Device.h"
#include "Node.h"

Device *Device::createInstance(byte eoj0, byte eoj1, byte eoj2, Node *node)
{
    // 制御対象＝エアコン、蓄電池
    if (eoj0 == 0x01 && eoj1 == 0x30)
        return new Aircon(eoj0, eoj1, eoj2, node);
    else if (eoj0 == 0x02 && eoj1 == 0x7d)
        return new Battery(eoj0, eoj1, eoj2, node);
    else
        return nullptr;
}

// base
Device::Device(byte eoj0, byte eoj1, byte eoj2, Node *node)
{
    setEoj(_eoj, eoj0, eoj1, eoj2);
    _node = node;
}

byte *Device::generateBuffer(const byte esv, const byte opc, byte *props, int propsLen)
{
    memcpy(&_cmd_buf[EL_DEOJ + 1], _eoj, sizeof(_eoj)); // DEOJ
    _cmd_buf[EL_ESV + 1] = esv;                         // ESV
    _cmd_buf[EL_OPC + 1] = opc;                         // OPC
    memcpy(&_cmd_buf[EL_EPC + 1], props, propsLen);     // EPC,PDC,EDT
    _cmd_buf[0] = propsLen + EL_EPC;                    // length
    return _cmd_buf;
}

// Aircon
Aircon::Aircon(byte eoj0, byte eoj1, byte eoj2, Node *node) : Device(eoj0, eoj1, eoj2, node)
{
    // 動作モード
    _map[0x40] = "その他";
    _map[0x41] = "　自動";
    _map[0x42] = "　冷房";
    _map[0x43] = "　暖房";
    _map[0x44] = "　除湿";
    _map[0x45] = "　送風";
}

byte *Aircon::request()
{
    if (_on == 0x30 || _on == 0x31)
    {
        unsigned long epoch = getNtp()->getEpochTime();
        uint16_t t = (uint16_t)((epoch / 60) % (24 * 60));
        for (const auto &p : getTimeTable())
        {
            // Serial.printf("%02d:%02d -> %04x\n", p.first / 60, p.first % 60, p.second);
            uint8_t toOnOff = p.second & 0xff;
            uint8_t epc = (p.second >> 8) & 0xff;
            Serial.printf(" %02x: %02x -> %02x\n", epc, _on, toOnOff);
            if (t >= p.first)
            {
                Serial.printf("%02d:%02d >= %02d:%02d", t / 60, t % 60, p.first / 60, p.first % 60);
                if (_on != toOnOff)
                {
                    // ON/OFF切替
                    Serial.printf(" %02x: %02x -> %02x\n", epc, _on, toOnOff);
                    byte setOnOff[] = {
                        epc, 0x01, toOnOff // EPC, PDC, EDT（ON/OFF）
                    };
                    return generateBuffer(EL_SETC, 1, (byte *)setOnOff, sizeof(setOnOff));
                }
                else
                    Serial.println();
                break;
            }
            else
                Serial.printf("%02d:%02d <  %02d:%02d\n", t / 60, t % 60, p.first / 60, p.first % 60);
        }
    }

    const byte getProps[] = {
        0x80, 0x00, // EPC, PDC
        0x84, 0x00, // EPC, PDC
        0xb0, 0x00, // EPC, PDC
        0xbe, 0x00, // EPC, PDC
        0xbb, 0x00, // EPC, PDC
    };
    return generateBuffer(EL_GET, 5, (byte *)getProps, sizeof(getProps));
}

void Aircon::parse(const byte *props)
{
    ECHONET_DATA *ed = (ECHONET_DATA *)props;
    switch (ed->epc)
    {
    case 0x80:
        _on = ed->edt[0];
#ifdef DEBUG
        Serial.printf("動作状態: %s", getOn());
        Serial.println();
#endif
        break;
    case 0xb0:
        if (_map.count(ed->edt[0]) > 0)
        {
            _mode = ed->edt[0];
#ifdef DEBUG
            Serial.printf("動作モード: %s", getMode().c_str());
            Serial.println();
#endif
        }
        break;
    case 0x84:
        if (ed->pdc == 2)
        {
            uint16_t d = edtInt16_t(ed->edt);
            if (d <= 65533)
            {
                _power = d;
#ifdef DEBUG
                Serial.printf("消費電力: %d[w]", _power);
                Serial.println();
#endif
            }
        }
        break;
    case 0xbe:
        if (ed->pdc == 1)
        {
            int8_t d = (int8_t)ed->edt[0];
            if (-127 <= d && d <= 125)
            {
                _tempOut = d;
#ifdef DEBUG
                Serial.printf("外温: %3d[℃]", _tempOut);
                Serial.println();
#endif
            }
        }
        break;
    case 0xbb:
        if (ed->pdc == 1)
        {
            int8_t d = (int8_t)ed->edt[0];
            if (-127 <= d && d <= 125)
            {
                _tempRoom = d;
#ifdef DEBUG
                Serial.printf("室温: %3d[℃]", _tempRoom);
                Serial.println();
#endif
            }
        }
        break;
    default:
        break;
    }
}

// Battery
Battery::Battery(byte eoj0, byte eoj1, byte eoj2, Node *node) : Device(eoj0, eoj1, eoj2, node)
{
    // 動作モード
    _map[0x40] = "　その他";
    _map[0x41] = "急速充電";
    _map[0x42] = "　　充電";
    _map[0x43] = "　　放電";
    _map[0x44] = "　　待機";
    _map[0x45] = "　テスト";
    _map[0x46] = "　　自動";
    _map[0x48] = "　再起動";
    _map[0x49] = "　再計算";
}

byte *Battery::request()
{
    // 時刻によって返すデータを変える
    // 分単位のキーと動作モード
    // 現在時刻から動作モードを算出、動作中のモードと違っていたらSETプロパティ
    if (_mode != 0x40)
    {
        unsigned long epoch = getNtp()->getEpochTime();
        uint16_t t = (uint16_t)((epoch / 60) % (24 * 60));
        for (const auto &p : getTimeTable())
        {
            // Serial.printf("%02d:%02d -> %04x\n", p.first / 60, p.first % 60, p.second);
            uint8_t toMode = p.second & 0xff;
            uint8_t epc = (p.second >> 8) & 0xff;
            Serial.printf(" %02x: %02x -> %02x\n", epc, _mode, toMode);
            if (t >= p.first)
            {
                Serial.printf("%02d:%02d >= %02d:%02d", t / 60, t % 60, p.first / 60, p.first % 60);
                if (_mode != toMode)
                {
                    // 動作モード切替
                    Serial.printf(" %02x: %02x -> %02x\n", epc, _mode, toMode);
                    byte setMode[] = {
                        epc, 0x01, toMode // EPC, PDC, EDT（動作モード）
                    };
                    return generateBuffer(EL_SETC, 1, (byte *)setMode, sizeof(setMode));
                }
                else
                    Serial.println();
                break;
            }
            else
                Serial.printf("%02d:%02d <  %02d:%02d\n", t / 60, t % 60, p.first / 60, p.first % 60);
        }
    }

    const byte getProps[] = {
        0xcf, 0x00, // EPC, PDC（運転動作状態）
        0xda, 0x00, // EPC, PDC（運転動作モード）
        0xe4, 0x00, // EPC, PDC（蓄電残量３）
    };
    return generateBuffer(EL_GET, 3, (byte *)getProps, sizeof(getProps));
}

void Battery::parse(const byte *props)
{
    ECHONET_DATA *ed = (ECHONET_DATA *)props;
    switch (ed->epc)
    {
    case 0xcf: // 運転動作状態
        if (_map.count(ed->edt[0]) > 0)
        {
            _state = ed->edt[0];
#ifdef DEBUG
            Serial.printf("動作状態: %s", getState().c_str());
            Serial.println();
#endif
        }
        break;
    case 0xda: // 運転モード
        if (_map.count(ed->edt[0]) > 0)
        {
            _mode = ed->edt[0];
#ifdef DEBUG
            Serial.printf("動作モード: %s", getMode().c_str());
            Serial.println();
#endif
        }
        break;
    case 0xe4: // 蓄電残量
        if (ed->edt[0] <= 100)
        {
            _percent = ed->edt[0];
#ifdef DEBUG
            Serial.printf("蓄電残量: %3d[%%]", getPercent());
            Serial.println();
#endif
        }
        break;
    default:
        break;
    }
}
