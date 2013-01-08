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
*/// rbross - 12/31/09

#ifndef _EXECUTESHELL_H
#define	_EXECUTESHELL_H

#include <string>

#define ESHELL_FORMAT_SIZE  2048

using namespace std;

class ExecuteShell {
public:
    ExecuteShell();
    virtual ~ExecuteShell();

    //! Execute command,return stdout in a string buffer
    //! strForkExit is set if the command will fork.  When Execute sees the string it will return.
    //! Otherwise, stdout doesn't close until all forked processes close.
static int Execute(const char *szCommand, string &sStdout, string *sStderr = NULL, const char *strForkExit = NULL);

    //! Find a command on the machine
static const char *FindBinary(const char *pName);

    //! Write to standard out
static void WriteStdout(bool bNewline = true);

public:
static  char    szStdOut[ESHELL_FORMAT_SIZE];

protected:

};

#endif	/* _EXECUTECLI_H */

