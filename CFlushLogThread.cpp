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
// CFlushLogThread.cpp: implementation of the CFlushLogThread class.
//
//////////////////////////////////////////////////////////////////////
#include "XPlat.h"
#include "CFlushLogThread.h"
#include "CLogging.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFlushLogThread::CFlushLogThread()
{
}

CFlushLogThread::~CFlushLogThread()
{
    StopThread();
}


// Called by Win32Thread to start work in object context

unsigned CFlushLogThread::WorkerFunction(void *pParam)
{
    CLogging *cLog = (CLogging *) pParam;

    if (!pParam || !cStopEvent.WasCreatedSuccessfully())
        return ((unsigned) - 1);

    while(TRUE)
    { // If signaled, quit
        if (cStopEvent.Lock(cLog->iFileFlushInterval * 1000) != XPLAT_TIMEOUT)
            break;

        // Flush the file
        if (!cLog->bActivity)
            continue;
        cLog->bActivity = FALSE;
        cLog->cCriticalSection.Lock();
        if (cLog->pFile)
        {
            ::fflush(cLog->pFile);
        };
        cLog->cCriticalSection.Unlock();
    };

    return (0);
};


// Stop the thread

void CFlushLogThread::StopThread()
{
    cStopEvent.SetEvent();
};

