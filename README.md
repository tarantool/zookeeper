# ZooKeeper client for Tarantool

## Overview

ZooKeeper is a distributed application for managing and coordinating a large number of hosts across a cluster. It helps maintain objects like configuration information and hierarchical naming space and provides various services, such as distributed synchronization and leader election.

## API reference

`z = zookeeper.init(hosts, timeout, opts)` - create a ZooKeeper instance. No connection is established at this point.

Parameters:

* `hosts` - a string of the format: *host1:port1,host2:port2,...*. Default is **127.0.0.1:2181**.
* `timeout` - *recv_timeout* (ZooKeeper session timeout) in seconds. Default is **30000**.
* `opts` - a Lua table with the following **fields**:

  * `clientid` - a Lua table of the format *{client_id = \<number\>, passwd = \<string\>}*. Default is **nil**.
  * `flags` - ZooKeeper init flags. Default is **0**.
  * `reconnect_timeout` - time in seconds to wait before reconnecting. Default is **1**.
  * `default_acl` - a default access control list (ACL) to use for all *create* requests. Must be a *zookeeper.acl.ACLList* instance. Default is **zookeeper.acl.ACLS.OPEN_ACL_UNSAFE**.

`err = zookeeper.zerror(errorcode)` - get a string description for a ZooKeeper error code

Parameters:

* `errorcode` - a numeric ZooKeeper error code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

`zookeeper.deterministic_conn_order(<boolean>)` - instruct ZooKeeper not to randomly choose a server from the hosts provided, but select them sequentially instead

Parameters:

* `<boolean>` - if **true**, a deterministic connection order is enforced

`zookeeper.set_log_level(zookeeper.const.log_level.*)` - set a ZooKeeper logging level

Parameters:

* `zookeeper.const.log_level.*` - a constant corresponding to a certain logging level. Refer to the [list of acceptable values](#log-level).

### ZooKeeper instance methods

`z:start()` - start a ZooKeeper I/O loop. Connection is established at this stage.

`z:close()` - destroy a ZooKeeper instance. After this method is called, nothing is operable and `zookeeper.init()` must be called again.

`z:state()` - return the current ZooKeeper state as a number. Refer to the list of [possible values](#states).

>Tip: to convert a number to a string name, use `zookeeper.const.states_rev[<number>]`.

`z:is_connected()` - return **true** when `z:state()` == *zookeeper.const.states.CONNECTED*

`z:wait_connected()` - wait until the value of `z:state()` becomes *CONNECTED*

`z:client_id()` - return a Lua table of the following form:

```
{
	client_id = <number>, -- current session ID
	passwd = <string> -- password
}
```

`z:set_watcher(watcher_func, extra_context)` - set a watcher function called on every change in ZooKeeper

Parameters:

* `watcher_func` - a function with the following signature:
  ```lua
  local function global_watcher(z, type, state, path, context)
      print(string.format(
  		    'Global watcher. type = %s, state = %s, path = %s',
  		    zookeeper.const.watch_types_rev[type],
  		    zookeeper.const.states_rev[state],
  		    path))
      print('Extra context:', json.encode(context))
  end
  ```
where:

  * `z` - a ZooKeeper instance
  * `type` - an event type. Refer to the [list of acceptable values](#watch-types).
  * `state` - an event state. Refer to the [list of acceptable values](#states).
  * `path` - a path that specifies where an event occurred
  * `context` - a variable passed to `z:set_watcher()` as the second argument

* `extra_context` - a context passed to the watcher function

`z:create(path, value, acl, flags)` - create a ZooKeeper node

Parameters:

* `path` - a string of the format: `/path/to/node`. `/path/to` must exist.
* `value` - a string value to store in a node (may be *nil*). Default is **nil**.
* `acl` (a *zookeeper.acl.ACLList* instance) - an ACL to use. Default is **z.default_acl**.
* `flags` - a combination of numeric [zookeeper.const.create_flags.\* constants](#create-flags).

`z:ensure_path(path)` - make sure that a path (including all the parent nodes) exists

Parameters:

* `path` - a path to check

`z:exists(path, watch)` - make sure that a path (including all the parent nodes) exists

Parameters:

* `path` - a path to check
* `watch` (boolean) - specifies whether to include a path to a global watcher

Returns:

* a boolean indicating if the path exists
* `stat` - node statistics

  > Note: `stat` has the following form:
  ```
  - cversion: 30
    mtime: 1511098164443
    pzxid: 108
    mzxid: 4
    ephemeralOwner: 0
    aversion: 0
    czxid: 4
    dataLength: 0
    numChildren: 2
    ctime: 1511098164443
    version: 0
  ```

* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

`z:delete(path, version)` - delete a node

Parameters:

* `path` - a path to a node to be deleted
* version - a number specifying which version to delete. Default is **-1**, which is *all versions*.

Returns:

* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

`z:get(path, watch)` - get the value of a node

Parameters:

* `path` - a path to a node that holds a needed value
* `watch` (boolean) - specifies whether to include a path to a global watcher

Returns:

* `value` - the value of a node
* `stat` - node statistics
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

`z:set(path, version)` - set the value of a node

Parameters:

* `path` - a path to a node to set a value on
* version - a number specifying which version to delete. Default is **-1**, which is *all versions*.

Returns:

* a boolean indicating if the path exists
* `stat` - node statistics
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

`z:get_children(path, watch)` - get the children of a node

Parameters:

* `path` - a path to a node to get the children of
* `watch` (boolean) - specifies whether to include a path to a global watcher

Returns:

* an array of strings, each representing a node's child
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

`z:get_children2(path, watch)` - get a node's children and stat

Parameters:

* `path` - a path to a node to get the children of
* `stat` - node statistics
* `watch` (boolean) - specifies whether to include a path to a global watcher

Returns:

* an array of strings, each representing a node's child
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

### ZooKeeper constants

`zookeeper.const` also contains a *\<key\>_rev* map for each key that holds a reverse (number-to-name) mapping. For example, *zookeeper.const.api_errors_rev*:

```
-100: ZAPIERROR
-101: ZNONODE
-102: ZNOAUTH
-103: ZBADVERSION
-108: ZNOCHILDRENFOREPHEMERALS
-110: ZNODEEXISTS
-111: ZNOTEMPTY
-112: ZSESSIONEXPIRED
-113: ZINVALIDCALLBACK
-114: ZINVALIDACL
-115: ZAUTHFAILED
-116: ZCLOSING
-117: ZNOTHING
-118: ZSESSIONMOVED
```

#### <a name="watch-types"></a>watch_types

* NOTWATCHING: -2
* SESSION: -1    
* CREATED: 1
* DELETED: 2
* CHANGED: 3
* CHILD: 4

#### <a name="errors"></a>errors

* ZINVALIDSTATE: -9
* ZBADARGUMENTS: -8
* ZOPERATIONTIMEOUT: -7
* ZUNIMPLEMENTED: -6
* ZMARSHALLINGERROR: -5
* ZCONNECTIONLOSS: -4
* ZRUNTIMEINCONSISTENCY: -2
* ZSYSTEMERROR: -1
* ZOK: 0

#### <a name="api-errors"></a>api_errors

* ZSESSIONMOVED: -118
* ZNOTHING: -117
* ZCLOSING: -116
* ZAUTHFAILED: -115
* ZINVALIDACL: -114
* ZINVALIDCALLBACK: -113
* ZSESSIONEXPIRED: -112
* ZNOTEMPTY: -111
* ZNODEEXISTS: -110
* ZNOCHILDRENFOREPHEMERALS: -108
* ZBADVERSION: -103
* ZNOAUTH: -102
* ZNONODE: -101
* ZAPIERROR: -100
* ZOK: 0

#### <a name="states"></a>states

* AUTH_FAILED: -113
* EXPIRED_SESSION: -112
* CONNECTING: 1
* ASSOCIATING: 2
* CONNECTED: 3
* READONLY: 5
* NOTCONNECTED: 999

#### <a name="log-level"></a>log_level

* ERROR: 1
* INFO: 3
* WARN: 2
* DEBUG: 4

#### <a name="create-flags"></a>create_flags

* EPHEMERAL: 1
* SEQUENCE: 2

#### <a name="permissions"></a>permissions

* READ: 1
* WRITE: 2
* DELETE: 8
* ADMIN: 16
* ALL: 31
