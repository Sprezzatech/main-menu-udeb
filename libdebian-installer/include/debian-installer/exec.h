/*
 * exec.h
 *
 * Copyright (C) 2003 Bastian Blank <waldi@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: exec.h,v 1.7 2003/11/06 07:52:16 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__EXEC_H
#define DEBIAN_INSTALLER__EXEC_H

#include <debian-installer/types.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

di_io_handler
  di_exec_io_log;
di_process_handler
  di_exec_prepare_chroot;

/**
 * @defgroup di_exec Exec functions
 * @{
 */

/**
 * execv like call
 *
 * @param path executable with path
 * @param argv NULL-terminated area of char pointer
 * @param stdout_handler di_io_handler which gets stdout
 * @param stderr_handler di_io_handler which gets stderr
 * @param io_user_data user_data for di_io_handler
 * @param parent_prepare_handler di_process_handler which is called after the fork in the parent
 * @param parent_prepare_user_data user_data for parent_prepare_handler
 * @param child_prepare_handler di_process_handler which is called after the fork in the child
 * @param child_prepare_user_data user_data for child_prepare_handler
 *
 * @return status or error
 */
int di_exec_full (const char *path, const char *const argv[], di_io_handler *stdout_handler, di_io_handler *stderr_handler, void *io_user_data, di_process_handler *parent_prepare_handler, void *parent_prepare_user_data, di_process_handler *child_prepare_handler, void *child_prepare_user_data);

/**
 * execv like call
 *
 * @param path executable with path
 * @param argv NULL-terminated area of char pointer
 *
 * @return status or error
 */
static inline int di_exec (const char *path, const char *const argv[])
{
  return di_exec_full (path, argv, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/**
 * system like call
 *
 * @param cmd command
 * @param stdout_handler di_io_handler which gets stdout
 * @param stderr_handler di_io_handler which gets stderr
 * @param io_user_data user_data for di_io_handler
 * @param parent_prepare_handler di_process_handler which is called after the fork in the parent
 * @param parent_prepare_user_data user_data for parent_prepare_handler
 * @param child_prepare_handler di_process_handler which is called after the fork in the child
 * @param child_prepare_user_data user_data for child_prepare_handler
 *
 * @return status or error
 */
int di_exec_shell_full (const char *const cmd, di_io_handler *stdout_handler, di_io_handler *stderr_handler, void *io_user_data, di_process_handler *parent_prepare_handler, void *parent_prepare_user_data, di_process_handler *child_prepare_handler, void *child_prepare_user_data);

/**
 * system like call
 *
 * @param cmd command
 *
 * @return status or error
 */
static inline int di_exec_shell (const char *const cmd)
{
  return di_exec_shell_full (cmd, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/**
 * system like call with output via log
 *
 * @param cmd command
 *
 * @return status or error
 */
inline static int di_exec_shell_log (const char *const cmd)
{
  return di_exec_shell_full (cmd, di_exec_io_log, di_exec_io_log, NULL, NULL, NULL, NULL, NULL);
}

/**
 * mangle status like sh does it:
 * * if signaled: 128 + signal
 * * else return code
 */
inline static int di_exec_mangle_status (int status)
{
  if (WIFEXITED (status))
    return WEXITSTATUS (status);
  if (WIFSIGNALED (status))
    return 128 + WTERMSIG (status);
  if (WIFSTOPPED (status))
    return 128 + WSTOPSIG (status);
  return status;
}

/**
 * @deprecated
 * Alias of di_exec_shell_log
 */
inline static int di_execlog (const char *const cmd) __attribute__ ((deprecated));
inline static int di_execlog (const char *const cmd)
{
  return di_exec_shell_log (cmd);
}

/** @} */
#endif
