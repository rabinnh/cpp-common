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
*/// Base64Coder.cpp: implementation of the Base64Coder class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "stdafx.h"
#endif
#include "Base64Coder.h"

// Digits...
static char Base64Digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

bool Base64Coder::m_Init = FALSE;
char Base64Coder::m_DecodeTable[256];

#ifndef PAGESIZE
#define PAGESIZE					4096
#endif

#ifndef ROUNDTOPAGE
#define ROUNDTOPAGE(a)			(((a/4096)+1)*4096)
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Base64Coder::Base64Coder() : m_pDBuffer(NULL), m_pEBuffer(NULL), m_nDBufLen(0), m_nEBufLen(0) {

}

Base64Coder::~Base64Coder() {
    if (m_pDBuffer != NULL)
        delete [] m_pDBuffer;

    if (m_pEBuffer != NULL)
        delete [] m_pEBuffer;
}

unsigned char *Base64Coder::DecodedMessage() const {
    return (unsigned char *) m_pDBuffer;
}

char * Base64Coder::EncodedMessage() const {
    return (char *) m_pEBuffer;
}

void Base64Coder::AllocEncode(int nSize) {
    if (m_nEBufLen < nSize) {
        if (m_pEBuffer != NULL)
            delete [] m_pEBuffer;

        m_nEBufLen = ROUNDTOPAGE(nSize);
        m_pEBuffer = new unsigned char[m_nEBufLen];
    }

    memset(m_pEBuffer, 0, m_nEBufLen);
    m_nEDataLen = 0;
}

void Base64Coder::AllocDecode(int nSize) {
    if (m_nDBufLen < nSize) {
        if (m_pDBuffer != NULL)
            delete [] m_pDBuffer;

        m_nDBufLen = ROUNDTOPAGE(nSize);
        m_pDBuffer = new unsigned char[m_nDBufLen];
    }

    memset(m_pDBuffer, 0, m_nDBufLen);
    m_nDDataLen = 0;
}

void Base64Coder::SetEncodeBuffer(const unsigned char *pBuffer, int nBufLen) {
    int i = 0;

    AllocEncode(nBufLen);
    while (i < nBufLen) {
        if (!_IsBadMimeChar(pBuffer[i])) {
            m_pEBuffer[m_nEDataLen] = pBuffer[i];
            m_nEDataLen++;
        }

        i++;
    }
}

void Base64Coder::SetDecodeBuffer(const unsigned char * pBuffer, int nBufLen) {
    AllocDecode(nBufLen);
    memcpy(m_pDBuffer, pBuffer, nBufLen);
    m_nDDataLen = nBufLen;
}

void Base64Coder::Encode(const unsigned char *pBuffer, int nBufLen) {
    SetDecodeBuffer(pBuffer, nBufLen);
    AllocEncode(nBufLen * 2);

    TempBucket Raw;
    int nIndex = 0;

    while ((nIndex + 3) <= nBufLen) {
        Raw.Clear();
        memcpy(&Raw, m_pDBuffer + nIndex, 3);
        Raw.nSize = 3;
        _EncodeToBuffer(Raw, m_pEBuffer + m_nEDataLen);
        nIndex += 3;
        m_nEDataLen += 4;
    }

    if (nBufLen > nIndex) {
        Raw.Clear();
        Raw.nSize = (unsigned char) (nBufLen - nIndex);
        memcpy(&Raw, m_pDBuffer + nIndex, nBufLen - nIndex);
        _EncodeToBuffer(Raw, m_pEBuffer + m_nEDataLen);
        m_nEDataLen += 4;
    }
}

void Base64Coder::Encode(const char *szMessage) {
    if (szMessage != NULL)
        Base64Coder::Encode((const unsigned char *) szMessage, strlen(szMessage));
}

void Base64Coder::Decode(const unsigned char *pBuffer, int dwBufLen) {
    if (!Base64Coder::m_Init)
        _Init();

    SetEncodeBuffer(pBuffer, dwBufLen);

    AllocDecode(dwBufLen);

    TempBucket Raw;

    int nIndex = 0;

    while ((nIndex + 4) <= m_nEDataLen) {
        Raw.Clear();
        Raw.nData[0] = Base64Coder::m_DecodeTable[m_pEBuffer[nIndex]];
        Raw.nData[1] = Base64Coder::m_DecodeTable[m_pEBuffer[nIndex + 1]];
        Raw.nData[2] = Base64Coder::m_DecodeTable[m_pEBuffer[nIndex + 2]];
        Raw.nData[3] = Base64Coder::m_DecodeTable[m_pEBuffer[nIndex + 3]];

        if (Raw.nData[2] == 255)
            Raw.nData[2] = 0;
        if (Raw.nData[3] == 255)
            Raw.nData[3] = 0;

        Raw.nSize = 4;
        _DecodeToBuffer(Raw, m_pDBuffer + m_nDDataLen);
        nIndex += 4;
        m_nDDataLen += 3;
    }

    // If nIndex < m_nEDataLen, then we got a decode message without padding.
    // We may want to throw some kind of warning here, but we are still required
    // to handle the decoding as if it was properly padded.
    if (nIndex < m_nEDataLen) {
        Raw.Clear();
        for (int i = nIndex; i < m_nEDataLen; i++) {
            Raw.nData[i - nIndex] = Base64Coder::m_DecodeTable[m_pEBuffer[i]];
            Raw.nSize++;
            if (Raw.nData[i - nIndex] == 255)
                Raw.nData[i - nIndex] = 0;
        }

        _DecodeToBuffer(Raw, m_pDBuffer + m_nDDataLen);
        m_nDDataLen += (m_nEDataLen - nIndex);
    }
}

void Base64Coder::Decode(const char *szMessage) {
    if (szMessage != NULL)
        Base64Coder::Decode((const unsigned char *) szMessage, strlen(szMessage));
}

int Base64Coder::_DecodeToBuffer(const TempBucket &Decode, unsigned char *pBuffer) {
    TempBucket Data;
    int nCount = 0;

    _DecodeRaw(Data, Decode);

    for (int i = 0; i < 3; i++) {
        pBuffer[i] = Data.nData[i];
        if (pBuffer[i] != 255)
            nCount++;
    }

    return nCount;
}

void Base64Coder::_EncodeToBuffer(const TempBucket &Decode, unsigned char * pBuffer) {
    TempBucket Data;

    _EncodeRaw(Data, Decode);

    for (int i = 0; i < 4; i++)
        pBuffer[i] = Base64Digits[Data.nData[i]];

    switch (Decode.nSize) {
        case 1:
            pBuffer[2] = '=';
        case 2:
            pBuffer[3] = '=';
    }
}

void Base64Coder::_DecodeRaw(TempBucket &Data, const TempBucket &Decode) {
    unsigned char nTemp;

    Data.nData[0] = Decode.nData[0];
    Data.nData[0] <<= 2;

    nTemp = Decode.nData[1];
    nTemp >>= 4;
    nTemp &= 0x03;
    Data.nData[0] |= nTemp;

    Data.nData[1] = Decode.nData[1];
    Data.nData[1] <<= 4;

    nTemp = Decode.nData[2];
    nTemp >>= 2;
    nTemp &= 0x0F;
    Data.nData[1] |= nTemp;

    Data.nData[2] = Decode.nData[2];
    Data.nData[2] <<= 6;
    nTemp = Decode.nData[3];
    nTemp &= 0x3F;
    Data.nData[2] |= nTemp;
}

void Base64Coder::_EncodeRaw(TempBucket &Data, const TempBucket &Decode) {
    unsigned char nTemp;

    Data.nData[0] = Decode.nData[0];
    Data.nData[0] >>= 2;

    Data.nData[1] = Decode.nData[0];
    Data.nData[1] <<= 4;
    nTemp = Decode.nData[1];
    nTemp >>= 4;
    Data.nData[1] |= nTemp;
    Data.nData[1] &= 0x3F;

    Data.nData[2] = Decode.nData[1];
    Data.nData[2] <<= 2;

    nTemp = Decode.nData[2];
    nTemp >>= 6;

    Data.nData[2] |= nTemp;
    Data.nData[2] &= 0x3F;

    Data.nData[3] = Decode.nData[2];
    Data.nData[3] &= 0x3F;
}

bool Base64Coder::_IsBadMimeChar(unsigned char nData) {
    switch (nData) {
        case '\r': case '\n': case '\t': case ' ':
        case '\b': case '\a': case '\f': case '\v':
            return TRUE;
        default:
            return FALSE;
    }
}

void Base64Coder::_Init() { // Initialize Decoding table.

    int i;

    for (i = 0; i < 256; i++)
        Base64Coder::m_DecodeTable[i] = -2;

    for (i = 0; i < 64; i++) {
        Base64Coder::m_DecodeTable[Base64Digits[i]] = i;
        Base64Coder::m_DecodeTable[Base64Digits[i] | 0x80] = i;
    }

    Base64Coder::m_DecodeTable['='] = -1;
    Base64Coder::m_DecodeTable['=' | 0x80] = -1;

    Base64Coder::m_Init = TRUE;
}

