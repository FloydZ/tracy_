#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../public/tracy/Tracy.hpp"

namespace tracy { extern uint32_t ___tracy_magic_pid_override; }

int main( int argc, char** argv ) {
    if( argc == 1 ) {
        printf( "== Tracy Profiler external monitor ==\n\n" );
        printf( "Use: %s program [arguments [...]]\n", argv[0] );
        return 1;
    }

    sem_t* sem = (sem_t*)mmap( nullptr, sizeof( sem_t ), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0 );
    if( !sem ) {
        fprintf( stderr, "Unable to mmap()!\n" );
        return 2;
    }

    if( sem_init( sem, 1, 0 ) != 0 ) {
        fprintf( stderr, "Unable to sem_init()!\n" );
        return 2;
    }

    auto pid = fork();
    if( pid < 0 ) {
        fprintf( stderr, "Unable to fork()!\n" );
        return 2;
    }

    if( pid == 0 ) {
        sem_wait( sem );
        execvp( argv[1], argv+1 );
        fprintf( stderr, "Unable to execvp()!\n" );
        fprintf( stderr, "%s\n", strerror( errno ) );
        return 2;
    }

    tracy::___tracy_magic_pid_override = pid;
    tracy::StartupProfiler();
    sem_post( sem );
    waitpid( pid, nullptr, 0 );
    tracy::ShutdownProfiler();

    sem_destroy( sem );
    munmap( sem, sizeof( sem_t ) );

    return 0;
}
