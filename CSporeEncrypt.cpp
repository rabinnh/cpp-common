/* 
 * File:   CSporeEncrypt.cpp
 * Author: rbross
 * 
 * Created on March 2, 2011, 12:43 PM
 */

#include "CSporeEncrypt.h"
#include <dlfcn.h>

int CSporeEncrypt::iKeySize = 0;
unsigned char *CSporeEncrypt::ucKey = NULL;

CSporeEncrypt::CSporeEncrypt()
{
}


CSporeEncrypt::~CSporeEncrypt()
{
}


// We will look for libsporekey.so in /usr/local/silverspore/bin/
void CSporeEncrypt::GetKey()
{
   // Returns size of key.  Caller must delete ucKey
   int (*generatekey)(unsigned char **ucKey);
   void *lib_handle;
   int iSize;

   CSporeEncrypt::ucKey = NULL;
   iSize = 0;

   lib_handle = dlopen("/usr/local/silverspore/bin/libspore.so", RTLD_LAZY);
   if (!lib_handle)
       return;

   generatekey = (int (*)(unsigned char **)) dlsym(lib_handle, "generatekey");
   if (dlerror() != NULL)
       return;

   CSporeEncrypt::iKeySize = (generatekey)(&CSporeEncrypt::ucKey);

   dlclose(lib_handle);
}


/*
 * ARGS:
 * keydata == ascii text, the encryption passphrase
 * keydatalen == how long keydata is
 * in == the data to be encrypted
 * out == the encrypted data.
 * inlen == length of the in array
 */
void CSporeEncrypt::BF_encrypt(const unsigned char *keydata, int keydatalen, unsigned char *in, unsigned char *out, unsigned int inlen)
{
    BF_KEY key;
    unsigned char ivec[32];
    int num = 0;

    // set up for encryption
    BF_set_key(&key, keydatalen, keydata);
    memset(ivec, '\0', 32);
    BF_cfb64_encrypt(in, out, inlen, &key, ivec, &num, BF_ENCRYPT);
}

void CSporeEncrypt::BF_decrypt(const unsigned char *keydata, int keydatalen, unsigned char *in, unsigned char *out, unsigned int inlen)
{
    BF_KEY key;
    unsigned char ivec[32];
    int num = 0;
    // set up for decryption
    BF_set_key(&key, keydatalen, keydata);
    memset(ivec, '\0', 32);
    BF_cfb64_encrypt(in, out, inlen, &key, ivec, &num, BF_DECRYPT);
}
