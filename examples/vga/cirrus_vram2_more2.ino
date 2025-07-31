// Extended Cirrus Logic VGA registers and banking implementation
// Add these definitions and functions to your existing code

// --- Extended Cirrus Logic Register Definitions ---

// Cirrus Logic Extended Sequencer Registers
#define CL_SEQ_EXT_UNLOCK       0x06    // Unlock key for extended registers
#define CL_SEQ_EXT_UNLOCK_KEY   0x12    // Magic unlock value
#define CL_SEQ_CIRRUS_ID        0x07    // Cirrus Logic ID register
#define CL_SEQ_SCRATCH_PAD      0x0A    // Scratch pad register for testing
#define CL_SEQ_VCLK_NUMERATOR   0x0E    // Video clock numerator
#define CL_SEQ_VCLK_DENOMINATOR 0x1E    // Video clock denominator

// Cirrus Logic Extended CRTC Registers
#define CL_CRTC_EXT_DISPLAY     0x1B    // Extended display control
#define CL_CRTC_SYNC_ADJUST     0x1C    // Sync adjustment
#define CL_CRTC_WHITE_TUNE      0x1D    // White tune register
#define CL_CRTC_EXT_OVERFLOW    0x1E    // Extended overflow register

// Cirrus Logic Graphics Controller Extensions
#define CL_GFX_BANK_0           0x09    // Memory bank register A
#define CL_GFX_BANK_1           0x0A    // Memory bank register B
#define CL_GFX_EXT_MODE         0x0B    // Extended graphics mode
#define CL_GFX_COLOR_KEY        0x0C    // Color key register (transparency)
#define CL_GFX_COLOR_KEY_MASK   0x0D    // Color key mask
#define CL_GFX_MISC_CTRL        0x0E    // Miscellaneous control
#define CL_GFX_DISPLAY_CTRL     0x0F    // Display control
#define CL_GFX_BLIT_WIDTH_LOW   0x20    // BitBLT width low
#define CL_GFX_BLIT_WIDTH_HIGH  0x21    // BitBLT width high
#define CL_GFX_BLIT_HEIGHT_LOW  0x22    // BitBLT height low
#define CL_GFX_BLIT_HEIGHT_HIGH 0x23    // BitBLT height high

// Hidden DAC Registers (accessed via special sequence)
#define CL_HIDDEN_DAC           0x3C6   // Same as VGA_DAC_PEL_MASK, but with special access

// Cirrus Logic chip identification values
#define CL_ID_CLGD5402          0x88
#define CL_ID_CLGD5420          0x8A
#define CL_ID_CLGD5422          0x8C
#define CL_ID_CLGD5424          0x94
#define CL_ID_CLGD5426          0x90
#define CL_ID_CLGD5428          0x98
#define CL_ID_CLGD5429          0x9C
#define CL_ID_CLGD5430          0xA0
#define CL_ID_CLGD5434          0xA8
#define CL_ID_CLGD5436          0xAC
#define CL_ID_CLGD5446          0xB8
#define CL_ID_CLGD5480          0xBC

// Banking modes
#define CL_BANK_SINGLE          0       // Single bank mode (64KB window)
#define CL_BANK_DUAL            1       // Dual bank mode (2x32KB windows)
#define CL_BANK_LINEAR          2       // Linear mode (if supported)

// --- Global Variables for Cirrus Logic Support ---
struct CirrusLogicInfo {
    uint8_t chipId;
    const char* chipName;
    uint32_t maxVRAM;
    bool supportsLinear;
    bool supportsBitBLT;
    bool supportsHiColor;
    uint8_t currentBank;
    uint8_t bankingMode;
    bool isExtended;
} cirrusInfo;

// --- Cirrus Logic Detection and Initialization Functions ---

/**
 * @brief Unlocks Cirrus Logic extended registers
 * @return true if unlock successful, false otherwise
 */
bool unlockCirrusExtended() {
    // Write unlock key to sequencer register 0x06
    writeIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, CL_SEQ_EXT_UNLOCK, CL_SEQ_EXT_UNLOCK_KEY);
    
    // Verify unlock by reading back
    uint8_t unlockValue = readIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, CL_SEQ_EXT_UNLOCK);
    return (unlockValue == CL_SEQ_EXT_UNLOCK_KEY);
}

/**
 * @brief Locks Cirrus Logic extended registers
 */
void lockCirrusExtended() {
    writeIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, CL_SEQ_EXT_UNLOCK, 0x00);
}

/**
 * @brief Gets the chip name string from chip ID
 * @param chipId The Cirrus Logic chip ID
 * @return Pointer to chip name string
 */
const char* getCirrusChipName(uint8_t chipId) {
    switch (chipId) {
        case CL_ID_CLGD5402: return "CL-GD5402";
        case CL_ID_CLGD5420: return "CL-GD5420";
        case CL_ID_CLGD5422: return "CL-GD5422";
        case CL_ID_CLGD5424: return "CL-GD5424";
        case CL_ID_CLGD5426: return "CL-GD5426";
        case CL_ID_CLGD5428: return "CL-GD5428";
        case CL_ID_CLGD5429: return "CL-GD5429";
        case CL_ID_CLGD5430: return "CL-GD5430";
        case CL_ID_CLGD5434: return "CL-GD5434";
        case CL_ID_CLGD5436: return "CL-GD5436";
        case CL_ID_CLGD5446: return "CL-GD5446";
        case CL_ID_CLGD5480: return "CL-GD5480";
        default: return "Unknown";
    }
}

/**
 * @brief Detects and identifies the Cirrus Logic chip
 * @return true if Cirrus Logic chip detected, false otherwise
 */
bool detectCirrusLogic() {
    Serial.println("Detecting Cirrus Logic chip...");
    
    // Initialize cirrus info structure
    memset(&cirrusInfo, 0, sizeof(cirrusInfo));
    cirrusInfo.currentBank = 0;
    cirrusInfo.bankingMode = CL_BANK_SINGLE;
    
    // Try to unlock extended registers
    if (!unlockCirrusExtended()) {
        Serial.println("Failed to unlock Cirrus extended registers");
        return false;
    }
    
    // Read chip ID from sequencer register 0x07
    cirrusInfo.chipId = readIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, CL_SEQ_CIRRUS_ID);
    cirrusInfo.chipName = getCirrusChipName(cirrusInfo.chipId);
    
    // Verify it's a valid Cirrus Logic chip
    if (cirrusInfo.chipId < 0x80) {
        Serial.println("No Cirrus Logic chip detected");
        lockCirrusExtended();
        return false;
    }
    
    // Set chip capabilities based on ID
    switch (cirrusInfo.chipId) {
        case CL_ID_CLGD5402:
            cirrusInfo.maxVRAM = 256 * 1024;    // 256KB
            cirrusInfo.supportsLinear = false;
            cirrusInfo.supportsBitBLT = false;
            cirrusInfo.supportsHiColor = false;
            break;
        case CL_ID_CLGD5420:
        case CL_ID_CLGD5422:
            cirrusInfo.maxVRAM = 512 * 1024;    // 512KB
            cirrusInfo.supportsLinear = false;
            cirrusInfo.supportsBitBLT = false;
            cirrusInfo.supportsHiColor = false;
            break;
        case CL_ID_CLGD5424:
        case CL_ID_CLGD5426:
        case CL_ID_CLGD5428:
            cirrusInfo.maxVRAM = 1024 * 1024;   // 1MB
            cirrusInfo.supportsLinear = true;
            cirrusInfo.supportsBitBLT = true;
            cirrusInfo.supportsHiColor = true;
            break;
        case CL_ID_CLGD5429:
        case CL_ID_CLGD5430:
        case CL_ID_CLGD5434:
        case CL_ID_CLGD5436:
            cirrusInfo.maxVRAM = 2 * 1024 * 1024; // 2MB
            cirrusInfo.supportsLinear = true;
            cirrusInfo.supportsBitBLT = true;
            cirrusInfo.supportsHiColor = true;
            break;
        case CL_ID_CLGD5446:
        case CL_ID_CLGD5480:
            cirrusInfo.maxVRAM = 4 * 1024 * 1024; // 4MB
            cirrusInfo.supportsLinear = true;
            cirrusInfo.supportsBitBLT = true;
            cirrusInfo.supportsHiColor = true;
            break;
        default:
            cirrusInfo.maxVRAM = 1024 * 1024;   // Default to 1MB
            cirrusInfo.supportsLinear = false;
            cirrusInfo.supportsBitBLT = false;
            cirrusInfo.supportsHiColor = false;
            break;
    }
    
    cirrusInfo.isExtended = true;
    
    Serial.print("Detected: ");
    Serial.print(cirrusInfo.chipName);
    Serial.print(" (ID: 0x");
    Serial.print(cirrusInfo.chipId, HEX);
    Serial.print("), Max VRAM: ");
    Serial.print(cirrusInfo.maxVRAM / 1024);
    Serial.println("KB");
    
    return true;
}

/**
 * @brief Sets the memory bank for accessing VRAM beyond 64KB
 * @param bank Bank number (0-255, depending on chip and VRAM size)
 */
void setCirrusBank(uint8_t bank) {
    if (!cirrusInfo.isExtended) {
        Serial.println("Warning: Cirrus extended mode not initialized");
        return;
    }
    
    // Ensure extended registers are unlocked
    if (!unlockCirrusExtended()) {
        Serial.println("Error: Cannot unlock Cirrus extended registers for banking");
        return;
    }
    
    switch (cirrusInfo.bankingMode) {
        case CL_BANK_SINGLE:
            // Single bank mode: both read and write banks set to same value
            writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_BANK_0, bank);
            writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_BANK_1, bank);
            break;
            
        case CL_BANK_DUAL:
            // Dual bank mode: separate read/write banks (bank parameter is read bank)
            writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_BANK_0, bank);      // Read bank
            writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_BANK_1, bank);      // Write bank (same for simplicity)
            break;
            
        case CL_BANK_LINEAR:
            // Linear mode: banking not needed, but we'll store the value anyway
            break;
    }
    
    cirrusInfo.currentBank = bank;
}

/**
 * @brief Gets the current memory bank
 * @return Current bank number
 */
uint8_t getCirrusBank() {
    return cirrusInfo.currentBank;
}

/**
 * @brief Sets the banking mode
 * @param mode Banking mode (CL_BANK_SINGLE, CL_BANK_DUAL, or CL_BANK_LINEAR)
 */
void setCirrusBankingMode(uint8_t mode) {
    if (!cirrusInfo.isExtended) return;
    
    if (!unlockCirrusExtended()) return;
    
    // Read current extended mode register
    uint8_t extMode = readIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_EXT_MODE);
    
    switch (mode) {
        case CL_BANK_SINGLE:
            extMode &= ~0x01;  // Clear bit 0 for single bank
            break;
        case CL_BANK_DUAL:
            extMode |= 0x01;   // Set bit 0 for dual bank
            break;
        case CL_BANK_LINEAR:
            if (cirrusInfo.supportsLinear) {
                extMode |= 0x02;   // Set bit 1 for linear mode
            } else {
                Serial.println("Linear mode not supported on this chip");
                return;
            }
            break;
    }
    
    writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_EXT_MODE, extMode);
    cirrusInfo.bankingMode = mode;
}

/**
 * @brief Enhanced memory write with automatic banking
 * @param address Physical memory address
 * @param data Data to write
 */
void cirrusMemWrite(uint32_t address, uint8_t data) {
    if (!cirrusInfo.isExtended || cirrusInfo.bankingMode == CL_BANK_LINEAR) {
        memWrite(address, data);
        return;
    }
    
    // Calculate bank and offset for banked access
    if (address >= VRAM_START_ADDRESS) {
        uint32_t offset = address - VRAM_START_ADDRESS;
        uint8_t requiredBank = offset / 65536;  // 64KB banks
        uint32_t bankOffset = offset % 65536;
        
        // Switch bank if necessary
        if (requiredBank != cirrusInfo.currentBank) {
            setCirrusBank(requiredBank);
        }
        
        // Write to banked address
        memWrite(VRAM_START_ADDRESS + bankOffset, data);
    } else {
        memWrite(address, data);
    }
}

/**
 * @brief Enhanced memory read with automatic banking
 * @param address Physical memory address
 * @return Data read from memory
 */
uint8_t cirrusMemRead(uint32_t address) {
    if (!cirrusInfo.isExtended || cirrusInfo.bankingMode == CL_BANK_LINEAR) {
        return memRead(address);
    }
    
    // Calculate bank and offset for banked access
    if (address >= VRAM_START_ADDRESS) {
        uint32_t offset = address - VRAM_START_ADDRESS;
        uint8_t requiredBank = offset / 65536;  // 64KB banks
        uint32_t bankOffset = offset % 65536;
        
        // Switch bank if necessary
        if (requiredBank != cirrusInfo.currentBank) {
            setCirrusBank(requiredBank);
        }
        
        // Read from banked address
        return memRead(VRAM_START_ADDRESS + bankOffset);
    } else {
        return memRead(address);
    }
}

/**
 * @brief Enhanced setPixel function using Cirrus banking
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Color value
 */
void cirrusSetPixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        uint32_t address = VRAM_START_ADDRESS + (uint32_t)y * SCREEN_WIDTH + x;
        cirrusMemWrite(address, color);
    }
}

/**
 * @brief Tests memory banking functionality
 */
void testCirrusBanking() {
    if (!cirrusInfo.isExtended) {
        Serial.println("Cirrus extended mode not available for banking test");
        return;
    }
    
    Serial.println("Testing Cirrus Logic memory banking...");
    
    // Test pattern for each bank
    uint8_t testPattern = 0xAA;
    uint8_t readValue;
    bool bankingWorks = true;
    
    // Test first few banks
    for (uint8_t bank = 0; bank < 4 && bank < (cirrusInfo.maxVRAM / 65536); bank++) {
        Serial.print("Testing bank ");
        Serial.println(bank);
        
        setCirrusBank(bank);
        
        // Write test pattern to start of bank
        memWrite(VRAM_START_ADDRESS, testPattern + bank);
        
        // Write different pattern to end of bank
        memWrite(VRAM_START_ADDRESS + 0xFFFF, ~(testPattern + bank));
        
        // Read back and verify
        readValue = memRead(VRAM_START_ADDRESS);
        if (readValue != (testPattern + bank)) {
            Serial.print("Bank ");
            Serial.print(bank);
            Serial.print(" test failed at start: wrote 0x");
            Serial.print(testPattern + bank, HEX);
            Serial.print(", read 0x");
            Serial.println(readValue, HEX);
            bankingWorks = false;
        }
        
        readValue = memRead(VRAM_START_ADDRESS + 0xFFFF);
        if (readValue != ~(testPattern + bank)) {
            Serial.print("Bank ");
            Serial.print(bank);
            Serial.print(" test failed at end: wrote 0x");
            Serial.print(~(testPattern + bank), HEX);
            Serial.print(", read 0x");
            Serial.println(readValue, HEX);
            bankingWorks = false;
        }
    }
    
    // Reset to bank 0
    setCirrusBank(0);
    
    if (bankingWorks) {
        Serial.println("Cirrus Logic banking test PASSED");
    } else {
        Serial.println("Cirrus Logic banking test FAILED");
    }
}

/**
 * @brief Enhanced VGA Mode 13h setup with Cirrus Logic optimizations
 */
void setCirrusVGAMode13h() {
    Serial.println("Setting Cirrus Logic VGA Mode 13h...");
    
    // Ensure extended registers are unlocked
    if (cirrusInfo.isExtended) {
        unlockCirrusExtended();
    }
    
    // Call the standard VGA Mode 13h setup
    setVGAMode13h();
    
    // Apply Cirrus Logic specific optimizations
    if (cirrusInfo.isExtended) {
        // Set up banking mode
        setCirrusBankingMode(CL_BANK_SINGLE);
        setCirrusBank(0);
        
        // Optimize memory timing if supported
        if (cirrusInfo.chipId >= CL_ID_CLGD5424) {
            // Enable extended features for newer chips
            uint8_t extMode = readIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_EXT_MODE);
            extMode |= 0x04;  // Enable enhanced memory timing
            writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_EXT_MODE, extMode);
        }
        
        // Set up display control for better compatibility
        writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_DISPLAY_CTRL, 0x00);
        
        Serial.println("Cirrus Logic optimizations applied");
    }
    
    Serial.println("Cirrus Logic VGA Mode 13h setup complete");
}

/**
 * @brief Enhanced VRAM detection using Cirrus Logic capabilities
 * @return Detected VRAM size in bytes
 */
uint32_t detectCirrusVRAM() {
    Serial.println("\n--- Detecting Cirrus Logic VRAM Size ---");
    
    if (!cirrusInfo.isExtended) {
        Serial.println("Using standard VRAM detection");
        return detectVRAMSize();
    }
    
    uint32_t detectedSize = 0;
    uint8_t originalBank = cirrusInfo.currentBank;
    
    // Test each possible bank
    uint32_t maxBanks = cirrusInfo.maxVRAM / 65536;  // 64KB per bank
    
    for (uint32_t bank = 0; bank < maxBanks; bank++) {
        setCirrusBank(bank);
        
        uint8_t testPattern = (uint8_t)(0xA5 + bank);
        
        // Write test pattern to start of bank
        memWrite(VRAM_START_ADDRESS, testPattern);
        memWrite(VRAM_START_ADDRESS + 0x1000, ~testPattern);
        
        // Read back and verify
        if (memRead(VRAM_START_ADDRESS) == testPattern && 
            memRead(VRAM_START_ADDRESS + 0x1000) == ~testPattern) {
            detectedSize += 65536;  // Add 64KB for this bank
            Serial.print("Bank ");
            Serial.print(bank);
            Serial.println(" detected (64KB)");
        } else {
            Serial.print("Bank ");
            Serial.print(bank);
            Serial.println(" not responding");
            break;
        }
    }
    
    // Restore original bank
    setCirrusBank(originalBank);
    
    Serial.print("--- Detected Cirrus VRAM Size: ");
    Serial.print(detectedSize / 1024);
    Serial.println(" KB ---");
    
    return detectedSize;
}

/**
 * @brief Print detailed Cirrus Logic information
 */
void printCirrusInfo() {
    if (!cirrusInfo.isExtended) {
        Serial.println("No Cirrus Logic chip detected or extended mode not enabled");
        return;
    }
    
    Serial.println("\n--- Cirrus Logic Information ---");
    Serial.print("Chip: ");
    Serial.print(cirrusInfo.chipName);
    Serial.print(" (ID: 0x");
    Serial.print(cirrusInfo.chipId, HEX);
    Serial.println(")");
    
    Serial.print("Max VRAM: ");
    Serial.print(cirrusInfo.maxVRAM / 1024);
    Serial.println(" KB");
    
    Serial.print("Current Bank: ");
    Serial.println(cirrusInfo.currentBank);
    
    Serial.print("Banking Mode: ");
    switch (cirrusInfo.bankingMode) {
        case CL_BANK_SINGLE: Serial.println("Single Bank"); break;
        case CL_BANK_DUAL: Serial.println("Dual Bank"); break;
        case CL_BANK_LINEAR: Serial.println("Linear"); break;
        default: Serial.println("Unknown"); break;
    }
    
    Serial.print("Supports Linear Mode: ");
    Serial.println(cirrusInfo.supportsLinear ? "Yes" : "No");
    
    Serial.print("Supports BitBLT: ");
    Serial.println(cirrusInfo.supportsBitBLT ? "Yes" : "No");
    
    Serial.print("Supports Hi-Color: ");
    Serial.println(cirrusInfo.supportsHiColor ? "Yes" : "No");
    
    Serial.println("--- End Cirrus Logic Information ---\n");
}

// --- Updated Setup Integration ---

/**
 * @brief Enhanced setup function with Cirrus Logic support
 * Add this to replace your existing setup() function
 */
void setupWithCirrus() {
    Serial.begin(1000000);
    
    // Standard hardware initialization (same as before)
    PORT_ADDR0 = 0x00;
    PORT_ADDR8 = 0x00;
    PORT_ADDR16 = 0x00;
    DDR_ADDR0 = 0xFF;
    DDR_ADDR8 = 0xFF;
    DDR_ADDR16 = 0xFF;
    
    digitalWrite(PIN_MEMW, HIGH);
    digitalWrite(PIN_MEMR, HIGH);
    digitalWrite(PIN_IOW, HIGH);
    digitalWrite(PIN_IOR, HIGH);
    
    digitalWrite(PIN_REFRESH, HIGH);
    pinMode(PIN_REFRESH, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_REFRESH), refreshInterrupt, FALLING);
    
    digitalWrite(PIN_AEN, LOW);
    pinMode(PIN_AEN, OUTPUT);
    digitalWrite(PIN_ALE, HIGH);
    pinMode(PIN_ALE, OUTPUT);
    digitalWrite(PIN_IOCHRDY, HIGH);
    pinMode(PIN_IOCHRDY, INPUT_PULLUP);
    
    pinMode(PIN_MEMW, OUTPUT);
    pinMode(PIN_MEMR, OUTPUT);
    pinMode(PIN_IOW, OUTPUT);
    pinMode(PIN_IOR, OUTPUT);
    
    // Reset sequence
    digitalWrite(PIN_RESET, HIGH);
    pinMode(PIN_RESET, OUTPUT);
    delay(100);
    digitalWrite(PIN_RESET, LOW);
    delay(1000);
    
    // Re-initialize after reset
    PORT_ADDR0 = 0x00;
    PORT_ADDR8 = 0x00;
    PORT_ADDR16 = 0x00;
    DDR_ADDR0 = 0xFF;
    DDR_ADDR8 = 0xFF;
    DDR_ADDR16 = 0xFF;
    
    Serial.println("Arduino ready. Detecting and initializing Cirrus Logic VGA...");
    
    // 1. Detect Cirrus Logic chip
    if (detectCirrusLogic()) {
        printCirrusInfo();
        
        // 2. Set up enhanced VGA mode with Cirrus optimizations
        setCirrusVGAMode13h();
        
        // 3. Test banking functionality
        testCirrusBanking();
        
        // 4. Detect VRAM using Cirrus capabilities
        detectedVRAMSize = detectCirrusVRAM();
        
    } else {
        Serial.println("No Cirrus Logic chip detected, using standard VGA mode");
        setVGAMode13h();
        detectedVRAMSize = detectVRAMSize();
    }
    
    // 5. Draw test pattern
    drawTestPattern();
    
    // 6. Run VRAM test
    if (cirrusInfo.isExtended) {
        runCirrusVRAMTest(detectedVRAMSize);
    } else {
        runVRAMTest(detectedVRAMSize);
    }
    
    // 7. Display results
    displayTestResultsOnVGA();
    
    Serial.println("\n--- Enhanced Cirrus Logic VGA Test Complete ---");
    Serial.println("Additional commands: 'c' - Cirrus info, 'b' - Set bank, 't' - Test banking");
}

/**
 * @brief Enhanced VRAM test using Cirrus Logic banking
 * @param vramSize Size of VRAM to test
 */
void runCirrusVRAMTest(uint32_t vramSize) {
    Serial.println("\n--- Starting Enhanced Cirrus VRAM Test ---");
    strcpy(vramTestResults, "Cirrus VRAM Test: PASS\n");
    
    bool testFailed = false;
    uint32_t firstErrorAddr = 0;
    char buffer[64];
    uint8_t originalBank = cirrusInfo.currentBank;
    
    // Progress bar setup
    int progressBarX = 10;
    int progressBarY = SCREEN_HEIGHT - 30;
    int progressBarW = SCREEN_WIDTH - 20;
    int progressBarH = 10;
    fillRect(progressBarX, progressBarY, progressBarW, progressBarH, 0xF4);
    
    Serial.print("Testing ");
    Serial.print(vramSize / 1024);
    Serial.println("KB using Cirrus banking...");
    
    // Test each bank individually
    uint32_t banksToTest = vramSize / 65536;  // 64KB per bank
    if (vramSize % 65536) banksToTest++;     // Round up for partial banks
    
    for (uint32_t bank = 0; bank < banksToTest && !testFailed; bank++) {
        Serial.print("Testing bank ");
        Serial.print(bank);
        Serial.print(" (");
        Serial.print(bank * 64);
        Serial.print("-");
        Serial.print((bank + 1) * 64 - 1);
        Serial.println("KB)...");
        
        setCirrusBank(bank);
        
        uint32_t bankSize = min(65536UL, vramSize - (bank * 65536));
        
        // Walking 1s test for this bank
        for (int bit = 0; bit < 8 && !testFailed; bit++) {
            uint8_t pattern = (1 << bit);
            
            // Write pattern to entire bank
            for (uint32_t offset = 0; offset < bankSize; offset += 256) {
                uint32_t addr = VRAM_START_ADDRESS + offset;
                memWrite(addr, pattern);
            }
            
            // Verify pattern in entire bank
            for (uint32_t offset = 0; offset < bankSize; offset += 256) {
                uint32_t addr = VRAM_START_ADDRESS + offset;
                uint8_t readValue = memRead(addr);
                if (readValue != pattern) {
                    testFailed = true;
                    firstErrorAddr = (bank * 65536) + offset;
                    sprintf(buffer, "FAIL! Bank %lu Addr 0x%05lX (W:0x%02X R:0x%02X)\n", 
                           bank, firstErrorAddr, pattern, readValue);
                    strcpy(vramTestResults, buffer);
                    Serial.print(buffer);
                    break;
                }
            }
            
            // Update progress bar
            int totalProgress = (bank * 8 + bit + 1);
            int maxProgress = banksToTest * 8;
            int progress = map(totalProgress, 0, maxProgress, 0, progressBarW);
            fillRect(progressBarX, progressBarY, progress, progressBarH, 0xF5);
        }
        
        // Walking 0s test for this bank
        for (int bit = 0; bit < 8 && !testFailed; bit++) {
            uint8_t pattern = ~(1 << bit);
            
            // Write pattern to entire bank
            for (uint32_t offset = 0; offset < bankSize; offset += 256) {
                uint32_t addr = VRAM_START_ADDRESS + offset;
                memWrite(addr, pattern);
            }
            
            // Verify pattern in entire bank
            for (uint32_t offset = 0; offset < bankSize; offset += 256) {
                uint32_t addr = VRAM_START_ADDRESS + offset;
                uint8_t readValue = memRead(addr);
                if (readValue != pattern) {
                    testFailed = true;
                    firstErrorAddr = (bank * 65536) + offset;
                    sprintf(buffer, "FAIL! Bank %lu Addr 0x%05lX (W:0x%02X R:0x%02X)\n", 
                           bank, firstErrorAddr, pattern, readValue);
                    strcpy(vramTestResults, buffer);
                    Serial.print(buffer);
                    break;
                }
            }
        }
    }
    
    // Restore original bank
    setCirrusBank(originalBank);
    
    if (!testFailed) {
        strcpy(vramTestResults, "Cirrus VRAM Test: PASS\n");
    }
    
    // Fill progress bar with result color
    fillRect(progressBarX, progressBarY, progressBarW, progressBarH, testFailed ? 0xF0 : 0xF1);
    Serial.println("--- Finished Enhanced Cirrus VRAM Test ---");
}

/**
 * @brief Advanced Cirrus Logic features demonstration
 */
void demonstrateCirrusFeatures() {
    if (!cirrusInfo.isExtended) {
        Serial.println("Cirrus extended features not available");
        return;
    }
    
    Serial.println("Demonstrating Cirrus Logic advanced features...");
    
    // Test different banking modes
    if (cirrusInfo.supportsLinear) {
        Serial.println("Testing linear mode...");
        setCirrusBankingMode(CL_BANK_LINEAR);
        
        // Draw a pattern that spans multiple banks
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                uint32_t address = VRAM_START_ADDRESS + (uint32_t)y * SCREEN_WIDTH + x;
                if (address < VRAM_START_ADDRESS + detectedVRAMSize) {
                    cirrusMemWrite(address, (x + y) & 0xFF);
                }
            }
        }
        
        delay(2000);
        
        // Switch back to banked mode
        setCirrusBankingMode(CL_BANK_SINGLE);
        Serial.println("Switched back to banked mode");
    }
    
    // Demonstrate color key (transparency) if supported
    if (cirrusInfo.chipId >= CL_ID_CLGD5424) {
        Serial.println("Setting up color key transparency...");
        unlockCirrusExtended();
        
        // Set color key to black (0x00)
        writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_COLOR_KEY, 0x00);
        writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_COLOR_KEY_MASK, 0xFF);
        
        // Enable color key
        uint8_t miscCtrl = readIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_MISC_CTRL);
        miscCtrl |= 0x01;  // Enable color key
        writeIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, CL_GFX_MISC_CTRL, miscCtrl);
        
        Serial.println("Color key enabled (black pixels will be transparent)");
    }
}

/**
 * @brief Enhanced loop function with Cirrus Logic commands
 * Add this to replace your existing loop() function or integrate the new commands
 */
void loopWithCirrus() {
    if (Serial.available()) {
        command = Serial.read();
        
        // Original commands (i, o, r, w) remain the same...
        if (command == 'i') {
            while (Serial.available() < 2) {}
            address = (Serial.read() << 8);
            address |= Serial.read();
            Serial.print("I/O Read 0x"); Serial.print(address, HEX); Serial.print(": 0x");
            Serial.println(ioRead(address), HEX);
        }

        if (command == 'o') {
            while (Serial.available() < 3) {}
            address = (Serial.read() << 8);
            address |= Serial.read();
            data = Serial.read();
            ioWrite(address, data);
            Serial.print("I/O Write 0x"); Serial.print(address, HEX); Serial.print(" <= 0x");
            Serial.println(data, HEX);
        }

        if (command == 'r') {
            while (Serial.available() < 3) {}
            address = ((uint32_t) Serial.read() << 16UL);
            address |= (Serial.read() << 8);
            address |= Serial.read();
            
            if (cirrusInfo.isExtended) {
                Serial.print("Cirrus Mem Read 0x"); Serial.print(address, HEX); Serial.print(": 0x");
                Serial.println(cirrusMemRead(address), HEX);
            } else {
                Serial.print("Mem Read 0x"); Serial.print(address, HEX); Serial.print(": 0x");
                Serial.println(memRead(address), HEX);
            }
        }

        if (command == 'w') {
            while (Serial.available() < 4) {}
            address = ((uint32_t)Serial.read() << 16UL);
            address |= (Serial.read() << 8);
            address |= Serial.read();
            data = Serial.read();
            
            if (cirrusInfo.isExtended) {
                cirrusMemWrite(address, data);
                Serial.print("Cirrus Mem Write 0x"); Serial.print(address, HEX); Serial.print(" <= 0x");
                Serial.println(data, HEX);
            } else {
                memWrite(address, data);
                Serial.print("Mem Write 0x"); Serial.print(address, HEX); Serial.print(" <= 0x");
                Serial.println(data, HEX);
            }
        }
        
        // New Cirrus Logic specific commands
        if (command == 'c') {
            // Display Cirrus Logic information
            printCirrusInfo();
        }
        
        if (command == 'b') {
            // Set memory bank
            if (!cirrusInfo.isExtended) {
                Serial.println("Cirrus extended features not available");
            } else {
                while (Serial.available() < 1) {}
                uint8_t bank = Serial.read();
                setCirrusBank(bank);
                Serial.print("Set Cirrus bank to: ");
                Serial.println(bank);
            }
        }
        
        if (command == 't') {
            // Test banking functionality
            testCirrusBanking();
        }
        
        if (command == 'd') {
            // Demonstrate advanced features
            demonstrateCirrusFeatures();
        }
        
        if (command == 'm') {
            // Switch banking mode
            if (!cirrusInfo.isExtended) {
                Serial.println("Cirrus extended features not available");
            } else {
                while (Serial.available() < 1) {}
                uint8_t mode = Serial.read();
                if (mode <= CL_BANK_LINEAR) {
                    setCirrusBankingMode(mode);
                    Serial.print("Set banking mode to: ");
                    switch (mode) {
                        case CL_BANK_SINGLE: Serial.println("Single Bank"); break;
                        case CL_BANK_DUAL: Serial.println("Dual Bank"); break;
                        case CL_BANK_LINEAR: Serial.println("Linear"); break;
                    }
                } else {
                    Serial.println("Invalid banking mode (0=Single, 1=Dual, 2=Linear)");
                }
            }
        }
        
        if (command == 'v') {
            // Re-run VRAM test
            Serial.println("Re-running VRAM test...");
            if (cirrusInfo.isExtended) {
                runCirrusVRAMTest(detectedVRAMSize);
            } else {
                runVRAMTest(detectedVRAMSize);
            }
            displayTestResultsOnVGA();
        }
        
        if (command == 'h') {
            // Help command
            Serial.println("\n--- Available Commands ---");
            Serial.println("Original commands:");
            Serial.println("  i - I/O Read (2 bytes: address)");
            Serial.println("  o - I/O Write (3 bytes: address, data)");
            Serial.println("  r - Memory Read (3 bytes: address)");
            Serial.println("  w - Memory Write (4 bytes: address, data)");
            Serial.println("\nCirrus Logic commands:");
            Serial.println("  c - Display Cirrus info");
            Serial.println("  b - Set bank (1 byte: bank number)");
            Serial.println("  t - Test banking");
            Serial.println("  d - Demonstrate advanced features");
            Serial.println("  m - Set banking mode (1 byte: 0=Single, 1=Dual, 2=Linear)");
            Serial.println("  v - Re-run VRAM test");
            Serial.println("  h - Show this help");
            Serial.println("--- End Commands ---\n");
        }
    }
}

// --- Additional Utility Functions ---

/**
 * @brief Safe I/O write with timeout protection
 * @param address I/O address
 * @param data Data to write
 * @return true if successful, false if timeout
 */
bool safeIoWrite(uint32_t address, uint8_t data) {
    PORT_ADDR0 = address & 0xFF;
    PORT_ADDR8 = (address >> 8) & 0xFF;
    PORT_ADDR16 = (address >> 16) & 0xFF;
    PORT_DATA = data;
    DDR_DATA = 0xFF;
    
    digitalWrite(PIN_IOW, LOW);
    
    unsigned long timeout = millis() + 100; // 100ms timeout
    while (!digitalRead(PIN_IOCHRDY) && millis() < timeout) {
        // Wait for ready or timeout
    }
    
    bool success = (millis() < timeout);
    digitalWrite(PIN_IOW, HIGH);
    DDR_DATA = 0x00;
    
    return success;
}

/**
 * @brief Safe I/O read with timeout protection
 * @param address I/O address
 * @param data Pointer to store read data
 * @return true if successful, false if timeout
 */
bool safeIoRead(uint32_t address, uint8_t* data) {
    PORT_ADDR0 = address & 0xFF;
    PORT_ADDR8 = (address >> 8) & 0xFF;
    PORT_ADDR16 = (address >> 16) & 0xFF;
    PORT_DATA = 0xFF;
    DDR_DATA = 0x00;
    
    digitalWrite(PIN_IOR, LOW);
    
    unsigned long timeout = millis() + 100; // 100ms timeout
    while (!digitalRead(PIN_IOCHRDY) && millis() < timeout) {
        // Wait for ready or timeout
    }
    
    bool success = (millis() < timeout);
    if (success) {
        *data = PIN_DATA;
    }
    digitalWrite(PIN_IOR, HIGH);
    
    return success;
}

/**
 * @brief Enhanced indexed register write with error checking
 * @param indexPort Index port address
 * @param dataPort Data port address
 * @param index Register index
 * @param data Data to write
 * @return true if successful, false if error
 */
bool safeWriteIndexedRegister(uint32_t indexPort, uint32_t dataPort, uint8_t index, uint8_t data) {
    if (!safeIoWrite(indexPort, index)) {
        Serial.print("Error writing to index port 0x");
        Serial.println(indexPort, HEX);
        return false;
    }
    
    if (!safeIoWrite(dataPort, data)) {
        Serial.print("Error writing to data port 0x");
        Serial.println(dataPort, HEX);
        return false;
    }
    
    return true;
}

/**
 * @brief Enhanced indexed register read with error checking
 * @param indexPort Index port address
 * @param dataPort Data port address
 * @param index Register index
 * @param data Pointer to store read data
 * @return true if successful, false if error
 */
bool safeReadIndexedRegister(uint32_t indexPort, uint32_t dataPort, uint8_t index, uint8_t* data) {
    if (!safeIoWrite(indexPort, index)) {
        Serial.print("Error writing to index port 0x");
        Serial.println(indexPort, HEX);
        return false;
    }
    
    if (!safeIoRead(dataPort, data)) {
        Serial.print("Error reading from data port 0x");
        Serial.println(dataPort, HEX);
        return false;
    }
    
    return true;
}

/**
 * @brief Verify Cirrus Logic register access
 * @return true if register access is working, false otherwise
 */
bool verifyCirrusRegisterAccess() {
    if (!cirrusInfo.isExtended) return false;
    
    Serial.println("Verifying Cirrus Logic register access...");
    
    // Test scratch pad register (should be read/write)
    uint8_t testValue = 0xA5;
    uint8_t readValue;
    
    if (!unlockCirrusExtended()) {
        Serial.println("Failed to unlock extended registers");
        return false;
    }
    
    // Write test pattern to scratch pad
    if (!safeWriteIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, CL_SEQ_SCRATCH_PAD, testValue)) {
        Serial.println("Failed to write to scratch pad register");
        return false;
    }
    
    // Read back and verify
    if (!safeReadIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, CL_SEQ_SCRATCH_PAD, &readValue)) {
        Serial.println("Failed to read from scratch pad register");
        return false;
    }
    
    if (readValue != testValue) {
        Serial.print("Scratch pad test failed: wrote 0x");
        Serial.print(testValue, HEX);
        Serial.print(", read 0x");
        Serial.println(readValue, HEX);
        return false;
    }
    
    // Test with inverted pattern
    testValue = 0x5A;
    if (!safeWriteIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, CL_SEQ_SCRATCH_PAD, testValue)) {
        return false;
    }
    
    if (!safeReadIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, CL_SEQ_SCRATCH_PAD, &readValue)) {
        return false;
    }
    
    if (readValue != testValue) {
        Serial.print("Scratch pad test 2 failed: wrote 0x");
        Serial.print(testValue, HEX);
        Serial.print(", read 0x");
        Serial.println(readValue, HEX);
        return false;
    }
    
    Serial.println("Cirrus Logic register access verified successfully");
    return true;
}

/**
 * @brief Backup critical VGA registers before testing
 */
struct VGARegisterBackup {
    uint8_t seq_regs[32];
    uint8_t crtc_regs[32];
    uint8_t gfx_regs[16];
    uint8_t att_regs[32];
    bool valid;
} vgaBackup;

void backupVGARegisters() {
    Serial.println("Backing up VGA registers...");
    vgaBackup.valid = false;
    
    // Backup Sequencer registers
    for (int i = 0; i < 32; i++) {
        if (!safeReadIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, i, &vgaBackup.seq_regs[i])) {
            Serial.print("Failed to backup sequencer register ");
            Serial.println(i);
            return;
        }
    }
    
    // Backup CRTC registers
    for (int i = 0; i < 32; i++) {
        if (!safeReadIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, i, &vgaBackup.crtc_regs[i])) {
            Serial.print("Failed to backup CRTC register ");
            Serial.println(i);
            return;
        }
    }
    
    // Backup Graphics Controller registers
    for (int i = 0; i < 16; i++) {
        if (!safeReadIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, i, &vgaBackup.gfx_regs[i])) {
            Serial.print("Failed to backup graphics register ");
            Serial.println(i);
            return;
        }
    }
    
    vgaBackup.valid = true;
    Serial.println("VGA registers backed up successfully");
}

void restoreVGARegisters() {
    if (!vgaBackup.valid) {
        Serial.println("No valid VGA register backup available");
        return;
    }
    
    Serial.println("Restoring VGA registers...");
    
    // Restore Sequencer registers
    for (int i = 0; i < 32; i++) {
        safeWriteIndexedRegister(VGA_SEQ_INDEX, VGA_SEQ_DATA, i, vgaBackup.seq_regs[i]);
    }
    
    // Restore CRTC registers
    for (int i = 0; i < 32; i++) {
        safeWriteIndexedRegister(VGA_CRTC_INDEX_C, VGA_CRTC_DATA_C, i, vgaBackup.crtc_regs[i]);
    }
    
    // Restore Graphics Controller registers
    for (int i = 0; i < 16; i++) {
        safeWriteIndexedRegister(VGA_GFX_INDEX, VGA_GFX_DATA, i, vgaBackup.gfx_regs[i]);
    }
    
    Serial.println("VGA registers restored");
}
