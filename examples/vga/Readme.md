Explanation and Key Changes:
Standardized Indexed Register Access:

I've introduced writeIndexedRegister(indexPort, dataPort, index, data) and readIndexedRegister(indexPort, dataPort, index) functions. These are the standard way to interact with VGA's indexed registers (like those for the Sequencer, Graphics Controller, and CRTC). Your original ioIndexedWrite and ioIndexedRead were a bit different and might not have worked as expected for these specific VGA registers.

runVGARegisterTests() Function:

This new function encapsulates the automatic tests.

Miscellaneous Output Register (MOR) Test: It writes a test value to the MOR (0x3C2) and reads it back from its read-back port (0x3CC) to verify basic I/O functionality.

Sequencer Register Test: It tests writing and reading from the Sequencer Reset Register (0x3C4/0x3C5, Index 0x00) and Clocking Mode Register (0x3C4/0x3C5, Index 0x01).

Graphics Controller Register Test: It tests writing and reading from the Graphics Mode Register (0x3CE/0x3CF, Index 0x05).

CRT Controller Register Test: It tests writing and reading from the Horizontal Total Register (0x3D4/0x3D5, Index 0x00).

For indexed register tests, the original value is read and stored, then the test value is written, read back, and finally, the original value is restored. This prevents unintended side effects on the VGA card's state.

runVRAMTest() Function:

This function performs a simple read/write test on the VRAM. It writes distinct patterns to two consecutive VRAM addresses (starting at 0xA0000) and then reads them back to verify data integrity. It also checks if the first address still holds its original pattern after writing to the second, ensuring memory cells are independent.

Integration into setup():

The runVGARegisterTests() and runVRAMTest() functions are called directly from setup() after the initial reset and setup of the Arduino pins. This ensures the tests run automatically upon startup.

Serial Output:

Extensive Serial.print statements have been added to provide clear feedback on which test is running, the values written and read, and whether each test passed or failed.

Pin Configuration Notes:

I've added comments regarding the PIN_REFRESH and PIN_IOCHRDY pins, emphasizing that they are typically inputs from the VGA card. Ensure your wiring and Arduino pin configuration match this expectation. INPUT_PULLUP is used for PIN_IOCHRDY and PIN_REFRESH to ensure a defined state when not driven by the VGA card.

This extended code provides a solid foundation for automatically testing your CL-GD5422-75QC based VGA card. Remember to double-check your wiring and pin assignments to ensure they match the definitions in the code.
