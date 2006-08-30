/*
 *  Stack Overflow Check User Extension Set
 *
 *  NOTE:  This extension set automatically determines at
 *         initialization time whether the stack for this
 *         CPU grows up or down and installs the correct
 *         extension routines for that direction.
 *
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id$
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems.h>
#include <inttypes.h>

/*
 * HACK
 * the stack dump information should be printed by a "fatal" extension.
 * Fatal extensions only get called via rtems_fatal_error_occurred()
 * and not when rtems_shutdown_executive() is called.
 * I hope/think this is changing so that fatal extensions are renamed
 * to "shutdown" extensions.
 * When that happens, this #define should be deleted and all the code
 * it marks.
 */
#define DONT_USE_FATAL_EXTENSION

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rtems/stackchk.h>
#include "internal.h"

/*
 *  This variable contains the name of the task which "blew" the stack.
 *  It is NULL if the system is all right.
 */

Thread_Control *Stack_check_Blown_task;

/*
 *  The extension table for the stack checker.
 */

rtems_extensions_table Stack_check_Extension_table = {
  Stack_check_Create_extension,     /* rtems_task_create  */
  0,                                /* rtems_task_start   */
  0,                                /* rtems_task_restart */
  0,                                /* rtems_task_delete  */
  Stack_check_Switch_extension,     /* task_switch  */
  Stack_check_Begin_extension,      /* task_begin   */
  0,                                /* task_exitted */
#ifdef DONT_USE_FATAL_EXTENSION
  0,                                /* fatal        */
#else
  Stack_check_Fatal_extension,      /* fatal        */
#endif
};

/*
 *  The "magic pattern" used to mark the end of the stack.
 */

Stack_check_Control Stack_check_Pattern;

/*
 *  Where the pattern goes in the stack area is dependent upon
 *  whether the stack grow to the high or low area of the memory.
 *
 */

#if ( CPU_STACK_GROWS_UP == TRUE )

#define Stack_check_Get_pattern_area( _the_stack ) \
  ((Stack_check_Control *) ((char *)(_the_stack)->area + \
       (_the_stack)->size - sizeof( Stack_check_Control ) ))

#define Stack_check_Calculate_used( _low, _size, _high_water ) \
    ((char *)(_high_water) - (char *)(_low))

#define Stack_check_usable_stack_start(_the_stack) \
    ((_the_stack)->area)

#else

#define Stack_check_Get_pattern_area( _the_stack ) \
  ((Stack_check_Control *) ((char *)(_the_stack)->area + HEAP_OVERHEAD))

#define Stack_check_Calculate_used( _low, _size, _high_water) \
    ( ((char *)(_low) + (_size)) - (char *)(_high_water) )

#define Stack_check_usable_stack_start(_the_stack) \
    ((char *)(_the_stack)->area + sizeof(Stack_check_Control))

#endif

#define Stack_check_usable_stack_size(_the_stack) \
    ((_the_stack)->size - sizeof(Stack_check_Control))


/*
 * Do we have an interrupt stack?
 * XXX it would sure be nice if the interrupt stack were also
 *     stored in a "stack" structure!
 */


Stack_Control stack_check_interrupt_stack;

/*
 * Prototypes necessary for forward references
 */

void Stack_check_Dump_usage( void );

/*
 * Fill an entire stack area with BYTE_PATTERN.
 * This will be used by a Fatal extension to check for
 * amount of actual stack used
 */

void
stack_check_dope_stack(Stack_Control *stack)
{
    memset(stack->area, BYTE_PATTERN, stack->size);
}


/*PAGE
 *
 *  Stack_check_Initialize
 */

static int   stack_check_initialized = 0;

void Stack_check_Initialize( void )
{
#if 0
  rtems_status_code    status;
  Objects_Id           id_ignored;
#endif
  uint32_t            *p;
#if 0
  uint32_t             i;
  uint32_t             api_index;
  Thread_Control      *the_thread;
  Objects_Information *information;
#endif

  if (stack_check_initialized)
      return;

  /*
   * Dope the pattern and fill areas
   */

  for ( p = Stack_check_Pattern.pattern;
        p < &Stack_check_Pattern.pattern[PATTERN_SIZE_WORDS];
        p += 4
      )
  {
      p[0] = 0xFEEDF00D;          /* FEED FOOD to BAD DOG */
      p[1] = 0x0BAD0D06;
      p[2] = 0xDEADF00D;          /* DEAD FOOD GOOD DOG */
      p[3] = 0x600D0D06;
  };

#if 0
  status = rtems_extension_create(
    rtems_build_name( 'S', 'T', 'C', 'K' ),
    &Stack_check_Extension_table,
    &id_ignored
  );
  assert ( status == RTEMS_SUCCESSFUL );
#endif

  Stack_check_Blown_task = 0;

  /*
   * If installed by a task, that task will not get setup properly
   * since it missed out on the create hook.  This will cause a
   * failure on first switch out of that task.
   * So pretend here that we actually ran create and begin extensions.
   */

  /* XXX
   *
   *  Technically this has not been done for any task created before this
   *  happened.  So just run through them and fix the situation.
   */
#if 0
  if (_Thread_Executing)
  {
      Stack_check_Create_extension(_Thread_Executing, _Thread_Executing);
  }
#endif

#if 0
  for ( api_index = 1;
        api_index <= OBJECTS_APIS_LAST ;
        api_index++ ) {
    if ( !_Objects_Information_table[ api_index ] )
      continue;
    information = _Objects_Information_table[ api_index ][ 1 ];
    if ( information ) {
      for ( i=1 ; i <= information->maximum ; i++ ) {
        the_thread = (Thread_Control *)information->local_table[ i ];
        Stack_check_Create_extension( the_thread, the_thread );
      }
    }
  }
#endif

  /*
   * If appropriate, setup the interrupt stack for high water testing
   * also.
   */
#if (CPU_ALLOCATE_INTERRUPT_STACK == TRUE)
  if (_CPU_Interrupt_stack_low && _CPU_Interrupt_stack_high)
  {
      stack_check_interrupt_stack.area = _CPU_Interrupt_stack_low;
      stack_check_interrupt_stack.size = (char *) _CPU_Interrupt_stack_high -
                                              (char *) _CPU_Interrupt_stack_low;

      stack_check_dope_stack(&stack_check_interrupt_stack);
  }
#endif

#ifdef DONT_USE_FATAL_EXTENSION
#ifdef RTEMS_DEBUG
    /*
     * this would normally be called by a fatal extension
     * handler, but we don't run fatal extensions unless
     * we fatal error.
     */
  atexit(Stack_check_Dump_usage);
#endif
#endif

  stack_check_initialized = 1;
}

/*PAGE
 *
 *  Stack_check_Create_extension
 */

boolean Stack_check_Create_extension(
  Thread_Control *running,
  Thread_Control *the_thread
)
{
    if (!stack_check_initialized)
      Stack_check_Initialize();

    if (the_thread /* XXX && (the_thread != _Thread_Executing) */ )
        stack_check_dope_stack(&the_thread->Start.Initial_stack);

    return TRUE;
}

/*PAGE
 *
 *  Stack_check_Begin_extension
 */

void Stack_check_Begin_extension(
  Thread_Control *the_thread
)
{
  Stack_check_Control  *the_pattern;

  if (!stack_check_initialized)
    Stack_check_Initialize();

  if ( the_thread->Object.id == 0 )        /* skip system tasks */
    return;

  the_pattern = Stack_check_Get_pattern_area(&the_thread->Start.Initial_stack);

  *the_pattern = Stack_check_Pattern;
}

/*PAGE
 *
 *  Stack_check_report_blown_task
 *  Report a blown stack.  Needs to be a separate routine
 *  so that interrupt handlers can use this too.
 *
 *  Caller must have set the Stack_check_Blown_task.
 *
 *  NOTE: The system is in a questionable state... we may not get
 *        the following message out.
 */

void Stack_check_report_blown_task(void)
{
    Stack_Control *stack;
    Thread_Control *running;

    running = Stack_check_Blown_task;
    stack = &running->Start.Initial_stack;

    fprintf(
        stderr,
        "BLOWN STACK!!! Offending task(%p): id=0x%08" PRIx32
             "; name=0x%08" PRIx32,
        running,
        running->Object.id,
        (uint32_t) running->Object.name
    );
    fflush(stderr);

    if (rtems_configuration_get_user_multiprocessing_table())
        fprintf(
          stderr,
          "; node=%" PRId32 "\n",
          rtems_configuration_get_user_multiprocessing_table()->node
       );
    else
        fprintf(stderr, "\n");
    fflush(stderr);

    fprintf(
        stderr,
        "  stack covers range %p - %p" PRIx32 " (%" PRId32 " bytes)\n",
        stack->area,
        stack->area + stack->size - 1,
        stack->size);
    fflush(stderr);

    fprintf(
        stderr,
        "  Damaged pattern begins at 0x%08lx and is %ld bytes long\n",
        (unsigned long) Stack_check_Get_pattern_area(stack),
        (long) PATTERN_SIZE_BYTES);
    fflush(stderr);

    rtems_fatal_error_occurred( (uint32_t  ) "STACK BLOWN" );
}

/*PAGE
 *
 *  Stack_check_Switch_extension
 */

void Stack_check_Switch_extension(
  Thread_Control *running,
  Thread_Control *heir
)
{
  if ( running->Object.id == 0 )        /* skip system tasks */
    return;

  if (0 != memcmp( (void *) Stack_check_Get_pattern_area( &running->Start.Initial_stack)->pattern,
                  (void *) Stack_check_Pattern.pattern,
                  PATTERN_SIZE_BYTES))
  {
      Stack_check_Blown_task = running;
      Stack_check_report_blown_task();
  }
}

void *Stack_check_find_high_water_mark(
  const void *s,
  size_t n
)
{
  const uint32_t   *base, *ebase;
  uint32_t   length;

  base = s;
  length = n/4;

#if ( CPU_STACK_GROWS_UP == TRUE )
  /*
   * start at higher memory and find first word that does not
   * match pattern
   */

  base += length - 1;
  for (ebase = s; base > ebase; base--)
      if (*base != U32_PATTERN)
          return (void *) base;
#else
  /*
   * start at lower memory and find first word that does not
   * match pattern
   */

  base += PATTERN_SIZE_WORDS;
  for (ebase = base + length; base < ebase; base++)
      if (*base != U32_PATTERN)
          return (void *) base;
#endif

  return (void *)0;
}

/*PAGE
 *
 *  Stack_check_Dump_threads_usage
 *  Try to print out how much stack was actually used by the task.
 *
 */

void Stack_check_Dump_threads_usage(
  Thread_Control *the_thread
)
{
  uint32_t        size, used;
  void           *low;
  void           *high_water_mark;
  Stack_Control  *stack;
  uint32_t        u32_name;
  char            name_str[5];
  char            *name;
  Objects_Information *info;

  if ( !the_thread )
    return;

  /*
   * XXX HACK to get to interrupt stack
   */

  if (the_thread == (Thread_Control *) -1)
  {
      if (stack_check_interrupt_stack.area)
      {
          stack = &stack_check_interrupt_stack;
          the_thread = 0;
      }
      else
          return;
  }
  else
      stack = &the_thread->Start.Initial_stack;

  low  = Stack_check_usable_stack_start(stack);
  size = Stack_check_usable_stack_size(stack);

  high_water_mark = Stack_check_find_high_water_mark(low, size);

  if ( high_water_mark )
    used = Stack_check_Calculate_used( low, size, high_water_mark );
  else
    used = 0;

  name = name_str;
  if ( the_thread ) {
    info = _Objects_Get_information(the_thread->Object.id);
    if ( info->is_string ) {
      name = (char *) the_thread->Object.name;
    } else {
      u32_name = (uint32_t )the_thread->Object.name;
      name[ 0 ] = (u32_name >> 24) & 0xff;
      name[ 1 ] = (u32_name >> 16) & 0xff;
      name[ 2 ] = (u32_name >>  8) & 0xff;
      name[ 3 ] = (u32_name >>  0) & 0xff;
      name[ 4 ] = '\0';
    }
  } else {
    u32_name = rtems_build_name('I', 'N', 'T', 'R');
    name[ 0 ] = (u32_name >> 24) & 0xff;
    name[ 1 ] = (u32_name >> 16) & 0xff;
    name[ 2 ] = (u32_name >>  8) & 0xff;
    name[ 3 ] = (u32_name >>  0) & 0xff;
    name[ 4 ] = '\0';
  }

  fprintf(stdout, "0x%08" PRIx32 "  %4s  %p - %p"
             "   %8" PRId32 "   %8" PRId32 "\n",
          the_thread ? the_thread->Object.id : ~0,
          name,
          stack->area,
          stack->area + stack->size - 1,
          size,
          used
  );
}

/*PAGE
 *
 *  Stack_check_Fatal_extension
 */

void Stack_check_Fatal_extension(
    Internal_errors_Source  source,
    boolean                 is_internal,
    uint32_t                status
)
{
#ifndef DONT_USE_FATAL_EXTENSION
    if (status == 0)
        Stack_check_Dump_usage();
#endif
}


/*PAGE
 *
 *  Stack_check_Dump_usage
 */

void Stack_check_Dump_usage( void )
{
  uint32_t             i;
  uint32_t             api_index;
  Thread_Control      *the_thread;
  uint32_t             hit_running = 0;
  Objects_Information *information;

  if (stack_check_initialized == 0)
      return;

  fprintf(stdout,"Stack usage by thread\n");
  fprintf(stdout,
    "    ID      NAME       LOW        HIGH     AVAILABLE      USED\n"
  );

  for ( api_index = 1 ;
        api_index <= OBJECTS_APIS_LAST ;
        api_index++ ) {
    if ( !_Objects_Information_table[ api_index ] )
      continue;
    information = _Objects_Information_table[ api_index ][ 1 ];
    if ( information ) {
      for ( i=1 ; i <= information->maximum ; i++ ) {
        the_thread = (Thread_Control *)information->local_table[ i ];
        Stack_check_Dump_threads_usage( the_thread );
        if ( the_thread == _Thread_Executing )
          hit_running = 1;
      }
    }
  }

  if ( !hit_running )
    Stack_check_Dump_threads_usage( _Thread_Executing );

  /* dump interrupt stack info if any */
  Stack_check_Dump_threads_usage((Thread_Control *) -1);
}
