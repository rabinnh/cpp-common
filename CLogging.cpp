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
// CLogging.cpp: implementation of the CLogging class.
//
//	Created: Richard A. Bross
//
////////////////////////////////////////////////////////////////////////
#include "XPlat.h"
#include "CFlushLogThread.h"
#include "CLogging.h"
#if defined(_WIN32)
#include "direct.h"
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLogging::CLogging(BOOL bUseFlushThread)
{
    Initialize(bUseFlushThread);
}

CLogging::CLogging(const char *szBaseName, const char *szDir, BOOL bDayExtension, BOOL bUseFlushThread)
{
    Initialize(bUseFlushThread);
    InitLoggingFile(szBaseName, szDir, bDayExtension);
}

// Called by constructors

void CLogging::Initialize(BOOL bUseFlushThread)
{
    bCloseMode = bActivity = bWasCreated = FALSE;
    eDisplayMode = D_FILE;
    pFile = NULL;
    iLogLevel = 99;
    bTimestamp = TRUE;
    iFileFlushInterval = FILE_FLUSH_INTERVAL;
    if (bUseFlushThread)
        cFlushThread.StartThread(this);
};

CLogging::~CLogging()
{
    cFlushThread.StopThread();
    cFlushThread.WaitForThreadCompletion(30000);
    CloseLogFile();
}


// If day extension is TRUE, the date will be appended to the file name; default extention is ".log"

BOOL CLogging::InitLoggingFile(const char *szBaseName, const char *szDir, BOOL bDayExtension)
{
    return InitLoggingFile(szBaseName, ".log", szDir, bDayExtension);
}


// If day extension is TRUE, the date will be appended to the file name

BOOL CLogging::InitLoggingFile(const char *szBaseName, const char *szExtension, const char *szDir, BOOL bDayExtension)
{
    int iErr;

    // See if we already have one
    if (pFile)
        return (TRUE);

    if (!szDir)
#if defined(_WIN32)
        szDir = ".\\";
#else
        szDir = "../";
#endif
    sBaseName = szBaseName;
    sLogFileDir = szDir;
    bDayExt = bDayExtension;

    // Create the file name
    sFileName = CreateFileName(szBaseName, szDir, bDayExtension, szExtension);

    // Open it
    pFile = OpenLogFile();
    if (!pFile)
    {
#if defined(_WIN32)
        iErr = ::GetLastError();
#else
        iErr = errno;
#endif
        printf("Unable to open log file. Error returned was: \"%s\"\n", ::strerror(iErr));
        return (iErr);
    };

    // Get the full path name
#if defined(_WIN32)
    // Win32 only!
    char *lpFilePart;
    DWORD dwSize;

    dwSize = ::GetFullPathName(sFileName.c_str(), 0, NULL, NULL);
    sPath.reserve(dwSize + 1);
    dwSize = ::GetFullPathName(sFileName.c_str(), dwSize + 1, (char *) sPath.c_str(), &lpFilePart);
#endif
    if (bCloseMode)
        CloseLogFile();
    return (0);
};


// Open the file

FILE *CLogging::OpenLogFile()
{
    const char *lpWork;
    const char *lpOld;
    int iError;
#if defined(_WIN32)
    char *szMode = "a+c";
#else
    const char *szMode = "a+";
    int iMode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
        S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
#endif

    // Make sure we have something
    if (!sFileName.size())
        return (NULL);

    // Close if already open
    if (pFile)
        CloseLogFile();

    // Make a copy of the path for tokenizing
    lpWork = ::strchr(sFileName.c_str(), '\\');
    if (!lpWork)
        lpWork = ::strchr(sFileName.c_str(), '/');
    sPath.erase();
    while(lpWork)
    {
        if (strlen(lpWork) == 1 && (*lpWork != PATH_CHAR))
            break;
        sPath = sFileName.substr(0, lpWork - sFileName.c_str());
#if defined(_WIN32)
        iError = ::_mkdir(sPath.c_str());
#else
        iError = ::mkdir(sPath.c_str(), iMode);
#endif
        lpOld = lpWork;
        lpWork = ::strchr(++lpWork, '\\');
        if (!lpWork)
            lpWork = ::strchr(++lpOld, '/');
    };

    cCriticalSection.Lock();
    // Check if file will be created
    pFile = ::fopen(sFileName.c_str(), "r");
    bWasCreated = pFile == NULL ? TRUE : FALSE;
    if (pFile)
        ::fclose(pFile);
    pFile = ::fopen(sFileName.c_str(), szMode);
    cCriticalSection.Unlock();

    return (pFile);
};


// Write to the log

int CLogging::WriteToLog(const char *szLine, int iLevel, BOOL bSuppressTimestamp)
{
    int iRet;
    BOOL bWTime = TRUE;
    time_t sNow;
    struct tm sTm;
    struct tm *pTm;
    static BOOL bInProgress = FALSE;
    char *pszSavedLine = 0;

    // Make sure everything is kosher
    if (!szLine || (iLevel > iLogLevel))
        return (-1);

    bActivity = TRUE;

    // Check for a day change if the log file has a date extension
    if (bDayExt && !bInProgress)
    {
        sNow = time(NULL);
        PROTECT_CALL
        pTm = localtime(&sNow);
        if (pTm)
        {
            memcpy(&sTm, pTm, sizeof ( sTm));
            UNPROTECT_CALL
            if ((sTm.tm_year != iYear || sTm.tm_yday != iDayofYear))
            { // Time to create a new file
                if (OKtoClose())
                {
                    pszSavedLine = new char[strlen(szLine) + 1];
                    assert(pszSavedLine != NULL);
                    strcpy(pszSavedLine, szLine);
                    bInProgress = TRUE;
                    CloseLogFile();
                    ChangingDay();
                    InitLoggingFile(sBaseName.c_str(), sLogFileDir.c_str(), bDayExt);
                    bInProgress = FALSE;
                }
            };
        }
        else
        {
            UNPROTECT_CALL
        }
    }

    // Don't write a timestamp if this is just a newline character
    char cCheck;
    if (pszSavedLine)
        cCheck = pszSavedLine[0];
    else
        cCheck = szLine[0];

    if (cCheck == '\0' || (cCheck == '\n' && cCheck == '\0') || (cCheck == '\r' && cCheck == '\0'))
        bWTime = FALSE;

    if (eDisplayMode == D_FILE || eDisplayMode == D_BOTH)
    { // Make sure it's open
        if (!pFile)
        {
            if (!sFileName.size())
            {
                if (pszSavedLine)
                {
                    delete pszSavedLine;
                }
                return (-1);
            }
            if (!OpenLogFile())
            {
                if (pszSavedLine)
                {
                    delete pszSavedLine;
                }
#if defined(_WIN32)
                return ((int) ::GetLastError);
#else
                return ((int) errno);
#endif
            }
        };

        // Write the line
        // Lock
        cCriticalSection.Lock();

        // If called for, add the timestamp
        if (bTimestamp && bWTime && !bSuppressTimestamp)
            if ((iRet = ::fputs(GetTimestamp(), pFile)) < 0)
            {
                cCriticalSection.Unlock();
                if (pszSavedLine)
                {
                    delete pszSavedLine;
                }
#if defined(_WIN32)
                return ((int) ::GetLastError);
#else
                return ((int) errno);
#endif
            };

        if (pszSavedLine)
            iRet = ::fputs(pszSavedLine, pFile);
        else
            iRet = ::fputs(szLine, pFile);
        ::fflush(pFile);

        // Unlock
        cCriticalSection.Unlock();

        if (iRet < 0)
        {
            if (pszSavedLine)
            {
                delete pszSavedLine;
            }
#if defined(_WIN32)
            return ((int) ::GetLastError);
#else
            return ((int) errno);
#endif
        }

        // Close?
        if (bCloseMode)
            CloseLogFile();
    };

    // Write to display?
    if (eDisplayMode == D_CONSOLE || eDisplayMode == D_BOTH)
    {
        cCriticalSection.Lock();
        if (bTimestamp && bWTime)
            printf("%s", GetTimestamp());
        if (pszSavedLine)
            printf("%s", pszSavedLine);
        else
            printf("%s", szLine);
        cCriticalSection.Unlock();
    };

    if (pszSavedLine)
    {
        delete pszSavedLine;
    }

    return (0);
};


// Close the log file

void CLogging::CloseLogFile()
{
    cCriticalSection.Lock();
    if (pFile)
    {
        ::fclose(pFile);
        pFile = NULL;
    }
    cCriticalSection.Unlock();
};




// If TRUE, will open before each write and close afterwards.  Defaults to false.

void CLogging::SetCloseMode(BOOL bClose)
{
    bCloseMode = bClose;
};



// Returns the file name

const char *CLogging::GetFileName()
{
    return (sFileName.c_str());
}

// Returns the full file path

const char *CLogging::GetFilePath()
{
    return (sPath.c_str());
};


// Set display mode

void CLogging::SetOutputMode(DISPLAY_MODE eMode)
{
    eDisplayMode = eMode;
};


// Get the file size

long CLogging::GetFileSize()
{
    long lLength = 0;
    struct stat buf;

    if (!pFile)
    {
        if (!OpenLogFile())
            return (-1);
    };

    if (!::stat(GetFileName(), &buf))
    {
        lLength = buf.st_size;
    }

    if (bCloseMode)
        CloseLogFile();
    return (lLength);
};


// Set the logging level. iLevel passed to WriteToLog must <= to be output.

void CLogging::SetLogLevel(int iLevel)
{
    if (iLevel < 0)
        iLevel = 0;
    if (iLevel > 99)
        iLevel = 99;
    iLogLevel = iLevel;
};


// Set file flush interval (seconds)

void CLogging::SetFileFlushInterval(int iFlush)
{
    if (iFlush < 1)
        iFlush = 1;
    iFileFlushInterval = iFlush;

    cFlushThread.StopThread();
    cFlushThread.WaitForThreadCompletion();
    cFlushThread.StartThread(this);

};

BOOL CLogging::OKtoClose(void)
{
    return TRUE;
};

void CLogging::ChangingDay(void)
{
    return;
};


// Prefix time stamp to all lines

void CLogging::PrefixTimestamp(BOOL bTime)
{
    bTimestamp = bTime;
};


// Get a timestamp

const char *CLogging::GetTimestamp()
{
    time_t sTime;
    struct tm *sTm;

    sTime = time(NULL);
    PROTECT_CALL
    sTm = localtime(&sTime);
    if (sTm)
    {
        sprintf(szTimeStamp, "%02d-%02d-%02d %02d:%02d:%02d", sTm->tm_year + 1900, sTm->tm_mon + 1, sTm->tm_mday, sTm->tm_hour, sTm->tm_min, sTm->tm_sec);
    }
    ::strcat(szTimeStamp, " ");
    UNPROTECT_CALL

    return (szTimeStamp);
};


// Create the log file name

const char *CLogging::CreateFileName(const char *szBaseName, const char *szDir, BOOL bDayExtension, const char *szExtension)
{
    char szTimeBuffer[32];
    time_t sTime;
    struct tm *sToday;
    const char *lpDir = szDir;
    char szFullPath[512];
    static string sName;

#if defined(_WIN32)
    char *lpWork;

    strcpy(szFullPath, "\\");
    if (::GetModuleFileName(NULL, szFullPath, sizeof (szFullPath)))
    {
        lpWork = strrchr(szFullPath, '\\');
        if (lpWork)
            *(++lpWork) = '\0';
    };
#else
    strcpy(szFullPath, szDir);
#endif

    sName.erase();

#if defined(_WIN32)
    // Create a fully qualified path in case we're running as a service
    if ((*lpDir == '.' && (lpDir[1] == '\\' || lpDir[1] == '/')) ||
        (lpDir[1] != ':' && *lpDir != '\\' && *lpDir != '/'))
        sName = szFullPath;

    if (*lpDir == '.' && (lpDir[1] == '\\' || lpDir[1] == '/'))
        lpDir += 2;
#endif

    // Create the file
    if ((!lpDir || !strlen(lpDir)) && !sName.size())
        lpDir = szFullPath;
    sName += lpDir;
    if (lpDir[strlen(lpDir) - 1] != '\\' && lpDir[strlen(lpDir) - 1] != '/')
        sName += PATH_CHAR;
    sName += szBaseName;
    if (bDayExtension)
    {
        sTime = ::time(NULL);
        PROTECT_CALL
        sToday = ::localtime(&sTime);
        if (sToday)
        {
            // save the year and data so we can check for a date change
            iYear = sToday->tm_year;
            iDayofYear = sToday->tm_yday;
            ::strftime(szTimeBuffer, sizeof (szTimeBuffer), "%m-%d-%Y", sToday);
        }
        else
            szTimeBuffer[0] = '\0';
        UNPROTECT_CALL
        if (szBaseName && strlen(szBaseName) > 0)
            sName += "-";
        sName += szTimeBuffer;
    };

    sName += szExtension;

    return (sName.c_str());
};


// Was the file created on the open?

BOOL CLogging::WasCreatedOnOpen()
{
    return (bWasCreated);
};
