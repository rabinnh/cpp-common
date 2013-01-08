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
/* 
 * File:   CLinuxConfigReader.cpp
 * Author: rbross
 * 
 * Created on April 20, 2012, 12:47 PM
 */

#include "CLinuxConfigReader.h"
#include <fstream>

string CLinuxConfigReader::sValue;

CLinuxConfigReader::CLinuxConfigReader()
{
}

CLinuxConfigReader::CLinuxConfigReader(const CLinuxConfigReader& orig)
{
    mConfig = orig.mConfig;
}

CLinuxConfigReader::~CLinuxConfigReader()
{
}

bool CLinuxConfigReader::ReadConfig(const char *pPath)
{   // Come on!
    if (!pPath || !mConfig.size())
        return false;
    
    // Open it
    ifstream configFile(pPath);
    if (!configFile.is_open())
        return false;
    
    MAP_CONFIG_ITER mIter;
    
    // While no flags set
    while(configFile.good())
    {
        string sLine;
        getline(configFile, sLine);
        // Get the first character of the line
        int iIndex = sLine.find_first_not_of(" \t\r");
        if (iIndex == string::npos)
            continue;
        // Comment?
        if (sLine.at(iIndex) == '#')
            continue;
        // Now find the '='
        int iEquals = sLine.find("=");
        if (iEquals == string::npos)
            continue;
        // Extract the key and the value
        string sKey = sLine.substr(iIndex, iEquals - iIndex);
        // Trim trailing white space
        iIndex = sKey.find_last_not_of(' ');
        if (iIndex == string::npos)
            continue;
        sKey.resize(iIndex + 1);
        
        // Before we parse for the value, let's see if we are looking for this key.
// ** This was so epically dumb on my part not just to store all valid values
//        mIter = mConfig.find(sKey);
//        if (mIter == mConfig.end())
//            continue;
        
        // Now the value
        // Get the first character of the value
        sLine = sLine.substr(iEquals + 1);
        iIndex = sLine.find_first_not_of(" \t\r");
        if (iIndex == string::npos)
            continue;
        sLine = sLine.substr(iIndex);
        // Find the end
        iIndex = sLine.find_first_of(" \t\r\n");
        if (iIndex != string::npos)
            sLine.resize(iIndex);
        
        // Now we can save the value
        mConfig[sKey] = sLine;
    }
    

    return false;
}


    // Read a configuration value
string &CLinuxConfigReader::ReadConfigValue(const char *pKey, const char *pDefault)
{
    const char *pBlank = "";;
    if (!pDefault)
        pDefault = pBlank;
    MAP_CONFIG_ITER mIter = mConfig.find(pKey);
    sValue = mIter != mConfig.end() ? mIter->second : pDefault;
    return sValue;
}
