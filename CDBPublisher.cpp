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

#include "CDBPublisher.h"
#include "CXPlatEvent.h"
#include <syslog.h>

// Static variable
DBStats CDBPublisher::cDBCopy;
const char *pRemoveStart = "{'%s' : {$in : [";
const char *pRemoveEnd = "]}}";

CDBPublisher::CDBPublisher() : CXPlatThread()
{
	bTerminate = false;
	tLastPublished = 0;
	bUpsert = false;
	this->StartThread();
}

CDBPublisher::~CDBPublisher()
{
	StopPublishing();
}


// Set DB config

void CDBPublisher::StartPublishing(const char *pDatabase, const char *pCollection, VEC_DB_KEYS &vKeys,
	const char *pHost, const char *pUser, const char *pPassword)
{
	if (!pDatabase || !pCollection || !pHost || !pUser || !pPassword)
		return;
	sDatabase = pDatabase;
	sCollection = pCollection;
	sDB_Collection = sDatabase + "." + sCollection;
	this->vKeys = vKeys;
	sHost = pHost;
	sUser = pUser;
	sPassword = pPassword;
	StartThread();
}


// Stop publishing.  Stops the publishing thread

void CDBPublisher::StopPublishing()
{ // Tell our thread to terminate.  The base class will do the rest.
	bTerminate = true;
	cStartEvent.SetEvent();
}


// Set upsert.  Since Mongo doesn't have an "upsert" per se, this is a very expensive
// setting, performance wise.  It will bulk delete all records before an insert is done.

void CDBPublisher::SetUpsert(bool bOnOff)
{
	bUpsert = bOnOff;
}
// See if the upsert flag is set

bool CDBPublisher::GetUpsert()
{
	return bUpsert;
}

// Publish records.  This will copy the passed in array and return immediately

bool CDBPublisher::Publish(CDBRecordsArray &cRecords)
{
	// Can't do it if we're still publishing the last call, 10
	if (cPublishMutex.Lock(0) == XPLAT_TIMEOUT)
		return false; // Don't wait at all

	// Lock and copy the public array to our private arrayy
	vDBRecords = *cRecords.LockDBRecords();
	cDBStats.fReqElapsedTotal += cRecords.fTotalElapsed;
	cRecords.fTotalElapsed = 0.0f;
	// Clear the caller's array
	cRecords.ClearDBRecords();
	// Unlock the caller
	cRecords.UnlockDBRecords();
	// We clear the existing buffer before the test so we don't have a memory leak on failure
	if (sHost.empty())
	{
		cPublishMutex.Unlock();
		return false;
	}

	// Start the publishing thread, which will unlock the mutex
	cStartEvent.SetEvent();

	return true;
}


// Function that actually does the publishing

unsigned CDBPublisher::WorkerFunction(void *pParam)
{
	vector<BSONObj> vBSON;
	bool bLocked = true;

	while(true)
	{ // Wait until we know that we have something to publish
		cStartEvent.Lock();
		tLastPublished = ::time(NULL);
		bool bAuthenticate = false;
		// Do they want us to quit
		if (bTerminate)
		{
			cPublishMutex.Unlock();
			break;
		}
		// Ensure that we can connect and do our thing
		try
		{ // Connect and authenticate
			string sErrmsg;
			DBClientConnection cMongoConnect;
			// The catch block will catch a failed connection and Mongo will handle the connection when we go out of scope
			bool bCheckReturn;
			bCheckReturn = cMongoConnect.connect(sHost.c_str(), sErrmsg);
			if (!bCheckReturn)
			{
				::openlog("Mongo publisher", LOG_PID, LOG_LOCAL0);
				::syslog(LOG_CRIT, "Unable to connect to Mongo database host %s", sDatabase.c_str());
				::closelog();
			}
			// Note that the "digest-password" flag has to be "true" for plain text passwords
			if (!sUser.empty() && !sPassword.empty() && !cMongoConnect.auth(sDatabase, sUser, sPassword, sErrmsg, true))
			{
				cDBStats.lAuthErrors++;
				cPublishMutex.Unlock();
				continue;
			}
			bAuthenticate = true;
			int iX;
			string sRemoveArray;
			// Create the BSON records and the array of records that we are going to replace
			// The problem here is that MongoDB has bulk insert, and an update verb, but not a bulk update
			for (iX = 0; iX < vDBRecords.size(); iX++)
			{
				BSONObj cBSON = fromjson(vDBRecords[iX]);
				vBSON.push_back(cBSON);
				// We'll create a JSON array of all the nodes we need to remove so we can do it in one call.
				if (iX)
					sRemoveArray += ',';
				sRemoveArray += vDBRecords[iX];
			}

			// We get one shot at this.  If it doesn't work we just throw the records away.
			// Ensure that we have the indices
			for (iX = 0; iX < vKeys.size(); iX++)
			{
				bCheckReturn = cMongoConnect.ensureIndex(sDB_Collection.c_str(), fromjson(vKeys[iX].c_str()), true);
				if (!bCheckReturn)
				{
					::openlog("Mongo publisher", LOG_PID, LOG_LOCAL0);
					::syslog(LOG_CRIT, "ensureIndex failed on database %s, collection %s, index %s\n",
						sDatabase.c_str(), sCollection.c_str(), vKeys[iX].c_str());
					::closelog();
				}
			}

			// Remove the records that we're going to replace.  2 calls is the best that we can do.
			if (bUpsert)
			{
				// *** RAB - substituion parameter must be replaced with keys
				string sRemove = pRemoveStart;
				sRemove += sRemoveArray;
				sRemove += pRemoveEnd;
				cMongoConnect.remove(sDB_Collection.c_str(), fromjson(sRemove));
			}

			// Now insert the records
			cMongoConnect.insert(sDB_Collection.c_str(), vBSON);

			// Clear our arrays
			vDBRecords.clear();
			vBSON.clear();

		}
		catch(...)
		{
			bAuthenticate ? cDBStats.lInsertErrors++ : cDBStats.lInsertErrors++;
			if (bLocked)
				cPublishMutex.Unlock();
			vBSON.clear();
			// If we simply can't reach mongo, we don't want to keep adding records.
			if (vDBRecords.size() > 50000)
				vDBRecords.clear();
		}
		// Unlock our stuff
		cPublishMutex.Unlock();
	}
	return 0;
}

// Get the error count. Also clears it.

DBStats & CDBPublisher::GetDBStats()
{
	cDBCopy = cDBStats;
	cDBStats.lAuthErrors = 0;
	cDBStats.lConnectErrors = 0;
	cDBStats.lInsertErrors = 0;
	cDBStats.fReqElapsedTotal = 0.0f;
	return cDBCopy;
}

