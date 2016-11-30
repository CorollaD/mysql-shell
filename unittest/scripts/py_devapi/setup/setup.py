def validate_crud_functions(crud, expected):
	actual = crud.__members__
  
	# Ensures expected functions are on the actual list
	missing = []
	for exp_funct in expected:
		try:
			pos = actual.index(exp_funct)
			actual.remove(exp_funct)
		except:
			missing.append(exp_funct)
  
	if len(missing) == 0:
		print ("All expected functions are available\n")
	else:
		print "Missing Functions:", missing
  
	if len(actual) == 0:
		print "No additional functions are available\n"
	else:
		print "Extra Functions:", actual

def ensure_schema_does_not_exist(session, name):
	try:
		schema = session.get_schema(name)
		session.drop_schema(name)
	except:
		# Nothing happens, it means the schema did not exist
		pass

def validateMember(memberList, member):
	index = -1
	try:
		index = memberList.index(member)
	except:
		pass
  
	if index != -1:
		print member + ": OK\n"
	else:
		print member + ": Missing\n"

def validateNotMember(memberList, member):
	index = -1
	try:
		index = memberList.index(member)
	except:
		pass
  
	if index != -1:
		print member + ": Unexpected\n"
	else:
		print member + ": OK\n"

def getSchemaFromList(schemas, name):
  for schema in schemas:
    if schema.name == name:
      return schema
  
  return None

import time
def wait(timeout, wait_interval, condition):
  waiting = 0
  while not condition() and waiting < timeout:
    time.sleep(wait_interval)
    waiting = waiting + 1


ro_session = None;
from mysqlsh import mysql as ro_module;
def wait_super_read_only_done():
  global ro_session
  
  super_read_only = ro_session.run_sql('select @@super_read_only').fetch_one()[0]
  
  print "---> Super Read Only = %s" % super_read_only
  
  return super_read_only == "0"

def check_super_read_only_done(connection):
  global ro_session
  
  ro_session = ro_module.get_classic_session(connection)
  wait(60, 1, wait_super_read_only_done)
  ro_session.close()

recov_cluster = None
recov_master_uri = None
recov_slave_uri = None
recov_state_list = None;

def _check_slave_state():
  global recov_cluster
  global recov_master_uri
  global recov_slave_uri
  global recov_state_list
  
  full_status = recov_cluster.status()
  slave_status = full_status.defaultReplicaSet.topology[recov_master_uri].leaves[recov_slave_uri].status
  
  print "--->%s: %s" % (recov_slave_uri, slave_status)
  
  ret_val = False
  for state in recov_state_list:
    if state == slave_status:
      ret_val = True
      print "Done!"
      break
  
  return ret_val

def wait_slave_state(cluster, master_uri, slave_uri, states):
  global recov_cluster
  global recov_master_uri
  global recov_slave_uri
  global recov_state_list
  
  recov_cluster = cluster
  recov_master_uri = master_uri
  recov_slave_uri = slave_uri
  
  if type(states) is list:
    recov_state_list = states
  else:
    recov_state_list = [states]
  
  print "WAITING for %s to be in one of these states: %s" % (slave_uri, states)
  
  wait(60, 1, _check_slave_state)
  
  recov_cluster = None

def wait_slave_online_multimaster():
  global recov_cluster
  global recov_master_uri
  
  full_status = recov_cluster.status()
  slave_status = full_status.defaultReplicaSet.topology[recov_master_uri].status
  
  print "---> %s: %s" % (recov_master_uri, slave_status)
  return slave_status == "ONLINE"

def wait_slave_offline_multimaster():
  global recov_cluster
  global recov_master_uri
  
  full_status = recov_cluster.status()
  slave_status = full_status.defaultReplicaSet.topology[recov_master_uri].status
  
  print "---> %s: %s" % (recov_master_uri, slave_status)
  return slave_status == "OFFLINE"

def check_slave_online_multimaster(cluster, master_uri):
  global recov_cluster
  global recov_master_uri
  
  recov_cluster = cluster
  recov_master_uri = master_uri
  
  wait(60, 1, wait_slave_online_multimaster)
  
  recov_cluster = None

def check_slave_offline_multimaster(cluster, master_uri):
  global recov_cluster
  global recov_master_uri
  
  recov_cluster = cluster
  recov_master_uri = master_uri
  
  wait(60, 1, wait_slave_offline_multimaster)
  
  recov_cluster = None

# Smart deployment routines
def reset_or_deploy_sandbox(port):
  deployed_here = False;
  
  options = {}
  if __sandbox_dir != '':
    options['sandboxDir'] = __sandbox_dir
  
  print 'Killing sandbox at: %s' % port
  
  try:
    dba.kill_sandbox_instance(port)
  except Exception, err:
    pass
  
  started = False
  try:
    print 'Starting sandbox at: %s' % port
    dba.start_sandbox_instance(port)
    started = True
    print 'succeeded'
  except Exception, err:
    print 'failed: %s' % str(err)
  
  if started:
    connected = False
    try:
      print 'Dropping metadata...'
      shell.connect({'host':localhost, 'port':port, 'password':'root'})
      connected = True
      session.run_sql('set sql_log_bin = 0')
      session.run_sql('drop schema mysql_innodb_cluster_metadata')
      session.run_sql('flush logs')
      session.run_sql('set sql_log_bin = 1')
      print 'succeeded'
    except Exception, err:
      print 'failed: %s' % str(err)
    
    if connected:
      session.run_sql('set sql_log_bin = 1')
      session.close()
  else:
    print 'Deploying instance'
    options['password'] = 'root'
    options['allowRootFrom'] = '%'
    
    if not __have_ssl:
      options['ignoreSslError'] = True
    
    dba.deploy_sandbox_instance(port, options)
    deployed_here = True
  
  return deployed_here

def reset_or_deploy_sandboxes():
  deploy1 = reset_or_deploy_sandbox(__mysql_sandbox_port1)
  deploy2 = reset_or_deploy_sandbox(__mysql_sandbox_port2)
  deploy3 = reset_or_deploy_sandbox(__mysql_sandbox_port3)
  
  return deploy1 or deploy2 or deploy3

def cleanup_sandbox(port):
  try:
    dba.kill_sandbox_instance(port)
  except Exception, err:
    pass
  
  try:
    dba.delete_sandbox_instance(port)
  except Exception, err:
    pass

def cleanup_sandboxes(deployed_here):
  if deployed_here:
    cleanup_sandbox(__mysql_sandbox_port1)
    cleanup_sandbox(__mysql_sandbox_port2)
    cleanup_sandbox(__mysql_sandbox_port3)
