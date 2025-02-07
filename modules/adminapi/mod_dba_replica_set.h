/*
 * Copyright (c) 2016, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_MOD_DBA_REPLICA_SET_H_
#define MODULES_ADMINAPI_MOD_DBA_REPLICA_SET_H_

#include <memory>
#include <string>
#include "modules/adminapi/base_cluster.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/replica_set/api_options.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {
class Dba;
class MetadataStorage;
class Global_topology_manager;

/**
 * \ingroup AdminAPI
 * $(REPLICASET_BRIEF)
 *
 * $(REPLICASET_DETAIL)
 */
class ReplicaSet : public std::enable_shared_from_this<ReplicaSet>,
                   public Base_cluster {
 public:
#if DOXYGEN_JS
  String name;  //!< $(REPLICASET_GETNAME_BRIEF)
  Undefined addInstance(String instance, Dictionary options);
  String describe();
  Undefined disconnect();
  Undefined dissolve(Dictionary options);
  String getName();
  Dictionary listRouters(Dictionary options);
  Undefined setRoutingOption(String option, String value);
  Undefined setRoutingOption(String router, String option, String value);
  Dictionary routingOptions(String router);
  Undefined rejoinInstance(String instance, Dictionary options);
  Undefined removeInstance(String instance, Dictionary options);
  String status(Dictionary options);
  Undefined setPrimaryInstance(String instance, Dictionary options);
  Undefined forcePrimaryInstance(String instance, Dictionary options);
  Boolean removeRouterMetadata(RouterDef routerDef);
  Undefined setupAdminAccount(String user, Dictionary options);
  Undefined setupRouterAccount(String user, Dictionary options);
  Undefined setOption(String option, String value);
  Undefined setInstanceOption(String instance, String option, String value);
  String options(Dictionary options);
#elif DOXYGEN_PY
  str name;  //!< $(REPLICASET_GETNAME_BRIEF)
  None add_instance(str instance, dict options);
  str describe();
  None disconnect();
  None dissolve(dict options);
  str get_name();
  dict list_routers(dict options);
  None set_routing_option(str option, str value);
  None set_routing_option(str router, str option, str value);
  dict routing_options(str router);
  None rejoin_instance(str instance, dict options);
  None remove_instance(str instance, dict options);
  str status(dict options);
  None set_primary_instance(str instance, dict options);
  None force_primary_instance(str instance, dict options);
  bool remove_router_metadata(RouterDef routerDef);
  None setup_admin_account(str user, dict options);
  None setup_router_account(str user, dict options);
  str options();
  None set_option(str option, str value);
  None set_instance_option(str instance, str option, str value);
#endif

  explicit ReplicaSet(const std::shared_ptr<Replica_set_impl> &cluster);

  virtual ~ReplicaSet();

  std::string class_name() const override { return "ReplicaSet"; }

  std::shared_ptr<Replica_set_impl> impl() const { return m_impl; }
  Base_cluster_impl *base_impl() const override { return m_impl.get(); }

  void assert_valid(const std::string &option_name) const override;

 public:
  void add_instance(
      const std::string &instance_def,
      const shcore::Option_pack_ref<replicaset::Add_instance_options> &options);

  void rejoin_instance(
      const std::string &instance_def,
      const shcore::Option_pack_ref<replicaset::Rejoin_instance_options>
          &options);

  void remove_instance(
      const std::string &instance_def,
      const shcore::Option_pack_ref<replicaset::Remove_instance_options>
          &options);

  shcore::Value describe(void);
  shcore::Value status(
      const shcore::Option_pack_ref<replicaset::Status_options> &options);
  void dissolve(const shcore::Dictionary_t &options);

  void disconnect();

  void set_primary_instance(
      const std::string &instance_def,
      const shcore::Option_pack_ref<replicaset::Set_primary_instance_options>
          &options);

  void force_primary_instance(
      const std::string &instance_def,
      const shcore::Option_pack_ref<replicaset::Force_primary_instance_options>
          &options);

  void remove_router_metadata(const std::string &router_def);

  shcore::Value options();

  void set_option(const std::string &option, const shcore::Value &value);

  void set_instance_option(const std::string &instance_def,
                           const std::string &option,
                           const shcore::Value &value);

 protected:
  void init();

 private:
  std::shared_ptr<Replica_set_impl> m_impl;

  template <typename TCallback>
  inline auto execute_with_pool(TCallback &&f, bool interactive = false) const {
    impl()->get_metadata_storage()->invalidate_cached();

    while (true) {
      Scoped_instance_pool scoped_pool(impl()->get_metadata_storage(),
                                       interactive,
                                       impl()->default_admin_credentials());

      try {
        return f();
      } catch (const shcore::Exception &e) {
        if (e.code() == SHERR_DBA_ASYNC_MEMBER_INVALIDATED &&
            e.error()->has_key("new_primary_endpoint")) {
          std::string new_primary =
              e.error()->get_string("new_primary_endpoint");

          current_console()->print_warning(e.format() + ": reconnecting to " +
                                           new_primary);

          Scoped_instance target(
              scoped_pool->connect_unchecked_endpoint(new_primary));

          impl()->set_target_server(target);

          target->steal();

          // Retry to execute the function using the new primary.
          continue;
        } else {
          throw;
        }
      }
    }
  }
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_REPLICA_SET_H_
