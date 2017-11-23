# Клиент ZooKeeper для Tarantool
--------------------------------

## <a name="toc"></a>Содержание
--------------------------------------

* [Что такое ZooKeeper](#overview)
* [Справочник по API](#api-ref)
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
* [Приложение 1: константы ZooKeeper](#appndx-zk-constants)
  * [watch_types](#watch-types)
  * [errors](#errors)
  * [api_errors](#api-errors)
  * [states](#states)
  * [log_level](#log-level)
  * [create_flags](#create-flags)
  * [permissions](#permissions)
* [Copyright & License](#copyright-license)

## <a name="overview"></a>Что такое ZooKeeper
----------------------------------

ZooKeeper - это распределенное приложение для управления кластером, состоящим из большого количества узлов. Оно облегчает работу с такими объектами, как информация о конфигурации системы и иерархиеское пространство имен, а также предоставляет различнные сервисы, среди которых распределенная синхронизация и выборы лидера.

[К содержанию](#toc)

## <a name="api-ref"></a>Справочник по API
--------------------------------------

#### <a name="zk-init"></a>z = zookeeper.init(hosts, timeout, opts)
-------------------------------------------------------------------

Создает экземпляр ZooKeeper. Подключение при этом не создается.

**Параметры:**

* `hosts` - строка следующего формата: *host1:port1,host2:port2,...*. Значение по умолчанию - **127.0.0.1:2181**.
* `timeout` - *recv_timeout* (таймаут для сессии ZooKeeper) в секундах. Значение по умолчанию - **30000**.
* `opts` - Lua-таблица со следующими **полями**:

  * `clientid` - Lua-таблица следующего формата: *{client_id = \<число\>, passwd = \<строка\>}*. Значение по умолчанию - **nil**.
  * `flags` - флаги инициализации ZooKeeper. Значение по умолчанию - **0**.
  * `reconnect_timeout` - время в секундах до переподключения. Значение по умолчанию - **1**.
  * `default_acl` - список прав доступа (ACL), используемый для всех *create*-запросов по умолчанию. Должен быть экземпляром *zookeeper.acl.ACLList*. Значение по умолчанию - **zookeeper.acl.ACLS.OPEN_ACL_UNSAFE**.

[К содержанию](#toc)

#### <a name="zk-zerror"></a>err = zookeeper.zerror(errorcode)
--------------------------------------------------------------

Получает строковое описание кода ошибки ZooKeeper.

**Параметры:**

* `errorcode` - числовой код ошибки ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="zk-det-conn-order"></a>zookeeper.deterministic_conn_order(\<boolean\>)
------------------------------------------------------------------------------------

Инструктирует ZooKeeper не выбирать сервер случайным образом из указанных хостов, а перебирать их последовательно.

**Параметры:**

* булевое значение, которое указывает, нужно ли создавать подключения в строго детерминированном порядке

[К содержанию](#toc)

#### <a name="zk-set-log-level"></a>zookeeper.set_log_level(zookeeper.const.log_level.*)
----------------------------------------------------------------------------------------

Задает уровень журналирования в ZooKeeper.

**Параметры:**

* `zookeeper.const.log_level.*` - a constant corresponding to a certain logging level. Refer to the [list of acceptable values](#log-level).

[К содержанию](#toc)

### Методы экземпляра ZooKeeper
------------------------------

#### <a name="z-start"></a>z:start()
------------------------------------

Start a ZooKeeper I/O loop. Connection is established at this stage.

[К содержанию](#toc)

#### <a name="z-close"></a>z:close()
------------------------------------

Destroy a ZooKeeper instance. After this method is called, nothing is operable and `zookeeper.init()` must be called again.

[К содержанию](#toc)

#### <a name="z-state"></a>z:state()
------------------------------------

Return the current ZooKeeper state as a number. Refer to the list of [possible values](#states).

>Tip: to convert a number to a string name, use `zookeeper.const.states_rev[<number>]`.

[К содержанию](#toc)

#### <a name="z-is-conn"></a>z:is_connected()
---------------------------------------------

Return **true** when `z:state()` == *zookeeper.const.states.CONNECTED*.

[К содержанию](#toc)

#### <a name="z-wait-conn"></a>z:wait_connected()
-------------------------------------------------

Wait until the value of `z:state()` becomes *CONNECTED*.

[К содержанию](#toc)

#### <a name="z-client-id"></a>z:client_id()
--------------------------------------------

Return a Lua table of the following form:

```
{
	client_id = <number>, -- current session ID
	passwd = <string> -- password
}
```

[К содержанию](#toc)

#### <a name="z-set-watcher"></a>z:set_watcher(watcher_func, extra_context)
---------------------------------------------------------------------------

Set a watcher function called on every change in ZooKeeper.

**Параметры:**

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
  *где:*

  |Параметр|Описание|
  |---------|-----------|
  |`z`|A ZooKeeper instance|
  |`type`|An event type. Refer to the [list of acceptable values](#watch-types).|
  |`state`|An event state. Refer to the [list of acceptable values](#states).|
  |`path`|A path that specifies where an event occurred|
  |`context`|A variable passed to `z:set_watcher()` as the second argument|

* `extra_context` - a context passed to the watcher function

[К содержанию](#toc)

#### <a name="z-create"></a>z:create(path, value, acl, flags)
-------------------------------------------------------------

Create a ZooKeeper node.

**Параметры:**

* `path` - a string of the format: `/path/to/node`. `/path/to` must exist.
* `value` - a string value to store in a node (may be *nil*). Default is **nil**.
* `acl` (a *zookeeper.acl.ACLList* instance) - an ACL to use. Default is **z.default_acl**.
* `flags` - a combination of numeric [zookeeper.const.create_flags.\* constants](#create-flags).

[К содержанию](#toc)

#### <a name="z-ensure-path"></a>z:ensure_path(path)
----------------------------------------------------

Make sure that a path (including all the parent nodes) exists.

**Параметры:**

* `path` - a path to check

[К содержанию](#toc)

#### <a name="z-exists"></a>z:exists(path, watch)
-------------------------------------------------

Make sure that a path (including all the parent nodes) exists.

**Параметры:**

* `path` - a path to check
* `watch` (boolean) - specifies whether to include a path to a global watcher

**Возвращаемые переменные:**

* a boolean indicating if the path exists
* `stat` - node statistics of the following form:

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

  *где:*

  |Параметр|Описание|
  |---------|-----------|
  |`cversion`|The number of changes to the children of this node|
  |`mtime`|The time in milliseconds from epoch when this node was last modified|
  |`pzxid`|The zxid of the change that last modified children of this node|
  |`mzxid`|The zxid of the change that last modified this node|
  |`ephemeralOwner`|The session ID of the owner of this node if the node is an ephemeral node. If it is not an ephemeral node, it is zero.|
  |`aversion`|The number of changes to the ACL of this node|
  |`czxid`|The zxid of the change that caused this node to be created|
  |`dataLength`|The length of the data field of this node|
  |`numChildren`|The number of children of this node|
  |`ctime`|The time in milliseconds from epoch when this node was created|
  |`version`|The number of changes to the data of this node|

* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[К содержанию](#toc)

#### <a name="z-delete"></a>z:delete(path, version)
---------------------------------------------------

Delete a node.

**Параметры:**

* `path` - a path to a node to be deleted
* `version` - a number specifying which version to delete. Default is **-1**, which is *all versions*.

**Возвращаемые переменные:**

* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[К содержанию](#toc)

#### <a name="z-get"></a>z:get(path, watch)
-------------------------------------------

Get the value of a node.

**Параметры:**

* `path` - a path to a node that holds a needed value
* `watch` (boolean) - specifies whether to include a path to a global watcher

**Возвращаемые переменные:**

* `value` - the value of a node
* `stat` - node statistics
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[К содержанию](#toc)

#### <a name="z-set"></a>z:set(path, version)
---------------------------------------------

Set the value of a node.

**Параметры:**

* `path` - a path to a node to set a value on
* `version` - a number specifying which version to delete. Default is **-1**, which is *all versions*.

**Возвращаемые переменные:**

* a boolean indicating if the path exists
* `stat` - node statistics
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[К содержанию](#toc)

#### <a name="z-get-children"></a>z:get_children(path, watch)
-------------------------------------------------------------

Get a node's children.

**Параметры:**

* `path` - a path to a node to get the children of
* `watch` (boolean) - specifies whether to include a path to a global watcher

**Возвращаемые переменные:**

* an array of strings, each representing a node's child
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[К содержанию](#toc)

#### <a name="z-get-children2"></a>z:get_children2(path, watch)
---------------------------------------------------------------

Get a node's children and statistics.

**Параметры:**

* `path` - a path to a node to get the children of
* `stat` - node statistics
* `watch` (boolean) - specifies whether to include a path to a global watcher

**Возвращаемые переменные:**

* an array of strings, each representing a node's child
* a ZooKeeper return code. Refer to the list of possible [API errors](#api-errors) and [client errors](#errors).

[К содержанию](#toc)

## <a name="appndx-zk-constants"></a>Приложение 1: константы ZooKeeper
--------------------------------------------------------------------

С помощью `zookeeper.const` для каждого ключа можно получить доступ к ассоциативному массиву *\<ключ\>_rev*, содержащему обратное отображение кода ошибки в ее имя. Например, *zookeeper.const.api_errors_rev* выглядит так:

|Код|Ошибка|
|----|----|
|-118|ZSESSIONMOVED|
|-117|ZNOTHING|
|-116|ZCLOSING|
|-115|ZAUTHFAILED|
|-114|ZINVALIDACL|
|-113|ZINVALIDCALLBACK|
|-112|ZSESSIONEXPIRED|
|-111|ZNOTEMPTY|
|-110|ZNODEEXISTS|
|-108|ZNOCHILDRENFOREPHEMERALS|
|-103|ZBADVERSION|
|-102|ZNOAUTH|
|-101|ZNONODE|
|-100|ZAPIERROR|

[К содержанию](#toc)

### <a name="watch-types"></a>watch_types
-----------------------------------------

|Тип|Код|Описание|
|----|----|-----------|
|NOTWATCHING|-2|Watcher is inactive|
|SESSION|-1|Watching for session-related events|
|CREATED|1|Watching for node creation events. Triggered with a call to [z:exists()](#z-exists).|
|DELETED|2|Watching for node deletion events. Triggerd with a call to [z:exists()](#z-exists), [z:get()](#z-get), [z:get_children()](#z-get-children), or [z:get_children2()](#z-get-children2).|
|CHANGED|3|Watching for node change events. Triggered with a call to [z:exists()](#z-exists) or [z:get()](#z-get).|
|CHILD|4|Watching for child-related events. Triggered with a call to [z:get_children()](#z-get-children) or [z:get_children2()](#z-get-children2).|

[К содержанию](#toc)

### <a name="errors"></a>errors
-------------------------------

|Ошибка|Код|Описание|
|----|----|-----------|
|ZINVALIDSTATE|-9|Invalid zhandle state|
|ZBADARGUMENTS|-8|Invalid arguments|
|ZOPERATIONTIMEOUT|-7|Operation timeout|
|ZUNIMPLEMENTED|-6|Operation is unimplemented|
|ZMARSHALLINGERROR|-5|Error while marshalling or unmarshalling data|
|ZCONNECTIONLOSS|-4|Connection to the server has been lost|
|ZRUNTIMEINCONSISTENCY|-2|A runtime inconsistency was found|
|ZSYSTEMERROR|-1|System error|
|ZOK|0|Everything is OK|

[К содержанию](#toc)

### <a name="api-errors"></a>api_errors
---------------------------------------

|Ошибка|Код|Описание|
|----|----|----|
|ZSESSIONMOVED|-118|Session moved to another server, so the operation is ignored|
|ZNOTHING|-117| (not an error) no server responses to process|
|ZCLOSING|-116| ZooKeeper is closing|
|ZAUTHFAILED|-115|Client authentication failed|
|ZINVALIDACL|-114|Invalid ACL specified|
|ZINVALIDCALLBACK|-113|Invalid callback specified|
|ZSESSIONEXPIRED|-112|The session has been expired by the server|
|ZNOTEMPTY|-111|The node has children|
|ZNODEEXISTS|-110|The node already exists|
|ZNOCHILDRENFOREPHEMERALS|-108|Ephemeral nodes may not have children|
|ZBADVERSION|-103|Version conflict|
|ZNOAUTH|-102|Not authenticated|
|ZNONODE|-101|Node does not exist|
|ZAPIERROR|-100|API error|
|ZOK|0|Everything is OK|

[К содержанию](#toc)

### <a name="states"></a>states
-------------------------------

|Состояние|Код|Описание|
|----|----|-----------|
|AUTH_FAILED|-113|Authentication has failed|
|EXPIRED_SESSION|-112|Session has expired|
|CONNECTING|1|ZooKeeper is connecting|
|ASSOCIATING|2|Information obtained from ZooKeeper is being associated with the connection|
|CONNECTED|3|ZooKeeper is connected|
|READONLY|5|ZooKeeper is in read-only mode, accepting only read requests|
|NOTCONNECTED|999|ZooKeeper is not connected|

[К содержанию](#toc)

### <a name="log-level"></a>log_level
-------------------------------------

|Уровень|Код|Описание|
|----|----|-----------|
|ERROR|1|Log error events that might still allow the application to continue running|
|WARN|2|Log potentially harmful situations|
|INFO|3|Log informational messages that highlight the progress of the application at coarse-grained level
|DEBUG|4|Log fine-grained informational events that are most useful to debug an application|

[К содержанию](#toc)

### <a name="create-flags"></a>create_flags
-------------------------------------------

|Флаг|Код|Описание|
|----|----|-----------|
|EPHEMERAL|1|Create an ephemeral node|
|SEQUENCE|2|Create a sequence node|

[К содержанию](#toc)

### <a name="permissions"></a>permissions
-----------------------------------------

|Полномочия|Код|Описание|
|----------|----|-----------|
|READ|1|Can get data from a node and list its children|
|WRITE|2|Can set data for a node|
|DELETE|8|Can delete a child node|
|ADMIN|16|Can set permissions|
|ALL|31|Can do all of the above|

[К содержанию](#toc)

## <a name="copyright-license"></a>Copyright & License

* [LICENSE](LICENSE.md)

[К содержанию](#toc)
