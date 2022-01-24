#include <Arduino.h>

constexpr uint8_t inputVoltPin = A1;
constexpr uint8_t outputVoltPin = A2;
constexpr uint8_t delayButtonPin = 5;
constexpr uint8_t delayLedPin = 6;
constexpr uint8_t networkConnectionLedPin = 7;
constexpr uint8_t protectionLedPin = 13;

constexpr uint8_t relays[3] = { 10, 11, 12 };
constexpr uint8_t outputRelay = 9;
constexpr uint8_t modesMap[6] = { 0, 1, 4, 5, 2, 3 };

constexpr unsigned long delayTimeout = 10000;
constexpr int jumpProtectionBoundary = 3;
constexpr int outputProtectionBottom = 220 - jumpProtectionBoundary;
constexpr int outputProtectionTop = 240 + jumpProtectionBoundary;
constexpr int relaySwitchDelay = 150;

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
    
    pinMode(networkConnectionLedPin, OUTPUT);
    digitalWrite(networkConnectionLedPin, HIGH);

    setRelayMode(4);
}

void loop()
{
    static bool isOutputAllowed = true;

    int inputVoltage = readVoltage(inputVoltPin) - 140;

    if (inputVoltage < 0 || inputVoltage > 119) isOutputAllowed = false;
    else
    {
        if ((inputVoltage + jumpProtectionBoundary) / 20 != relayMode &&
            (inputVoltage - jumpProtectionBoundary) / 20 != relayMode)
        {
            toggleOutputRelay(LOW);
            setRelayMode(inputVoltage / 20);
        }

        if (!isOutputAllowed && digitalRead(delayButtonPin))
        {
            digitalWrite(delayLedPin, HIGH);
            delay(delayTimeout);
            digitalWrite(delayLedPin, LOW);
        }
        
        isOutputAllowed = isOutputVoltageSafe();
    }

    toggleOutputRelay(isOutputAllowed);
    digitalWrite(protectionLedPin, !isOutputAllowed);
}

void decomposeRelayMode(uint8_t mode, uint8_t* output)
{
    for (int i = 2; i > -1; --i, mode >>= 1)
        output[i] = mode & 1;
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
    uint8_t targetPositions[3];
    decomposeRelayMode(modesMap[mode], targetPositions);

    relaysDigitalWrite(targetPositions, [](uint8_t pos) { return pos == 1; });
    relaysDigitalWrite(targetPositions, [](uint8_t pos) { return pos == 0; });

    relayMode = mode;
}

void toggleOutputRelay(uint8_t state)
{
    digitalWrite(outputRelay, state);
    delay(relaySwitchDelay);
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