/* 
 * File:   CSecureFileIO.h
 * Author: rbross
 *
 * Created on October 14, 2010, 2:56 PM
 */

#ifndef CSECUREFILEIO_H
#define	CSECUREFILEIO_H

#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <string>
#include <vector>

using namespace std;

class CSecureFileIO
{
public:
    CSecureFileIO();
    virtual ~CSecureFileIO();

    // !Retrieve an encrypted file.  Returns bytes read or -1
    int ReadEncyptedFile(const char *pFilePath, string &sBuffer);
    // !Write an encrypted file. Returns bytes written or -1
    int WriteEncyptedFile(const char *pFilePath, const char *pBuffer);
    //! Encrypt buffer
    void BF_encrypt(const unsigned char *keydata, int keydatalen, unsigned char *in, unsigned char *out, unsigned int inlen);
    //! Decrypt buffer
    void BF_decrypt(const unsigned char *keydata, int keydatalen, unsigned char *in, unsigned char *out, unsigned int inlen);
protected:


public:
    unsigned char *ucKey;
    int iKeySize;
};

#endif	/* CSECUREFILEIO_H */

