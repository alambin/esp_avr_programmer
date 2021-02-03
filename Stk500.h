#ifndef STK500_H
#define STK500_H

#include <Arduino.h>

class Stk500
{
public:
    Stk500(Stream* serial, int resPin, Print* log = nullptr);
    bool setupDevice();
    bool flashPage(byte* loadAddress, byte* data);
    int  exitProgMode();

private:
    void resetMCU();
    int  getSync();
    int  enterProgMode();
    int  loadAddress(byte adrHi, byte adrLo);
    int  setProgParams();
    int  setExtProgParams();

    byte execCmd(byte cmd);
    byte execParam(byte cmd, byte* params, int count);
    byte sendBytes(byte* bytes, int count);
    int  waitForSerialData(int dataCount, int timeout);
    int  getFlashPageCount(byte flashData[][131]);

    int     _resetPin;
    Stream* _serial;
    Print*  _log;
};

#endif  // STK500_H
