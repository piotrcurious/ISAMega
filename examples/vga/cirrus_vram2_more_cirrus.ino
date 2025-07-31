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
