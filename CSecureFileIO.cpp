/* 
 * File:   CSecureFileIO.cpp
 * Author: rbross
 * 
 * Created on October 14, 2010, 2:56 PM
 */

#include "CSecureFileIO.h"
#include <openssl/evp.h>
#include <openssl/blowfish.h>

// Open libsporekey if it exists.  If it doesn't, no encryption is done.

CSecureFileIO::CSecureFileIO()
{
    void *lib_handle;

    int (*generatekey)(unsigned char **ucKey);

    ucKey = NULL;
    iKeySize = 0;

    try
    {
        lib_handle = dlopen("/usr/local/silverspore/bin/libsporekey.so", RTLD_LAZY);
        if (!lib_handle)
            throw(0);
        generatekey = (int (*)(unsigned char **ucKey)) dlsym(lib_handle, "generatekey");
        char *pError = dlerror();
        if (pError)
            throw(0);

        iKeySize = (*generatekey)(&ucKey);
        if (!iKeySize || !ucKey)
            throw(0);

    }
    catch(int e)
    { // We don't know or care which, just ensure that they are not set
        iKeySize = 0;
        ucKey = NULL;
    }

    if (lib_handle)
        dlclose(lib_handle);

}

CSecureFileIO::~CSecureFileIO()
{
    if (ucKey)
        delete[] ucKey;
}

// Retrieve an encrypted file.  Returns bytes read or -1

int CSecureFileIO::ReadEncyptedFile(const char *pFilePath, string &sBuffer)
{
    char cFileBuffer[512];
    vector<unsigned char> mVec;

    sBuffer.clear();

    // Open the file
    FILE *pFile = ::fopen(pFilePath, "r");
    if (!pFile)
    { // If it is a permission error, send a service failure
        if (errno == EACCES)
            return -1;
        else
            return 0;
    }
    // Read the contents of the file in chunks
    int iRead;
    while((iRead = ::fread(cFileBuffer, 1, sizeof(cFileBuffer) - 1, pFile)) != 0)
    {
        for(int iX = 0; iX < iRead; iX++)
            mVec.push_back(cFileBuffer[iX]);
    }
    ::fclose(pFile);

    // If 0 length, bail
    if (mVec.size() < 1)
        return 0;

    // See if we have a key
    if (ucKey)
    {   // Reserve our output buffer
        // Ensure that we have a terminating 0, since decrypt doesn't know it's a string
        char *pDecrypted = new char[mVec.size() + 1];
        // Decrypt
        BF_decrypt(ucKey, iKeySize, &mVec[0], (unsigned char *) pDecrypted, mVec.size());
        sBuffer = pDecrypted;
        delete[] pDecrypted;
    }
    else
        sBuffer.assign(mVec.begin(), mVec.end());

    return sBuffer.length();
}


// Write an encrypted file. Returns bytes written or -1

int CSecureFileIO::WriteEncyptedFile(const char *pFilePath, const char *pWriteBuffer)
{
    // Open the file
    FILE *pFile = ::fopen(pFilePath, "w+");
    if (!pFile)
        return -1;

    // Encrypt the buffer
    int iLen = ::strlen(pWriteBuffer);
    if (iLen < 1)
        return -1;

    unsigned char *pEncrypt = new unsigned char[iLen];

    // If we have a key
    if (ucKey)
       BF_encrypt(ucKey, iKeySize, (unsigned char *) pWriteBuffer, pEncrypt, iLen);

    // Write the buffer
    int iWrite;
    unsigned char *pBuffer = ucKey ? pEncrypt : (unsigned char *) pWriteBuffer;
    iWrite = ::fwrite(pBuffer, 1, iLen, pFile);
    ::fclose(pFile);
    delete[] pEncrypt;

    return iWrite == iLen ? iLen : -1;
}

/*
 * ARGS:
 * keydata == ascii text, the encryption passphrase
 * keydatalen == how long keydata is
 * in == the data to be encrypted
 * out == the encrypted data.
 * inlen == length of the in array
 */
void CSecureFileIO::BF_encrypt(const unsigned char *keydata, int keydatalen, unsigned char *in, unsigned char *out, unsigned int inlen)
{
    BF_KEY key;
    unsigned char ivec[32];
    int num = 0;

    // set up for encryption
    BF_set_key(&key, keydatalen, keydata);
    memset(ivec, '\0', 32);
    BF_cfb64_encrypt(in, out, inlen, &key, ivec, &num, BF_ENCRYPT);
}

void CSecureFileIO::BF_decrypt(const unsigned char *keydata, int keydatalen, unsigned char *in, unsigned char *out, unsigned int inlen)
{
    BF_KEY key;
    unsigned char ivec[32];
    int num = 0;
    // set up for decryption
    BF_set_key(&key, keydatalen, keydata);
    memset(ivec, '\0', 32);
    BF_cfb64_encrypt(in, out, inlen, &key, ivec, &num, BF_DECRYPT);
}
