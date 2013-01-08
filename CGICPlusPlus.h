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
 * File:   CGICPlusPlus.h
 * Author: rbross
 *
 * Created on September 23, 2010, 10:14 AM
 */

#ifndef CGICPLUSPLUS_H
#define	CGICPLUSPLUS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <string>

#define MIME_HTML               "text/html"
#define MIME_XML                "text/xml"
#define CONTENT_FORM            "application/x-www-form-urlencoded"
#define STATUS_SUCCESS          "Status: 200 success"
#define STATUS_INVALID_REQUEST  "Status: 400 invalid request"
#define STATUS_NOT_FOUND        "Status: 404 not found"
#define STATUS_INVALID_REQUEST  "Status: 400 invalid request"
#define STATUS_ERROR            "Status: 500 web service error"

using namespace std;

typedef map<string, string>         MAP_CGI_VALUES;
typedef MAP_CGI_VALUES::iterator    CVIter;

//! Wrapper to simplify C++ creation of CGI programs
class CGICPlusPlus
{
public:
    CGICPlusPlus();
    virtual ~CGICPlusPlus();

    //! Get the number of variables that we found
    int         GetVariableCount();
    //! Return the value of an environment variable
    const char  *GetEnvironmentValue(const char *pVariable);
    //! Get the value of a CGI variable
    const char  *GetCGIVariable(const char *pKey);
    //! Add response header
    void        SetResponseMimeType(const char *pMimeType);
    //! Add to header
    void        AddToHeader(const char *pContent);
    //! Add to response
    void        AddToResponse(const char *pContent);
    //! Output the response
    void        OutputResponse();
    //! This function will direct to one of the following 4 function based on the http verb
    void        ProcessRequest();
    //! You must implement this method in a derived class
    virtual void ProcessHEADRequest() {};
    //! You must implement this method in a derived class
    virtual void ProcessGETRequest() {};
    //! You must implement this method in a derived class
    virtual void ProcessPOSTRequest() {};
    //! You must implement this method in a derived class
    virtual void ProcessDELETERequest() {};
    //! You must implement this method in a derived class
    virtual void ProcessPUTRequest() {};
protected:
    //! Convert a two-char hex string into a char
    char        x2c(char *what);
    //! Reduce any %xx escape sequences to the characters they represent.
    void        unescape_url(char *url);
    //! Parse request
    bool        ParseRequest();

protected:
    MAP_CGI_VALUES  mCgiMap;
    CVIter          cIter;
    string          sResponse;
    string          sHeader;
    string          sEnvVar;
};

#endif	/* CGICPLUSPLUS_H */

