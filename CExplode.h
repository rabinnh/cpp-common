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
 * File:   CExplode.h
 * Author: rbross
 *
 * Created on October 5, 2011, 7:49 AM
 */

#ifndef CEXPLODE_H
#define	CEXPLODE_H

#include <vector>
#include <string>

using namespace std;

typedef vector<string>  VECTOR_PHRASES;

class CExplode
{
public:
    CExplode();
    virtual ~CExplode();

// Given a string and a delimiter and a delimiter, pass back sections in a vector reference,
// Returns number of elements or -1 on error
static int Explode(const char *pString, const char *pDelims, VECTOR_PHRASES &vPhrases);
private:

};

#endif	/* CEXPLODE_H */

