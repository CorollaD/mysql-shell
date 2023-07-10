//@ Initialization
||

//@ Create cluster fails because port default GR local address port is already in use. {!__replaying && !__recording}
||The port '<<<__busy_port>>>' for localAddress option is already in use. Specify an available port to be used with localAddress option or free port '<<<__busy_port>>>'. (RuntimeError)

//@ Create cluster errors using localAddress option on busy port {!__replaying && !__recording}
||The port '<<<__mysql_port>>>' for localAddress option is already in use. Specify an available port to be used with localAddress option or free port '<<<__mysql_port>>>'. (RuntimeError)

//@ Create cluster specifying :<valid_port> for localAddress (FR1-TS-1-2)
||

//@ Dissolve cluster (FR1-TS-1-2)
||

//@ Create cluster specifying <valid_host>: for localAddress (FR1-TS-1-3)
||

//@ Dissolve cluster (FR1-TS-1-3)
||

//@ Create cluster specifying <valid_port> for localAddress (FR1-TS-1-4)
||

//@ Dissolve cluster (FR1-TS-1-4)
||

//@ Create cluster specifying <valid_host> for localAddress (FR1-TS-1-9)
||

//@ Dissolve cluster (FR1-TS-1-9)
||

//@ Create cluster specifying <valid_host>:<valid_port> for localAddress (FR1-TS-1-10)
||

//@ Dissolve cluster (FR1-TS-1-10)
||

//@ Create cluster specifying aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa for groupName (FR3-TS-1-1)
||

//@ Dissolve cluster (FR3-TS-1-1)
||

//@ Create cluster
||

//@ Add instance errors using localAddress option on busy port {!__replaying && !__recording}
||The port '<<<__mysql_port>>>' for localAddress option is already in use. Specify an available port to be used with localAddress option or free port '<<<__mysql_port>>>'. (RuntimeError)

//@ Add instance error using groupName (not a valid option)
||Invalid options: groupName (ArgumentError)

//@ Add instance specifying :<valid_port> for localAddress (FR1-TS-2-2)
||

//@ Remove instance (FR1-TS-2-2)
||

//@ Add instance specifying <valid_host>: for localAddress (FR1-TS-2-3)
||

//@ Remove instance (FR1-TS-2-3)
||

//@ Add instance specifying <valid_port> for localAddress (FR1-TS-2-4)
||

//@ Remove instance (FR1-TS-2-4)
||

//@ Add instance specifying <valid_host> for localAddress (FR1-TS-2-9)
||

//@ Remove instance (FR1-TS-2-9)
||

//@ Add instance specifying <valid_host>:<valid_port> for localAddress (FR1-TS-2-10)
||

//@ Dissolve cluster
||

//@ Create cluster with a specific localAddress and groupName (FR1-TS-4)
||

//@ Add instance with a specific localAddress (FR1-TS-4)
||

//@ Add a 3rd instance to ensure it will not affect the persisted group seed values specified on others (FR1-TS-4)
||

//@ Configure seed instance (FR1-TS-4)
||

//@ Configure added instance (FR1-TS-4)
||

//@ Stop seed and added instance with specific options (FR1-TS-4)
||

//@ Restart added instance (FR1-TS-4)
||

//@ Restart seed instance (FR1-TS-4)
||

//@ Dissolve cluster (FR1-TS-4)
||

//@ Finalization
||
