# EERecordSystem
Trivial EEPROM-based tinyfilesystem for Arduino

## Highlights:
  * Minimalist efficiency.  It's a single header file containing only template code.  
  * File system holds records, let's not call them files.
  * Designed for storing configuration choices.  It's friendlier than having fixed locations for things.  Having configuration items tied to alphanumeric keys makes it trivial to create a configuration editor (e.g. serial) to edit the values in EEPROM.
  * Minimalist in EEPROM consumption.  Five EEPROM byte overhead for entire class, plus two bytes per record (one byte per key, one byte per record for length, plus your data)
  * Longer keys and record lengths supported.  Just use something other than ```byte``` as needed (this is a C++ template class)
  * Each record has an character or integer "key"
  ** Example: using uint16_t as a key suitable for a multicharacter character constant (e.g. 'ab' == 0x6162)
  * There's no deleting or resizing records in this implementation, but you can add and overwrite them.
  
## List feature:
  * Designed for also maintaining a list.  The prototypical use case is as an access control database of valid 32-bit user ID numbers that occasionally get added or removed.  (The size of 32-bits is fixed and isn't templated).
  * You can add/remove ID numbers.  You can query if ID numbers are in the list.
  * ID 0xFFFFFFFF is reserved for free space.  Removing ID's sets them to this value.  Space from deleted IDs remains reserved for new IDs within the same list.
  * There can be more than one ID list.  Each whole list is accessed by a single key.
  
# Usage

Example usage with 16-bit keys (two characters) and byte-lengths (so, max record payload size 253 bytes)
```c++
#include <EEPROM.h>
#include "EERecordSystem.h"
EERecordSystem<uint16_t,byte> EERS;

uint64_t our_serial_number;


void setup() {
  // Formats the whole EEPROM for use with this class, if not already done.
  // See alternative overload for begin() to use partial EEPROM
  EERS.begin();
  
  // Let's pretend that serial baud is changeable in configuration, but
  // otherwise defaults to 9600.  
  Serial.begin(EERS.getRecordData<uint32_t>('SB', 9600));
  
  // Let's pretend the same for the serial number, but also pretend we want
  // to ask the user (via the serial port) to provide it, rather than accept a default.
  int location_of_serial_number = EERS.getRecordAddress('SN', sizeof(our_serial_number));
  if (location_of_serial_number == -1) {
    Serial.println("Serial number is not set.  Enter it now");
    our_serial_number = get_entry_of_serial_number_from_serial();
    EERS.updateRecord('SN', &our_serial_number, sizeof(our_serial_number));
  } else {
    EEPROM.get(location_of_serial_number, our_serial_number);
  }
```

# Caveats

  * There's currently no functionality for deleting or resizing records.  Saving a record with a new size creates a new record.
  * This readme could use a good example of how to use the list functionality, which does allow deletions.
  


  
