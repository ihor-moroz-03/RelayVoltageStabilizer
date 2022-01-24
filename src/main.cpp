#include <Arduino.h>

constexpr uint8_t inputVoltPin = A1;
constexpr uint8_t outputVoltPin = A2;
constexpr uint8_t delayButtonPin = 5;
constexpr uint8_t delayLedPin = 6;
constexpr uint8_t powerGridConnectionLedPin = 7;
constexpr uint8_t protectionLedPin = 13;

constexpr uint8_t relays[3] = { 10, 11, 12 };
constexpr uint8_t outputRelay = 9;
constexpr uint8_t modesMap[6] = { 0, 1, 4, 5, 2, 3 };

constexpr unsigned long delayTimeout = 10000;
constexpr int jumpProtectionBoundary = 3;
constexpr int outputProtectionBottom = 210;
constexpr int outputProtectionTop = 250;
constexpr int relaySwitchDelay = 50;

static uint8_t relayMode;


int readVoltage(uint8_t pin);
void setRelayMode(uint8_t mode);
void toggleOutputRelay(uint8_t state);
bool isOutputVoltageSafe();

void setup()
{
    for (int i = 0; i < 3; ++i) pinMode(relays[i], OUTPUT);
    pinMode(outputRelay, OUTPUT);
    pinMode(protectionLedPin, OUTPUT);
    pinMode(delayLedPin, OUTPUT);
    
    pinMode(powerGridConnectionLedPin, OUTPUT);
    digitalWrite(powerGridConnectionLedPin, HIGH);

    setRelayMode(4);
}

void loop()
{
    static bool isOutputAllowed = false;

    int inputVoltage = readVoltage(inputVoltPin);
    int outputVoltage = readVoltage(outputVoltPin);

    if (inputVoltage < 140 || inputVoltage > 259) isOutputAllowed = false;
    else
    {
        if (!isOutputAllowed && digitalRead(delayButtonPin))
        {
            digitalWrite(delayLedPin, HIGH);
            delay(delayTimeout);
            digitalWrite(delayLedPin, LOW);
        }

        if (outputVoltage > 240 + jumpProtectionBoundary ||
            outputVoltage < 220 - jumpProtectionBoundary)
        {
            setRelayMode((inputVoltage - 140) / 20);
        }
        
        isOutputAllowed = isOutputVoltageSafe();
    }

    toggleOutputRelay(isOutputAllowed);
    digitalWrite(protectionLedPin, !isOutputAllowed);
}

void relaysDigitalWrite(uint8_t* targetPositions, bool (*predicate)(uint8_t))
{
    static constexpr uint8_t relayPriority[3] = { 1, 0, 2 };
    for (uint8_t i : relayPriority)
        if (predicate(targetPositions[i]) && digitalRead(relays[i]) != targetPositions[i])
        {
            digitalWrite(relays[i], targetPositions[i]);
            delay(relaySwitchDelay);
        }
}

void setRelayMode(uint8_t mode)
{
    relayMode = mode;

    uint8_t targetPositions[3];
    mode = modesMap[mode];
    for (int i = 2; i > -1; --i, mode >>= 1)
        targetPositions[i] = mode & 1;

    relaysDigitalWrite(targetPositions, [](uint8_t pos) { return pos == 1; });
    relaysDigitalWrite(targetPositions, [](uint8_t pos) { return pos == 0; });
}

void toggleOutputRelay(uint8_t state)
{
    digitalWrite(outputRelay, state);
    delay(relaySwitchDelay);
}

bool isOutputVoltageSafe()
{
    int outputVoltage = readVoltage(outputVoltPin);
    return outputVoltage >= outputProtectionBottom && outputVoltage <= outputProtectionTop;
}

int readVoltage(uint8_t pin)
{
    int result = 0;
    for (int j = 0; j < 3; ++j)
    {
        int amplitude = 0;
        for (int i = 0; i < 190; ++i)
        {
            int newRead = analogRead(pin);
            if (newRead > amplitude) amplitude = newRead;
        }
        result += amplitude;
    }

    return (result / 3) - (1023 / 5 * 2) - 24;
}