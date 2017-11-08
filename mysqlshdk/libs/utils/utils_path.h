/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_PATH_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_PATH_H_

#include <string>
#include <utility>
#include <vector>
#include "scripting/common.h"

namespace shcore {
namespace path {
namespace detail {
std::string expand_user(const std::string &path, const std::string &sep);
}  // namespace detail

std::string SHCORE_PUBLIC join_path(const std::vector<std::string> &components);

std::pair<std::string, std::string> SHCORE_PUBLIC
splitdrive(const std::string &path);

/**
 * Get home directory path of the user executing the shell.
 *
 * On Unix: `HOME` environment variable is returned if is set, otherwise current
 *          user home directory is looked up in password directory.
 *
 * On Windows: `%HOME%` or `%USERPROFILE%` or `%HOMEDRIVE%` + `%HOMEPATH%`
 *             environment variable is returned if is set, otherwise user home
 *             directory is obtained from system KnownFolder ID
 *             `FOLDERID_Profile`.
 *             If all of above fail, empty string is returned.
 *
 * @return User home directory path.
 */
std::string SHCORE_PUBLIC home();

/**
 * Get home directory path of user associated with the specified login name.
 *
 * On Unix: User home directory path associated with `loginname` is looked up
 *          directly in the password directory.
 *
 * On Windows: Retrieving home directory of another user on Windows platform
 *             is *NOT SUPPORTED*.
 *
 * @param loginname Login name of existing user in system.
 *
 * @return User home directory associated with `loginname` or empty string
 *         if such user does not exist.
 */
std::string SHCORE_PUBLIC home(const std::string &loginname);

/**
 * `expand_user` expand paths beginning with `~` or `~user` (also known as
 * "tilde expansion").
 *
 * Home directory for `~` prefix is obtained by `home()` function.
 * Home directory for `~user` prefix is obtained by `home(user)` function.
 *
 * @param path Path string.
 *
 * @return Return `path` with prefix `~` or `~user` replaced by that user's home
 *         directory. @see home() and @see home(user).
 *         If the expansion fails or if the path does not begin with `~`, the
 *         path is returned unchanged.
 */
std::string SHCORE_PUBLIC expand_user(const std::string &path);

/**
 * Unix:
 *   Normalize a path collapsing redundant separators and relative references
 *   (`.`, `..`). This string path manipulation, might affect paths containing
 *   symbolic links.
 *
 * Windows:
 *   Retrieves the full path and file name of the specified file. If error
 *   occur path isn't altered.
 *
 * @param path Input path string to normalize.
 * @return Normalized string path.
 */
std::string SHCORE_PUBLIC normalize(const std::string &path);

}  // namespace path
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_PATH_H_