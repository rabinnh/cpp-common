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

#ifndef CDBPUBLISHER_H
#define	CDBPUBLISHER_H

#include "CXPlatThread.h"
#include "CXPlatEvent.h"
#include "CDBRecordsArray.h"
#include <mongo/client/dbclient.h>

using namespace mongo;

// Same template
#define VEC_DB_KEYS	VEC_DB_RECORDS

//! Class DBStats keeps track of DB statistics
class DBStats
{
public:
    DBStats()
    {
        lConnectErrors = 0;
        lAuthErrors = 0;
        lInsertErrors = 0;
        fReqElapsedTotal = 0.0f;
    };
    DBStats(const DBStats& orig)
    {
        lConnectErrors = orig.lConnectErrors;
        lAuthErrors = orig.lAuthErrors;
        lInsertErrors = orig.lInsertErrors;
        fReqElapsedTotal = orig.fReqElapsedTotal;
    };
    virtual ~DBStats(){};

	//! Connection error count
    long lConnectErrors;    // Connect and insert errors
	//! Authentication error count
    long lAuthErrors;       // Authentication errors
	//! Insert errors
    long lInsertErrors;     // Insert errors
	//! Elapsed time of requests
    float fReqElapsedTotal; // Total elapsed time for requests in ms
};

//!	CDBPublisher - publisher thread
//!	This class takes a set of records to be published to the MongoDB and writes them
//! in bulk on a schedule
class CDBPublisher : public CXPlatThread
{
public:
    CDBPublisher();
    virtual ~CDBPublisher();

    //! Set DB config.  vKeys is an array of index strings (JSON, ex. "{'node': 1 }") 
	//! to ensure that the indices exist in the collection.  Starts publishing thread.
    void StartPublishing(const char *pDatabase, const char *pCollection, VEC_DB_KEYS &vKeys, 
					const char *pHost, const char *pUser, const char *pPassword);
	//! Stop publishing.  Stops the publishing thread
	void StopPublishing();
	//! Publish records.  This will copy the data from the passed in array, clear the array, 
	//! and return immediately while connecting and writing in the background.
    bool Publish(CDBRecordsArray &cRecords);
    //! Get and clear statistics.
    //! This returns a call to a static variable, which means the next call changes it.
    DBStats &GetDBStats();
	//! Set upsert.  Since Mongo doesn't have an "upsert" per se, this is a very expensive
	//! setting, performance wise.  It will bulk delete all records before an insert is done.
	void SetUpsert(bool bOnOff);
	//! See if the upsert flag is set
	bool GetUpsert();
    
public:
	//! Our last published timestamp
    time_t tLastPublished;

protected:
    // Called by CXPlatThread to start work in object context
    virtual unsigned WorkerFunction(void *pParam);
protected:
    DBStats cDBStats;
static DBStats cDBCopy;
    CXPlatCriticalSection cPublishMutex;
    CXPlatEvent cStartEvent;
    string sHost;
    string sUser;
    string sPassword;
    bool bTerminate;
    string sDatabase;
    VEC_DB_RECORDS vDBRecords;
	VEC_DB_KEYS	vKeys;
	string sCollection;
	string sDB_Collection;
private:
	bool	bUpsert;
};

#endif	/* CDBPUBLISHER_H */

