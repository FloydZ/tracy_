#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#else
#  include <unistd.h>
#endif

#include <atomic>
#include <chrono>
#include <inttypes.h>
#include <mutex>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/syscall.h>

// todo
#define TRACY_MANUAL_LIFETIME

#include "../../public/common/TracyProtocol.hpp"
#include "../../public/common/TracyStackFrames.hpp"
#include "../../public/client/TracySysTrace.hpp"
#include "../../public/client/TracyProfiler.hpp"

#ifdef _WIN32
#  include "../../getopt/getopt.h"
#endif

[[noreturn]] void Usage() {
    printf( "Usage: perf output../tracy executable\n" );
    exit( 1 );
}

using namespace tracy;

int main( int argc, char** argv )
{
#ifdef _WIN32
    if( !AttachConsole( ATTACH_PARENT_PROCESS ) )
    {
        AllocConsole();
        SetConsoleMode( GetStdHandle( STD_OUTPUT_HANDLE ), 0x07 );
    }
#endif

    char* binary = "./ls";
    const char* address = "127.0.0.1";
    const char* output = nullptr;
    int port = 8086;
    int seconds = -1;
    int64_t memoryLimit = -1;

    int c;
    while( ( c = getopt( argc, argv, "a:o:p:fs:m:" ) ) != -1 )
    {
        switch( c )
        {
        case 'a':
            address = optarg;
            break;
        case 'o':
            output = optarg;
            break;
        default:
            Usage();
            break;
        }
    }
    pid_t child;
    child = fork(); //create child

    if(child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        tracy::StartupProfiler();
        char* child_argv[] = {binary, NULL};
        execv("/bin/ls", child_argv);
    } else {
        int status;
        long long ins_count = 0;
        while(true) {
            //stop tracing if child terminated successfully
            wait(&status);
            if(WIFEXITED(status))
                break;

            ins_count++;
            ptrace(PTRACE_SINGLESTEP, child, NULL, NULL);
        }

        printf("\n%lld Instructions executed.\n", ins_count);
    }

    return 0;
}
