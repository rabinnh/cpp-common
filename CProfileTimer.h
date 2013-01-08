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
 * File:   CProfileTimer.h
 * Author: rbross
 *
 * Created on April 11, 2012, 8:36 AM
 */

#ifndef CPROFILETIMER_H
#define	CPROFILETIMER_H

#include <time.h>
#include <sys/time.h>
#include <map>
#include <iostream>
#include <string>

using namespace std;

typedef map<string, struct timeval> MAP_TIMES;
typedef MAP_TIMES::iterator MAP_TIMES_ITER;

class ElapsedTime
{
public:
    ElapsedTime() 
    {
        lSec = 0;
        lMS = 0;
        lUS = 0;
    };
    ElapsedTime(const ElapsedTime& orig)
    {
        lSec = orig.lSec;
        lMS = orig.lMS;
        lUS = orig.lUS;
    };
    
    virtual ~ElapsedTime() {};
    
    long    lSec;
    long    lMS;
    long    lUS;
};

class CProfileTimer
{
public:
    CProfileTimer();
    virtual ~CProfileTimer();

    // Non static call without labels to start a timer.  For very time sensitive processes, this avoids using the map
    void Start();
    // Get the elapsed time, if bReset is true reset the start time
    void GetElapsedTime(ElapsedTime &cElapsed, bool bReset = true);
    // Set and store a timestamp
    static void SetTimestamp(const char *pLabel);
    // Set the second stamp and get the difference at the same time.  If bMarkLabel2 is true, save current time for label 2
    static void GetElapsed(ElapsedTime &cElapsed, const char *pLabel1, const char *pLabel2, bool bMarkLabel2 = false);
    // Get the elapsed time from start until now
    static void GetElapsedNow(ElapsedTime &cElapsed, const char *pLabel1);
    // Set the second, get the difference and print
    static void PrintElapsed(const char *pLabel1, const char *pLabel2, bool bMarkLabel2 = false, const char *pTag = NULL);
    // Return just the number of ms 
    static long GetElapsedMS(const char *pLabel1, const char *pLabel2, bool bMarkLabel2 = false);
    // Clear the map
    static void Clear();

protected:
    static struct timeval GetLabelTimestamp(const char *pLabel);
    static ElapsedTime GetDifference(ElapsedTime &cElapsed, struct timeval *sT1, struct timeval *sT2);

protected:
    struct timeval sStart;
    struct timeval sEnd;
    
private:
    static MAP_TIMES mTimes;

};

#endif	/* CPROFILETIMER_H */

