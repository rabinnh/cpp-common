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
/////////////////////////////////////////////////////////////////////////////
//	CRC32.h - header file for CRC32 object
//		Richard A. Bross, 1/23/97

#ifndef __CRC32_H
#define __CRC32_H

class CRC32 
{
public:
	CRC32();
	virtual ~CRC32();

public:		
	unsigned long	CalcCRC(unsigned long lInit, 
								unsigned char *lpBuffer, int iLength);
protected:	// Variables
	unsigned long	lCrcTable[256];

public:		// Methods
};
#endif


