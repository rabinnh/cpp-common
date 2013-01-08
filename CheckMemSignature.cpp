/* 
 * File:   CheckMemSignature.cpp
 * Author: rbross
 * 
 * Created on September 22, 2010, 10:48 AM
 */

#include "CheckMemSignature.h"
#include "executeshell.h"
#include <sstream>
#include <iomanip>

CheckMemSignature::CheckMemSignature(bool bMemLog)
{   // Get our hardware key
    if (!GetHardwareKey())
    {
        ::openlog("[spore].checkmemsignature", LOG_PID, LOG_LOCAL0);
        ::syslog(LOG_CRIT, "Unable to create encryption key");
        ::closelog();
        return;
    }
}

CheckMemSignature::~CheckMemSignature()
{
}

bool CheckMemSignature::GetResult()
{
#ifdef PRODUCTION_UNIT
    char strSig[SIG_LENGTH + 1];
    unsigned char uSig[HASH_LENGTH];
    char strHex[3];

    // Read physical memory
    if (ReadPhysicalMem(SIG_OFFSET, SIG_LENGTH, (unsigned char *) strSig) < 0)
        return false;
    strSig[SIG_LENGTH] = '\0';

    unsigned char *pMD5Value = GetMD5Hash((unsigned char *) strSig, SIG_LENGTH + 1);
    if (!pMD5Value)
    {
        ::openlog("[spore].checkmemsignature", LOG_PID, LOG_LOCAL0);
        ::syslog(LOG_WARNING, "Unable to create a valid MD5 hash");
        ::closelog();
        return false;
    }

    // Convert hex hash string to binary
    strHex[2] = '\0';
    // ALIX 3D2
    for(int iIndex = 0; iIndex < HASH_LENGTH << 1; iIndex += 2)
    {
        ::memcpy(strHex, SIG_HASH3 + iIndex, 2);
        uSig[iIndex >> 1] = ConvertHexByte(strHex);
    }

    // Compare result
    int iX = ::memcmp(uSig, pMD5Value, HASH_LENGTH);
    if (iX == 0)
        return true;

    // ALIX 6E1
    for(int iIndex = 0; iIndex < HASH_LENGTH << 1; iIndex += 2)
    {
        ::memcpy(strHex, SIG_HASH6 + iIndex, 2);
        uSig[iIndex >> 1] = ConvertHexByte(strHex);
    }

    // Compare result
    iX = ::memcmp(uSig, pMD5Value, HASH_LENGTH);
    if (iX == 0)
        return true;
    ::openlog("[spore].checkmemsignature", LOG_PID, LOG_LOCAL0);
    ::syslog(LOG_CRIT, "Hardware error; unable to find valid MD5 hash.");
    ::closelog();

    return false;
#else 
    return true;
#endif
}

// Get an MD5hash

unsigned char *CheckMemSignature::GetMD5Hash(unsigned char *pBuffer, int iBufLen)
{
    EVP_MD_CTX mdctx;
    const EVP_MD *md;
    unsigned int md_len;

    // Now calculate the hash for the buffer and compared to the stored hash
    OpenSSL_add_all_digests();

    md = EVP_get_digestbyname("md5");

    if (!md)
    {
        ::openlog("[spore].checkmemsignature", LOG_PID, LOG_LOCAL0);
        ::syslog(LOG_ERR, "Unable to initialize MD5 digest");
        ::closelog();
        return NULL;
    }

    EVP_MD_CTX_init(&mdctx);
    EVP_DigestInit_ex(&mdctx, md, NULL);
    EVP_DigestUpdate(&mdctx, pBuffer, iBufLen);
    EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
    EVP_MD_CTX_cleanup(&mdctx);

    return md_value;
}

// Read physical memory

int CheckMemSignature::ReadPhysicalMem(int iOffset, int iSize, unsigned char *pBuffer)
{
    int mem_fd;
    // Read the memory at the designated offset
    if ((mem_fd = ::open("/dev/mem", O_RDONLY)) < 0)
    {
        string sLogOutput = "Error checking Spore memory: ";
        sLogOutput += ::strerror(errno);
        ::openlog("[spore].checkmemsignature", LOG_PID, LOG_LOCAL0);
        ::syslog(LOG_ERR, "%s", sLogOutput.c_str());
        ::closelog();
        return -1;
    }
    ::lseek(mem_fd, iOffset, SEEK_SET);
    ::memset(pBuffer, 0, iSize);
    int iRet = ::read(mem_fd, pBuffer, iSize);
    ::close(mem_fd);
    if (iRet != iSize)
    {
        ::openlog("[spore].checkmemsignature", LOG_PID, LOG_LOCAL0);
        ::syslog(LOG_ERR, "Cannot read sufficient hardware memory buffer");
        ::closelog();
        return -1;
    }

    return 0;
}


// Get a hardware specific key

unsigned char *CheckMemSignature::GetHardwareKey()
{
    string  sKey;

    ::memset(ucHardwareKey, 0, sizeof(ucHardwareKey));

//  This is no good because if the user is plugged into eth1 you can't get the address
//    string sIPv6MacAddr = GetIPv6MacAddr("eth0");

        // Get MAC address the easy way
    ExecuteShell::Execute("/sbin/ifconfig eth0 | grep HWaddr | cut -d' ' -f11", sKey);
    // Make sure that we got something back
    if (!sKey.length())
    {
        ::openlog("[spore].checkmemsignature", LOG_PID, LOG_LOCAL0);
        ::syslog(LOG_ERR, "Unable to retrieve network parameter");
        ::closelog();
        return NULL;
    }

    sKey.insert(0, "silverspore");
    sKey += "checkmemsignatute";


    // Now turn it into an MD5 hash for the key
    unsigned char *pHash = GetMD5Hash((unsigned char *) sKey.c_str(), sKey.length());
    if (!pHash)
        return NULL;
    ::memcpy(ucHardwareKey, pHash, sizeof(ucHardwareKey));
    return ucHardwareKey;
}

// Get the hardware key
const char *CheckMemSignature::DisplayKey()
{
static string sKey;
    char szFormat[3];

    sKey.clear();
    for (int iX = 0; iX < sizeof(ucHardwareKey); iX++)
    {
        ::sprintf(szFormat, "%02x", ucHardwareKey[iX]);
        sKey += szFormat;
    }

    return sKey.c_str();
}

// Get serial number

const char *CheckMemSignature::GetSerialNumber()
{
    char strSig[SIG_LENGTH + 1];
    string sBuffer;

    // Get MAC address the easy way
    ExecuteShell::Execute("/sbin/ifconfig eth0 | grep HWaddr | cut -d' ' -f11", sBuffer);

    // Convert MAC address to something else
    string sHexKey;

    for(int iX = 6; iX < sBuffer.length(); iX++)
    {
        char strNibble[3];
        if (sBuffer[iX] == ':' || sBuffer[iX] == '\n')
            continue;
        string sNibble = "0x";
        sNibble += sBuffer[iX];
        char *p;
        unsigned char cNibble = ::strtoul(sNibble.c_str(), &p, 16);
        cNibble ^= 0xf;
        ::sprintf(strNibble, "%x", cNibble);
        sHexKey += strNibble[0];
    }

    // Read physical memory
    if (ReadPhysicalMem(SIG_OFFSET, SIG_LENGTH, (unsigned char *) strSig) < 0)
    {
        sSerialNo = "DEMO_MODE";
        return(sSerialNo.c_str());
    }

    strSig[SIG_LENGTH] = '\0';

    if (!::strcmp(strSig, ALIX3D2_SIG))
    {
        sSerialNo = "PCX.32.";
        sSerialNo += sHexKey;
    }
    else if (!::strcmp(strSig, ALIX6E1_SIG))
    {
        sSerialNo = "PCX.61.";
        sSerialNo += sHexKey;
    }
    else
    {
        sSerialNo = "DEMO_MODE";
    }

    return(sSerialNo.c_str());

}


// Convert hex byte string to unsigned char

unsigned int CheckMemSignature::ConvertHexByte(const char *pHex)
{
    unsigned int ucDigit;

    stringstream ss;
    ss << std::hex << pHex;
    ss >> ucDigit;
    return(ucDigit);
}


// Get the IPv6 MAC address
const char *CheckMemSignature::GetIPv6MacAddr(const char *strIfName)
{
static string sAddress;

    sAddress.clear();

    struct ifaddrs *ifa = NULL, *ifEntry = NULL;
    void *addPtr = NULL;
    int rc = 0;
    char addressBuffer[INET6_ADDRSTRLEN];

    rc = getifaddrs(&ifa);
    if (rc == 0)
    {
        for(ifEntry = ifa; ifEntry != NULL; ifEntry = ifEntry->ifa_next)
        {
            if (ifEntry->ifa_addr->sa_data == NULL)
            {
                continue;
            }
            if (::strcmp(ifEntry->ifa_name, strIfName))
                continue;
            else if (ifEntry->ifa_addr->sa_family == AF_INET6)
            {
                addPtr = &((struct sockaddr_in6 *) ifEntry->ifa_addr)->sin6_addr;
            }
            else
            {   // It IPv6
                continue;
            }

            // Make printable
            const char *a = inet_ntop(ifEntry->ifa_addr->sa_family, addPtr, addressBuffer, sizeof(addressBuffer));
            if (a != NULL)
            {
                sAddress = a;
                break;
            }
        }
    freeifaddrs(ifa);
    }

    return sAddress.c_str();
}
