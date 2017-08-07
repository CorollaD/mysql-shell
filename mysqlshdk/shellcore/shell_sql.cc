/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/shell_sql.h"
#include "shellcore/base_session.h"
#include "../modules/mod_mysql_session.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include <fstream>
#include "utils/utils_string.h"

using namespace shcore;

Shell_sql::Shell_sql(IShell_core *owner)
  : Shell_language(owner), _delimiters({";", "\\G", "\\g"})
{
  static const std::string cmd_help_G =
      "SYNTAX:\n"
      "   <statement>\\G\n\n"
      "Execute the statement in the MySQL server and display results in a vertical\n"
      "format, one field and value per line.\n"
      "Useful for results that are too wide to fit the screen horizontally.\n";

  static const std::string cmd_help_g =
      "SYNTAX:\n"
      "   <statement>\\g\n\n"
      "Execute the statement in the MySQL server and display results.\n"
      "Same as executing with the current delimiter (default ;)\n";

  // Inject help for statement commands. Actual handling of these
  // commands is done in a way different from other commands
  SET_CUSTOM_SHELL_COMMAND("\\G", "Send command to mysql server, display result vertically.", cmd_help_G, Shell_command_function());
  SET_CUSTOM_SHELL_COMMAND("\\g", "Send command to mysql server.", cmd_help_g, Shell_command_function());
}

Value Shell_sql::process_sql(const std::string &query_str,
    mysql::splitter::Delimiters::delim_type_t delimiter,
    std::shared_ptr<mysqlsh::ShellBaseSession> session,
    std::function<void(shcore::Value)> result_processor) {
  Value ret_val;
  if (!session) {
    print_exception(shcore::Exception::logic_error("Not connected."));
  } else {
    try {
      shcore::Argument_list query;
      query.push_back(Value(query_str));

      // ClassicSession has runSql and returns a ClassicResult object
      if (session->has_member("runSql"))
        ret_val = session->call("runSql", query);

      // NodeSession uses SqlExecute object in which we need to call
      // .execute() to get the Resultset object
      else if (session->has_member("sql"))
        ret_val = session->call("sql", query).as_object()->call("execute",
                                shcore::Argument_list());
      else
        throw shcore::Exception::logic_error("The current session type (" +
            session->class_name() + ") can't be used for SQL execution.");

      // If reached this point, processes the returned result object
      auto shcore_options = Shell_core_options::get();
      auto old_format = (*shcore_options)[SHCORE_OUTPUT_FORMAT];
      if (delimiter == "\\G")
        (*shcore_options)[SHCORE_OUTPUT_FORMAT] = Value("vertical");
      result_processor(ret_val);
      (*shcore_options)[SHCORE_OUTPUT_FORMAT] = old_format;
    } catch (shcore::Exception &exc) {
      print_exception(exc);
      ret_val = Value();
    }
  }
  _last_handled += query_str + delimiter;

  return ret_val;
}

void Shell_sql::handle_input(std::string &code, Input_state &state,
    std::function<void(shcore::Value)> result_processor) {
  Value ret_val;
  state = Input_state::Ok;
  auto session = _owner->get_dev_session();
  bool no_query_executed = true;

  _last_handled.clear();

  {

    // NOTE: We need to find a nice way to decide whether parsing or not multiline blocks
    // is enabled or not, for now will let this commented out and do parsing all the time
    //-----------------------------------------------------------------------------------
    // If no cached code and new code is a multiline statement
    // allows multiline code to bypass the splitter
    // This way no delimiter change is needed for i.e.
    // stored procedures and functions
    //if (_sql_cache.empty() && code.find("\n") != std::string::npos)
    //{
    //  ranges.push_back(std::make_pair<size_t, size_t>(0, code.length()));
    //  statement_count = 1;
    //}
    //else
    //{
    // Parses the input string to identify individual statements in it.
    // Will return a range for every statement that ends with the delimiter, if there
    // is additional code after the last delimiter, a range for it will be included too.
    auto ranges = shcore::mysql::splitter::determineStatementRanges(code.data(),
        code.length(), _delimiters, "\n", _parsing_context_stack);
    //}

    size_t range_index = 0;

    for (; range_index < ranges.size(); range_index++)
    {
      if (ranges[range_index].get_delimiter().empty()) {
        // There is no delimiter, partial command added to cache
        std::string line = code.substr(ranges[range_index].offset(),
            ranges[range_index].length());

        str_rstrip(line, "\n");
        if (_sql_cache.empty())
          _sql_cache = line;
        else
          _sql_cache.append("\n").append(line);
      } else {
        no_query_executed = false;
        if (!_sql_cache.empty()){
          no_query_executed = false;
          std::string cached_query = _sql_cache + "\n" +
              code.substr(ranges[range_index].offset(),
              ranges[range_index].length());

          _sql_cache.clear();

          ret_val = process_sql(cached_query,
              ranges[range_index].get_delimiter(),
              session, result_processor);
        }
        else {
          ret_val = process_sql(code.substr(ranges[range_index].offset(),
              ranges[range_index].length()),
              ranges[range_index].get_delimiter(),
              session, result_processor);
        }
      }
    }
    code = _sql_cache;

    if (_parsing_context_stack.empty())
      state = Input_state::Ok;
    else
      state = Input_state::ContinuedSingle;

    // Nothing was processed so it is not an error
    if (no_query_executed)
      ret_val = Value::Null();

  }

  // TODO: previous to file processing the caller was caching unprocessed code and sending it again on next
  //       call. On file processing an internal handling of this cache was required.
  //       Clearing the code here prevents it being sent again.
  //       We need to decide if the caching logic we introduced on the caller is still required or not.
  code = "";

  // If ret_val still Undefined, it means there was an error on the processing
  if (ret_val.type == Undefined)
    result_processor(ret_val);
}

void Shell_sql::clear_input() {
  std::stack<std::string> empty;
  _parsing_context_stack.swap(empty);
}

std::string Shell_sql::get_continued_input_context() {
  if (_parsing_context_stack.empty())
    return "";
  return _parsing_context_stack.top();
}

bool Shell_sql::print_help(const std::string& topic) {
  bool ret_val = true;
  if (topic.empty())
    _owner->print(_shell_command_handler.get_commands("===== SQL Mode Commands ====="));
  else {
    std::string help;
    ret_val = _shell_command_handler.get_command_help(topic, help);
    if (ret_val) {
      help += "\n";
      _owner->print(help);
    }
  }

  return ret_val;
}

void Shell_sql::print_exception(const shcore::Exception &e) {
  // Sends a description of the exception data to the error handler wich will define the final format.
  shcore::Value exception(e.error());
  _owner->get_delegate()->print_value(_owner->get_delegate()->user_data, exception, "error");
}
