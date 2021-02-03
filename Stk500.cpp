#include "Stk500.h"

Stk500::Stk500(Stream* serial, int resetPin, Print* log)
  : resetPin_(resetPin)
  , serial_(serial)
  , log_(log)
{
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin_, HIGH);
}


bool
Stk500::setupDevice()
{
    resetMCU();
    int s = getSync();
    if (log_)
        log_->printf("avrflash: sync=d%d/0x%x\n", s, s);
    if (!s)
        return false;
    s = setProgParams();
    if (log_)
        log_->printf("avrflash: setparam=d%d/0x%x\n", s, s);
    if (!s)
        return false;
    s = setExtProgParams();
    if (log_)
        log_->printf("avrflash: setext=d%d/0x%x\n", s, s);
    if (!s)
        return false;
    s = enterProgMode();
    if (log_)
        log_->printf("avrflash: progmode=d%d/0x%x\n", s, s);
    if (!s)
        return false;
    return true;
}

bool
Stk500::flashPage(byte* address, byte* data)
{
    byte header[] = {0x64, 0x00, 0x80, 0x46};
    int  s        = loadAddress(address[1], address[0]);
    if (log_)
        log_->printf("avrflash: loadAddr(%d,%d)=%d\n", address[1], address[0], s);

    serial_->write(header, 4);
    for (int i = 0; i < 128; i++)
        serial_->write(data[i]);
    serial_->write(0x20);

    s = waitForSerialData(2, 1000);
    if (s == 0 && log_) {
        log_->printf("avrflash: flashpage: ack: error\n");
        return false;
    }
    s     = serial_->read();
    int t = serial_->read();
    if (log_)
        log_->printf("avrflash: flashpage: ack: d%d/d%d - 0x%x/0x%x\n", s, t, s, t);

    return true;
}

void
Stk500::resetMCU()
{
    digitalWrite(resetPin_, LOW);
    delay(1);
    digitalWrite(resetPin_, HIGH);
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
    serial_->write(bytes, count);
    waitForSerialData(2, 1000);

    byte sync = serial_->read();
    byte ok   = serial_->read();
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
        if (serial_->available() >= dataCount) {
            return 1;
        }
        delay(1);
        timer++;
    }

    return 0;
}
