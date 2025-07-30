#include <Arduino.h>
#include <string.h> // For strcpy, strcat, strtok
#include <stdio.h>  // For sprintf

// Pin Definitions (from previous code)
#define PIN_MEMW 21
#define PIN_MEMR 20
#define PIN_IOW 19
#define PIN_IOR 18

#define PIN_RESET 17
#define PIN_AEN 16
#define PIN_ALE 15
#define PIN_IOCHRDY 14
#define PIN_REFRESH 2 // This pin is used for an interrupt, ensure it's suitable for your board

// Port Definitions (from previous code)
// Adjust these based on your Arduino board's pin mapping.
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

// --- VGA Register Addresses ---
#define VGA_MISC_OUTPUT_W   0x3C2 // Miscellaneous Output Register (Write)
#define VGA_MISC_OUTPUT_R   0x3CC // Miscellaneous Output Register (Read)
#define VGA_INPUT_STATUS_0  0x3C2 // Input Status Register 0 (Read)
#define VGA_INPUT_STATUS_1_C 0x3DA // Input Status Register 1 (Read) (for Color Emulation)
#define VGA_INPUT_STATUS_1_M 0x3BA // Input Status Register 1 (Read) (for Mono Emulation)

// Sequencer Registers (SR)
#define VGA_SEQ_INDEX       0x3C4
#define VGA_SEQ_DATA        0x3C5

// CRT Controller Registers (CRTC)
#define VGA_CRTC_INDEX_C    0x3D4 // Color Emulation
#define VGA_CRTC_DATA_C     0x3D5 // Color Emulation
#define VGA_CRTC_INDEX_M    0x3B4 // Mono Emulation
#define VGA_CRTC_DATA_M     0x3B5 // Mono Emulation

// Graphics Controller Registers (GR)
#define VGA_GFX_INDEX       0x3CE
#define VGA_GFX_DATA        0x3CF

// Attribute Controller Registers (AR)
#define VGA_ATT_INDEX       0x3C0
#define VGA_ATT_DATA        0x3C1

// DAC Registers
#define VGA_DAC_PEL_MASK    0x3C6 // Pixel Mask Register
#define VGA_DAC_READ_INDEX  0x3C7 // DAC Read Index Register
#define VGA_DAC_WRITE_INDEX 0x3C8 // DAC Write Index Register
#define VGA_DAC_DATA        0x3C9 // DAC Data Register

// --- Global Variables for Display ---
const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 200;
const uint32_t VRAM_START_ADDRESS = 0xA0000; // Common VRAM start for graphics modes

// Max VRAM to test (1MB)
const uint32_t MAX_VRAM_SIZE = 1024 * 1024; // 1MB

// Simple 8x8 font (ASCII 32-90, uppercase letters and numbers)
// Each character is 8 bytes, representing 8 rows of 8 pixels.
// Using PROGMEM to store in flash memory.
const PROGMEM uint8_t font8x8[][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 32: Space
    {0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00}, // 33: !
    {0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00}, // 34: "
    {0x14, 0x7E, 0x14, 0x7E, 0x14, 0x00, 0x00, 0x00}, // 35: #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00, 0x00, 0x00}, // 36: $
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 37: % (Placeholder)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 38: & (Placeholder)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 39: ' (Placeholder)
    {0x00, 0x08, 0x14, 0x22, 0x22, 0x14, 0x08, 0x00}, // 40: (
    {0x00, 0x20, 0x14, 0x08, 0x08, 0x14, 0x20, 0x00}, // 41: )
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 42: * (Placeholder)
    {0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00}, // 43: +
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 44: , (Placeholder)
    {0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00}, // 45: -
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 46: . (Placeholder)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 47: / (Placeholder)
    {0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x00}, // 48: 0
    {0x00, 0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, 0x00}, // 49: 1
    {0x00, 0x42, 0x61, 0x51, 0x49, 0x46, 0x00, 0x00}, // 50: 2
    {0x00, 0x21, 0x41, 0x45, 0x4B, 0x31, 0x00, 0x00}, // 51: 3
    {0x00, 0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, 0x00}, // 52: 4
    {0x00, 0x27, 0x45, 0x45, 0x45, 0x39, 0x00, 0x00}, // 53: 5
    {0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00, 0x00}, // 54: 6
    {0x00, 0x01, 0x71, 0x09, 0x05, 0x03, 0x00, 0x00}, // 55: 7
    {0x00, 0x36, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00}, // 56: 8
    {0x00, 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x00}, // 57: 9
    {0x00, 0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00}, // 58: :
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 59: ; (Placeholder)
    {0x00, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00}, // 60: <
    {0x00, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00}, // 61: =
    {0x00, 0x41, 0x22, 0x14, 0x08, 0x00, 0x00, 0x00}, // 62: >
    {0x00, 0x02, 0x01, 0x51, 0x09, 0x06, 0x00, 0x00}, // 63: ?
    {0x00, 0x3E, 0x41, 0x5D, 0x5D, 0x5D, 0x1C, 0x00}, // 64: @
    {0x00, 0x7E, 0x09, 0x09, 0x09, 0x7E, 0x00, 0x00}, // 65: A
    {0x00, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00}, // 66: B
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00, 0x00}, // 67: C
    {0x00, 0x7F, 0x41, 0x41, 0x41, 0x3E, 0x00, 0x00}, // 68: D
    {0x00, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00, 0x00}, // 69: E
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x01, 0x00, 0x00}, // 70: F
    {0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A, 0x00, 0x00}, // 71: G
    {0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x00}, // 72: H
    {0x00, 0x00, 0x41, 0x7F, 0x41, 0x00, 0x00, 0x00}, // 73: I
    {0x00, 0x30, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x00}, // 74: J
    {0x00, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00}, // 75: K
    {0x00, 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00}, // 76: L
    {0x00, 0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00, 0x00}, // 77: M
    {0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00, 0x00}, // 78: N
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, 0x00}, // 79: O
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x06, 0x00, 0x00}, // 80: P
    {0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00, 0x00}, // 81: Q
    {0x00, 0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, 0x00}, // 82: R
    {0x00, 0x22, 0x49, 0x49, 0x49, 0x32, 0x00, 0x00}, // 83: S
    {0x00, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00, 0x00}, // 84: T
    {0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x00}, // 85: U
    {0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00, 0x00}, // 86: V
    {0x00, 0x3F, 0x40, 0x30, 0x40, 0x3F, 0x00, 0x00}, // 87: W
    {0x00, 0x63, 0x14, 0x08, 0x14, 0x63, 0x00, 0x00}, // 88: X
    {0x00, 0x07, 0x08, 0x70, 0x08, 0x07, 0x00, 0x00}, // 89: Y
    {0x00, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x00}, // 90: Z
};


// --- Function Prototypes ---
void ioWrite(uint32_t address, uint8_t data);
uint8_t ioRead(uint32_t address);
void memWrite(uint32_t address, uint8_t data);
uint8_t memRead(uint32_t address);
void writeIndexedRegister(uint32_t indexPort, uint32_t dataPort, uint8_t index, uint8_t data);
uint8_t readIndexedRegister(uint32_t indexPort, uint32_t dataPort, uint8_t index);
void refreshInterrupt();
void setVGAMode13h();
void setPixel(int x, int y, uint8_t color);
void fillRect(int x, int y, int w, int h, uint8_t color);
void drawChar(int x, int y, char c, uint8_t color);
void drawString(int x, int y, const char* str, uint8_t color);
void fillScreen(uint8_t color);
void drawTestPattern();
uint32_t detectVRAMSize();
void runVRAMTest(uint32_t vramSize);
void displayTestResultsOnVGA();


// --- I/O and Memory Access Functions (from previous code) ---
void ioWrite(uint32_t address, uint8_t data)
{
    PORT_ADDR0 = address & 0xFF;
    PORT_ADDR8 = (address >> 8) & 0xFF;
    PORT_ADDR16 = (address >> 16) & 0xFF;
    PORT_DATA = data;
    DDR_DATA = 0xFF; // Set data pins as outputs
    digitalWrite(PIN_IOW, LOW);
    while (!digitalRead(PIN_IOCHRDY)) {}
    digitalWrite(PIN_IOW, HIGH);
    DDR_DATA = 0x00; // Set data pins as inputs (high-impedance)
}

uint8_t ioRead(uint32_t address)
{
    uint8_t data = 0x00;
    PORT_ADDR0 = address & 0xFF;
    PORT_ADDR8 = (address >> 8) & 0xFF;
    PORT_ADDR16 = (address >> 16) & 0xFF;
    PORT_DATA = 0xFF; // Enable pull-ups on data lines (optional, depends on hardware)
    DDR_DATA = 0x00;  // Set data pins as inputs
    digitalWrite(PIN_IOR, LOW);
    while (!digitalRead(PIN_IOCHRDY)) {}
    data = PIN_DATA;
    digitalWrite(PIN_IOR, HIGH);
    return data;
}

void memWrite(uint32_t address, uint8_t data)
{
    PORT_DATA = data;
    DDR_DATA = 0xFF; // Set data pins as outputs
    PORT_ADDR0 = address & 0xFF;
    PORT_ADDR8 = (address >> 8) & 0xFF;
    PORT_ADDR16 = (address >> 16) & 0xFF;
    digitalWrite(PIN_MEMW, LOW);
    while (!digitalRead(PIN_IOCHRDY)) {}
    digitalWrite(PIN_MEMW, HIGH);
    DDR_DATA = 0x00; // Set data pins as inputs
}

uint8_t memRead(uint32_t address)
{
    uint8_t data = 0x00;
    PORT_ADDR0 = address & 0xFF;
    PORT_ADDR8 = (address >> 8) & 0xFF;
    PORT_ADDR16 = (address >> 16) & 0xFF;
    PORT_DATA = 0xFF; // Enable pull-ups on data lines
    DDR_DATA = 0x00;  // Set data pins as inputs
    digitalWrite(PIN_MEMR, LOW);
    while (!digitalRead(PIN_IOCHRDY)) {}
    data = PIN_DATA;
    digitalWrite(PIN_MEMR, HIGH);
    return data;
}

void writeIndexedRegister(uint32_t indexPort, uint32_t dataPort, uint8_t index, uint8_t data) {
    ioWrite(indexPort, index); // Write the index to the index port
    ioWrite(dataPort, data);   // Write the data to the data port
}

uint8_t readIndexedRegister(uint32_t indexPort, uint32_t dataPort, uint8_t index) {
    ioWrite(indexPort, index); // Write the index to the index port
    return ioRead(dataPort);   // Read data from the data port
}

volatile bool refreshDetected = false;
void refreshInterrupt()
{
    refreshDetected = true;
}

// --- VGA Graphics Functions ---

/**
 * @brief Sets a pixel on the VGA display in 320x200 256-color mode.
 * @param x X-coordinate (0-319)
 * @param y Y-coordinate (0-199)
 * @param color 8-bit color index (0-255)
 */
void setPixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        uint32_t address = VRAM_START_ADDRESS + (uint32_t)y * SCREEN_WIDTH + x;
        memWrite(address, color);
    }
}

/**
 * @brief Fills a rectangular area on the screen with a single color.
 * @param x X-coordinate of the top-left corner.
 * @param y Y-coordinate of the top-left corner.
 * @param w Width of the rectangle.
 * @param h Height of the rectangle.
 * @param color 8-bit color index.
 */
void fillRect(int x, int y, int w, int h, uint8_t color) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            setPixel(i, j, color);
        }
    }
}

/**
 * @brief Fills the entire screen with a single color.
 * @param color 8-bit color index.
 */
void fillScreen(uint8_t color) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            setPixel(x, y, color);
        }
    }
}

/**
 * @brief Draws a character using a simple 8x8 font.
 * @param x Top-left X-coordinate for the character.
 * @param y Top-left Y-coordinate for the character.
 * @param c The character to draw.
 * @param color The color of the character.
 */
void drawChar(int x, int y, char c, uint8_t color) {
    if (c < 32 || c > 90) return; // Only draw printable ASCII (Space to Z)
    uint8_t charIndex = c - 32;

    for (int row = 0; row < 8; row++) {
        uint8_t line = pgm_read_byte(&(font8x8[charIndex][row]));
        for (int col = 0; col < 8; col++) {
            if ((line >> (7 - col)) & 0x01) { // Check each bit
                setPixel(x + col, y + row, color);
            }
        }
    }
}

/**
 * @brief Draws a string using the 8x8 font.
 * @param x Top-left X-coordinate for the string.
 * @param y Top-left Y-coordinate for the string.
 * @param str The string to draw.
 * @param color The color of the string.
 */
void drawString(int x, int y, const char* str, uint8_t color) {
    int currentX = x;
    for (int i = 0; str[i] != '\0'; i++) {
        drawChar(currentX, y, str[i], color);
        currentX += 8; // Move to the next character position (8 pixels wide)
    }
}

/**
 * @brief Sets up VGA Mode 13h (320x200, 256 colors).
 * This involves programming various VGA registers with standard values for Mode 13h.
 */
void setVGAMode13h() {
    Serial.println("Setting VGA Mode 13h (320x200, 256 colors)...");

    // Disable video output while programming
    ioRead(VGA_INPUT_STATUS_1_C); // Read 0x3DA (color) to reset internal flip-flop for AC access
    ioWrite(VGA_ATT_INDEX, 0x00); // Reset AC flip-flop by writing to index 0x3C0
    ioWrite(VGA_ATT_INDEX, 0x10); // Select Mode Control Register (0x10)
    ioWrite(VGA_ATT_DATA, 0x01);  // Set bit 0 (8/16-bit color mode), clear bit 5 (Video Enable)

    // --- Reset Sequencer ---
    writeIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x00, 0x01); // Synchronous Reset

    // --- Set Sequencer Registers ---
    writeIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x01, 0x01); // Clocking Mode Register: 8 dots/char, screen off
    writeIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x02, 0x0F); // Map Mask Register: Enable all 4 planes
    writeIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x03, 0x00); // Character Map Select Register
    writeIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x04, 0x0E); // Memory Mode Register: Extended memory, odd/even disabled

    // --- Set CRTC Registers ---
    // Unlock CRTC registers 0-7 (bit 7 of CRTC Index 0x11)
    uint8_t crtc_protect = readIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x11);
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x11, crtc_protect & 0x7F); // Clear bit 7

    // CRTC values for 320x200 256-color mode (Mode 13h)
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x00, 0x5F); // Horizontal Total
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x01, 0x4F); // Horizontal Display End
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x02, 0x50); // Horizontal Blank Start
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x03, 0x82); // Horizontal Blank End (bit 7 for display enable skew)
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x04, 0x54); // Horizontal Sync Start
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x05, 0x80); // Horizontal Sync End (bit 7 for sync pulse width)
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x06, 0xBF); // Vertical Total
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x07, 0x1F); // Overflow Register (bits 0-4 VTotal, 5-6 VDisplayEnd, 7 VSyncStart)
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x08, 0x00); // Preset Row Scan
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x09, 0x40); // Maximum Scan Line (bit 6 for line compare)
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x0A, 0x00); // Cursor Start Register (disable cursor)
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x0B, 0x00); // Cursor End Register
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x0C, 0x00); // Start Address High
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x0D, 0x00); // Start Address Low
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x0E, 0x00); // Cursor Location High
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x0F, 0x00); // Cursor Location Low
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x10, 0x90); // Vertical Sync Start
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x11, 0x8A); // Vertical Sync End (bit 7 re-locks CRTC registers 0-7)
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x12, 0x9C); // Vertical Display End
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x13, 0x2B); // Offset Register (bytes per scanline / 8) -> 320/8 = 40 (0x28) + 3 for CL-GD5422 quirk?
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x14, 0x8D); // Underline Location Register
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x15, 0x2B); // Vertical Blank Start
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x16, 0xAE); // Vertical Blank End
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x17, 0xA3); // Mode Control Register (CRTC)
    writeIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, 0x18, 0xFF); // Line Compare Register

    // --- Set Graphics Controller Registers ---
    writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, 0x00, 0x00); // Set/Reset Register
    writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, 0x01, 0x00); // Enable Set/Reset Register
    writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, 0x02, 0x00); // Color Compare Register
    writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, 0x03, 0x00); // Data Rotate Register
    writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, 0x04, 0x00); // Read Mode Register
    writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, 0x05, 0x40); // Graphics Mode Register: 256-color mode, write mode 0
    writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, 0x06, 0x05); // Miscellaneous Graphics Register: A000-B000 segment, graphics mode
    writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, 0x07, 0x0F); // Color Don't Care Register
    writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, 0x08, 0xFF); // Bit Mask Register

    // --- Set Attribute Controller Registers ---
    // Make sure AC flip-flop is reset before writing to AC registers
    ioRead(VGA_INPUT_STATUS_1_C); // Read Input Status 1 to reset AC flip-flop
    // Program 16 palette registers (0x00-0x0F)
    for (int i = 0; i < 16; i++) {
        ioWrite(VGA_ATT_INDEX, i | 0x20); // Select palette register and enable palette access
        ioWrite(VGA_ATT_DATA, i);         // Set palette entry to its index (0-15)
    }
    ioWrite(VGA_ATT_INDEX, 0x10); // Mode Control Register (0x10)
    ioWrite(VGA_ATT_DATA, 0x01);  // Set bit 0 (8/16-bit color mode), clear bit 5 (Video Enable)
    ioWrite(VGA_ATT_INDEX, 0x11); // Overscan Color Register
    ioWrite(VGA_ATT_DATA, 0x00);  // Black overscan
    ioWrite(VGA_ATT_INDEX, 0x12); // Color Plane Enable Register
    ioWrite(VGA_ATT_DATA, 0x0F);  // Enable all planes
    ioWrite(VGA_ATT_INDEX, 0x13); // Horizontal Pixel Panning Register
    ioWrite(VGA_ATT_DATA, 0x00);
    ioWrite(VGA_ATT_INDEX, 0x14); // Color Select Register
    ioWrite(VGA_ATT_DATA, 0x00);

    // --- Load DAC Palette (Grayscale Ramp and basic colors) ---
    // For 256 colors, load a simple grayscale ramp and some primary colors
    for (int i = 0; i < 256; i++) {
        ioWrite(VGA_DAC_WRITE_INDEX, i); // Set DAC write index
        // Simple grayscale:
        ioWrite(VGA_DAC_DATA, i >> 2);   // Red (0-63)
        ioWrite(VGA_DAC_DATA, i >> 2);   // Green (0-63)
        ioWrite(VGA_DAC_DATA, i >> 2);   // Blue (0-63)
    }
    // Add some distinct colors for the test pattern
    ioWrite(VGA_DAC_WRITE_INDEX, 0xF0); // Index for Red (light red)
    ioWrite(VGA_DAC_DATA, 63); ioWrite(VGA_DAC_DATA, 30); ioWrite(VGA_DAC_DATA, 30);
    ioWrite(VGA_DAC_WRITE_INDEX, 0xF1); // Index for Green (light green)
    ioWrite(VGA_DAC_DATA, 30); ioWrite(VGA_DAC_DATA, 63); ioWrite(VGA_DAC_DATA, 30);
    ioWrite(VGA_DAC_WRITE_INDEX, 0xF2); // Index for Blue (light blue)
    ioWrite(VGA_DAC_DATA, 30); ioWrite(VGA_DAC_DATA, 30); ioWrite(VGA_DAC_DATA, 63);
    ioWrite(VGA_DAC_WRITE_INDEX, 0xF3); // Index for White
    ioWrite(VGA_DAC_DATA, 63); ioWrite(VGA_DAC_DATA, 63); ioWrite(VGA_DAC_DATA, 63);
    ioWrite(VGA_DAC_WRITE_INDEX, 0xF4); // Index for Dark Gray (for progress bar background)
    ioWrite(VGA_DAC_DATA, 10); ioWrite(VGA_DAC_DATA, 10); ioWrite(VGA_DAC_DATA, 10);
    ioWrite(VGA_DAC_WRITE_INDEX, 0xF5); // Index for Light Gray (for progress bar fill)
    ioWrite(VGA_DAC_DATA, 40); ioWrite(VGA_DAC_DATA, 40); ioWrite(VGA_DAC_DATA, 40);


    // --- Enable Video Output ---
    ioRead(VGA_INPUT_STATUS_1_C); // Read Input Status 1 to reset AC flip-flop
    ioWrite(VGA_ATT_INDEX, 0x10); // Select Mode Control Register (0x10)
    ioWrite(VGA_ATT_DATA, 0x21);  // Set bit 5 (Video Enable) and bit 0 (8/16-bit color mode)

    // --- Clear Sequencer Reset ---
    writeIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x00, 0x03); // Clear Synchronous Reset

    Serial.println("VGA Mode 13h setup complete.");
}

/**
 * @brief Draws a simple color and graphics test pattern.
 */
void drawTestPattern() {
    Serial.println("Drawing test pattern...");
    // Fill with a background color (e.g., black or a dark color)
    fillScreen(0x00); // Black

    // Draw horizontal color bars using grayscale palette
    for (int y = 0; y < SCREEN_HEIGHT / 2; y++) {
        uint8_t color = (y * 255) / (SCREEN_HEIGHT / 2); // Gradient from 0 to 255
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            setPixel(x, y, color);
        }
    }

    // Draw vertical color bars using distinct colors
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        uint8_t color;
        if (x < SCREEN_WIDTH / 3) {
            color = 0xF0; // Light Red
        } else if (x < (SCREEN_WIDTH * 2) / 3) {
            color = 0xF1; // Light Green
        } else {
            color = 0xF2; // Light Blue
        }
        for (int y = SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT; y++) {
            setPixel(x, y, color);
        }
    }

    // Draw a white rectangle in the center
    int rectX = SCREEN_WIDTH / 4;
    int rectY = SCREEN_HEIGHT / 4;
    int rectW = SCREEN_WIDTH / 2;
    int rectH = SCREEN_HEIGHT / 2;
    for (int y = rectY; y < rectY + rectH; y++) {
        for (int x = rectX; x < rectX + rectW; x++) {
            setPixel(x, y, 0xF3); // White
        }
    }

    // Draw a red cross
    for (int i = 0; i < 50; i++) {
        setPixel(SCREEN_WIDTH / 2 - 25 + i, SCREEN_HEIGHT / 2, 0xF0); // Red
        setPixel(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 25 + i, 0xF0); // Red
    }
    Serial.println("Test pattern drawn.");
}

/**
 * @brief Automatically detects the installed VRAM size.
 * Probes memory by writing and reading patterns, temporarily disabling video output.
 * @return The detected VRAM size in bytes.
 */
uint32_t detectVRAMSize() {
    Serial.println("\n--- Detecting VRAM Size ---");
    uint32_t detectedSize = 0;
    uint8_t originalAC10; // To store original Attribute Controller Mode Control Register value

    // Temporarily disable video output to avoid screen corruption during probing
    ioRead(VGA_INPUT_STATUS_1_C); // Reset AC flip-flop
    ioWrite(VGA_ATT_INDEX, 0x10); // Select Mode Control Register
    originalAC10 = ioRead(VGA_ATT_DATA); // Read current value
    ioWrite(VGA_ATT_INDEX, 0x10); // Select Mode Control Register again
    ioWrite(VGA_ATT_DATA, originalAC10 & ~0x20); // Clear bit 5 (Video Enable)

    // Test in 64KB (0x10000 bytes) chunks
    for (uint32_t currentAddr = VRAM_START_ADDRESS; currentAddr < VRAM_START_ADDRESS + MAX_VRAM_SIZE; currentAddr += 0x10000) {
        uint8_t testPattern = (uint8_t)((currentAddr >> 8) & 0xFF); // Unique pattern for each 64KB block

        // Write pattern to start and end of the 64KB block
        memWrite(currentAddr, testPattern);
        memWrite(currentAddr + 0xFFF0, ~testPattern); // Write inverted pattern near end of block

        // Read back and verify
        if (memRead(currentAddr) == testPattern && memRead(currentAddr + 0xFFF0) == ~testPattern) {
            detectedSize += 0x10000;
            Serial.print("Detected 0x"); Serial.print(detectedSize, HEX); Serial.println(" bytes...");
        } else {
            Serial.print("VRAM detection stopped at 0x"); Serial.print(currentAddr, HEX); Serial.println(" (read error).");
            break; // Stop if a block fails
        }
    }

    // Re-enable video output
    ioRead(VGA_INPUT_STATUS_1_C); // Reset AC flip-flop
    ioWrite(VGA_ATT_INDEX, 0x10); // Select Mode Control Register
    ioWrite(VGA_ATT_DATA, originalAC10); // Restore original value (with Video Enable)

    Serial.print("--- Detected VRAM Size: ");
    Serial.print(detectedSize / 1024);
    Serial.println(" KB ---");
    return detectedSize;
}

// Global variable to store detected VRAM size
uint32_t detectedVRAMSize = 0;
char vramTestResults[128]; // Smaller, as we're reporting overall status + first error

/**
 * @brief Performs a comprehensive VRAM read/write test.
 * Updates a progress bar on the VGA display.
 * @param vramSize The size of VRAM to test in bytes.
 */
void runVRAMTest(uint32_t vramSize) {
    Serial.println("\n--- Starting VRAM Test ---");
    strcpy(vramTestResults, "VRAM Test: PASS\n"); // Assume pass initially

    uint8_t readValue;
    bool testFailed = false;
    uint32_t firstErrorAddr = 0;
    char buffer[64];

    // Progress bar area (adjust as needed)
    int progressBarX = 10;
    int progressBarY = SCREEN_HEIGHT - 30;
    int progressBarW = SCREEN_WIDTH - 20;
    int progressBarH = 10;

    // Draw progress bar outline
    fillRect(progressBarX, progressBarY, progressBarW, progressBarH, 0xF4); // Dark Gray background

    Serial.print("Testing 0x"); Serial.print(VRAM_START_ADDRESS, HEX);
    Serial.print(" to 0x"); Serial.print(VRAM_START_ADDRESS + vramSize - 1, HEX);
    Serial.print(" ("); Serial.print(vramSize / 1024); Serial.println("KB)");

    // --- Phase 1: Walking 1s test ---
    Serial.println("Phase 1: Walking 1s test...");
    for (int bit = 0; bit < 8; bit++) {
        uint8_t pattern = (1 << bit);
        for (uint32_t addr = VRAM_START_ADDRESS; addr < VRAM_START_ADDRESS + vramSize; addr++) {
            memWrite(addr, pattern);
        }
        for (uint32_t addr = VRAM_START_ADDRESS; addr < VRAM_START_ADDRESS + vramSize; addr++) {
            readValue = memRead(addr);
            if (readValue != pattern) {
                testFailed = true;
                firstErrorAddr = addr;
                sprintf(buffer, "FAIL! Addr 0x%05lX (W:0x%02X R:0x%02X)\n", addr, pattern, readValue);
                strcpy(vramTestResults, buffer);
                Serial.print(buffer);
                goto end_test; // Exit all loops on first error
            }
            // Update progress bar
            int progress = map(addr - VRAM_START_ADDRESS, 0, vramSize, 0, progressBarW);
            fillRect(progressBarX, progressBarY, progress, progressBarH, 0xF5); // Light Gray fill
        }
    }

    // --- Phase 2: Walking 0s test ---
    Serial.println("Phase 2: Walking 0s test...");
    for (int bit = 0; bit < 8; bit++) {
        uint8_t pattern = ~(1 << bit); // Inverted walking 1s
        for (uint32_t addr = VRAM_START_ADDRESS; addr < VRAM_START_ADDRESS + vramSize; addr++) {
            memWrite(addr, pattern);
        }
        for (uint32_t addr = VRAM_START_ADDRESS; addr < VRAM_START_ADDRESS + vramSize; addr++) {
            readValue = memRead(addr);
            if (readValue != pattern) {
                testFailed = true;
                firstErrorAddr = addr;
                sprintf(buffer, "FAIL! Addr 0x%05lX (W:0x%02X R:0x%02X)\n", addr, pattern, readValue);
                strcpy(vramTestResults, buffer);
                Serial.print(buffer);
                goto end_test; // Exit all loops on first error
            }
            // Update progress bar
            int progress = map(addr - VRAM_START_ADDRESS, 0, vramSize, 0, progressBarW);
            fillRect(progressBarX, progressBarY, progress, progressBarH, 0xF5); // Light Gray fill
        }
    }

end_test:
    if (testFailed) {
        if (strstr(vramTestResults, "FAIL") == NULL) {
             strcat(vramTestResults, "VRAM Test: FAIL\n");
        }
    } else {
        strcpy(vramTestResults, "VRAM Test: PASS\n");
    }
    // Fill progress bar completely green on success, or red on failure
    fillRect(progressBarX, progressBarY, progressBarW, progressBarH, testFailed ? 0xF0 : 0xF1); // Red or Green
    Serial.println("--- Finished VRAM Test ---");
}

/**
 * @brief Displays the stored VRAM test results and memory map on the VGA screen.
 */
void displayTestResultsOnVGA() {
    Serial.println("Displaying VRAM test results and memory map on VGA...");
    int startY = 10; // Starting Y position for text
    int lineHeight = 10; // 8 pixels for char + 2 pixels spacing

    // Clear a small area for text to be readable over the pattern
    // Clear the top part of the screen where results will be shown
    fillRect(0, 0, SCREEN_WIDTH, 70, 0x00); // Clear top 70 rows with black

    drawString(10, startY, "VGA Memory Map & Test:", 0xF3); // White text
    startY += lineHeight * 2;

    char memMapStr[32];
    sprintf(memMapStr, "VRAM: %lu KB", detectedVRAMSize / 1024);
    drawString(10, startY, memMapStr, 0xF3); // White text
    startY += lineHeight;

    // Display VRAM Test Results
    char tempResults[128];
    strcpy(tempResults, vramTestResults); // Make a copy for strtok
    char* token_vram = strtok(tempResults, "\n");
    while (token_vram != NULL) {
        drawString(10, startY, token_vram, 0x0E); // Yellow for results
        startY += lineHeight;
        token_vram = strtok(NULL, "\n");
    }
    Serial.println("VRAM test results and memory map displayed on VGA.");
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
    digitalWrite(PIN_REFRESH, HIGH);
    pinMode(PIN_REFRESH, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_REFRESH), refreshInterrupt, FALLING);

    // Set AEN (Address Enable) LOW to enable I/O and Memory cycles
    digitalWrite(PIN_AEN, LOW);
    pinMode(PIN_AEN, OUTPUT);

    // Set ALE (Address Latch Enable) HIGH
    digitalWrite(PIN_ALE, HIGH);
    pinMode(PIN_ALE, OUTPUT);

    // IOCHRDY (IO Channel Ready) is typically an input from the peripheral.
    digitalWrite(PIN_IOCHRDY, HIGH);
    pinMode(PIN_IOCHRDY, INPUT_PULLUP);

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

    Serial.print("Arduino ready. Starting VGA setup and VRAM test...\n");

    // 1. Set up VGA video mode
    setVGAMode13h();

    // 2. Draw the color and graphics test pattern first
    drawTestPattern();

    // 3. Automatically detect VRAM size
    detectedVRAMSize = detectVRAMSize();

    // 4. Run the VRAM test using the detected size
    runVRAMTest(detectedVRAMSize);

    // 5. Display VRAM test results and memory map on the VGA screen
    displayTestResultsOnVGA();

    Serial.println("\n--- Automatic VRAM Test and Display Complete ---");
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
}
