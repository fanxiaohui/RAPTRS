/*
main.cpp
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

#include "WProgram.h"
#include "hardware-defs.h"
#include "comms.h"
#include "mission.h"
#include "configuration.h"
#include "sensors.h"
#include "control.h"
#include "effector.h"
#include "utils.h"
#include "definition-tree.h"

// global aircraft data tree
DefinitionTree GlobalData;
// class for communicating with SOC
AircraftSocComms SocComms(kSocUart,kSocBaud);
// class for aircraft level configs (i.e. address, FMU orientation in vehicle, etc)
AircraftConfiguration Config;
// class for sensor configuration and data acquisition
AircraftSensors Sensors;
// class for mission management (modes, states, and reference commands)
AircraftMission Mission;
// class for control laws
ControlLaws Control;
// class for effectors
AircraftEffectors Effectors;
// current mode
AircraftMission::Mode MissionMode;
// current state
AircraftMission::State MissionState;
// requested mode
AircraftMission::Mode RequestedMode;
// effector commands
std::vector<float> EffectorCommands;

// runs with the FMU integrated IMU data ready interrupt
void ImuInterrupt() {
  // set the IMU data ready flag
  Mission.SetImuDataReady();
}

int main() 
{
  // serial port for debug messages
  Serial.begin(kDebugBaud);
  delay(5000);
  Serial.println("Bolder Flight Systems");
  Serial.print("Flight Management Unit Software Version ");
  Serial.println(SoftwareVersion);
  Serial.println();
  // communication with SOC
  SocComms.Begin();
  // load configuration
  Config.Load();
  // attach IMU interrupt
  pinMode(kMpu9250IntPin,INPUT);
  attachInterrupt(kMpu9250IntPin,ImuInterrupt,RISING);
  // set BFS pins to output
  pinMode(kBfsInt1Pin,OUTPUT);
  pinMode(kBfsInt2Pin,OUTPUT);

  while (1) {
    // update the mission mode
    Mission.UpdateMode(&Sensors,&Control,&Effectors,&GlobalData);
    Mission.GetMode(&MissionMode);
    if (MissionMode == AircraftMission::Run) {
      // update the mission state
      Mission.UpdateState();
      Mission.GetState(&MissionState);
      if (MissionState == AircraftMission::SyncDataCollection) {
        Mission.ClearImuDataReady();
        // read synchronous sensors
        Sensors.ReadSyncSensors();
        // buffer for transmitting data
        std::vector<uint8_t> DataBuffer;
        Sensors.GetDataBuffer(&DataBuffer);
        // transmit data to SOC
        SocComms.SendSensorData(DataBuffer);
      }
      if (MissionState == AircraftMission::AsyncDataCollection) {
        // read the asynchronous sensors
        Sensors.ReadAsyncSensors();
      }
      if (MissionState == AircraftMission::FlightControl) {
        Mission.ClearFlightControlFlag();
        // run control laws
        for (size_t ControlLevelIndex=0; ControlLevelIndex < Control.ActiveControlLevels(); ControlLevelIndex++) {
          Control.Run(ControlLevelIndex);
        }
        // compute effector PWM and SBUS commands from angles
        Effectors.ComputeOutputs(Mission.ThrottleSafed());
      }
      if (MissionState == AircraftMission::EffectorOutput) {
        Mission.ClearEffectorOutputFlag();
        // command the effectors to move
        Effectors.CommandEffectors();
      }
      // effector commands from SOC
      if (SocComms.ReceiveEffectorCommand(&EffectorCommands)) {
        if (Mission.UseSocEffectorComands()) {
          // set the received commands to be used
          Effectors.SetCommands(EffectorCommands,Mission.ThrottleSafed());
        }
      }
    }
    if (MissionMode == AircraftMission::Configuration) {
      // buffer for receiving configurations
      std::vector<char> ConfigBuffer;
      // update configuration
      if (SocComms.ReceiveConfigMessage(&ConfigBuffer)) {
        Config.Update(ConfigBuffer.data(),&Mission,&Sensors,&Control,&Effectors,&GlobalData);
      }
    }
    // request mode
    if (SocComms.ReceiveModeCommand(&RequestedMode)) {
      Mission.SetRequestedMode(RequestedMode);
    }
    // check for new messages from SOC
    SocComms.CheckMessages();
  }
}
