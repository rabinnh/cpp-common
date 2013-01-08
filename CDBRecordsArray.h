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



#ifndef CDBRECORDSARRAY_H
#define	CDBRECORDSARRAY_H

#include <map>
#include <string>
#include "CXPlatCriticalSection.h"

using namespace std;

typedef vector<string>  VEC_DB_RECORDS;

//! CDBRecordsArray
//! Class to encapsulate the records that we want to publish to Mongo
class CDBRecordsArray
{
public:
    CDBRecordsArray();
    CDBRecordsArray(const CDBRecordsArray& orig);
    virtual ~CDBRecordsArray();

    //! Add a record to the DB record array.. Synchronized with the publisher.
    void DBRecordAdd(const char *pRecord, float fElapsed = 0.0f);
	//! Add a whole array of records
    void DBRecordAdd(VEC_DB_RECORDS &vRec, float fElapsed = 0.0f);
    //! Get DB record array - locks the array 
    VEC_DB_RECORDS *LockDBRecords();
    //! Clear the array
    bool ClearDBRecords();
    //! Release DB record array - must be called after "GetRecordArray"
    void UnlockDBRecords();

public:    
	//! Array of the records
    VEC_DB_RECORDS  vDBRecords;
	//! Total elapsed time to publish the records.
    float fTotalElapsed;

protected:
    
    
private:
    CXPlatCriticalSection   cDBMutex;
    bool bIsLocked;
};

#endif	/* CDBRECORDSARRAY_H */

