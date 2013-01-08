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
//! Base64Coder.h: interface for the Base64Coder class.

#if !defined(AFX_BASE64CODER_H__B2E45717_0625_11D2_A80A_00C04FB6794C__INCLUDED_)
#define AFX_BASE64CODER_H__B2E45717_0625_11D2_A80A_00C04FB6794C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif //! _MSC_VER >= 1000

#include "XPlat.h"

//! Class to implement Base64 encoding and decoding

class Base64Coder {
    //! Internal bucket class.

    class TempBucket {
    public:
        unsigned char nData[4];
        unsigned char nSize;

        void Clear() {
            memset(nData, 0, 4);
            nSize = 0;
        };
    };

    unsigned char *m_pDBuffer;
    unsigned char *m_pEBuffer;
    int m_nDBufLen;
    int m_nEBufLen;
    int m_nDDataLen;
    int m_nEDataLen;

public:
    Base64Coder();
    virtual ~Base64Coder();

public:
    virtual void Encode(const unsigned char *, int);
    virtual void Decode(const unsigned char *, int);
    virtual void Encode(const char *sMessage);
    virtual void Decode(const char *sMessage);

    virtual unsigned char *DecodedMessage() const;
    virtual char *EncodedMessage() const;

    virtual void AllocEncode(int);
    virtual void AllocDecode(int);
    virtual void SetEncodeBuffer(const unsigned char * pBuffer, int nBufLen);
    virtual void SetDecodeBuffer(const unsigned char * pBuffer, int nBufLen);

    virtual int GetEncodedMessageLength() {
        return m_nEDataLen;
    };

    virtual int GetDecodedMessageLength() {
        return m_nDDataLen;
    };

protected:
    virtual void _EncodeToBuffer(const TempBucket &Decode, unsigned char * pBuffer);
    virtual int _DecodeToBuffer(const TempBucket &Decode, unsigned char *pBuffer);
    virtual void _EncodeRaw(TempBucket &, const TempBucket &);
    virtual void _DecodeRaw(TempBucket &, const TempBucket &);
    virtual bool _IsBadMimeChar(unsigned char);

    static char m_DecodeTable[256];
    static bool m_Init;
    void _Init();
};

#endif //! !defined(AFX_BASE64CODER_H__B2E45717_0625_11D2_A80A_00C04FB6794C__INCLUDED_)
