const uint8_t HEADER1 = 0xAA;
const uint8_t HEADER2 = 0xBB;
const uint8_t HEADER3 = 0xCC;

const uint8_t FOOTER1 = 0xDD;
const uint8_t FOOTER2 = 0xEE;
const uint8_t FOOTER3 = 0xFF;

const uint8_t PACKET_SIZE = 12;

uint8_t rxBuffer[PACKET_SIZE];
uint8_t rxIndex = 0;

bool packetReady = false;

// Command Id's from GUI
const uint8_t CMD_ID = 0x01;
const uint8_t PWM_ID = 0x02;
const uint8_t ADC_ON_ID = 0x03;
const uint8_t ADC_OFF_ID = 0x04;

// PIN NAMES
const uint8_t PWM_PIN = 10;

const uint8_t ANALOG_PIN = A1;

// GLOBAL VARIABLES FOR ADC SENSING
bool adcStreaming = false;
uint32_t previousADCmillis = 0;
const uint16_t ADC_INTERVAL = 2;

void receivePacket() 
{
  while (Serial.available()) 
  {
    uint8_t data = Serial.read();

    // HEADER Detection
    if (rxIndex == 0 && data != HEADER1) 
    {
      continue;
    }

    if (rxIndex == 1 && data != HEADER2)
     {
      rxIndex = 0;
      continue;
    }

    if (rxIndex == 2 && data != HEADER3)
     {
      rxIndex = 0;
      continue;
    }

    rxBuffer[rxIndex++] = data;

    // Full Packet Received
    if (rxIndex >= PACKET_SIZE) 
    {
      //Validate Footer
      if (rxBuffer[9] == FOOTER1 && rxBuffer[10] == FOOTER2 && rxBuffer[11] == FOOTER3) 
      {
        packetReady = true;
      }

      break;
    }
  }
}

void processPacket() {
  uint8_t commandId = rxBuffer[3];

  switch (commandId) {
    case CMD_ID:
      {
        uint16_t blinkCount = (rxBuffer[5] << 8) | rxBuffer[4];
        uint8_t duration = rxBuffer[6];

        sendAck();
        blinkLed(blinkCount, duration);
      }
      break;

    case PWM_ID:
      {
        uint8_t brightness = rxBuffer[4];
        uint16_t duration = ( rxBuffer[6] << 8 ) | rxBuffer[5];
        uint8_t mode = rxBuffer[7];

        sendAck();
        startPWM(brightness, duration, mode);
      }
      break;

      case ADC_ON_ID:
      {
        sendAck();
        adcStreaming = true;
      }
      break;

       case ADC_OFF_ID:
      {
        sendAck();
        adcStreaming = false;
      }
      break;
  }
}

void sendAck() 
{
  uint8_t ackPacket[] = 
  {
    0x41,
    0x42,
    0x43
  };

  Serial.write(ackPacket,
               sizeof(ackPacket));
}

void blinkLed(uint16_t count, uint8_t duration) 
{
  for (uint16_t i = 0; i < count; i++) 
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(duration * 1000);

    digitalWrite(LED_BUILTIN, LOW);
    delay(duration * 1000);
  }
}

void startPWM(uint8_t brightness, uint16_t duration, uint8_t mode) 
{

  if (mode == 0xBC)
  {
    for (int brightness = 0; brightness <= 255; brightness++) 
    {
      // Fade In
      analogWrite(PWM_PIN, brightness);
      delay(duration);
    }


    for (int brightness = 255; brightness >= 0; brightness--) 
    {
      // Fade Out
      analogWrite(PWM_PIN, brightness);
      delay(duration);
    }

    delay(100);
  }

  if(mode == 0xAB)
  {
    analogWrite(PWM_PIN, brightness);
    delay(duration);

    analogWrite(PWM_PIN, 0);
  }
}

void handleADCstreaming()
{
  if(adcStreaming)
  {
    uint32_t currentMillis = millis();

    if(currentMillis - previousADCmillis >= ADC_INTERVAL)
    {
      previousADCmillis = currentMillis;

      uint16_t adcValue = analogRead(ANALOG_PIN);

      sendADCvalue(adcValue);
    }
  }
}

void sendADCvalue(uint16_t adcValue)
{
  uint8_t adcPacket[4];

  //Header
  adcPacket[0] = 0xAA;

  adcPacket[1] = adcValue & 0xFF;
  adcPacket[2] = (adcValue >> 8) & 0xFF;

  adcPacket[3] = 0xFF;

  Serial.write(adcPacket, sizeof(adcPacket));
}

void setup() {

  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  //PWM Raw Thing
  pinMode(PWM_PIN, OUTPUT);
}

void loop() {

  receivePacket();

  if (packetReady) {
    processPacket();

    packetReady = false;
    rxIndex = 0;
  }

  handleADCstreaming();
}
