/*
 * Copyright (c) 2021, 2023, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "modules/adminapi/cluster/api_options.h"

#include <cinttypes>
#include <utility>
#include <vector>

#include "adminapi/common/api_options.h"
#include "adminapi/common/async_topology.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "shellcore/shell_options.h"
#include "utils/utils_net.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

const shcore::Option_pack_def<Add_instance_options>
    &Add_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Add_instance_options>()
          .include(&Add_instance_options::gr_options)
          .include(&Add_instance_options::clone_options)
          .optional(kLabel, &Add_instance_options::label)
          .include<Wait_recovery_option>()
          .include<Recovery_progress_option>()
          .optional(kCertSubject, &Add_instance_options::set_cert_subject)
          .include<Password_interactive_options>();
  return opts;
}

void Add_instance_options::set_cert_subject(const std::string &value) {
  if (value.empty())
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for '%s' option. Value cannot be an empty string.",
        kCertSubject));

  cert_subject = value;
}

const shcore::Option_pack_def<Rejoin_instance_options>
    &Rejoin_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Rejoin_instance_options>()
          .include(&Rejoin_instance_options::gr_options)
          .include(&Rejoin_instance_options::clone_options)
          .include<Recovery_progress_option>()
          .optional(kDryRun, &Rejoin_instance_options::dry_run)
          .include<Password_interactive_options>()
          .include<Timeout_option>();

  return opts;
}

const shcore::Option_pack_def<Remove_instance_options>
    &Remove_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Remove_instance_options>()
          .optional(kForce, &Remove_instance_options::force)
          .optional(kDryRun, &Remove_instance_options::dry_run)
          .include<Password_interactive_options>()
          .include<Timeout_option>();

  return opts;
}

const shcore::Option_pack_def<Status_options> &Status_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Status_options>()
          .optional(kExtended, &Status_options::set_extended)
          .optional(kQueryMembers, &Status_options::set_query_members);

  return opts;
}

void Status_options::set_extended(uint64_t value) {
  // Validate extended option UInteger [0, 3] or Boolean.
  if (value > 3) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%" PRIu64
                           "' for option '%s'. It must be an integer in the "
                           "range [0, 3].",
                           value, kExtended));
  }

  extended = value;
}

void Status_options::set_query_members(bool value) {
  auto console = mysqlsh::current_console();

  std::string specific_value = value ? " with value 3" : "";
  console->print_warning(
      shcore::str_format("The '%s' option is deprecated. "
                         "Please use the 'extended' option%s instead.",
                         kQueryMembers, specific_value.c_str()));

  console->print_info();

  if (value) {
    extended = 3;
  }
}

const shcore::Option_pack_def<Options_options> &Options_options::options() {
  static const auto opts = shcore::Option_pack_def<Options_options>().optional(
      kAll, &Options_options::all);

  return opts;
}

const shcore::Option_pack_def<Rescan_options> &Rescan_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Rescan_options>()
          .include<Interactive_option>()
          .optional(kUpdateTopologyMode,
                    &Rescan_options::set_update_topology_mode)
          .optional(kAddInstances, &Rescan_options::set_list_option)
          .optional(kRemoveInstances, &Rescan_options::set_list_option)
          .optional(kUpgradeCommProtocol,
                    &Rescan_options::upgrade_comm_protocol)
          .optional(kUpdateViewChangeUuid,
                    &Rescan_options::update_view_change_uuid);

  return opts;
}

void Rescan_options::set_update_topology_mode(bool /*value*/) {
  auto console = mysqlsh::current_console();
  console->print_info(shcore::str_format(
      "The %s option is deprecated. The topology-mode is now automatically "
      "updated.",
      kUpdateTopologyMode));
  console->print_info();
}

void Rescan_options::set_list_option(const std::string &option,
                                     const shcore::Value &value) {
  std::vector<mysqlshdk::db::Connection_options> *instances_list;

  // Selects the target list
  instances_list =
      option == kRemoveInstances ? &remove_instances_list : &add_instances_list;

  bool *auto_option = option == kRemoveInstances ? &auto_remove : &auto_add;

  if (value.type == shcore::Value_type::String) {
    auto str_val = value.as_string();
    if (shcore::str_caseeq(str_val, "auto")) {
      *auto_option = true;
    } else {
      throw shcore::Exception::argument_error(shcore::str_format(
          "Option '%s' only accepts 'auto' as a valid string "
          "value, otherwise a list of instances is expected.",
          option.c_str()));
    }
  } else if (value.type == shcore::Value_type::Array) {
    auto instances_array = value.as_array();
    if (instances_array->empty()) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "The list for '%s' option cannot be empty.", option.c_str()));
    }

    // Process values from addInstances list (must be valid connection data).
    for (const shcore::Value &instance : *instances_array.get()) {
      try {
        auto cnx_opt = mysqlsh::get_connection_options(instance);

        if (cnx_opt.get_host().empty()) {
          throw shcore::Exception::argument_error("host cannot be empty.");
        }

        if (!cnx_opt.has_port()) {
          throw shcore::Exception::argument_error("port is missing.");
        }

        instances_list->emplace_back(std::move(cnx_opt));
      } catch (const std::exception &err) {
        std::string error(err.what());
        throw shcore::Exception::argument_error(shcore::str_format(
            "Invalid value '%s' for '%s' option: %s", instance.descr().c_str(),
            option.c_str(), error.c_str()));
      }
    }
  } else {
    throw shcore::Exception::argument_error(shcore::str_format(
        "The '%s' option must be a string or a list of strings.",
        option.c_str()));
  }
}

const shcore::Option_pack_def<Set_primary_instance_options>
    &Set_primary_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Set_primary_instance_options>().optional(
          "runningTransactionsTimeout",
          &Set_primary_instance_options::running_transactions_timeout);

  return opts;
}

// Read-Replicas

void Add_replica_instance_options::set_replication_sources(
    const shcore::Value &value) {
  validate_replication_sources_option(value);

  // Iterate the list to build the vector of Managed_async_channel_source(s)
  if (value.type == shcore::Value_type::Array) {
    auto sources = value.to_string_container<std::vector<std::string>>();

    replication_sources_option.replication_sources =
        std::set<Managed_async_channel_source,
                 std::greater<Managed_async_channel_source>>{};

    // The source list is ordered by weight
    int64_t source_weight = k_read_replica_max_weight;

    for (const auto &src : sources) {
      Managed_async_channel_source managed_src_address(src);

      // Add it if not in the list already. The one kept in the list is the one
      // with the highest priority/weight
      if (replication_sources_option.replication_sources.find(
              managed_src_address) ==
          replication_sources_option.replication_sources.end()) {
        Managed_async_channel_source managed_src(src, source_weight--);
        replication_sources_option.replication_sources.emplace(
            std::move(managed_src));
      }
    }

    // Set the source type to CUSTOM
    replication_sources_option.source_type = Source_type::CUSTOM;
  } else if (value.type == shcore::Value_type::String) {
    auto string = value.as_string();

    if (shcore::str_caseeq(string, kReplicationSourcesAutoPrimary)) {
      replication_sources_option.source_type = Source_type::PRIMARY;
    } else if (shcore::str_caseeq(string, kReplicationSourcesAutoSecondary)) {
      replication_sources_option.source_type = Source_type::SECONDARY;
    }
  }
}

const shcore::Option_pack_def<Add_replica_instance_options>
    &Add_replica_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Add_replica_instance_options>()
          .include<Timeout_option>()
          .optional(kDryRun, &Add_replica_instance_options::dry_run)
          .optional(kLabel, &Add_replica_instance_options::label)
          .optional(kReplicationSources,
                    &Add_replica_instance_options::set_replication_sources)
          .include<Recovery_progress_option>()
          .include(&Add_replica_instance_options::clone_options);

  return opts;
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
