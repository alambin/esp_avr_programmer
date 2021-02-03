#include "Stk500.h"

Stk500::Stk500(Stream* serial, int resetPin, Print* log)
  : _resetPin(resetPin)
  , _serial(serial)
  , _log(log)
{
    pinMode(resetPin, OUTPUT);
    digitalWrite(_resetPin, HIGH);
}


bool
Stk500::setupDevice()
{
    resetMCU();
    int s = getSync();
    if (_log)
        _log->printf("avrflash: sync=d%d/0x%x\n", s, s);
    if (!s)
        return false;
    s = setProgParams();
    if (_log)
        _log->printf("avrflash: setparam=d%d/0x%x\n", s, s);
    if (!s)
        return false;
    s = setExtProgParams();
    if (_log)
        _log->printf("avrflash: setext=d%d/0x%x\n", s, s);
    if (!s)
        return false;
    s = enterProgMode();
    if (_log)
        _log->printf("avrflash: progmode=d%d/0x%x\n", s, s);
    if (!s)
        return false;
    return true;
}

bool
Stk500::flashPage(byte* address, byte* data)
{
    byte header[] = {0x64, 0x00, 0x80, 0x46};
    int  s        = loadAddress(address[1], address[0]);
    if (_log)
        _log->printf("avrflash: loadAddr(%d,%d)=%d\n", address[1], address[0], s);

    _serial->write(header, 4);
    for (int i = 0; i < 128; i++)
        _serial->write(data[i]);
    _serial->write(0x20);

    s = waitForSerialData(2, 1000);
    if (s == 0 && _log) {
        _log->printf("avrflash: flashpage: ack: error\n");
        return false;
    }
    s     = _serial->read();
    int t = _serial->read();
    if (_log)
        _log->printf("avrflash: flashpage: ack: d%d/d%d - 0x%x/0x%x\n", s, t, s, t);

    return true;
}

void
Stk500::resetMCU()
{
    digitalWrite(_resetPin, LOW);
    delay(1);
    digitalWrite(_resetPin, HIGH);
    delay(200);
}

int
Stk500::getSync()
{
    return execCmd(0x30);
}

int
Stk500::enterProgMode()
{
    return execCmd(0x50);
}

int
Stk500::exitProgMode()
{
    return execCmd(0x51);
}

int
Stk500::setExtProgParams()
{
    byte params[] = {0x05, 0x04, 0xd7, 0xc2, 0x00};
    return execParam(0x45, params, sizeof(params));
}

int
Stk500::setProgParams()
{
    byte params[] = {0x86, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0xff, 0xff,
                     0xff, 0xff, 0x00, 0x80, 0x04, 0x00, 0x00, 0x00, 0x80, 0x00};
    return execParam(0x42, params, sizeof(params));
}

int
Stk500::loadAddress(byte adrHi, byte adrLo)
{
    byte params[] = {adrHi, adrLo};
    return execParam(0x55, params, sizeof(params));
}

byte
Stk500::execCmd(byte cmd)
{
    byte bytes[] = {cmd, 0x20};
    return sendBytes(bytes, 2);
}

byte
Stk500::execParam(byte cmd, byte* params, int count)
{
    byte bytes[count + 2];
    bytes[0] = cmd;

    int i = 0;
    while (i < count) {
        bytes[i + 1] = params[i];
        i++;
    }

    bytes[i + 1] = 0x20;

    return sendBytes(bytes, i + 2);
}

byte
Stk500::sendBytes(byte* bytes, int count)
{
    _serial->write(bytes, count);
    waitForSerialData(2, 1000);

    byte sync = _serial->read();
    byte ok   = _serial->read();
    if (sync == 0x14 && ok == 0x10) {
        return 1;
    }

    return 0;
}

int
Stk500::waitForSerialData(int dataCount, int timeout)
{
    int timer = 0;

    while (timer < timeout) {
        if (_serial->available() >= dataCount) {
            return 1;
        }
        delay(1);
        timer++;
    }

    return 0;
}
