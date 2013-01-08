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
//! CFlushLogThread.h: interface for the CFlushLogThread class.

#if !defined(AFX_CFLUSHLOGTHREAD_H__C80CD926_F97A_11D2_823E_00A0CC20AAD9__INCLUDED_)
#define AFX_CFLUSHLOGTHREAD_H__C80CD926_F97A_11D2_823E_00A0CC20AAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif //! _MSC_VER > 1000

#include "CXPlatThread.h"
#include "CXPlatEvent.h"

//! Used by CLogging to flush the buffer at regular intervals
class CFlushLogThread : public CXPlatThread  
{
public:
	CFlushLogThread();
	virtual ~CFlushLogThread();

	void	StopThread();

protected:
			//! Called by Win32Thread to start work in object context
virtual	unsigned	WorkerFunction(void *pParam);	

protected:
	CXPlatEvent			cStopEvent;
};

#endif //! !defined(AFX_CFLUSHLOGTHREAD_H__C80CD926_F97A_11D2_823E_00A0CC20AAD9__INCLUDED_)
