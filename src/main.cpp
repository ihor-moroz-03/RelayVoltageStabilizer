#include <Arduino.h>

constexpr uint8_t inputVoltPin = A1;
constexpr uint8_t outputVoltPin = A2;
constexpr uint8_t delayButtonPin = A0;
constexpr uint8_t delayLedPin = 5;
constexpr uint8_t powerGridConnectionLedPin = 4;
constexpr uint8_t protectionLedPin = 13;

constexpr uint8_t relays[3] = { 10, 11, 12 };
constexpr uint8_t outputRelay = 9;
constexpr uint8_t modesMap[6] = { 0, 1, 4, 5, 2, 3 };

constexpr unsigned long delayTimeout = 10000;
constexpr int jumpProtectionBoundary = 3;
// constexpr int outputProtectionBottom = 212;
constexpr int outputProtectionTop = 248;
constexpr int relaySwitchDelay = 50;

static uint8_t relayMode;


int readVoltage(uint8_t pin);
void setRelayMode(uint8_t mode);
void toggleOutputRelay(uint8_t state);
bool isOutputVoltageSafe();
bool delayButtonPseudoDigitalRead();

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

    if (inputVoltage < 259)
    {
        if (!isOutputAllowed && delayButtonPseudoDigitalRead())
        {
            digitalWrite(delayLedPin, HIGH);
            delay(delayTimeout);
            digitalWrite(delayLedPin, LOW);
        }

        if (outputVoltage > 240 + jumpProtectionBoundary ||
            outputVoltage < 220 - jumpProtectionBoundary)
        {
            inputVoltage -= 140;
            setRelayMode((inputVoltage < 0 ? 0 : inputVoltage) / 20);
        }
        
        isOutputAllowed = isOutputVoltageSafe();
    }
    else isOutputAllowed = false;

    toggleOutputRelay(isOutputAllowed);
    digitalWrite(protectionLedPin, !isOutputAllowed);
}

// using this because button signal is around 2.2V
// it's not recognised as HIGH with digitalRead()
bool delayButtonPseudoDigitalRead()
{
    return analogRead(delayButtonPin) > 350;
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
    return outputVoltage / 3 <= outputProtectionTop / 3;
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