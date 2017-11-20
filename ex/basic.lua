local fiber = require 'fiber'
local json = require 'json'

local zookeeper = require 'zookeeper'

local function global_watcher(z, type, state, path, context)
    print(string.format(
        'Global watcher. type = %s, state = %s, path = %s',
        zookeeper.const.watch_types_rev[type],
        zookeeper.const.states_rev[state],
        path))
    print('Extra context:', json.encode(context))
end

local z = zookeeper.init('127.0.0.1:2181')
z:set_watcher(global_watcher, {k = 'extra context'})
z:start()  -- starts zookeeper IO loop
z:wait_connected() -- waits until state is moved to connected

z:ensure_path('/zoo')
if not z:exists('/zoo/node1') then
    print(z:create('/zoo/node1', 'hello1'))
end
if not z:exists('/zoo/node2') then
    print(z:create('/zoo/node2', 'hello2'))
end
if not z:exists('/zoo/node3') then
    print(z:create('/zoo/node3', 'hello3'))
end

local value, stat = z:get('/zoo/node1')
print('/zoo/node1:', value, json.encode(stat))


z:set('/zoo/node1', 'hello11')

local value, stat = z:get('/zoo/node1')
print('/zoo/node1:', value, json.encode(stat))


local children = z:get_children('/zoo')
print('/zoo children:', json.encode(children))

z:delete('/zoo/node3')
print('deleted /zoo/node3')

local children = z:get_children('/zoo')
print('/zoo children:', json.encode(children))

z:close()
