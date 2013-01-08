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
//	CRC32.cpp - implementation of CRC object
//		Richard A. Bross, 1/23/97
#if defined(_WIN32)
#include <stdafx.h>
#endif
#include "Crc32.h"

	// Constructor
CRC32::CRC32()
{		// Load the  CRC32 table
	unsigned int	ix;			// Index 
	unsigned char	bx;
	int				carry;

		// Build the table 
	for (ix = 0; ix < 256; ix++)
		{
		lCrcTable[ix] = ix;
		for (bx = 0; bx < 8; bx++)
			{
			carry = lCrcTable[ix] & 1;
			lCrcTable[ix] >>= 1;
			if (carry)
				lCrcTable[ix] ^= 0xEDB88320;
			};
		};
};


	// Destructor
CRC32::~CRC32()
{	
};

	// Calculate a CRC.  For first call when calculating for a stream, 
	// lInit should be = 0xFFFFFFFF.
unsigned long CRC32::CalcCRC(unsigned long lInit, 
								unsigned char *lpBuffer, int iLength)
{
	int				ix;			/* Index */
	unsigned char	bx;

		/* Now calculate the CRC */
	for (ix = 0; ix < iLength; ix++)
		{
		bx = ((unsigned char *)(&lInit))[0]; 	/* Set up index into table */
		bx ^= lpBuffer[ix];
		lInit >>= 8;                 /* Shift a complete byte */
		((unsigned char*)(&lInit))[3] = 0;	/* Make sure MSB = 0 */
		lInit ^= lCrcTable[bx];		/* XOR with table entry */
		};
	return(lInit);
};


	
