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

#include "../../public/tracy/Tracy.hpp"
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

    char* child_argv[] = {"/home/duda/Downloads/crypto/schemes/mirith-dss/Optimized_Implementation/mirith_tcith_avx2/build/bench_mirith", NULL};

    //tracy::StartupProfiler();
    ZoneScoped;
    execv(child_argv[0], child_argv);
    //tracy::ShutdownProfiler();

    //pid_t child;
    //child = fork(); //create child
    //if(child == 0) {
    //    tracy::StartupProfiler();
    //    printf("child init\n");
    //    execv(child_argv[0], child_argv);
    //    perror(0);
    //    tracy::ShutdownProfiler();
    //    printf("child finished\n");
    //    _exit(0);
    //} else {
    //    // keep alive as long as the child is alive
    //    siginfo_t info;
    //    if(0>waitid(P_PID, child, &info, WEXITED)){
    //        perror(0);
    //        return -1;
    //    }
    //    printf("parent finished\n");

    //    return 0;
    //}

}
