/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

// Interactive session access module
// Exposed as "session" in the shell

#ifndef _MOD_CORE_SESSION_H_
#define _MOD_CORE_SESSION_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"

namespace mysh
{
  bool SHCORE_PUBLIC parse_mysql_connstring(const std::string &connstring,
                              std::string &protocol, std::string &user, std::string &password,
                              std::string &host, int &port, std::string &sock,
                              std::string &db, int &pwd_found, std::string& ssl_ca, std::string& ssl_cert, std::string& ssl_key);

  std::string SHCORE_PUBLIC strip_password(const std::string &connstring);

  std::string SHCORE_PUBLIC strip_ssl_args(const std::string &connstring);

  // The session types that can be produced by connect_session
  enum SessionType
  {
    Application,
    Node,
    Classic
  };

  class SHCORE_PUBLIC ShellBaseSession : public shcore::Cpp_object_bridge
  {
  public:
    ShellBaseSession();
    virtual ~ShellBaseSession() {};

    // Virtual methods from object bridge
    virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
    virtual std::string &append_repr(std::string &s_out) const;
    virtual void append_json(const shcore::JSON_dumper& dumper) const;

    virtual bool operator == (const Object_bridge &other) const;

    virtual std::vector<std::string> get_members() const;

    // Virtual methods from ISession
    virtual shcore::Value connect(const shcore::Argument_list &args) = 0;
    virtual shcore::Value close(const shcore::Argument_list &args) = 0;
    virtual shcore::Value createSchema(const shcore::Argument_list &args) = 0;
    virtual shcore::Value dropSchema(const shcore::Argument_list &args) = 0;
    virtual shcore::Value dropSchemaObject(const shcore::Argument_list &args, const std::string& type) = 0;
    virtual bool is_connected() const = 0;
    virtual std::string uri() const = 0;

    virtual shcore::Value get_schema(const shcore::Argument_list &args) const = 0;
    virtual bool db_object_exists(std::string &type, const std::string &name, const std::string& owner) = 0;

    // Helper method to retrieve properties using a method
    shcore::Value get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop);

  protected:
    std::string get_quoted_name(const std::string& name);
  };

  boost::shared_ptr<mysh::ShellBaseSession> SHCORE_PUBLIC connect_session(const shcore::Argument_list &args, SessionType session_type);
};

#endif
