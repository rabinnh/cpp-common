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
/* 
 * File:   CProfileTimer.cpp
 * Author: rbross
 * 
 * Created on April 11, 2012, 8:36 AM
 */

#include "CProfileTimer.h"

// Static declarations
MAP_TIMES CProfileTimer::mTimes;

CProfileTimer::CProfileTimer()
{
    Clear();
}

CProfileTimer::~CProfileTimer()
{
}


// Search the map for the timestamp

struct timeval CProfileTimer::GetLabelTimestamp(const char *pLabel)
{
    struct timeval t1;
    MAP_TIMES_ITER mIter;

    mIter = mTimes.find(pLabel);
    if (mIter == mTimes.end())
        t1.tv_sec = -1;
    else
        t1 = mIter->second;
    
    return t1;
}



// Set and store a timestamp

void CProfileTimer::SetTimestamp(const char *pLabel)
{
    if (!pLabel)
        return;

    struct timeval sTime;
    ::gettimeofday(&sTime, NULL);

    mTimes[pLabel] = sTime;
}




// For very time sensitive processes, this avoids using the map
void CProfileTimer::Start()
{
    ::gettimeofday(&sStart, NULL);
}

// Get the elapsed time, if bReset is true reset the start time
void CProfileTimer::GetElapsedTime(ElapsedTime &cElapsed, bool bReset)
{
    ::gettimeofday(&sEnd, NULL);
    
    GetDifference(cElapsed, &sStart, &sEnd);
    
    if (bReset)
        sStart = sEnd;
}

// Get difference between timestamps

ElapsedTime CProfileTimer::GetDifference(ElapsedTime &cElapsed, struct timeval *sT1, struct timeval *sT2)
{
    cElapsed.lMS = 0;
    cElapsed.lSec = 0;
    cElapsed.lUS = 0;
    
    // First convert to ms, usec to make it easier
    // This can't be negative, just >= 0
    cElapsed.lMS = (sT2->tv_sec - sT1->tv_sec) * 1000;  // Sec to ms
    cElapsed.lUS = sT2->tv_usec - sT1->tv_usec; // usec
    if (cElapsed.lUS < 0)
    {   // This is based on millions, so
        cElapsed.lUS = 1000000 + cElapsed.lUS;
        cElapsed.lMS -= 1000; // And that's one less second
    }
    // Make it secs:ms:usec
    cElapsed.lMS += cElapsed.lUS / 1000;    // How many ms are in the usec field?
    cElapsed.lUS %= 1000;                   // Remainder
    cElapsed.lSec = cElapsed.lMS / 1000;    // How many secs are in the ms field?
    cElapsed.lMS %= 1000;                   // Remainder
}


// Get the difference between 2 timestamps based on their labels

void CProfileTimer::GetElapsed(ElapsedTime &cElapsed, const char *pLabel1, const char *pLabel2, bool bMarkLabel2)
{   // Set as an error flag
    cElapsed.lSec = -1;
    if (!pLabel1 || !pLabel2)
        return;

    if (bMarkLabel2)
        SetTimestamp(pLabel2);

    struct timeval t1 = GetLabelTimestamp(pLabel1);
    if (t1.tv_sec == -1)
        return;
    struct timeval t2 = GetLabelTimestamp(pLabel2);
    if (t2.tv_sec == -1)
        return;

    GetDifference(cElapsed, &t1, &t2);
}

// Get the elapsed time from start until now
void CProfileTimer::GetElapsedNow(ElapsedTime &cElapsed, const char *pLabel1)
{
    // Set as an error flag
    cElapsed.lSec = -1;
    if (!pLabel1)
        return;

    struct timeval t1 = GetLabelTimestamp(pLabel1);
    if (t1.tv_sec == -1)
        return;
    struct timeval t2; 
    ::gettimeofday(&t2, NULL);

    GetDifference(cElapsed, &t1, &t2);
}


// Set the second, get the difference and print

void CProfileTimer::PrintElapsed(const char *pLabel1, const char *pLabel2, bool bMarkLabel2, const char *pTag)
{
    ElapsedTime cElapsed;
    cElapsed.lSec = -1;
    if (!pLabel1 || !pLabel2)
        return;

    if (bMarkLabel2)
        SetTimestamp(pLabel2);

    GetElapsed(cElapsed, pLabel1, pLabel2, bMarkLabel2);

    if (pTag)
        cout << pTag << "sec:ms:usec " << cElapsed.lSec << ":" << cElapsed.lMS << ":" << cElapsed.lUS << endl;
    else
        cout << "sec:ms:usec " << cElapsed.lSec << ":" << cElapsed.lMS << ":" << cElapsed.lUS << endl;
}


    // Return just the number of ms 
long CProfileTimer::GetElapsedMS(const char *pLabel1, const char *pLabel2, bool bMarkLabel2)
{
    ElapsedTime cElapsed;
    GetElapsed(cElapsed, pLabel1, pLabel2, bMarkLabel2);

    return((cElapsed.lSec * 1000) + cElapsed.lMS);
}


// Clear the map

void CProfileTimer::Clear()
{
    mTimes.clear();
}

