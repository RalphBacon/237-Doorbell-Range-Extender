/*
    Door bell relay - detects ringing and repeats or relays the message onwards

    Front                Bell       Remote                    Bell
    Door    ---------> Receiver    Bell Push --------------> Receiver
    Bell               in Attic    in Attic                  in Workshop
    Push                |___ Arduino __|
                            interface
*/

#include <Arduino.h>
#define signalPin 2
#define ledPin 11
#define relayPin 10

// Debugging without Serial.print
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
#include "debug.h"

// Variable updated by ISR (must be decorated with 'volatile' attribute)
volatile uint8_t bellDetectCnt = 0;
uint8_t prevBellCnt            = 0;

//--------------------------------------------------------------------
// ISR     ISR     ISR     ISR     ISR     ISR     ISR     ISR     ISR
//--------------------------------------------------------------------
// Interrupt function when pin 2 goes high from doorbell signal
void isr() {
    bellDetectCnt++;
}

//--------------------------------------------------------------------
// SETUP     SETUP     SETUP     SETUP     SETUP     SETUP     SETUP
//--------------------------------------------------------------------
void setup() {
    // Serial monitor
    Serial.begin(115200);

    // LED/Relay control. Use seperate pins as relay needs 38mA.
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);

    // Flash the LED a few times to show we're running
    for (auto cnt = 0; cnt < 5; cnt++) {
        digitalWrite(ledPin, HIGH);
        delay(100);
        digitalWrite(ledPin, LOW);
        delay(50);
    }

    // When pin 2 goes HIGH, trigger the interrupt routine 'isr'
    attachInterrupt(digitalPinToInterrupt(signalPin), isr, RISING);

    // All done here
    debuglnD("Setup completed");
}

//--------------------------------------------------------------------
// LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
//--------------------------------------------------------------------
void loop() {
    // Keep track of how long since last signal pulse from doorbell
    static unsigned long bellMillis = 0;

    // Have we received a (further) pulse from ISR
    if (bellDetectCnt != prevBellCnt) {

        // Monitor how many pulses we've had
        debugD("Bell detect count:");
        debuglnD(bellDetectCnt);
        debuglnD(bellDetectCnt + 2748, HEX);

        // Capture the time when it was turned on
        bellMillis = millis();

        // If we have had more than 8 then we know this is valid
        // so switch on relay and LED
        if (bellDetectCnt > 8) {
            digitalWrite(ledPin, HIGH);
            digitalWrite(relayPin, HIGH);

            debuglnD("LED/relay on");
        }

        // Update the stored count for comparison
        prevBellCnt = bellDetectCnt;
    }

    // Extinguish LED/relay after N milliseconds of no signal
    if (millis() - bellMillis > 1000) {

        // If the relay/LED was turned on, turn it off now
        if (digitalRead(ledPin)) {
            digitalWrite(ledPin, LOW);
            digitalWrite(relayPin, LOW);
            debuglnD("LED/relay off");
        }

        // Always reset the counts after N millis in case we had spurious signal
        // Check that we have counted the most recent trigger first (could happen anytime)
        if (bellDetectCnt == prevBellCnt && bellDetectCnt > 0) {
            bellDetectCnt = 0;
            prevBellCnt   = 0;
            debuglnD("Bell count reset");
        }
    }
}