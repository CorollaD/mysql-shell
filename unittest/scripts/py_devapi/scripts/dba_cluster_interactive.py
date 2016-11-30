# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script

#@<OUT> Cluster: get_cluster with interaction
cluster = dba.get_cluster('devCluster');

desc = cluster.describe();
localhost = desc.defaultReplicaSet.instances[0].name.split(':')[0];


all_members = dir(cluster)

# Remove the python built in members
members = []
for member in all_members:
  if not member.startswith('__'):
    members.append(member)

#@ Cluster: validating members
print "Cluster Members: %d" % len(members)
validateMember(members, 'name');
validateMember(members, 'get_name');
validateMember(members, 'admin_type');
validateMember(members, 'get_admin_type');
validateMember(members, 'add_instance');
validateMember(members, 'remove_instance');
validateMember(members, 'rejoin_instance');
validateMember(members, 'check_instance_state');
validateMember(members, 'describe');
validateMember(members, 'status');
validateMember(members, 'help');
validateMember(members, 'dissolve');
validateMember(members, 'rescan');

#@ Cluster: add_instance errors
cluster.add_instance()
cluster.add_instance(5,6,7,1)
cluster.add_instance(5, 5)
cluster.add_instance('', 5)
cluster.add_instance({'host': 'localhost', 'schema': 'abs', 'user': 'sample', 'authMethod': 56});
cluster.add_instance({'port': __mysql_sandbox_port1});

uri1 = "%s:%s" % (localhost, __mysql_sandbox_port1)
uri2 = "%s:%s" % (localhost, __mysql_sandbox_port2)
uri3 = "%s:%s" % (localhost, __mysql_sandbox_port3)

#@ Cluster: add_instance with interaction, error
cluster.add_instance({'host': 'localhost', 'port':__mysql_sandbox_port1});

#@<OUT> Cluster: add_instance with interaction, ok
if __have_ssl:
  cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port2})
else:
  cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port2, 'ssl': False})

wait_slave_state(cluster, uri1, uri2, "ONLINE");

#@<OUT> Cluster: add_instance 3 with interaction, ok
if __have_ssl:
  cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3})
else:
  cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3, 'ssl': False})

wait_slave_state(cluster, uri1, uri3, "ONLINE");

#@<OUT> Cluster: describe1
cluster.describe()

#@<OUT> Cluster: status1
cluster.status()

#@ Cluster: remove_instance errors
cluster.remove_instance();
cluster.remove_instance(1,2);
cluster.remove_instance(1);
cluster.remove_instance({'host': 'localhost', 'port': 33060});
cluster.remove_instance({'host': 'localhost', 'port': 33060, 'schema': 'abs', 'user': 'sample', 'authMethod': 56});
cluster.remove_instance("somehost:3306");

#@ Cluster: remove_instance
cluster.remove_instance({'host': 'localhost', 'port': __mysql_sandbox_port2})

#@<OUT> Cluster: describe2
cluster.describe()

#@<OUT> Cluster: status2
cluster.status()

#@<OUT> Cluster: dissolve error: not empty
cluster.dissolve()

#@ Cluster: dissolve errors
cluster.dissolve(1)
cluster.dissolve(1,2)
cluster.dissolve("")
cluster.dissolve({'enforce': True})
cluster.dissolve({'force': 'sample'})

#@ Cluster: remove_instance 3
cluster.remove_instance({'host': 'localhost', 'port': __mysql_sandbox_port3})

#@<OUT> Cluster: add_instance with interaction, ok 3
if __have_ssl:
  cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port2})
else:
  cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port2, 'ssl': False})

wait_slave_state(cluster, uri1, uri2, "ONLINE");

#@<OUT> Cluster: add_instance with interaction, ok 4
if __have_ssl:
  cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3})
else:
  cluster.add_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3, 'ssl': False})

wait_slave_state(cluster, uri1, uri3, "ONLINE");

#@<OUT> Cluster: status: success
cluster.status()

# Rejoin tests

#@# Dba: kill instance 3
if __sandbox_dir:
  dba.kill_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir":__sandbox_dir})
else:
  dba.kill_sandbox_instance(__mysql_sandbox_port3)

wait_slave_state(cluster, uri1, uri3, ["UNREACHABLE", "OFFLINE"])

#@# Dba: start instance 3
if __sandbox_dir:
  dba.start_sandbox_instance(__mysql_sandbox_port3, {"sandboxDir": __sandbox_dir})
else:
  dba.start_sandbox_instance(__mysql_sandbox_port3)

#@: Cluster: rejoin_instance errors
cluster.rejoin_instance();
cluster.rejoin_instance(1,2,3);
cluster.rejoin_instance(1);
cluster.rejoin_instance({"host": "localhost"});
cluster.rejoin_instance({"host": "localhost", "schema": 'abs', "authMethod":56});
cluster.rejoin_instance("somehost:3306");

#@<OUT> Cluster: rejoin_instance with interaction, ok
if __have_ssl:
  cluster.rejoin_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3})
else:
  cluster.rejoin_instance({'dbUser': 'root', 'host': 'localhost', 'port': __mysql_sandbox_port3, 'ssl': False})

wait_slave_state(cluster, uri1, uri3, "ONLINE");

# Verify if the cluster is OK

#@<OUT> Cluster: status for rejoin: success
cluster.status()

#cluster.dissolve({'force': True})

reset_or_deploy_sandboxes()

