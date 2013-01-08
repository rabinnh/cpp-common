/*
   Copyright [2010] [Richard Bross]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "CDBRecordsArray.h"

CDBRecordsArray::CDBRecordsArray()
{
    bIsLocked = false;
    fTotalElapsed = 0.0f;
}

CDBRecordsArray::CDBRecordsArray(const CDBRecordsArray& orig)
{
    cDBMutex.Lock();
    vDBRecords = orig.vDBRecords;
    fTotalElapsed = orig.fTotalElapsed;
    cDBMutex.Unlock();
}

CDBRecordsArray::~CDBRecordsArray()
{
}


// Add to DB record array

void CDBRecordsArray::DBRecordAdd(const char *pRecord, float fElapsed)
{
    cDBMutex.Lock();
    vDBRecords.push_back(pRecord);
    fTotalElapsed += fElapsed;
    cDBMutex.Unlock();
}

//! Add a whole array of records
void CDBRecordsArray::DBRecordAdd(VEC_DB_RECORDS &vRec, float fElapsed)
{
    cDBMutex.Lock();
	vDBRecords.insert(vDBRecords.end(), vRec.begin(), vRec.end());
    fTotalElapsed += fElapsed;
    cDBMutex.Unlock();
}

// Get DB record array - locks the array

VEC_DB_RECORDS *CDBRecordsArray::LockDBRecords()
{
    cDBMutex.Lock();
    bIsLocked = true;
    
    return &vDBRecords;
}


// Release DB record array - must be called after "GetRecordArray"

void CDBRecordsArray::UnlockDBRecords()
{
    cDBMutex.Unlock();
    bIsLocked = false;
}

// Warning - MUST BE LOCKED FIRST
bool CDBRecordsArray::ClearDBRecords()
{   // Make sure at least someone has a lock on it
    if (bIsLocked)
        vDBRecords.clear();
    return (bIsLocked);
}
