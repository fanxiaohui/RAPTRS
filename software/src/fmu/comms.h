/*
comms.h
Brian R Taylor
brian.taylor@bolderflight.com

Copyright (c) 2018 Bolder Flight Systems
Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#ifndef COMMS_H_
#define COMMS_H_

#include "mission.h"
#include "SerialLink.h"
#include "Vector.h"
#include "i2c_t3.h"
#include "Arduino.h"

class AircraftSocComms {
  public:
    enum Message {
      ModeCommand,
      Configuration,
      SensorData,
      EffectorCommand
    };
    AircraftSocComms(HardwareSerial& bus,uint32_t baud);
    void Begin();
    void SendSensorData(std::vector<uint8_t> &DataBuffer);
    bool ReceiveModeCommand(AircraftMission::Mode *mode);
    bool ReceiveConfigMessage(std::vector<char> *ConfigString);
    bool ReceiveEffectorCommand(std::vector<float> *EffectorCommands);
    void CheckMessages();
    void SendMessage(Message message,std::vector<uint8_t> &Payload);
    bool ReceiveMessage(Message *message,std::vector<uint8_t> *Payload);
  private:
    SerialLink *bus_;
    uint32_t baud_;
    bool MessageReceived_ = false;
    Message ReceivedMessage_;
    std::vector<uint8_t> ReceivedPayload_;
};

#endif
