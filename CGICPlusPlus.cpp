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
 * File:   CGICPlusPlus.cpp
 * Author: rbross
 * 
 * Created on September 23, 2010, 10:14 AM
 */

#include "CGICPlusPlus.h"

CGICPlusPlus::CGICPlusPlus()
{
    sResponse.clear();
    sHeader.clear();
    mCgiMap.clear();
    if (!ParseRequest())
    {
        SetResponseMimeType("text/plain");
        AddToResponse(STATUS_INVALID_REQUEST);
        OutputResponse();
        exit(1); // Bad request
    }
}

CGICPlusPlus::~CGICPlusPlus()
{
}

// Parse request

bool CGICPlusPlus::ParseRequest()
{
    register int i;
    char *request_method;
    int content_length;
    char *cgiinput;
    char *nvpair;
    char *eqpos;

    // Depending on the request method, read all CGI input into cgiinput.
    request_method = getenv("REQUEST_METHOD");

    if (!strcmp(request_method, "GET") || !strcmp(request_method, "HEAD"))
    {
        char *qs;
        qs = getenv("QUERY_STRING");
        cgiinput = strdup(qs ? qs : "");
    }
    else if (!strcmp(request_method, "POST") || !strcmp(request_method, "PUT"))
    {
        if (strncasecmp(getenv("CONTENT_TYPE"), CONTENT_FORM, ::strlen(CONTENT_FORM)))
            return false;
        if (!(content_length = atoi(getenv("CONTENT_LENGTH"))))
            return false;
        // Read the input data
        if (!(cgiinput = (char *) malloc(content_length + 1)))
            return false;
        if (!fread(cgiinput, content_length, 1, stdin))
            return false;
        cgiinput[content_length] = '\0';
    }
    else
        return false;

    // Change all plusses back to spaces.
    for (i = 0; cgiinput[i]; i++)
        if (cgiinput[i] == '+')
            cgiinput[i] = ' ';

    // Split on "&" and ";" to extract the name-value pairs into
    nvpair = strtok(cgiinput, "&;");
    while (nvpair)
    {
        string sKey;
        string sValue;
        // Get the value
        if (eqpos = strchr(nvpair, '='))
        {
            *eqpos = '\0';
            unescape_url(eqpos + 1);
        }
        else
        { // If no '=' sign, set the value to blank
            *(eqpos + 1) = '\0';
        }
        // Now escape the key
        unescape_url(nvpair);
        // Set the strings
        sKey = nvpair;
        sValue = eqpos + 1;
        // Save in the mape
        mCgiMap[sKey] = sValue;
        // Get the next one
        nvpair = strtok(NULL, "&;");
    }

    // Free buffer
    free(cgiinput);

    // We were successful
    return true;
}

// Direct the request based on http verb

void CGICPlusPlus::ProcessRequest()
{
    char *request_method = getenv("REQUEST_METHOD");
    if (!strcmp(request_method, "HEAD"))
        ProcessHEADRequest();
    else if (!strcmp(request_method, "GET"))
        ProcessGETRequest();
    else if (!strcmp(request_method, "POST"))
        ProcessPOSTRequest();
    else if (!strcmp(request_method, "DEL"))
        ProcessDELETERequest();
    else if (!strcmp(request_method, "PUT"))
        ProcessPUTRequest();
    else
        exit(1);
}

// Add to response

void CGICPlusPlus::AddToResponse(const char *pContent)
{
    sResponse += pContent;
}

// Add to header
void CGICPlusPlus::AddToHeader(const char *pContent)
{
    sHeader += pContent;
    sHeader += "\n";
}

// Add response header

void CGICPlusPlus::SetResponseMimeType(const char *pMimeType)
{
    sHeader = "Content-Type: ";
    sHeader += pMimeType;
    sHeader += "\n";
}

// Output the response

void CGICPlusPlus::OutputResponse()
{
    sHeader += "\n";
    sHeader += sResponse;
    ::printf("%s\n\n", sHeader.c_str());
}


// Get the number of variables that we found

int CGICPlusPlus::GetVariableCount()
{
    return mCgiMap.size();
}


// Return the value of an environment variable

const char *CGICPlusPlus::GetEnvironmentValue(const char *pVariable)
{
    char *pEnv = ::getenv(pVariable);
    return pEnv;
}


// Get the value of a CGI variable

const char *CGICPlusPlus::GetCGIVariable(const char *pKey)
{
    cIter = mCgiMap.find(pKey);
    if (cIter == mCgiMap.end())
        return NULL;
    return cIter->second.c_str();
}


// Convert a two-char hex string into a char

char CGICPlusPlus::x2c(char *what)
{
    register char digit;

    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));

    return (digit);
}

// Reduce any %xx escape sequences to the characters they represent.

void CGICPlusPlus::unescape_url(char *url)
{
    register int i, j;

    for (i = 0, j = 0; url[j]; ++i, ++j)
    {
        if ((url[i] = url[j]) == '%')
        {
            url[i] = x2c(&url[j + 1]);
            j += 2;
        }
    }

    url[i] = '\0';
}


