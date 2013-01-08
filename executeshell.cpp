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

#include <errno.h>
#include "executeshell.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define STDOUT_BUFFER_SIZE      8192

char ExecuteShell::szStdOut[ESHELL_FORMAT_SIZE];

ExecuteShell::ExecuteShell()
{
    ::memset(szStdOut, 0, sizeof (szStdOut));
}

ExecuteShell::~ExecuteShell()
{
}

// Execute a command and return stdout in a string buffer
// If a command is forked, we could sit and wait for stdout forever

int ExecuteShell::Execute(const char *szCommand, string &sStdout, string *sStderr, const char *strForkExit)
{
    FILE *pFile;
    int iRet = 0;
    int filedes[2];
    char strCommand[64];
    bool bStderr = true;
    bool bForkExit = false;
    char sReadBuffer[STDOUT_BUFFER_SIZE];
    int iExit;

    // Open file
    try
    {
        sStdout.clear();
        string sCommand = szCommand;
        ::memset(filedes, 0, sizeof (filedes));
        // Redirect stderr.  "pipe" creates a pipe with a read end and a write end
        if (sStderr == NULL || pipe(filedes) == -1)
            bStderr = false;
        else
        {
            ::sprintf(strCommand, " 2>&%d", filedes[1]);
            sCommand += strCommand;
            sStderr->clear();
        }

        // Execute command
        pFile = ::popen(sCommand.c_str(), "r");
        if (!pFile)
        {
            iRet = -1;
            throw (-1);
        }

        // Fill buffer if command writes to stdout
        // No blocking
        int fd = fileno(pFile); // Get file descriptor
        int flags = fcntl(fd, F_GETFL, 0); // Get flags
        flags |= O_NONBLOCK; // Add non-blocking
        fcntl(fd, F_SETFL, flags); // Set flags
        // Read
        while (!::feof(pFile))
        {
            if (::fgets(sReadBuffer, STDOUT_BUFFER_SIZE, pFile) != NULL)
            {
                sStdout += sReadBuffer;
                // If we know that we have been forked, we are waiting for an explicit string to exit
                if (strForkExit)
                {
                    if ((iExit = sStdout.find(strForkExit)) != string::npos)
                    { // We got that string, so set exit flag
                        sStdout.resize(iExit);
                        bForkExit = true;
                        break;
                    }
                }
            }
            // This SHOULD work, but it only returns errno EAGAIN
            if (::ferror(pFile) && errno != EAGAIN && errno != EWOULDBLOCK)
                break;
        }

        // Fill buffer if command writes to stderr and we did NOT get the fork exit flag
        if (bStderr && !bForkExit)
        {
            sStderr->clear();
            char cBuf;
            int flags = ::fcntl(filedes[0], F_GETFL, 0);
            ::fcntl(filedes[0], F_SETFL, flags | O_NONBLOCK);
            while (::read(filedes[0], &cBuf, 1) > 0)
                *sStderr += cBuf;
        }

        // pclose returns the exit code of the process
        iRet = ::pclose(pFile);
    }
    catch (exception exc)
    {
        exc.what();
    }

    // Make sure that we close the pipes
    if (filedes[0])
        ::close(filedes[0]);
    if (filedes[1])
        ::close(filedes[1]);

    return (iRet);
}


// Find the executables that we need

const char *ExecuteShell::FindBinary(const char *pName)
{
    static string sExecPath;
    char sShellCmd[1024];
    const char *pFormat = "find %s -type f -name %s -perm -g+x,u+x";
    int iRet;

    // Format command.  First look in /usr
    ::memset(sShellCmd, 0, sizeof (sShellCmd));
    ::sprintf(sShellCmd, pFormat, "/usr", pName);
    iRet = Execute(sShellCmd, sExecPath);
    if (iRet) // No success. Look in /etc
    {
        ::sprintf(sShellCmd, pFormat, "/etc", pName);
        iRet = Execute(sShellCmd, sExecPath);
        if (iRet) // No success. Look in /bin
        {
            ::sprintf(sShellCmd, pFormat, "/bin", pName);
            iRet = Execute(sShellCmd, sExecPath);
        }
    }

    if (!iRet) // Success
    {
        int iIndex = sExecPath.find('\n', 0);
        if (iIndex != -1)
            sExecPath.resize(iIndex);
        return sExecPath.c_str();
    }

    // Fail
    return NULL;
}


// Write to standard out

void ExecuteShell::WriteStdout(bool bNewline)
{
    // One way or another, this WILL be 0 terminated
    if (bNewline)
    {
        szStdOut[ESHELL_FORMAT_SIZE - 3] = '\0';
        ::strcat(szStdOut, "\n\r");
    }
    else
        szStdOut[ESHELL_FORMAT_SIZE - 1] = '\0';

    if (::write(STDOUT_FILENO, szStdOut, ::strlen(szStdOut) - 1))
        return;
}

