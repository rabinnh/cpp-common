/* 
 * File:   CSporeEncrypt.h
 * Author: rbross
 *
 * Created on March 2, 2011, 12:43 PM
 */

#ifndef CSPOREENCRYPT_H
#define	CSPOREENCRYPT_H

#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/blowfish.h>

//! Class to encrypt data
class CSporeEncrypt
{
public:
    CSporeEncrypt();
    virtual ~CSporeEncrypt();

    //! Encrypt buffer
static void BF_encrypt(const unsigned char *keydata, int keydatalen, unsigned char *in, unsigned char *out, unsigned int inlen);
    //! Decrypt buffer
static void BF_decrypt(const unsigned char *keydata, int keydatalen, unsigned char *in, unsigned char *out, unsigned int inlen);

protected:
static void GetKey();

public:
static int iKeySize;
static unsigned char *ucKey;
};

#endif	/* CSPOREENCRYPT_H */

