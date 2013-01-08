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
#if !defined(_RTMINET_)
#define _RTMINET_

#define INTERNET_MAX_HOST_NAME_LENGTH   256
#define INTERNET_MAX_USER_NAME_LENGTH   128
#define INTERNET_MAX_PASSWORD_LENGTH    128
#define INTERNET_MAX_PORT_NUMBER_LENGTH 5           // INTERNET_PORT is unsigned short
#define INTERNET_MAX_PORT_NUMBER_VALUE  65535       // maximum unsigned short value
#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_PROTOCOL_NAME      "gopher"    // longest protocol name
#define INTERNET_MAX_URL_LENGTH_RTM     ((sizeof(INTERNET_MAX_PROTOCOL_NAME) - 1) \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)
                                        
//
// HTTP Response Status Codes:
//

#define HTTP_STATUS_OK              200     // request completed
#define HTTP_STATUS_CREATED         201     // object created, reason = new URI
#define HTTP_STATUS_ACCEPTED        202     // async completion (TBS)
#define HTTP_STATUS_PARTIAL         203     // partial completion
#define HTTP_STATUS_NO_CONTENT      204     // no info to return

#define HTTP_STATUS_AMBIGUOUS       300     // server couldn't decide what to return
#define HTTP_STATUS_MOVED           301     // object permanently moved
#define HTTP_STATUS_REDIRECT        302     // object temporarily moved
#define HTTP_STATUS_REDIRECT_METHOD 303     // redirection w/ new access method
#define HTTP_STATUS_NOT_MODIFIED    304     // if-modified-since was not modified

#define HTTP_STATUS_BAD_REQUEST     400     // invalid syntax
#define HTTP_STATUS_DENIED          401     // access denied
#define HTTP_STATUS_PAYMENT_REQ     402     // payment required
#define HTTP_STATUS_FORBIDDEN       403     // request forbidden
#define HTTP_STATUS_NOT_FOUND       404     // object not found
#define HTTP_STATUS_BAD_METHOD      405     // method is not allowed
#define HTTP_STATUS_NONE_ACCEPTABLE 406     // no response acceptable to client found
#define HTTP_STATUS_PROXY_AUTH_REQ  407     // proxy authentication required
#define HTTP_STATUS_REQUEST_TIMEOUT 408     // server timed out waiting for request
#define HTTP_STATUS_CONFLICT        409     // user should resubmit with more info
#define HTTP_STATUS_GONE            410     // the resource is no longer available
#define HTTP_STATUS_AUTH_REFUSED    411     // couldn't authorize client

#define HTTP_STATUS_SERVER_ERROR    500     // internal server error
#define HTTP_STATUS_NOT_SUPPORTED   501     // required not supported
#define HTTP_STATUS_BAD_GATEWAY     502     // error response received from gateway
#define HTTP_STATUS_SERVICE_UNAVAIL 503     // temporarily overloaded
#define HTTP_STATUS_GATEWAY_TIMEOUT 504     // timed out waiting for gateway

#endif                                        
