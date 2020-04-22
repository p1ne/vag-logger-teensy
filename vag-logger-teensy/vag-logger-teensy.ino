#include "FlexCAN.h"
#include "SD.h"
#include <Metro.h>

Metro serialMetro = Metro(120);

FlexCAN Can0(500000,0,1,1);

String str;
File dataFile;

uint32_t time;

uint32_t lastDirectInjections = 0;
uint32_t lastMPIInjections = 0;
String currentInjectors = "";

uint16_t lastRPM = 0;

CAN_message_t msg;
CAN_message_t txmsg[20];

CAN_filter_t defaultMask;
CAN_filter_t defaultFilter;

uint8_t messagesNum = 0;
uint8_t currentMessage = 0;

void can_msg_set(uint8_t idx, uint32_t id, uint8_t len, uint8_t buf_0, uint8_t buf_1, uint8_t buf_2, uint8_t buf_3, uint8_t buf_4, uint8_t buf_5, uint8_t buf_6, uint8_t buf_7)
{
  txmsg[idx].id = id;
  txmsg[idx].len = len;
  txmsg[idx].buf[0] = buf_0;
  txmsg[idx].buf[1] = buf_1;
  txmsg[idx].buf[2] = buf_2;
  txmsg[idx].buf[3] = buf_3;
  txmsg[idx].buf[4] = buf_4;
  txmsg[idx].buf[5] = buf_5;
  txmsg[idx].buf[6] = buf_6;
  txmsg[idx].buf[7] = buf_7;
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  delay(2000);

  defaultMask.id = 0x7FF;

  defaultFilter.id = 0x7E8;

  pinMode(A2, OUTPUT);

  Serial.println("Init");

  Can0.begin();
  for (int i=0;i<8;i++) {
    Can0.setFilter(defaultFilter,i);
  }

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Card failed, or not present");
    return;
  }

  Serial.println("card initialized.");
  dataFile = SD.open("can-bus.txt", FILE_WRITE);
  if (dataFile) {            
    dataFile.println("---");
    dataFile.flush();
    Serial.println("New session started");
  }

  str = "";

  // no retard messages as they're sent by CAN sensor
  
  // RPM
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x20, 0x6F, 0x55, 0x55, 0x55, 0x55 );
  // misfires
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x29, 0x1D, 0x55, 0x55, 0x55, 0x55 );
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x29, 0x1E, 0x55, 0x55, 0x55, 0x55 );
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x29, 0x1F, 0x55, 0x55, 0x55, 0x55 );
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x29, 0x20, 0x55, 0x55, 0x55, 0x55 );
  // number of direct injections
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x42, 0x67, 0x55, 0x55, 0x55, 0x55 );
  // number of MPI injections
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x42, 0x68, 0x55, 0x55, 0x55, 0x55 );
  // fuel low: actual
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x20, 0x25, 0x55, 0x55, 0x55, 0x55 );
  // fuel low: specified
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x29, 0x32, 0x55, 0x55, 0x55, 0x55 );
  // fuel high: actual
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x20, 0x27, 0x55, 0x55, 0x55, 0x55 );
  // fuel high: specified
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x29, 0x3B, 0x55, 0x55, 0x55, 0x55 );
  // knock sensor voltages
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x39, 0x60, 0x55, 0x55, 0x55, 0x55 );
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x39, 0x61, 0x55, 0x55, 0x55, 0x55 );
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x39, 0x62, 0x55, 0x55, 0x55, 0x55 );
  can_msg_set(messagesNum++, 0x7E0, 8, 0x03, 0x22, 0x39, 0x63, 0x55, 0x55, 0x55, 0x55 );
}

void loop()
{
 if (serialMetro.check() == 1) {
    //Serial.println(">>> " + String(millis()) + " " + String(currentMessage));
    Can0.write(txmsg[currentMessage]);
    currentMessage++;
    if (currentMessage == messagesNum) currentMessage = 0;
  
    msg.timeout = 30;

    while (Can0.read(msg)) {
      time = millis();
      String str = "";
      
      //Serial.print(String(time) + ": " + String(msg.id, HEX) + " " + String(msg.len) + " ");
      //for (int i=0;i<8;i++)
      //  Serial.print(String(msg.buf[i], HEX) + " ");
      //Serial.println();
      
      // current RPM
      if ((msg.id == 0x7E8) && (msg.buf[0] == 0x05) && (msg.buf[1] == 0x62) && (msg.buf[2] == 0x20) && (msg.buf[3] == 0x6F)) {
        lastRPM = msg.buf[4]*256+msg.buf[5];
      }

      // retards
      else if ((msg.id == 0x7E8) && (msg.buf[0] == 0x05) && (msg.buf[1] == 0x62) && (msg.buf[2] == 0x20) && (msg.buf[3] >= 0x0A) && (msg.buf[3] <= 0x0D)) {
        int canValue = msg.buf[4]*256+msg.buf[5];
        if (canValue != 0) {
          str = str + "ret \t" + String(msg.buf[3]-0x9) + "\t" + String((float)(unsigned int)(canValue) / 100.0);;
        }
      }
      
      // misfires
      else if ((msg.id == 0x7E8) && (msg.buf[0] == 0x05) && (msg.buf[1] == 0x62) && (msg.buf[2] == 0x29) && (msg.buf[3] >= 0x1D) && (msg.buf[3] <= 0x20)) {
        int misfires = msg.buf[4]*256+msg.buf[5];
        if (misfires > 0) {
          str = str + "misfire \t" + String(msg.buf[3]-0x1C) + "\t" + String(misfires);
          tone(A2,1000,500);
        }
      }

      // knock voltage
      else if ((msg.id == 0x7E8) && (msg.buf[0] == 0x05) && (msg.buf[1] == 0x62) && (msg.buf[2] == 0x39) && (msg.buf[3] >= 0x60) && (msg.buf[3] <= 0x63)) {
        int knockV = msg.buf[4]*256+msg.buf[5];
        str = str + "knock V \t" + String(msg.buf[3]-0x5F) + "\t" + String(knockV/1000.0);
      }

      // direct injections
      else if ((msg.id == 0x7E8) && (msg.buf[0] == 0x07) && (msg.buf[1] == 0x62) && (msg.buf[2] == 0x42) && (msg.buf[3] == 0x67)) {
        uint32_t directInjections = (msg.buf[4] << 24) + (msg.buf[5] << 16) + (msg.buf[6] << 8) + msg.buf[7];
        if (directInjections > lastDirectInjections) {
          str = str + "directInjections:\t" + String(directInjections);
          lastDirectInjections = directInjections;
          currentInjectors = "[FSI]";
        }
      }

      // MPI injections
      else if ((msg.id == 0x7E8) && (msg.buf[0] == 0x07) && (msg.buf[1] == 0x62) && (msg.buf[2] == 0x42) && (msg.buf[3] == 0x68)) {
        uint32_t mpiInjections = (msg.buf[4] << 24) + (msg.buf[5] << 16) + (msg.buf[6] << 8) + msg.buf[7];
        if (mpiInjections > lastMPIInjections) {
          str = str + "MPIInjections:\t" + String(mpiInjections);
          lastMPIInjections = mpiInjections;
          currentInjectors = "[MPI]";
        }
      }

      // fuel low: specified
      else if ((msg.id == 0x7E8) && (msg.buf[0] == 0x05) && (msg.buf[1] == 0x62) && (msg.buf[2] == 0x29) && (msg.buf[3] == 0x32)) {
        int pressure = msg.buf[4]*256+msg.buf[5];
        str = str + "fuel low - spec \t" + String(pressure/1000.0);
      }

      // fuel low: actual
      else if ((msg.id == 0x7E8) && (msg.buf[0] == 0x05) && (msg.buf[1] == 0x62) && (msg.buf[2] == 0x20) && (msg.buf[3] == 0x25)) {
        int pressure = msg.buf[4]*256+msg.buf[5];
        str = str + "fuel low - actual \t" + String(pressure/1000.0);
      }

      // fuel high: specified
      else if ((msg.id == 0x7E8) && (msg.buf[0] == 0x05) && (msg.buf[1] == 0x62) && (msg.buf[2] == 0x29) && (msg.buf[3] == 0x3B)) {
        int pressure = msg.buf[4]*256+msg.buf[5];
        str = str + "fuel high - specified \t" + String(pressure/10.0);
      }

      // fuel high: actual
      else if ((msg.id == 0x7E8) && (msg.buf[0] == 0x05) && (msg.buf[1] == 0x62) && (msg.buf[2] == 0x20) && (msg.buf[3] == 0x27)) {
        int pressure = msg.buf[4]*256+msg.buf[5];
        str = str + "fuel high - actual \t" + String(pressure/10.0);
      }

      if (str!="") {
        unsigned long allSeconds=time/1000;
        int runHours= allSeconds/3600;
        int secsRemaining=allSeconds%3600;
        int runMinutes=secsRemaining/60;
        int runSeconds=secsRemaining%60;
        str = String(time) + "\t" + String(runHours) + ":" + String(runMinutes) + ":" + String(runSeconds) + "\t" + String(lastRPM) + "\t" + currentInjectors + "\t" + str;
        //Serial.println(str);
        if (dataFile) {
          dataFile.println(str);
          dataFile.flush();
        }
      }
      msg.timeout = 0;
    }
  }
}
