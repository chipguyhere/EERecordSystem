/*
EERecordSystem Copyright 2021 chipguyhere, License: GPLv3

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


/*****************************************

EERecordSystem - Trivial File System for EEPROM
https://github.com/chipguyhere/EERecordSystem

*****************************************/

#ifndef __chipguy_EERecordSystem_h_included
#define __chipguy_EERecordSystem_h_included

#include <Arduino.h>
#include <EEPROM.h>



// Suggestion: byte for T_key, byte for T_datasize
//  For two-character keys, use uint16_t for T_key
//  For records longer than 253 characters, use uint16_t for T_datasize
template <typename T_key, typename T_datasize> class EERecordSystem {
public:
    EERecordSystem();
    
    // Starts up, and writes a header to the EEPROM if a header is not found.
    void begin(); // use whole EEPROM
    void begin(int _starting_address, int _ending_address); // use portion of EEPROM    
    
    // Read a record from EEPROM by its key, returning the default if it's not found.
    template <typename T_datatype> T_datatype getRecordData(T_key key, T_datatype defaultValue);
    template <typename T_datatype> T_datatype updateRecordData(T_key, T_datatype data);
    
    // Add or update a record.  Returns true if successful
    bool updateRecord(T_key key, byte data);
    bool updateRecord(T_key key, void *recorddata, T_datasize datasize);
    
    // Get EEPROM address of a record, suitable for using EEPROM.get for the data.
    // Will only select a record with matching key and size when datasize is nonzero
    // If datasize==0, match on key only, any size matches, datasizeOut will be returned.   
    int getRecordDataAddress(T_key key, T_datasize datasize, uint16_t startWhere=0, T_datasize *datasizeOut=NULL);

    // List operations
    bool queryList(uint32_t id, T_key listkey, uint32_t comparison_mask=0xFFFFFFFF);
    bool addEntryToList(uint32_t id, T_key listkey);
    bool deleteListEntry(uint32_t id, T_key listkey, uint32_t comparison_mask=0xFFFFFFFF);
    void enumerateList(T_key listkey, void (*enumcallback)(void*,uint32_t), void *objptr);
    
private:
  bool idop(char op, uint32_t id, T_key listkey, uint32_t comparison_mask, void (*enumcallback)(void*,uint32_t)=NULL);
  bool began=false;
  bool addnewrecord(T_key key, void *recorddata, T_datasize datasize);
  
  uint16_t starting_address;
  uint16_t ending_address;


};





#define EEPROMREAD(x) EEPROM.read(x)
#define EEPROMUPDATE(x,y) EEPROM.update(x,y)

template <typename T_key, typename T_datasize> EERecordSystem<T_key,T_datasize>::EERecordSystem() {}


// begin()
// Begins using the record system.  Writes formatting information to the EEPROM if not already found.
template <typename T_key, typename T_datasize> void EERecordSystem<T_key,T_datasize>::begin() {
  begin(0, EEPROM.length()-1);
}

// begin()
// Begins using the record system at a specific starting and ending address.
template <typename T_key, typename T_datasize> void EERecordSystem<T_key,T_datasize>::begin(int _starting_address, int _ending_address) {
  began=true;
  int s = _starting_address+1;
  starting_address=_starting_address+4;
  ending_address=_ending_address;
  if (EEPROMREAD(s++)=='C') {
    if (EEPROMREAD(s++)=='A') {
      if (EEPROMREAD(s)=='S') return;
    }
  }  
  s -= 2;
  EEPROMUPDATE(s++, 'C');
  EEPROMUPDATE(s++, 'A');
  EEPROMUPDATE(s++, 'S');
  EEPROMUPDATE(s++, 0);
  EEPROMUPDATE(s, 0);
}


template <typename T_key, typename T_datasize>  
template <typename T_datatype>
T_datatype EERecordSystem<T_key,T_datasize>::getRecordData(T_key key, T_datatype defaultValue) {
  int addr = getRecordDataAddress(key, sizeof(T_datatype));
  if (addr==-1) return defaultValue;
  T_datatype rv;
  EEPROM.get(addr, rv);
  return rv;
}

template <typename T_key, typename T_datasize>  
template <typename T_datatype>
T_datatype EERecordSystem<T_key,T_datasize>::updateRecordData(T_key key, T_datatype data) {
  return updateRecord(key, &data, sizeof(data));
}


// updateRecord
// Updates a single-byte record, adding it as a new record if there is no match on key with length 1.
template <typename T_key, typename T_datasize> bool EERecordSystem<T_key,T_datasize>::updateRecord(T_key key, byte data) {
  if (began==false) return false;
  return updateRecord(key, &data, 1);
}

// updaterecord
// Updates a record, adding it as a new record if there is no match on key and length (datasize).
template <typename T_key, typename T_datasize> bool EERecordSystem<T_key,T_datasize>::updateRecord(T_key key, void *recorddata, T_datasize datasize) {
    if (began==false) return false;
    if (datasize==0) return false;
    if (datasize > (sizeof(datasize)==2 ? 65533 : 254) - sizeof(key)) return false;

    int address = getRecordDataAddress(key, datasize);
    // address points to data portion, if valid.

    // write record if it doesn't already exist
    if (address == -1) return addnewrecord(key, recorddata, datasize);


    // update the record when it does
    for (int i = 0; i < datasize; i++) EEPROMUPDATE(address + i, ((byte*)(recorddata))[i]);
    return true;
}




template<typename T_key> static T_key eeprom_read_fkey(int where) {
  T_key fkey;
  if (sizeof(fkey)>2) {
    for (int b=0; b<sizeof(fkey); b++) ((byte*)&fkey)[0] = EEPROMREAD(where++);
  } else {
    // sizeof fkey is 1 or 2, avoid compiling an unnecessary loop
    fkey = EEPROMREAD(where); 
    if (sizeof(fkey)==2) ((byte*)&fkey)[1] = EEPROMREAD(where+1);
  }
  return fkey;
}

// getRecordDataAddress
// Gets the starting EEPROM address of the data portion of the record that matches on key and length (datasize).
// This can be used for external manipulation of the data itself, or use within a counter.
// If datasize is provided as 0, then match is on key only; record datasize is returned via the datasize parameter.
// Parameter startWhere can be used to find the next match
template <typename T_key, typename T_datasize> int EERecordSystem<T_key,T_datasize>::getRecordDataAddress(T_key key, T_datasize datasize, uint16_t startWhere, T_datasize *datasizeOut) {
  if (began==false) return -1;
    
  uint16_t EEaddress = starting_address;          // points at a length byte, or 0 if no more
  while (EEaddress < (ending_address - sizeof(key))) {
    T_datasize recordlength = EEPROM.read(EEaddress);
    if (sizeof(recordlength)==2) ((byte*)&recordlength)[1] = EEPROM.read(EEaddress+1);
    if (recordlength==0) break;
  
    if ((EEaddress+sizeof(recordlength)+sizeof(key)) > startWhere) {
      T_key fkey = eeprom_read_fkey<T_key>(EEaddress+sizeof(recordlength));
      int currentrecordaddress = EEaddress + sizeof(recordlength) + sizeof(fkey);
      int currentrecordlength = recordlength - sizeof(recordlength) - sizeof(key);
      if (key==fkey) { 
          if (datasize==0 && datasizeOut != NULL) {
            *datasizeOut = currentrecordlength;
            return currentrecordaddress;
        } else if (datasize==currentrecordlength) {
            return currentrecordaddress;
        }
      }
    }
    EEaddress += recordlength;    // next record

  } 
  return -1;
}

// Adds a new record to EEPROM.
// Returns false if not possible (unformatted or not enough space)
// Does not check to see if any matching record already exists, use updaterecord to get this.
// Beware of incomplete writes, however, consequence of incomplete write is the creation of a
// record whose first byte is guaranteed to be a null (i.e. identical to a "deleted" record)
template <typename T_key, typename T_datasize> bool EERecordSystem<T_key,T_datasize>::addnewrecord(T_key key, void *recorddata, T_datasize datasize) {
  // uninitialized eeprom? abort
  if (began==false) return false;
  
  uint16_t EEaddress = starting_address;
  while (EEaddress < ending_address) {
    T_datasize recordlength = EEPROM.read(EEaddress);
    if (sizeof(recordlength)==2) ((byte*)&recordlength)[1] = EEPROM.read(EEaddress+1);
    if (recordlength == 0) break;
    if (recordlength + EEaddress > ending_address) return false;
    EEaddress = EEaddress + recordlength;
  }
  
  
  // EEaddress shall now point to the last byte we're using, which should be a 0
  // (to indicate no more records) rather than the length of the next record.
  // This byte will become our new length byte.
  
  // enough room?
  // We'll need to hold the key, the data, and the empty length at the end.
  // (The length byte will replace the zero we're already looking at)
  if ((EEaddress + datasize + sizeof(datasize) + sizeof(key) + sizeof(datasize)) > (1+ending_address)) return false;
  // example: want to add a 87 byte record plus key in a 10000 byte eeprom (ending_address 9999) assume key/datasize 2
  //   address 9907.  9907-9908 length, 9909-9910 key, 9911-9997 data, 9998-9999 two zeros for end length, fits exactly.
  //    9907 + 87 + 2 + 2 + 2 = 10000


  int new_record_key_location = EEaddress + sizeof(datasize);
  int new_record_data_location = EEaddress + sizeof(datasize)+sizeof(key);

  // Asserting that the first byte of the record is null because it used to be eeprom_counter_sense
  // which we made sure was zero.

  // Write the data into the record, last byte to first byte
  for (int i = 0; i < datasize; i++) EEPROM.update(new_record_data_location + i, ((byte*)(recorddata))[i]);
  // Write the key
  if (sizeof(key)==2) EEPROM.update(new_record_key_location+1, (byte)(key>>8));
  EEPROM.update(new_record_key_location, (byte)key&0xff);
  // Write the trailing zero
  if (sizeof(datasize)==2) EEPROM.update(new_record_data_location+datasize+1, 0);  
  EEPROM.update(new_record_data_location+datasize, 0);
  // Write the length (this makes the record alive)
  if (sizeof(datasize)==2) EEPROM.update(EEaddress+1, (datasize+sizeof(datasize)+sizeof(key))>>8);    
  EEPROM.update(EEaddress, (byte)(datasize+sizeof(datasize)+sizeof(key))&0xff);
  return true;




}



// Do an operation on an ID list (all of which involves walking the list)
template <typename T_key, typename T_datasize> bool EERecordSystem<T_key,T_datasize>::idop(char op, uint32_t id, T_key listkey, uint32_t comparison_mask, void (*enumcallback)(void*,uint32_t)) {
  // uninitialized eeprom? abort
  if (began==false) return false; // eeprom has not been initialized

  uint16_t EEaddress = starting_address;
  int freeSlot=0;
  
  // if op is A (Add), temporarily treat op as Q (Query), adding only if fails.
  bool isAdd=false;
  if (op=='A') {
    isAdd=true;
    op='Q';
  }

  bool delete_success=false;
  
  while (EEaddress <= ending_address) {
    T_datasize recordlength = EEPROM.read(EEaddress);
    if (sizeof(recordlength)==2) ((byte*)&recordlength)[1] = EEPROM.read(EEaddress+1);
    if (recordlength == 0) break;
    // EEaddress points to record length, after this there is Key, and then Record Data.
    T_key fkey = eeprom_read_fkey<T_key>(EEaddress+sizeof(recordlength));
    if (listkey==fkey) {
      T_datasize datalength = recordlength - sizeof(recordlength) - sizeof(fkey);
      uint16_t dataaddress = EEaddress + sizeof(fkey) + sizeof(recordlength);

      if (op=='Q' || op=='D' || op=='E') { // Query/Delete to see if ID exists E=enumerate
        while (datalength >= 4) {
          int i=0;
          bool match=true;
          byte *bid = (byte*)(&id);
          uint32_t v=0;
          byte *vid = (byte*)(&v);
          byte *msk = (byte*)(&comparison_mask);
          for (int i=0; i<4; i++) {
            datalength--;            
            byte rb = EEPROMREAD(dataaddress++);
            *vid++ = rb;
            byte rbmasked = rb & (*msk);
            byte bidmasked = (*bid++) & (*msk++);
            if (rbmasked != bidmasked) match=false;

          }
          if (match) {
            if (op=='Q') return true;
            // Delete the record by overwriting it with 0xFF
            if (op=='D') {
              delete_success=true;
              for (i=-4; i<0; i++) EEPROMUPDATE(dataaddress+i, 0xFF);
            
            }
          }
          if (v==0xFFFFFFFF) {
            if (freeSlot==0) freeSlot = dataaddress-4;
          } else if (op=='E') {
            void *objptr = (void*)comparison_mask;
            enumcallback(objptr, v);
          }
        }
      }
    }
    EEaddress += recordlength;

  }
  if (op=='d' || op=='D' || op=='x') return delete_success;
  if (isAdd) {
    // if we found a free slot, use it.
    if (freeSlot) {
//      SerialMonitor->println(F("found free slot"));          
      
      byte *bid = (byte*)(&id);
      for (int i=0; i<4; i++) EEPROMUPDATE(freeSlot+i, *bid++);
    } else {
      // otherwise, invent a new slot.
      byte newslot[12];
      memset(&newslot, 0xFF, 12);
      *(uint32_t*)(&newslot[0])=id;
//      SerialMonitor->println(F("added new record"));          

      return addnewrecord(listkey, &newslot, 12);
    }
  }
  return false;

}

// queryList
// Look for an Entry on the list having a specific key, returns true if there is a match.
// Comparison mask allows matching on a subset of the id bits.
template <typename T_key, typename T_datasize> bool EERecordSystem<T_key,T_datasize>::queryList(uint32_t id, T_key listkey, uint32_t comparison_mask) {
  return idop('Q', id, listkey, comparison_mask);
}

// addEntryToList
// Adds a 32-bit Entry to the database, but only if it does not already exist (no effect if it exists).
// This may allocate a new 12-byte Record that can hold 3 Entries if there are no Records with free space.
template <typename T_key, typename T_datasize> bool EERecordSystem<T_key,T_datasize>::addEntryToList(uint32_t id, T_key listkey) {
  return idop('A', id, listkey, 0xFFFFFFFF);
}

// deleteListEntry
// Deletes an Entry by setting its value to zero.  This signifies that the space is available
// for a future add of another Entry.  The space is only available for Entries on the same list,
// nothing in this implementation frees the space for use as another Record.
template <typename T_key, typename T_datasize> bool EERecordSystem<T_key,T_datasize>::deleteListEntry(uint32_t id, T_key listkey, uint32_t comparison_mask) {
  return idop('D', id, listkey, comparison_mask);
}

// enumerateList
// Enumerates a list by calling the function of your choice for each uint32_t in the list.
// Passes your object pointer with it, if needed.
template <typename T_key, typename T_datasize> void EERecordSystem<T_key,T_datasize>::enumerateList(T_key listkey, void (*enumcallback)(void*,uint32_t), void *objptr) {
  idop('E', 0, listkey, (uint32_t)objptr, enumcallback);


}


#endif // #ifdef __chipguy_EERecordSystem_h_included
