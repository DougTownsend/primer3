/* thal_util.c — shared utilities used by multiple thal*.c TUs but not
   tied to either the algorithm (thal.c), the parameter loaders
   (thal_params.c), the dump instrumentation (thal_trace.c), or the
   structure-rendering code (thal_draw.c).

   For now, just the longjmp-on-OOM malloc wrappers. */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thal_internal.h"

void *
safe_calloc(size_t m, size_t n, jmp_buf _jmp_buf, thal_results *o)
{
   void *ptr;
   if (!(ptr = calloc(m, n))) {
#ifdef DEBUG
      fputs("Error in calloc()\n", stderr);
#endif
      strcpy(o->msg, "Out of memory");
      errno = ENOMEM;
      longjmp(_jmp_buf, 1);
   }
   return ptr;
}

void *
safe_malloc(size_t n, jmp_buf _jmp_buf, thal_results *o)
{
   void *ptr;
   if (!(ptr = malloc(n))) {
#ifdef DEBUG
      fputs("Error in malloc()\n", stderr);
#endif
      strcpy(o->msg, "Out of memory");
      errno = ENOMEM;
      longjmp(_jmp_buf, 1);
   }
   return ptr;
}

void *
safe_realloc(void *ptr, size_t n, jmp_buf _jmp_buf, thal_results *o)
{
   ptr = realloc(ptr, n);
   if (ptr == NULL) {
#ifdef DEBUG
      fputs("Error in realloc()\n", stderr);
#endif
      strcpy(o->msg, "Out of memory");
      errno = ENOMEM;
      longjmp(_jmp_buf, 1);
   }
   return ptr;
}
