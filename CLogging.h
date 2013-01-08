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
//! CLogging.h: interface for the CLogging class.

#if !defined(AFX_CLOGGING_H__45321585_E6C5_11D2_AF79_00C04F6E1532__INCLUDED_)
#define AFX_CLOGGING_H__45321585_E6C5_11D2_AF79_00C04F6E1532__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif //! _MSC_VER >= 1000

#include "CFlushLogThread.h"
#include "CXPlatCriticalSection.h"

//! CLogging implements an atomic logging capability with the additional
//! ability to to have a separate thread to flush the log buffer to disk
//! at programmed intervals.
class CLogging  
{
friend class CFlushLogThread;
public:
	enum DISPLAY_MODE	{
						D_CONSOLE	= 0,
						D_FILE		= 1,
						D_BOTH		= 2
						};	
						
	enum	{
			FILE_FLUSH_INTERVAL		= 120
			};
														
	CLogging(BOOL bUseFlushThread = FALSE);
	CLogging(const char *szBaseName, const char *szDir = NULL, BOOL bDayExtension = TRUE, BOOL bUseFlushThread= FALSE);
	virtual ~CLogging();

		//! If day extension is TRUE, the date will be appended to the file name			
	virtual BOOL InitLoggingFile(const char *szBaseName, const char *szDir = NULL, BOOL bDayExtension = TRUE);
		//! If day extension is TRUE, the date will be appended to the file name			
	virtual	BOOL InitLoggingFile(const char *szBaseName, const char *szExtenssion, const char *szDir = NULL, BOOL bDayExtension = TRUE);
		//! Write to the log
	int     WriteToLog(const char *szLine, int iLevel = 0, BOOL bSuppressTimestamp = FALSE);
		//! If TRUE, will open before each write and close afterwards.  Defaults to FALSE.
	void	SetCloseMode(BOOL bClose);
		//! Returns the file name
	const char *GetFileName();
		//! Returns the full file path
	const char *GetFilePath();
		//! Set display mode
	void	SetOutputMode(DISPLAY_MODE eMode);
		//! Set file flush interval (seconds)
	void	SetFileFlushInterval(int iFlush);
		//! Force a close file
	int	CloseFile();
        //! See if OK to close at this point
    virtual BOOL OKtoClose();
        //! Changing day routine
    virtual void ChangingDay();
		//! Get the file size
	long	GetFileSize();
		//! Set the logging level. iLevel passed to WriteToLog must <= to be output. (0 - 99)
	void	SetLogLevel(int iLevel);
		//! Prefix time stamp to all lines
	void	PrefixTimestamp(BOOL bTime = TRUE);
		//! Set so file name doesn't change on a new day
	void	SetNameChange(BOOL bChange);
		//! Was the file created on the open?
	BOOL	WasCreatedOnOpen();
protected:
		//! Open the file
	virtual	FILE *OpenLogFile();
		//! Get a timestamp string
	const char *GetTimestamp();
		//! Close the file			
	virtual void CloseLogFile();
		//! Initialize the file
	void	Initialize(BOOL bUseFlushThread);
		//! Create the file name
	const char *CreateFileName(const char *szBaseName, const char *szDir, BOOL bDayExtension, const char *szExtension);

protected:
	CFlushLogThread     cFlushThread;
	BOOL                bActivity;
	BOOL                bTimestamp;
	int                 iLogLevel;
	DISPLAY_MODE        eDisplayMode;
	BOOL                bCloseMode;
	BOOL                bDayExt;
	FILE                *pFile;
	string              sBaseName;
	string              sLogFileDir;
	string              sFileName;
	string              sPath;
	int                 iYear;
	int                 iDayofYear;
	int                 iFileFlushInterval;
	CXPlatCriticalSection cCriticalSection;
        char                szTimeStamp[64];
	BOOL                bNameChange;
	BOOL                bWasCreated;
};
#endif //! !defined(AFX_CLOGGING_H__45321585_E6C5_11D2_AF79_00C04F6E1532__INCLUDED_)
