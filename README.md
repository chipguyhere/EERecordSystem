# EERecordSystem
Trivial EEPROM-based tinyfilesystem for Arduino

## Highlights:
  * Minimalist efficiency.  File system holds records, let's not call them files.
  * Designed for storing configuration choices.  It's friendlier than having fixed locations for things.  Having configuration items tied to alphanumeric keys makes it trivial to create a configuration editor (e.g. serial) to edit the values in EEPROM.
  * Minimalist in EEPROM consumption.  Four-byte header, one byte per key, one byte per record for length, plus your data.
  * Longer keys and record lengths supported, this is a C++ template, just change the type from byte to something else.
  * Each record has an character or integer "key"
  ** Example: using uint16_t as a key suitable for a multicharacter character constant (e.g. 'ab' == 0x6162)
  * There's no deleting or resizing records in this implementation, but you can add and overwrite them.
  
## List feature:
  * Designed for maintaining an access control database of valid 32-bit ID numbers.  (The size of 32-bits isn't currently templated).
  * You can add/remove ID numbers.  You can query if ID numbers are in the list.
  * ID 0xFFFFFFFF is reserved for free space.  Space from deleted IDs remains reserved for new IDs within the same list.
  * There can be more than one ID list.  Each whole list is accessed by a single key.
  
# Usage

Example usage with 16-bit keys (two characters) and byte lengths (max record size 253 bytes)
```c++
#include <EEPROM.h>
#include "EERecordSystem.h"
EERecordSystem<uint16_t,byte> EERS;

uint64_t our_serial_number;

void setup() {
  // Formats the whole EEPROM for use with this class, if not already done.
  // See alternative overload for begin() to use partial EEPROM
  EERS.begin();

  // Read our serial number from EEPROM, if available
  int location_of_serial_number = EERS.getRecordAddress('SN', sizeof(uint64_t));
  if (location_of_serial_number == -1) {
    Serial.println("Serial number is not set.  Enter it now");
    our_serial_number = get_entry_of_serial_number_from_serial();
    EERS.updateRecord('SN', &our_serial_number, sizeof(uint64_t));
  } else {
    EEPROM.get(location_of_serial_number, our_serial_number);
  }
```



  
