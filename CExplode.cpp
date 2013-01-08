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
 * File:   CExplode.cpp
 * Author: rbross
 * 
 * Created on October 5, 2011, 7:49 AM
 */

#include "CExplode.h"
#include "string.h"

CExplode::CExplode()
{
}


CExplode::~CExplode()
{
}

// Given a string and a delimiter, pass back sections in a vector reference,
int CExplode::Explode(const char *pString, const char *pDelims, VECTOR_PHRASES &vPhrases)
{
    char *pTok;
    char *pStringCopy;
    
    vPhrases.clear();

    // Not void, right?
    if (!pString)
        return -1;
    // Make a copy so strtok doesn't destroy the string
    pStringCopy = new char[::strlen(pString) + 1];
    ::strcpy(pStringCopy, pString);

    pTok = ::strtok(pStringCopy, pDelims);
    while (pTok != NULL)
    {
        vPhrases.push_back(pTok);
        pTok = ::strtok(NULL, pDelims);
    }
    delete[] pStringCopy;

    return(vPhrases.size());
}


