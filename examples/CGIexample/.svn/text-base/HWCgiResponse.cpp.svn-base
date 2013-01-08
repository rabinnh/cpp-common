/* 
 * File:   HWCgiResponse.cpp
 * Author: rbross
 * 
 * Created on September 23, 2010, 10:55 AM
 */

#include "HWCgiResponse.h"

HWCgiResponse::HWCgiResponse() : CGICPlusPlus()
{
}


HWCgiResponse::~HWCgiResponse()
{
}

// Process the request.
void HWCgiResponse::ProcessPOSTRequest()
{
    ProcessGETRequest();
}


// Process the request.
void HWCgiResponse::ProcessGETRequest()
{
    // Set reponse type
    SetResponseMimeType("text/html");

    // Add the HTML response page.
    AddToResponse("<html>\n");
    AddToResponse("<head><title>CGI Results</title></head>\n");
    AddToResponse("<body>\n");
    AddToResponse("<h1>CGICplusPlus CGI Echo Demonstration</h1>\n");
    AddToResponse("Your CGI input variables were:\n");
    AddToResponse("<ul>\n");

    // Print of the CGI variables
    for(cIter = mCgiMap.begin(); cIter != mCgiMap.end(); cIter++)
    {
        char    strBuffer[512];
        ::sprintf(strBuffer, "<li>[%s] = [%s]\n", cIter->first.c_str(), cIter->second.c_str());
        AddToResponse(strBuffer);
    }

    // Finish the page
    AddToResponse("</ul>\n");
    AddToResponse("</body>\n");
    AddToResponse("</html>\n");
}

