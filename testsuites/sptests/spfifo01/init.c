/*
 *  COPYRIGHT (c) 2010
 *  Bharath Suri<bharath.s.jois@gmail.com>.
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "tmacros.h"

/* forward declarations to avoid warnings */
rtems_task Init(rtems_task_argument argument);

#define FIFO_PATH "/fifo01"

static void test_main(void)
{
  mode_t rwx = S_IRWXU | S_IRWXG | S_IRWXO;
  int status;

  puts("\n\n*** FIFO / PIPE OPEN TEST - 1 ***");

  puts(
    "\nConfiguration: Pipes disabled.\n"
    "Creating named fifo '" FIFO_PATH "'.\n"
    "Must result in failure since pipes are disabled in the configuration."
  );

  errno = 0;
  status = mkfifo(FIFO_PATH, rwx);
  rtems_test_assert(status == -1);
  rtems_test_assert(errno == ENOSYS);

  errno = 0;
  status = mknod(FIFO_PATH, S_IFIFO | rwx, 0);
  rtems_test_assert(status == -1);
  rtems_test_assert(errno == ENOSYS);

  puts("*** END OF FIFO / PIPE OPEN TEST - 1 ***");
}

rtems_task Init(rtems_task_argument not_used)
{
  test_main();
  rtems_test_exit(0);
}

#define CONFIGURE_APPLICATION_DOES_NOT_NEED_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM

#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 4

#define CONFIGURE_MAXIMUM_TASKS 1

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
