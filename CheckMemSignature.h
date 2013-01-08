/* 
 * File:   CheckMemSignature.h
 * Author: rbross
 *
 * Created on September 22, 2010, 10:48 AM
 */

#ifndef CHECKMEMSIGNATURE_H
#define	CHECKMEMSIGNATURE_H

#include <memory.h>
#include <stdio.h>
#include <fcntl.h>
#include <string>
#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/blowfish.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

using namespace std;

#define DEMO_LIMIT 5    // Number of endpoints that we allow for demo builds

#define SIG_OFFSET  0xFFFD0
#define SIG_LENGTH  17
#define SIG_HASH3   "c7e2846040d97eaf727e2266e257ac0d" //! In this case, this is the md5 hash for "PC Engines ALIX.3"
#define SIG_HASH6   "ddd8376f0dd9fa44c01897b62d0bf376" //! In this case, this is the md5 hash for "PC Engines ALIX.2"
#define HASH_LENGTH 16
#define ALIX3D2_SIG "PC Engines ALIX.3"
#define ALIX6E1_SIG "PC Engines ALIX.2"
#define KEY_SIZE    108

//! This class must be changed for each type of hardware a program is going to be run on.
//! Given a physical memory location (usually in the BIOS) it will check that we are on the proper HW.
class CheckMemSignature
{
public:
    CheckMemSignature(bool bMemLog = true);
    virtual ~CheckMemSignature();

    //! Return the result of the hardware check
    bool GetResult();
    //! Get an MD5 hash
    unsigned char *GetMD5Hash(unsigned char *pBuffer, int iBufLen);
    //! Get the hardware key
    const char *DisplayKey();

    //! Get serial number
    const char *GetSerialNumber();

protected:
    //! Read physical memory
    int ReadPhysicalMem(int iOffset, int iSize, unsigned char *pBuffer);
    //! Get a hardware specific key
    unsigned char *GetHardwareKey();
    //! Convert hex byte string to unsigned char
    unsigned int ConvertHexByte(const char *pHex);
    //! Given the interface name (ex; "eth0") get the IPv6 MAC address
    const char *GetIPv6MacAddr(const char *strIfName);

public:
    unsigned char ucHardwareKey[HASH_LENGTH];

protected:
    unsigned char md_value[EVP_MAX_MD_SIZE];
    string ipv6mac;
    string sSerialNo;
};

#endif	/* CHECKMEMSIGNATURE_H */

