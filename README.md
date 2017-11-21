# ZooKeeper client for Tarantool
--------------------------------

## <a name="toc"></a>Table of contents
--------------------------------------

* [Overview](#overview)
* [API reference](#api-ref)
  * [zookeper.init()](#zk-init)
  * [zookeeper.zerror()](#zk-zerror)
  * [zookeeper.deterministic_conn_order()](#zk-det-conn-order)
  * [zookeeper.set_log_level()](#zk-set-log-level)
  * [z:start()](#z-start)
  * [z:close()](#z-close)
  * [z:state()](#z-state)
  * [z:is_connected()](#z-is-conn)
  * [z:wait_connected()](#z-wait-conn)
  * [z:client_id()](#z-client-id)
  * [z:set_watcher()](#z-set-watcher)
  * [z:create()](#z-create)
  * [z:ensure_path()](#z-ensure-path)
  * [z:exists()](#z-exists)
  * [z:delete()](#z-delete)
  * [z:get()](#z-get)
  * [z:set()](#z-set)
  * [z:get_children()](#z-get-children)
  * [z:get_children2()](#z-get-children2)

## <a name="overview"></a>Overview
----------------------------------

ZooKeeper is a distributed application for managing and coordinating a large number of hosts across a cluster. It helps maintain objects like configuration information and hierarchical naming space and provides various services, such as distributed synchronization and leader election.

[Back to TOC](#toc)

## <a name="api-ref"></a>API reference
--------------------------------------

##### <a name="zk-init"></a>z = zookeeper.init(hosts, timeout, opts)
--------------------------------------------------------------------

Create a ZooKeeper instance. No connection is established at this point.

**Parameters:**

* `hosts` - a string of the format: *host1:port1,host2:port2,...*. Default is **127.0.0.1:2181**.
* `timeout` - *recv_timeout* (ZooKeeper session timeout) in seconds. Default is **30000**.
* `opts` - a Lua table with the following **fields**:

  * `clientid` - a Lua table of the format *{client_id = \<number\>, passwd = \<string\>}*. Default is **nil**.
  * `flags` - ZooKeeper init flags. Default is **0**.
  * `reconnect_timeout` - time in seconds to wait before reconnecting. Default is **1**.
  * `default_acl` - a default access control list (ACL) to use for all *create* requests. Must be a *zookeeper.acl.ACLList* instance. Default is **zookeeper.acl.ACLS.OPEN_ACL_UNSAFE**.

[Back to TOC](#toc)

##### <a name="zk-zerror"></a>err = zookeeper.zerror(errorcode)
---------------------------------------------------------------

Get a string description for a ZooKeeper error code.

**Parameters:**

* `errorcode` - a numeric ZooKeeper error code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[Back to TOC](#toc)

##### <a name="zk-det-conn-order"></a>zookeeper.deterministic_conn_order(\<boolean\>)
------------------------------------------------------------------------

Instruct ZooKeeper not to randomly choose a server from the hosts provided, but select them sequentially instead.

**Parameters:**

* a boolean specifying whether a deterministic connection order should be enforced

[Back to TOC](#toc)

##### <a name="zk-set-log-level"></a>zookeeper.set_log_level(zookeeper.const.log_level.*)
------------------------------------------------------------------------

Set a ZooKeeper logging level.

**Parameters:**

* `zookeeper.const.log_level.*` - a constant corresponding to a certain logging level. Refer to the [list of acceptable values](#log-level).

[Back to TOC](#toc)

### ZooKeeper instance methods
------------------------------

##### <a name="z-start"></a>z:start()
-------------------------------------

Start a ZooKeeper I/O loop. Connection is established at this stage.

[Back to TOC](#toc)

##### <a name="z-close"></a>z:close()
-------------------------------------

Destroy a ZooKeeper instance. After this method is called, nothing is operable and `zookeeper.init()` must be called again.

[Back to TOC](#toc)

##### <a name="z-state"></a>z:state()
-------------------------------------

Return the current ZooKeeper state as a number. Refer to the list of [possible values](#states).

>Tip: to convert a number to a string name, use `zookeeper.const.states_rev[<number>]`.

[Back to TOC](#toc)

##### <a name="z-is-conn"></a>z:is_connected()
----------------------------------------------

Return **true** when `z:state()` == *zookeeper.const.states.CONNECTED*.

[Back to TOC](#toc)

##### <a name="z-wait-conn"></a>z:wait_connected()
--------------------------------------------------

Wait until the value of `z:state()` becomes *CONNECTED*.

[Back to TOC](#toc)

##### <a name="z-client-id"></a>z:client_id()
---------------------------------------------

Return a Lua table of the following form:

```
{
	client_id = <number>, -- current session ID
	passwd = <string> -- password
}
```

[Back to TOC](#toc)

##### <a name="z-set-watcher"></a>z:set_watcher(watcher_func, extra_context)
------------------------------------------------------------------------

Set a watcher function called on every change in ZooKeeper.

**Parameters:**

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
*where:*

  * `z` - a ZooKeeper instance
  * `type` - an event type. Refer to the [list of acceptable values](#watch-types).
  * `state` - an event state. Refer to the [list of acceptable values](#states).
  * `path` - a path that specifies where an event occurred
  * `context` - a variable passed to `z:set_watcher()` as the second argument

* `extra_context` - a context passed to the watcher function

[Back to TOC](#toc)

##### <a name="z-create"></a>z:create(path, value, acl, flags)
--------------------------------------------------------------

Create a ZooKeeper node.

**Parameters:**

* `path` - a string of the format: `/path/to/node`. `/path/to` must exist.
* `value` - a string value to store in a node (may be *nil*). Default is **nil**.
* `acl` (a *zookeeper.acl.ACLList* instance) - an ACL to use. Default is **z.default_acl**.
* `flags` - a combination of numeric [zookeeper.const.create_flags.\* constants](#create-flags).

[Back to TOC](#toc)

##### <a name="z-ensure-path"></a>z:ensure_path(path)
-----------------------------------------------------

Make sure that a path (including all the parent nodes) exists.

**Parameters:**

* `path` - a path to check

[Back to TOC](#toc)

##### <a name="z-exists"></a>z:exists(path, watch)
--------------------------------------------------

Make sure that a path (including all the parent nodes) exists.

**Parameters:**

* `path` - a path to check
* `watch` (boolean) - specifies whether to include a path to a global watcher

**Returns:**

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

[Back to TOC](#toc)

##### <a name="z-delete"></a>z:delete(path, version)
----------------------------------------------------

Delete a node.

**Parameters:**

* `path` - a path to a node to be deleted
* `version` - a number specifying which version to delete. Default is **-1**, which is *all versions*.

**Returns:**

* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[Back to TOC](#toc)

##### <a name="z-get"></a>z:get(path, watch)
--------------------------------------------

Get the value of a node.

**Parameters:**

* `path` - a path to a node that holds a needed value
* `watch` (boolean) - specifies whether to include a path to a global watcher

**Returns:**

* `value` - the value of a node
* `stat` - node statistics
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[Back to TOC](#toc)

##### <a name="z-set"></a>z:set(path, version)
----------------------------------------------

Set the value of a node.

**Parameters:**

* `path` - a path to a node to set a value on
* `version` - a number specifying which version to delete. Default is **-1**, which is *all versions*.

**Returns:**

* a boolean indicating if the path exists
* `stat` - node statistics
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[Back to TOC](#toc)

##### <a name="z-get-children"></a>z:get_children(path, watch)
--------------------------------------------------------------

Get a node's children.

**Parameters:**

* `path` - a path to a node to get the children of
* `watch` (boolean) - specifies whether to include a path to a global watcher

**Returns:**

* an array of strings, each representing a node's child
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[Back to TOC](#toc)

##### <a name="z-get-children2"></a>z:get_children2(path, watch)
----------------------------------------------------------------

Get a node's children and statistics.

**Parameters:**

* `path` - a path to a node to get the children of
* `stat` - node statistics
* `watch` (boolean) - specifies whether to include a path to a global watcher

**Returns:**

* an array of strings, each representing a node's child
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[Back to TOC](#toc)

### ZooKeeper constants
-----------------------

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

[Back to TOC](#toc)

#### <a name="watch-types"></a>watch_types

* NOTWATCHING: -2
* SESSION: -1
* CREATED: 1
* DELETED: 2
* CHANGED: 3
* CHILD: 4

[Back to TOC](#toc)

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

[Back to TOC](#toc)

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

[Back to TOC](#toc)

#### <a name="states"></a>states

* AUTH_FAILED: -113
* EXPIRED_SESSION: -112
* CONNECTING: 1
* ASSOCIATING: 2
* CONNECTED: 3
* READONLY: 5
* NOTCONNECTED: 999

[Back to TOC](#toc)

#### <a name="log-level"></a>log_level

* ERROR: 1
* INFO: 3
* WARN: 2
* DEBUG: 4

[Back to TOC](#toc)

#### <a name="create-flags"></a>create_flags

* EPHEMERAL: 1
* SEQUENCE: 2

[Back to TOC](#toc)

#### <a name="permissions"></a>permissions

* READ: 1
* WRITE: 2
* DELETE: 8
* ADMIN: 16
* ALL: 31

[Back to TOC](#toc)
