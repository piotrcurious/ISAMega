#include <Arduino.h>

// Pin Definitions
#define PIN_MEMW 21
#define PIN_MEMR 20
#define PIN_IOW 19
#define PIN_IOR 18

#define PIN_RESET 17
#define PIN_AEN 16
#define PIN_ALE 15
#define PIN_IOCHRDY 14
#define PIN_REFRESH 2 // This pin is used for an interrupt, ensure it's suitable for your board

// Port Definitions (Adjust these based on your Arduino board's pin mapping)
// For example, on an Arduino Mega:
// PORTA maps to digital pins 22-29 (A0-A7)
// PORTC maps to digital pins 30-37 (A8-A15)
// PORTL maps to digital pins 42-49 (A16-A23)
// PORTK maps to digital pins 62-69 (D0-D7 for data bus)
#define PORT_ADDR0 PORTA   // [A0:A7]
#define PORT_ADDR8 PORTC   // [A8:A15]
#define PORT_ADDR16 PORTL  // [A16:A19] (assuming you only need up to A19 for 1MB addressing)
#define PORT_DATA PORTK    // Data bus D0-D7
#define PIN_DATA PINK      // Input register for PORTK

#define DDR_ADDR0 DDRA
#define DDR_ADDR8 DDRC
#define DDR_ADDR16 DDRL
#define DDR_DATA DDRK

// --- I/O and Memory Access Functions ---

/**
 * @brief Writes a byte of data to a specified I/O address.
 *
 * @param address The 20-bit I/O address.
 * @param data The 8-bit data to write.
 */
void ioWrite(uint32_t address, uint8_t data)
{
    // Set address lines
    PORT_ADDR0 = address & 0xFF;
    PORT_ADDR8 = (address >> 8) & 0xFF;
    PORT_ADDR16 = (address >> 16) & 0xFF;

    // Set data bus as output and put data
    PORT_DATA = data;
    DDR_DATA = 0xFF; // Set data pins as outputs

    // Assert IOW (IO Write) low to initiate write
    digitalWrite(PIN_IOW, LOW);

    // Wait for IOCHRDY (IO Channel Ready) to go high, indicating completion
    while (!digitalRead(PIN_IOCHRDY)) {}

    // De-assert IOW high
    digitalWrite(PIN_IOW, HIGH);

    // Set data bus as input (high-impedance)
    DDR_DATA = 0x00;
}

/**
 * @brief Reads a byte of data from a specified I/O address.
 *
 * @param address The 20-bit I/O address.
 * @return The 8-bit data read from the address.
 */
uint8_t ioRead(uint32_t address)
{
    uint8_t data = 0x00;

    // Set address lines
    PORT_ADDR0 = address & 0xFF;
    PORT_ADDR8 = (address >> 8) & 0xFF;
    PORT_ADDR16 = (address >> 16) & 0xFF;

    // Set data bus as input (high-impedance) and enable pull-ups if needed
    PORT_DATA = 0xFF; // Enable pull-ups on data lines (optional, depends on hardware)
    DDR_DATA = 0x00;  // Set data pins as inputs

    // Assert IOR (IO Read) low to initiate read
    digitalWrite(PIN_IOR, LOW);

    // Wait for IOCHRDY to go high
    while (!digitalRead(PIN_IOCHRDY)) {}

    // Read data from the data bus
    data = PIN_DATA;

    // De-assert IOR high
    digitalWrite(PIN_IOR, HIGH);

    return data;
}

/**
 * @brief Writes a byte of data to a specified memory (VRAM) address.
 *
 * @param address The 20-bit memory address.
 * @param data The 8-bit data to write.
 */
void memWrite(uint32_t address, uint8_t data)
{
    // Set data bus as output and put data
    PORT_DATA = data;
    DDR_DATA = 0xFF; // Set data pins as outputs

    // Set address lines
    PORT_ADDR0 = address & 0xFF;
    PORT_ADDR8 = (address >> 8) & 0xFF;
    PORT_ADDR16 = (address >> 16) & 0xFF;

    // Assert MEMW (Memory Write) low to initiate write
    digitalWrite(PIN_MEMW, LOW);

    // Wait for IOCHRDY to go high
    while (!digitalRead(PIN_IOCHRDY)) {}

    // De-assert MEMW high
    digitalWrite(PIN_MEMW, HIGH);

    // Set data bus as input
    DDR_DATA = 0x00;
}

/**
 * @brief Reads a byte of data from a specified memory (VRAM) address.
 *
 * @param address The 20-bit memory address.
 * @return The 8-bit data read from the address.
 */
uint8_t memRead(uint32_t address)
{
    uint8_t data = 0x00;

    // Set address lines
    PORT_ADDR0 = address & 0xFF;
    PORT_ADDR8 = (address >> 8) & 0xFF;
    PORT_ADDR16 = (address >> 16) & 0xFF;

    // Set data bus as input and enable pull-ups
    PORT_DATA = 0xFF; // Enable pull-ups on data lines
    DDR_DATA = 0x00;  // Set data pins as inputs

    // Assert MEMR (Memory Read) low to initiate read
    digitalWrite(PIN_MEMR, LOW);

    // Wait for IOCHRDY to go high
    while (!digitalRead(PIN_IOCHRDY)) {}

    // Read data from the data bus
    data = PIN_DATA;

    // De-assert MEMR high
    digitalWrite(PIN_MEMR, HIGH);

    return data;
}

/**
 * @brief Writes data to an indexed VGA register.
 * This is the standard way to access indexed registers like Sequencer, CRTC, GC.
 *
 * @param indexPort The I/O address of the index register (e.g., 0x3C4 for Sequencer).
 * @param dataPort The I/O address of the data register (e.g., 0x3C5 for Sequencer).
 * @param index The index of the sub-register.
 * @param data The data to write to the sub-register.
 */
void writeIndexedRegister(uint32_t indexPort, uint32_t dataPort, uint8_t index, uint8_t data) {
    ioWrite(indexPort, index); // Write the index to the index port
    ioWrite(dataPort, data);   // Write the data to the data port
}

/**
 * @brief Reads data from an indexed VGA register.
 * This is the standard way to access indexed registers like Sequencer, CRTC, GC.
 *
 * @param indexPort The I/O address of the index register (e.g., 0x3C4 for Sequencer).
 * @param dataPort The I/O address of the data register (e.g., 0x3C5 for Sequencer).
 * @param index The index of the sub-register to read.
 * @return The data read from the sub-register.
 */
uint8_t readIndexedRegister(uint32_t indexPort, uint32_t dataPort, uint8_t index) {
    ioWrite(indexPort, index); // Write the index to the index port
    return ioRead(dataPort);   // Read data from the data port
}

// --- Interrupt Service Routine ---
volatile bool refreshDetected = false;
void refreshInterrupt()
{
    // This ISR is triggered on the falling edge of the REFRESH signal.
    // It's a placeholder for more complex refresh handling if needed.
    refreshDetected = true;
}

// --- VGA Test Functions ---

/**
 * @brief Performs basic read/write tests on key VGA registers.
 */
void runVGARegisterTests() {
    Serial.println("\n--- Starting VGA Register Tests ---");

    uint8_t testValue = 0x5A;
    uint8_t readValue;
    bool testPassed;

    // --- 1. Test Miscellaneous Output Register (MOR) ---
    // MOR Write: 0x3C2, Read: 0x3CC
    Serial.print("Testing Miscellaneous Output Register (0x3C2/0x3CC)... ");
    ioWrite(0x3C2, testValue); // Write a test value
    readValue = ioRead(0x3CC); // Read from the read-back port
    testPassed = (readValue == testValue);
    Serial.print(testPassed ? "PASS" : "FAIL");
    Serial.print(" (Wrote: 0x"); Serial.print(testValue, HEX);
    Serial.print(", Read: 0x"); Serial.print(readValue, HEX);
    Serial.println(")");

    // --- 2. Test Sequencer Registers (Indexed) ---
    // Index Port: 0x3C4, Data Port: 0x3C5
    // Test Sequencer Reset Register (Index 0x00) - write 0x01 (sync reset)
    Serial.print("Testing Sequencer Reset Register (0x3C4/0x3C5, Index 0x00)... ");
    writeIndexedRegister(0x3C4, 0x3C5, 0x00, 0x01); // Set sync reset
    readValue = readIndexedRegister(0x3C4, 0x3C5, 0x00); // Read back
    testPassed = (readValue == 0x01);
    Serial.print(testPassed ? "PASS" : "FAIL");
    Serial.print(" (Wrote: 0x01, Read: 0x"); Serial.print(readValue, HEX);
    Serial.println(")");
    writeIndexedRegister(0x3C4, 0x3C5, 0x00, 0x03); // Clear sync reset (normal operation)

    // Test Sequencer Clocking Mode Register (Index 0x01) - write a known value
    Serial.print("Testing Sequencer Clocking Mode Register (0x3C4/0x3C5, Index 0x01)... ");
    uint8_t originalClockMode = readIndexedRegister(0x3C4, 0x3C5, 0x01); // Save original
    writeIndexedRegister(0x3C4, 0x3C5, 0x01, testValue); // Write test value
    readValue = readIndexedRegister(0x3C4, 0x3C5, 0x01); // Read back
    testPassed = (readValue == testValue);
    Serial.print(testPassed ? "PASS" : "FAIL");
    Serial.print(" (Wrote: 0x"); Serial.print(testValue, HEX);
    Serial.print(", Read: 0x"); Serial.print(readValue, HEX);
    Serial.println(")");
    writeIndexedRegister(0x3C4, 0x3C5, 0x01, originalClockMode); // Restore original

    // --- 3. Test Graphics Controller Registers (Indexed) ---
    // Index Port: 0x3CE, Data Port: 0x3CF
    // Test Graphics Mode Register (Index 0x05)
    Serial.print("Testing Graphics Mode Register (0x3CE/0x3CF, Index 0x05)... ");
    uint8_t originalGraphicsMode = readIndexedRegister(0x3CE, 0x3CF, 0x05); // Save original
    writeIndexedRegister(0x3CE, 0x3CF, 0x05, testValue); // Write test value
    readValue = readIndexedRegister(0x3CE, 0x3CF, 0x05); // Read back
    testPassed = (readValue == testValue);
    Serial.print(testPassed ? "PASS" : "FAIL");
    Serial.print(" (Wrote: 0x"); Serial.print(testValue, HEX);
    Serial.print(", Read: 0x"); Serial.print(readValue, HEX);
    Serial.println(")");
    writeIndexedRegister(0x3CE, 0x3CF, 0x05, originalGraphicsMode); // Restore original

    // --- 4. Test CRT Controller Registers (Indexed) ---
    // Index Port: 0x3D4, Data Port: 0x3D5
    // Test Horizontal Total Register (Index 0x00)
    Serial.print("Testing CRTC Horizontal Total Register (0x3D4/0x3D5, Index 0x00)... ");
    uint8_t originalCRTC_HT = readIndexedRegister(0x3D4, 0x3D5, 0x00); // Save original
    writeIndexedRegister(0x3D4, 0x3D5, 0x00, testValue); // Write test value
    readValue = readIndexedRegister(0x3D4, 0x3D5, 0x00); // Read back
    testPassed = (readValue == testValue);
    Serial.print(testPassed ? "PASS" : "FAIL");
    Serial.print(" (Wrote: 0x"); Serial.print(testValue, HEX);
    Serial.print(", Read: 0x"); Serial.print(readValue, HEX);
    Serial.println(")");
    writeIndexedRegister(0x3D4, 0x3D5, 0x00, originalCRTC_HT); // Restore original

    Serial.println("--- Finished VGA Register Tests ---");
}

/**
 * @brief Performs a simple VRAM read/write test.
 */
void runVRAMTest() {
    Serial.println("\n--- Starting VRAM Test ---");

    uint32_t vramAddress = 0xA0000; // Common start address for VGA VRAM
    uint8_t pattern1 = 0xAA;
    uint8_t pattern2 = 0x55;
    uint8_t readValue;
    bool testPassed = true;

    Serial.print("Writing 0xAA to 0xA0000... ");
    memWrite(vramAddress, pattern1);
    readValue = memRead(vramAddress);
    if (readValue == pattern1) {
        Serial.println("PASS");
    } else {
        Serial.print("FAIL (Wrote: 0x"); Serial.print(pattern1, HEX);
        Serial.print(", Read: 0x"); Serial.print(readValue, HEX);
        Serial.println(")");
        testPassed = false;
    }

    Serial.print("Writing 0x55 to 0xA0001... ");
    memWrite(vramAddress + 1, pattern2);
    readValue = memRead(vramAddress + 1);
    if (readValue == pattern2) {
        Serial.println("PASS");
    } else {
        Serial.print("FAIL (Wrote: 0x"); Serial.print(pattern2, HEX);
        Serial.print(", Read: 0x"); Serial.print(readValue, HEX);
        Serial.println(")");
        testPassed = false;
    }

    Serial.print("Verifying 0xA0000 still holds 0xAA... ");
    readValue = memRead(vramAddress);
    if (readValue == pattern1) {
        Serial.println("PASS");
    } else {
        Serial.print("FAIL (Expected: 0x"); Serial.print(pattern1, HEX);
        Serial.print(", Read: 0x"); Serial.print(readValue, HEX);
        Serial.println(")");
        testPassed = false;
    }

    Serial.println("--- Finished VRAM Test ---");
}


// --- Arduino Setup and Loop Functions ---

void setup()
{
    Serial.begin(1000000); // Initialize Serial communication at 1Mbps

    // Initialize Address Ports to 0 and set as outputs
    PORT_ADDR0 = 0x00;
    PORT_ADDR8 = 0x00;
    PORT_ADDR16 = 0x00;

    DDR_ADDR0 = 0xFF;   // Set [A0:A7] as outputs
    DDR_ADDR8 = 0xFF;   // Set [A8:A15] as outputs
    DDR_ADDR16 = 0xFF;  // Set [A16:A19] as outputs

    // Initialize control pins to HIGH (inactive)
    digitalWrite(PIN_MEMW, HIGH);
    digitalWrite(PIN_MEMR, HIGH);
    digitalWrite(PIN_IOW, HIGH);
    digitalWrite(PIN_IOR, HIGH);

    // Set PIN_REFRESH to INPUT_PULLUP and attach interrupt
    // Note: PIN_REFRESH is typically an output from the VGA card,
    // so setting it as INPUT_PULLUP and attaching interrupt might be for monitoring
    // a signal from the card, not controlling it.
    digitalWrite(PIN_REFRESH, HIGH); // Ensure pull-up is active if configured as input
    pinMode(PIN_REFRESH, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_REFRESH), refreshInterrupt, FALLING);

    // Set AEN (Address Enable) LOW to enable I/O and Memory cycles
    digitalWrite(PIN_AEN, LOW);
    pinMode(PIN_AEN, OUTPUT);

    // Set ALE (Address Latch Enable) HIGH (active high, typically pulsed low to latch)
    // For direct address setting, keeping it high might be fine if not latching external registers
    digitalWrite(PIN_ALE, HIGH);
    pinMode(PIN_ALE, OUTPUT);

    // IOCHRDY (IO Channel Ready) is typically an input from the peripheral.
    // Setting it HIGH here might be a pull-up or an initial state.
    // The `while (!digitalRead(PIN_IOCHRDY))` loop expects it to be an input.
    digitalWrite(PIN_IOCHRDY, HIGH); // Ensure pull-up if it's an input
    pinMode(PIN_IOCHRDY, INPUT_PULLUP); // Confirm it's configured as input

    // Configure control pins as outputs
    pinMode(PIN_MEMW, OUTPUT);
    pinMode(PIN_MEMR, OUTPUT);
    pinMode(PIN_IOW, OUTPUT);
    pinMode(PIN_IOR, OUTPUT);

    // Reset the VGA card
    digitalWrite(PIN_RESET, HIGH); // Assert RESET high
    pinMode(PIN_RESET, OUTPUT);
    delay(100); // Hold reset for a short duration
    digitalWrite(PIN_RESET, LOW); // De-assert RESET (active low)
    delay(1000); // Allow card to initialize

    // Re-initialize address ports after reset (redundant but harmless)
    PORT_ADDR0 = 0x00;
    PORT_ADDR8 = 0x00;
    PORT_ADDR16 = 0x00;

    DDR_ADDR0 = 0xFF;
    DDR_ADDR8 = 0xFF;
    DDR_ADDR16 = 0xFF;

    Serial.print("Arduino ready. Starting VGA tests...\n");

    // Run the automatic VGA tests
    runVGARegisterTests();
    runVRAMTest();

    Serial.println("\n--- All Automatic Tests Complete ---");
    Serial.println("You can now send serial commands (i, o, r, w) for manual interaction.");
}

uint8_t command;
uint32_t address;
uint8_t data;

void loop()
{
    // Check for serial commands for manual interaction
    if (Serial.available())
    {
        command = Serial.read();
        if (command == 'i') // Read from I/O port
        {
            while (Serial.available() < 2) {} // Wait for 2 address bytes
            address = (Serial.read() << 8);
            address |= Serial.read();
            Serial.print("I/O Read 0x"); Serial.print(address, HEX); Serial.print(": 0x");
            Serial.println(ioRead(address), HEX);
        }

        if (command == 'o') // Write to I/O port
        {
            while (Serial.available() < 3) {} // Wait for 2 address bytes + 1 data byte
            address = (Serial.read() << 8);
            address |= Serial.read();
            data = Serial.read();
            ioWrite(address, data);
            Serial.print("I/O Write 0x"); Serial.print(address, HEX); Serial.print(" <= 0x");
            Serial.println(data, HEX);
        }

        if (command == 'r') // Read from Memory (VRAM)
        {
            while (Serial.available() < 3) {} // Wait for 3 address bytes
            address = ((uint32_t) Serial.read() << 16UL);
            address |= (Serial.read() << 8);
            address |= Serial.read();
            Serial.print("Mem Read 0x"); Serial.print(address, HEX); Serial.print(": 0x");
            Serial.println(memRead(address), HEX);
        }

        if (command == 'w') // Write to Memory (VRAM)
        {
            while (Serial.available() < 4) {} // Wait for 3 address bytes + 1 data byte
            address = ((uint32_t)Serial.read() << 16UL);
            address |= (Serial.read() << 8);
            address |= Serial.read();
            data = Serial.read();
            memWrite(address, data);
            Serial.print("Mem Write 0x"); Serial.print(address, HEX); Serial.print(" <= 0x");
            Serial.println(data, HEX);
        }
    }

    // You can add periodic checks or other logic here if needed.
    // For example, if you want to continuously monitor the refresh signal:
    // if (refreshDetected) {
    //     Serial.println("Refresh signal detected!");
    //     refreshDetected = false; // Reset flag
    // }
}
