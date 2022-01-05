#include <Arduino.h>

constexpr uint8_t inputVoltPin = A1;
constexpr uint8_t outputVoltPin = A2;
constexpr uint8_t delayButtonPin = 5;
constexpr uint8_t delayLedPin = 6;
constexpr uint8_t networkConnectionLedPin = 7;
constexpr uint8_t protectionLedPin = 13;

constexpr uint8_t relay1 = 10;
constexpr uint8_t relay2 = 11;
constexpr uint8_t relay3 = 12;
constexpr uint8_t outputRelay = 9;
constexpr uint8_t modesMap[6] = { 0, 1, 4, 5, 2, 3 };

constexpr unsigned long delayTimeout = 5000;
constexpr int jumpProtectionBoundary = 8;
constexpr int outputProtectionBottom = 195;
constexpr int outputProtectionTop = 245;
constexpr int relaySwitchDelay = 225;

static uint8_t relayMode;


int readVoltage(uint8_t pin);
void setRelayMode(uint8_t mode);
void toggleOutputRelay(uint8_t state);
bool isOutputVoltageSafe();
void enableProtection(bool& protectionFlag);

void setup()
{
    pinMode(relay1, OUTPUT);
    pinMode(relay2, OUTPUT);
    pinMode(relay3, OUTPUT);
    pinMode(outputRelay, OUTPUT);
    pinMode(protectionLedPin, OUTPUT);
    pinMode(delayLedPin, OUTPUT);
    
    pinMode(networkConnectionLedPin, OUTPUT);
    digitalWrite(networkConnectionLedPin, HIGH);

    setRelayMode(4);
}

void loop()
{
    static bool protection = true;
    int inputVoltage = readVoltage(inputVoltPin) - 140;

    if (inputVoltage < 0 || inputVoltage > 119) enableProtection(protection);
    else
    {
        digitalWrite(protectionLedPin, LOW);

        if ((inputVoltage + jumpProtectionBoundary) / 20 != relayMode &&
            (inputVoltage - jumpProtectionBoundary) / 20 != relayMode)
        {
            toggleOutputRelay(LOW);
            setRelayMode(inputVoltage / 20);
        }

        if (protection && digitalRead(delayButtonPin))
        {
            digitalWrite(delayLedPin, HIGH);
            delay(delayTimeout);
            digitalWrite(delayLedPin, LOW);
        }
        
        if (!isOutputVoltageSafe()) enableProtection(protection);
        else
        {
            toggleOutputRelay(HIGH);
            protection = false;
        }
    }
}

void setRelayMode(uint8_t mode)
{
    relayMode = mode;

    mode = modesMap[mode];
    digitalWrite(relay3, mode & 1);
    digitalWrite(relay2, (mode >>= 1) & 1);
    digitalWrite(relay1, (mode >>= 1) & 1);

    delay(relaySwitchDelay);
}

void toggleOutputRelay(uint8_t state)
{
    digitalWrite(outputRelay, state);
    delay(relaySwitchDelay);
}

void enableProtection(bool& protectionFlag)
{
    toggleOutputRelay(LOW);
    digitalWrite(protectionLedPin, HIGH);
    protectionFlag = true;
}

bool isOutputVoltageSafe()
{
    int outputVoltage = readVoltage(outputVoltPin);
    return outputVoltage < outputProtectionBottom || outputVoltage > outputProtectionTop;
}

int readVoltage(uint8_t pin)
{
    int result = 0;
    for (int j = 0; j < 3; ++j)
    {
        int amplitude = 0;
        for (int i = 0; i < 180; ++i)
        {
            int newRead = analogRead(pin);
            if (newRead > amplitude) amplitude = newRead;
        }
        result += amplitude;
    }

    return (result / 3) - (1023 / 5 * 2) - 24;
}