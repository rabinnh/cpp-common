/* 
 * File:   HWCgiResponse.h
 * Author: rbross
 *
 * Created on September 23, 2010, 10:55 AM
 */

#ifndef HWCGIRESPONSE_H
#define	HWCGIRESPONSE_H

#include "CGICPlusPlus.h"

class HWCgiResponse : public CGICPlusPlus
{
public:
    HWCgiResponse();
    virtual ~HWCgiResponse();

    // Process the request.
    void    ProcessGETRequest();  // Must be implemented by derived class
    // Process the request.
    void    ProcessPOSTRequest();  // Must be implemented by derived class
};

#endif	/* HWCGIRESPONSE_H */

