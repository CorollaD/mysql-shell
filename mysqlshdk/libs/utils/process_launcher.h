/* Copyright (c) 2014, 2017 Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef _PROCESS_LAUNCHER_H_
#define _PROCESS_LAUNCHER_H_

#ifdef WIN32
#  define _CRT_SECURE_NO_WARNINGS 1
#  ifdef UNICODE
#    #undef UNICODE
#  endif
#  include <windows.h>
#else
#  include <unistd.h>
//#  include <poll.h>
#endif
#include <stdint.h>
#include <string>

namespace ngcommon {
// Launches a process as child of current process and exposes the stdin & stdout of the child process
// (implemented thru pipelines) so the client of this class can read from the child's stdout and write to the child's stdin.
// For usage, see unit tests.
//
// TODO: Make scenario 4 work correctly:
//   spawn
//   while not eof :
//     stdin.write
//     stdout.read
//   wait
class Process_launcher {
public:

  /**
   * Creates a new process and launch it.
   * Argument 'args' must have a last entry that is NULL.
   * If redirect_stderr is true, the child's stderr is redirected to the same stream than child's stdout.
   */
  Process_launcher(const char ** argv, bool redirect_stderr = true) : is_alive(false) {
    this->argv = argv;
    this->redirect_stderr = redirect_stderr;
  }

  ~Process_launcher() { if (is_alive) close(); }

  /** Launches the child process, and makes pipes available for read/write. */
  void start();

  /**
   * Reads a single byte (and returns it).
   * Throws an shcore::Exception in case of error when reading.
   */
  int read_one_char();		// read from stdout of child process

  /**
   * Read up to a 'count' bytes from the stdout of the child process.
   * This method blocks until the amount of bytes is read.
   * @param buf already allocated buffer where the read data will be stored.
   * @param count the maximum amount of bytes to read.
   * @return the real number of bytes read.
   * Returns an shcore::Exception in case of error when reading.
   */
  int read(char *buf, size_t count);

  /**
   * Write into stdin of child process.
   * Returns an shcore::Exception in case of error when writing.
   */
  int write_one_char(int c);

  /**
   * Writes several butes into stdin of child process.
   * Returns an shcore::Exception in case of error when writing.
   */
  int write(const char *buf, size_t count);

  /**
   * Kills the child process.
   */
  void kill();

  /**
   * Returns the child process handle.
   * In Linux this needs to be cast to pid_t, in Windows to cast to HANDLE.
   */
  uint64_t get_pid();

  /**
   * Wait for the child process to exists and returns its exit code.
   * If the child process is already dead, wait() it just returns.
   * Returns the exit code of the process.
   */
  int wait();

  /**
  * Returns the file descriptor write handle (to write child's stdin).
  * In Linux this needs to be cast to int, in Windows to cast to HANDLE.
  */
  uint64_t get_fd_write();

  /**
  * Returns the file descriptor read handle (to read child's stdout).
  * In Linux this needs to be cast to int, in Windows to cast to HANDLE.
  */
  uint64_t get_fd_read();

  /** Perform Windows specific quoting of args and build a command line */
  static std::string make_windows_cmdline(const char **argv);

private:
  /**
   * Throws an exception with the specified message, if msg == NULL, the exception's message is specific of the platform error.
   * (errno in Linux / GetLastError in Windows).
   */
  void report_error(const char *msg);
  /** Closes child process */
  void close();

  const char **argv;
  bool is_alive;
#ifdef WIN32
  HANDLE child_in_rd;
  HANDLE child_in_wr;
  HANDLE child_out_rd;
  HANDLE child_out_wr;
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
#else
  pid_t childpid;
  int fd_in[2];
  int fd_out[2];
  //  struct pollfd _s_pollfd[2];
#endif
  bool redirect_stderr;
};
}  // namespace ngcommon

#endif // _PROCESS_LAUNCHER_H_
