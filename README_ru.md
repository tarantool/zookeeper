# Клиент ZooKeeper для Tarantool
--------------------------------

## <a name="toc"></a>Содержание
-------------------------------

* [Что такое ZooKeeper](#overview)
* [Установка](#installation)
* [Справочник по API](#api-ref)
  * [zookeeper.init()](#zk-init)
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
  * [z:wexists()](#z-wexists)
  * [z:wget()](#z-wget)
  * [z:wget_children()](#z-wget-children)
  * [z:wget_children2()](#z-wget-children2)
  * [z:get_acl()](#z-get-acl)
  * [z:set_acl()](#z-set-acl)
  * [z:sync()](#z-sync)
  * [acl.ACLList()](#acl-acllist)
  * [a:totable()](#a-totable)
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
---------------------------------------------

ZooKeeper - это распределенное приложение для управления кластером, состоящим из большого количества узлов. Оно облегчает работу с такими объектами, как информация о конфигурации системы и иерархическое пространство имен, а также предоставляет различные сервисы, среди которых распределенная синхронизация и выборы лидера.

[К содержанию](#toc)

## <a name="installation"></a>Установка

С помощью пакетного менеджера (CentOS, Fedora, Debian, Ubuntu):

* [Добавить][tarantool_repo] репозиторий tarantool.
* Установить пакет tarantool-zookeeper.

С помощью tarantoolctl rocks:

* Установить зависимости:
  - CentOS / Fedora: tarantool-devel, zookeeper-native
    ([полный список][deps_centos]).
  - Debian / Ubuntu: tarantool-dev, libzookeeper-st-dev, libzookeeper-st2
    ([полный список][deps_debian]).
* tarantoolctl rocks install zookeeper

[К содержанию](#toc)

[tarantool_repo]: https://tarantool.io/ru/download/
[deps_centos]: https://github.com/tarantool/zookeeper/blob/master/rpm/tarantool-zookeeper.spec#L9-L16
[deps_debian]: https://github.com/tarantool/zookeeper/blob/master/debian/control#L5-L15

## <a name="api-ref"></a>Справочник по API
------------------------------------------

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

* `zookeeper.const.log_level.*` - константа, соответствующая определенному уровню журналирования. См. [список допустимых значений](#log-level).

[К содержанию](#toc)

### Методы экземпляра ZooKeeper
-------------------------------

#### <a name="z-start"></a>z:start()
------------------------------------

Запускает цикл ввода-вывода ZooKeeper. Подключение создается на данном этапе.

[К содержанию](#toc)

#### <a name="z-close"></a>z:close()
------------------------------------

Уничтожает экземпляр ZooKeeper. После вызова данного метода для возобновления работы необходимо снова вызвать `zookeeper.init()`.

[К содержанию](#toc)

#### <a name="z-state"></a>z:state()
------------------------------------

Возвращает текущее состояние ZooKeeper в виде числа. См. список [возможных значений](#states).

>**Подсказка:** чтобы перевести число в строковое описание состояния, используйте `zookeeper.const.states_rev[<число>]`.

[К содержанию](#toc)

#### <a name="z-is-conn"></a>z:is_connected()
---------------------------------------------

Возвращает **true**, если `z:state()` == *zookeeper.const.states.CONNECTED*.

[К содержанию](#toc)

#### <a name="z-wait-conn"></a>z:wait_connected()
-------------------------------------------------

Ждет, пока `z:state()` не примет значение *zookeeper.const.states.CONNECTED*.

[К содержанию](#toc)

#### <a name="z-client-id"></a>z:client_id()
--------------------------------------------

Возвращает Lua-таблицу следующего вида:

```
{
	client_id = <число>, -- ID текущей сессии
	passwd = <строка> -- пароль
}
```

[К содержанию](#toc)

#### <a name="z-set-watcher"></a>z:set_watcher(watcher_func, extra_context)
---------------------------------------------------------------------------

Устанавливает функцию-наблюдатель, вызываемую при каждом изменении в ZooKeeper.

**Параметры:**

* `watcher_func` - функция со следующей сигнатурой:
  ```lua
  local function global_watcher(z, type, state, path, context)
      print(string.format(
  		    'Глобальная функция-наблюдатель. тип = %s, состояние = %s, путь = %s',
  		    zookeeper.const.watch_types_rev[type],
  		    zookeeper.const.states_rev[state],
  		    path))
      print('Дополнительный контекст:', json.encode(context))
  end
  ```
  *где:*

  |Параметр|Описание|
  |---------|-----------|
  |`z`|Экземпляр ZooKeeper|
  |`type`|Тип события. См. [список допустимых значений](#watch-types).|
  |`state`|Состояние события. См. [список допустимых значений](#states).|
  |`path`|Путь, указывающий, где произошло событие|
  |`context`|Переменная, которая передается в `z:set_watcher()` в качестве второго аргумента|

* `extra_context` - контекст, который передается в функцию-наблюдатель

[К содержанию](#toc)

#### <a name="z-create"></a>z:create(path, value, acl, flags)
-------------------------------------------------------------

Создает узел ZooKeeper.

**Параметры:**

* `path` - строка следующего формата: `/путь/до/узла`. `/путь/до` должен существовать.
* `value` - строковое значение, которое будет храниться на узле (может быть *nil*). Значение по умолчанию - **nil**.
* `acl` (экземпляр *zookeeper.acl.ACLList*) - используемый ACL. Значение по умолчанию - **z.default_acl**.
* `flags` - комбинация числовых [констант zookeeper.const.create_flags.\*](#create-flags).

[К содержанию](#toc)

#### <a name="z-ensure-path"></a>z:ensure_path(path)
----------------------------------------------------

Проверяет, что данный путь существует.

**Параметры:**

* `path` - проверяемый путь

[К содержанию](#toc)

#### <a name="z-exists"></a>z:exists(path, watch)
-------------------------------------------------

Проверяет, что данный узел (включая все родительские узлы) существует.

**Параметры:**

* `path` - проверяемый путь
* `watch` (булевое значение) - определяет, необходимо ли указывать путь до глобальной функции-наблюдателя

**Возвращаемые переменные:**

* булевое значение, указывающее, существует ли данный путь
* `stat` - статистика узла в следующей форме:

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
  |`cversion`|Число изменений, внесенных в потомков данного узла|
  |`mtime`|Время (в миллисекундах), прошедшее с опорной даты до момента последнего изменения в данном узле|
  |`pzxid`|Идентификатор zxid последнего изменения в потомках данного узла|
  |`mzxid`|Идентификатор zxid последнего изменения в данном узле|
  |`ephemeralOwner`|ID сессии владельца данного узла (если узел эфемерный). Если узел не эфемерный, данный параметр равен нулю.|
  |`aversion`|Число изменений, внесенных в ACL данного узла|
  |`czxid`|Идентификатор zxid изменения, в результате которого был создан данный узел|
  |`dataLength`|Длина поля данных данного узла|
  |`numChildren`|Число потомков данного узла|
  |`ctime`|Время (в миллисекундах), прошедшее с опорной даты до момента создания данного узла|
  |`version`|Число изменений данных в данном узле|

* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-delete"></a>z:delete(path, version)
---------------------------------------------------

Удаляет узел.

**Параметры:**

* `path` - путь до удаляемого узла
* `version` - номер, указывающий, какую версию необходимо удалить. Значение по умолчанию - **-1** (*все версии*).

**Возвращаемые переменные:**

* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-get"></a>z:get(path, watch)
-------------------------------------------

Получает значение узла.

**Параметры:**

* `path` - путь до узла, содержащего необходимое значение
* `watch` (булевое значение) - определяет, необходимо ли указывать путь до глобальной функции-наблюдателя

**Возвращаемые переменные:**

* `value` - значение узла
* `stat` - статистика узла
* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-set"></a>z:set(path, version)
---------------------------------------------

Устанавливает значение узла.

**Параметры:**

* `path` - путь до узла, на котором устанавливается значение
* `version` - номер, указывающий, какую версию необходимо удалить. Значение по умолчанию - **-1** (создать новую версию).

**Возвращаемые переменные:**

* булевое значение, указывающее, существует ли данный путь
* `stat` - статистика узла
* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-get-children"></a>z:get_children(path, watch)
-------------------------------------------------------------

Получает потомков узла.

**Параметры:**

* `path` - путь до узла, потомков которого необходимо получить
* `watch` (булевое значение) - определяет, необходимо ли указывать путь до глобальной функции-наблюдателя

**Возвращаемые переменные:**

* массив строк, каждая из которых представляет собой потомка узла
* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-get-children2"></a>z:get_children2(path, watch)
---------------------------------------------------------------

Получает потомков и статистику узла.

**Параметры:**

* `path` - путь до узла, потомков и статистику которого необходимо получить
* `watch` (булевое значение) - определяет, необходимо ли указывать путь до глобальной функции-наблюдателя

**Возвращаемые переменные:**

* массив строк, каждая из которых представляет собой потомка узла
* `stat` - статистика узла
* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-wexists"></a>z:wexists(path, func, context)
-----------------------------------------------------------

Проверяет, что данный путь существует, и устанавливает по этому пути локальную функцию-наблюдателя. Если функция не вызывалась ранее, она вызывается при создании (*zookeeper.const.watch_types.CREATED*) или удалении (*zookeeper.const.watch_types.DELETED*) узла.

>**Примечание:** во всех методах вида `z:w*()` функция-наблюдатель вызывается лишь единожды.

**Параметры:**

* `path` - проверяемый путь
* `func` - функция-наблюдатель (прототип такой же, как и для глобальной функции-наблюдателя (в [z:set_watcher()](#z-set-watcher)))
* `context `- любой дополнительный контекст, передаваемый в функцию

**Возвращаемые переменные:**

* булевое значение, указывающее, существует ли данный путь
* `stat` - статистика узла
* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-wget"></a>z:wget(path, func, context)
-----------------------------------------------------

Получает значение узла и устанавливает функцию-наблюдателя по указанному пути. Функция вызывается при внесении в узел изменений (*zookeeper.const.watch_types.CHANGED*).

**Параметры:**

* `path` - путь до узла, содержащего необходимое значение
* `func` - функция-наблюдатель (прототип такой же, как и для глобальной функции-наблюдателя (в [z:set_watcher()](#z-set-watcher)))
* `context `- любой дополнительный контекст, передаваемый в функцию

**Возвращаемые переменные:**

* `value` - значение узла
* `stat` - статистика узла
* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-wget-children"></a>z:wget_children(path, func, context)
-----------------------------------------------------------------------

Получает потомков узла и устанавливает функцию-наблюдателя по указанному пути. Функция вызывается, когда у узла появляется новый потомок (*zookeeper.const.watch_types.CHILD*).

**Параметры:**

* `path` - путь до узла, потомков которого необходимо получить
* `func` - функция-наблюдатель (прототип такой же, как и для глобальной функции-наблюдателя (в [z:set_watcher()](#z-set-watcher)))
* `context `- любой дополнительный контекст, передаваемый в функцию

**Возвращаемые переменные:**

* массив строк, каждая из которых представляет собой потомка узла
* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-wget-children2"></a>z:wget_children2(path, func, context)
-------------------------------------------------------------------------

Получает потомков и статистику узла и устанавливает функцию-наблюдателя по указанному пути. Функция вызывается, когда у узла появляется новый потомок (*zookeeper.const.watch_types.CHILD*).

**Параметры:**

* `path` - путь до узла, потомков и статистику которого необходимо получить
* `func` - функция-наблюдатель (прототип такой же, как и для глобальной функции-наблюдателя (в [z:set_watcher()](#z-set-watcher)))
* `context `- любой дополнительный контекст, передаваемый в функцию

**Возвращаемые переменные:**

* массив строк, каждая из которых представляет собой потомка узла
* `stat` - статистика узла
* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-get-acl"></a>z:get_acl(path)
--------------------------------------------

Получает ACL узла.

**Параметры:**

* `path` - путь до узла, содержащего необходимый ACL

**Возвращаемые переменные:**

* `acl` (экземпляр *zookeeper.acl.ACLList*) - ACL узла
* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-set-acl"></a>z:set_acl(path, acl)
-------------------------------------------------

Устанавливает ACL на узле.

**Параметры:**

* `path` - путь до узла, на котором устанавливается ACL
* `acl` (экземпляр *zookeeper.acl.ACLList*) - ACL, устанавливаемый на узле
* `version` - <number> version to change

**Возвращаемые переменные:**

* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

#### <a name="z-sync"></a>z:sync(path)
--------------------------------------

Производит синхронизацию по указанному пути.

**Параметры:**

* `path` - путь, по которому проводится синхронизация

**Возвращаемые переменные:**

* код возврата ZooKeeper. См. список возможных [ошибок API](#api-errors) и [ошибок клиента](#errors).

[К содержанию](#toc)

### Cписок прав доступа (ACL) в ZooKeeper
-----------------------------------------

#### <a name="acl-acllist"></a>local a = acl.ACLList({\<acl_table1\>, \<acl_table2\>, ...})
-------------------------------------------------------------------------------------------

Создает экземпляр *zookeeper.acl.ACLList*.

**Параметры:**

* массив ACL-таблиц следующего формата:

```
{
		perms = <комбинация флагов zookeeper.const.permissions.*>,
		scheme = <схема ZooKeeper>,
		id = <идентификатор ZooKeeper>
}
```

**Возвращаемые переменные:**

* экземпляр *zookeeper.acl.ACLList*

>**Примечание:** таблица *zookeeper.acl.ACLS* содержит три предопределенных экземпляра *zookeeper.acl.ACLList*:

```
local ACLS = {
    OPEN_ACL_UNSAFE = ACLList({
        {
            perms = permissions.ALL,
            scheme = 'world',
            id = 'anyone'
        }
    }),

    CREATOR_ALL_ACL = ACLList({
        {
            perms = permissions.ALL,
            scheme = 'auth',
            id = ''
        }
    }),

    READ_ACL_UNSAFE = ACLList({
        {
            perms = permissions.READ,
            scheme = 'world',
            id = 'anyone'
        }
    }),
}
```

>**Подсказка:** два экземпляра *zookeeper.acl.ACLList* можно сравнивать между собой:

```
local a = acl.ACLList({
	{
		perms = 31,
		scheme = 'world',
		id = 'anyone'
	}
})
print(a == zookeeper.acl.ACLS.OPEN_ACL_UNSAFE)  -- будет выведено true
```

[К содержанию](#toc)

#### <a name="a-totable"></a>a:totable()
----------------------------------------

Конвертирует экземпляр *zookeeper.acl.ACLList* в Lua-таблицу.

**Возвращаемые переменные:**

* Lua-таблица, использовавшаяся при создании экземпляра *zookeeper.acl.ACLList*

[К содержанию](#toc)

## <a name="appndx-zk-constants"></a>Приложение 1: константы ZooKeeper
----------------------------------------------------------------------

С помощью *zookeeper.const* для каждого ключа можно получить доступ к ассоциативному массиву *\<ключ\>_rev*, содержащему обратное отображение кода ошибки в ее имя. Например, *zookeeper.const.api_errors_rev* выглядит так:

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
|NOTWATCHING|-2|Функция-наблюдатель не установлена|
|SESSION|-1|Отслеживаются связанные с сессией события|
|CREATED|1|Отслеживаются случаи создания узла|
|DELETED|2|Отслеживаются случаи удаления узла|
|CHANGED|3|Отслеживаются случаи изменения узла|
|CHILD|4|Отслеживаются связанные с потомками события|

[К содержанию](#toc)

### <a name="errors"></a>errors
-------------------------------

|Ошибка|Код|Описание|
|----|----|-----------|
|ZINVALIDSTATE|-9|Неверное состояние zhandle|
|ZBADARGUMENTS|-8|Неверные аргументы|
|ZOPERATIONTIMEOUT|-7|Операция закончилась по таймауту|
|ZUNIMPLEMENTED|-6|Операция не реализована|
|ZMARSHALLINGERROR|-5|Ошибка при маршалинге или демаршалинге данных|
|ZCONNECTIONLOSS|-4|Утеряно подключение к серверу|
|ZRUNTIMEINCONSISTENCY|-2|При выполнении программы обнаружена неконсистентность|
|ZSYSTEMERROR|-1|Системная ошибка|
|ZOK|0|Операция выполнена успешно|

[К содержанию](#toc)

### <a name="api-errors"></a>api_errors
---------------------------------------

|Ошибка|Код|Описание|
|----|----|----|
|ZSESSIONMOVED|-118|Сессия перемещена на другой сервер, поэтому операция игнорируется|
|ZNOTHING|-117| (Не является ошибкой) нечего обрабатывать, ответы сервера отсутствуют|
|ZCLOSING|-116| ZooKeeper завершает работу|
|ZAUTHFAILED|-115|Не удалось аутентифицировать клиента|
|ZINVALIDACL|-114|Указан неверный ACL|
|ZINVALIDCALLBACK|-113|Указана неверная функция обратного вызова|
|ZSESSIONEXPIRED|-112|Сервер принудительно завершил сессию|
|ZNOTEMPTY|-111|Узел имеет потомков|
|ZNODEEXISTS|-110|Узел уже существует|
|ZNOCHILDRENFOREPHEMERALS|-108|Эфемерные узлы не могут иметь потомков|
|ZBADVERSION|-103|Конфликт версий|
|ZNOAUTH|-102|Аутентификация не пройдена|
|ZNONODE|-101|Узел не существует|
|ZAPIERROR|-100|Ошибка API|
|ZOK|0|Операция выполнена успешно|

[К содержанию](#toc)

### <a name="states"></a>states
-------------------------------

|Состояние|Код|Описание|
|----|----|-----------|
|AUTH_FAILED|-113|Аутентификация не удалась|
|EXPIRED_SESSION|-112|Сессия истекла|
|CONNECTING|1|ZooKeeper подключается|
|ASSOCIATING|2|Информация, полученная от ZooKeeper, привязывается к подключению|
|CONNECTED|3|ZooKeeper подключен|
|READONLY|5|ZooKeeper находится в режиме *read-only* и принимает только запросы на чтение|
|NOTCONNECTED|999|ZooKeeper не подключен|

[К содержанию](#toc)

### <a name="log-level"></a>log_level
-------------------------------------

|Уровень|Код|Описание|
|----|----|-----------|
|ERROR|1|Журналировать ошибки, после которых приложение может продолжить работу|
|WARN|2|Журналировать ситуации, которые потенциально могут нанести приложению вред|
|INFO|3|Журналировать высокоуровневые информационные сообщения, описывающие прогресс приложения|
|DEBUG|4|Журналировать детальные информационные сообщения, помогающие при отладке приложения|

[К содержанию](#toc)

### <a name="create-flags"></a>create_flags
-------------------------------------------

|Флаг|Код|Описание|
|----|----|-----------|
|EPHEMERAL|1|Создать эфемерный узел|
|SEQUENCE|2|Создать последовательный узел|

[К содержанию](#toc)

### <a name="permissions"></a>permissions
-----------------------------------------

|Полномочия|Код|Описание|
|----------|----|-----------|
|READ|1|Можно получать данные узла и список его потомков|
|WRITE|2|Можно устанавливать значения узла|
|DELETE|8|Можно удалять дочерние узлы|
|ADMIN|16|Можно выдавать полномочия|
|ALL|31|Можно делать все перечисленное выше|

[К содержанию](#toc)

## <a name="copyright-license"></a>Copyright & License
------------------------------------------------------

* [LICENSE](LICENSE.md)

[К содержанию](#toc)
