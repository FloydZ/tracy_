#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../public/tracy/Tracy.hpp"

// todo remove
#include "../../public/client/TracySysTrace.hpp"

namespace tracy { extern uint32_t ___tracy_magic_pid_override; }



#include <filesystem>
#include <fstream>
#include <iostream>

// apparently this nonsense is needed, see:
// https://sourceware.org/bugzilla/show_bug.cgi?id=14243
#ifndef PACKAGE
#define PACKAGE
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION
#endif
#    include <bfd.h>


static int demangle_flags = 0 ; // TODO
static bool unwind_inlines = true;
static bool do_demangle = true;
static bool strip_dir_name = true;

/* Flags passed to the name demangler.  */
// static int demangle_flags = DMGL_PARAMS | DMGL_ANSI;

static int naddr;               /* Number of addresses to process.  */
static char **addr;             /* Hex addresses to process.  */

static long symcount;
static asymbol **syms;          /* Symbol table.  */

std::uintmax_t get_file_size(const char* filename) {
    std::filesystem::path example = filename;
    return std::filesystem::file_size(example);
}

/* After a FALSE return from bfd_check_format_matches with
   bfd_get_error () == bfd_error_file_ambiguously_recognized, print
   the possible matching targets and free the list of targets.  */
void
list_matching_formats (char **matching) {
    fflush (stdout);
    char **p = matching;
    while (*p)
        fprintf (stderr, " %s", *p++);
    free (matching);
    fputc ('\n', stderr);
}


static bool slurp_symtab (bfd *abfd) {
    long storage;
    long symcount;
    bfd_boolean dynamic = 0;

    // check if the file exports anything
    if ((bfd_get_file_flags(abfd) & HAS_SYMS) == 0) {
        printf("not exporting anytying \n");
        return false;
    }

    storage = bfd_get_symtab_upper_bound (abfd);
    if (storage == 0) {
        storage = bfd_get_dynamic_symtab_upper_bound (abfd);
        dynamic = 1;
    }
    if (storage < 0) {
        printf("negative size?\n");
        return false;
    }

    syms = (asymbol **) malloc(storage);
    if (dynamic) {
        symcount = bfd_canonicalize_dynamic_symtab(abfd, syms);
    } else {
        symcount = bfd_canonicalize_symtab(abfd, syms);
    }

    if (symcount < 0) {
        printf("no symbols\n");
        return false;
    }

    /* If there are no symbols left after canonicalization and
       we have not tried the dynamic symbols then give them a go.  */
    if (symcount == 0
        && ! dynamic
        && (storage = bfd_get_dynamic_symtab_upper_bound (abfd)) > 0)
    {
        free (syms);
        syms = (asymbol **)malloc (storage);
        symcount = bfd_canonicalize_dynamic_symtab (abfd, syms);
    }

    return true;
}
/* These global variables are used to pass information between
   translate_addresses and find_address_in_section.  */

static bfd_vma pc;
static const char *filename;
static const char *functionname;
static unsigned int line;
static unsigned int discriminator;
static bool found;

/* Look for an address in a section.  This is called via
   bfd_map_over_sections.  */

static void
find_address_in_section (bfd *abfd,
                         asection *section,
                         void *data ATTRIBUTE_UNUSED)
{
  bfd_vma vma;
  bfd_size_type size;

  if (found) {
      return;
  }

  if ((bfd_section_flags (section) & SEC_ALLOC) == 0) {
      return;
  }

  vma = bfd_section_vma (section);
  if (pc < vma) {
      return;
  }

  size = bfd_section_size (section);
  if (pc >= vma + size) {
      return;
  }

  found = bfd_find_nearest_line_discriminator (abfd, section, syms, pc - vma,
                                               &filename, &functionname,
                                               &line, &discriminator);
}

/* Look for an offset in a section.  This is directly called.  */

static void
find_offset_in_section (bfd *abfd, asection *section)
{
  bfd_size_type size;

  if (found)
    return;

  if ((bfd_section_flags (section) & SEC_ALLOC) == 0)
    return;

  size = bfd_section_size (section);
  if (pc >= size)
    return;

  found = bfd_find_nearest_line_discriminator (abfd, section, syms, pc,
                                               &filename, &functionname,
                                               &line, &discriminator);
}

/* Lookup a symbol with offset in symbol table.  */
static bfd_vma
lookup_symbol (bfd *abfd, char *sym, size_t offset) {
  long i;

  for (i = 0; i < symcount; i++) {
      if (!strcmp (syms[i]->name, sym))
        return syms[i]->value + offset + bfd_asymbol_section (syms[i])->vma;
  }

  /* Try again mangled */
  for (i = 0; i < symcount; i++) {
      char *d = bfd_demangle (abfd, syms[i]->name, demangle_flags);
      bool match = d && !strcmp (d, sym);
      free (d);

      if (match)
        return syms[i]->value + offset + bfd_asymbol_section (syms[i])->vma;
    }
  return 0;
}

bfd* process_file(const char *file_name,
                 const char *section_name,
                 const char *target) {
    bfd *abfd;
    asection *section;
    char **matching;

    if (get_file_size(file_name) < 1) {
        printf("no data?\n");
        return nullptr;
    }

    abfd = bfd_openr(file_name, target);
    if (abfd == NULL) {
        printf("could no open file\n");
        return nullptr;
    }

    /* Decompress sections.  */
    abfd->flags |= BFD_DECOMPRESS;

    if (bfd_check_format(abfd, bfd_archive)) {
        printf("%s: cannot get addresses from archive", file_name);
        bfd_close(abfd);
        return nullptr;
    }

    if (!bfd_check_format_matches(abfd, bfd_object, &matching)) {
        printf("wrong format %s\n", bfd_get_filename(abfd));
        if (bfd_get_error() == bfd_error_file_ambiguously_recognized) {
            list_matching_formats(matching);
        }

        bfd_close(abfd);
        return nullptr;
    }

    if (section_name != NULL) {
        section = bfd_get_section_by_name(abfd, section_name);
        if (section == NULL) {
            printf("%s: cannot find section %s", file_name, section_name);
            bfd_close(abfd);
            return nullptr;
        }
    } else {
        section = NULL;
    }

    slurp_symtab(abfd);
    // translate_addresses(abfd, section);
    // free(syms);
    // syms = NULL;
    return abfd;
    return 0;
}

void translate_address(bfd *abfd, const uint64_t adr) {
    if (bfd_get_flavour(abfd) == bfd_target_elf_flavour) {
        const uint64_t arch_size = 64; // TODO
        const uint64_t sign_extend_vma = 0; // TODO
        bfd_vma sign = (bfd_vma) 1 << (arch_size - 1);

        pc &= (sign << 1) - 1;
        if (sign_extend_vma) {
            pc = (pc ^ sign) - sign;
        }
    }

    // print address
    printf("0x");
    bfd_printf_vma (abfd, pc);
    printf(": ");

    bfd_map_over_sections(abfd, find_address_in_section, NULL);
    if (!found) {
        printf("?? ");
    } else {
        const char *name;
        char *alloc = NULL;

        name = functionname;
        if (name == NULL || *name == '\0') {
            name = "??";
        } else if (do_demangle) {
            alloc = bfd_demangle(abfd, name, demangle_flags);
            if (alloc != NULL)
                name = alloc;
        }
        printf("%s", name);
        /* Note for translators:  This printf is used to join the
           function name just printed above to the line number/
           file name pair that is about to be printed below.  Eg:

             foo at 123:bar.c  */
        printf(" at ");
        free(alloc);
    }

    if (strip_dir_name && filename != NULL) {
        char *h;

        h = strrchr((char *)filename, '/');
        if (h != NULL) {
            filename = h + 1;
        }
    }

    printf("%s:", filename ? filename : "??");
    if (line != 0) {
        if (discriminator != 0)
            printf("%u (discriminator %u)\n", line, discriminator);
        else
            printf("%u\n", line);
    } else
        printf("?\n");
    if (!unwind_inlines) {
        found = false;
    } else {
        found = bfd_find_inliner_info (abfd, &filename, &functionname, &line);
    }

    if (!found) {
        return;
    }

    /* Note for translators: This printf is used to join the
       line number/file name pair that has just been printed with
       the line number/file name pair that is going to be printed
       by the next iteration of the while loop.  Eg:

         123:bar.c (inlined by) 456:main.c  */
    printf(" (inlined by) ");
}

void translate_address(bfd *abfd, const char *adr) {
    pc = bfd_scan_vma(adr, NULL, 16);
    translate_address(abfd, pc);
}


int main( int argc, char** argv ) {
    if( argc == 1 ) {
        printf( "== Tracy Profiler external monitor ==\n\n" );
        printf( "Use: %s program [arguments [...]]\n", argv[0] );
        return 1;
    }

    auto *abfd = process_file(argv[1], nullptr, nullptr);
    translate_address(abfd, 0x40000);
    bfd_close(abfd);
    exit(1);

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
