#!/usr/bin/env tarantool

package.path = "../?/init.lua;./?/init.lua;" .. package.path
package.cpath = "../?.so;../?.dylib;./?.so;./?.dylib;" .. package.cpath

local fiber = require 'fiber'
local tap = require 'tap'
local zookeeper = require 'zookeeper'
local zkacl = require 'zookeeper.acl'
local zkconst = require 'zookeeper.const'

local function get_hosts()
    return os.getenv('ZOOKEEPER') or '127.0.0.1:2181'
end


local function test_global_watch_connect(t)
    t:plan(6)
    
    local cond = fiber.cond()
    local user_context = {k1 = 'v1'}
    
    local function global_watcher(z_, type, state, path, context)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.SESSION, 'type is session')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '', 'path is empty')
        t:ok(rawequal(context, user_context), 'context is same')
        cond:signal()
    end
    
    z = zookeeper.init(get_hosts())
    z:set_watcher(global_watcher, user_context)
    z:start()
    
    if not cond:wait(1) then
        t:fail('global watcher timed out')
    else
        t:ok(true, 'global watcher triggered')
    end
    
    z:close()
end


local function test_global_watch_connect_watcher_sleep(t)
    t:plan(6)
    
    local cond = fiber.cond()
    local user_context = {k1 = 'v1'}
    
    local function global_watcher(z_, type, state, path, context)
        fiber.sleep(1)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.SESSION, 'type is session')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '', 'path is empty')
        t:ok(rawequal(context, user_context), 'context is same')
        cond:signal()
    end
    
    z = zookeeper.init(get_hosts())
    z:set_watcher(global_watcher, user_context)
    z:start()
    
    if not cond:wait(1.5) then
        t:fail('global watcher timed out')
    else
        t:ok(true, 'global watcher triggered')
    end
    
    z:close()
end


local function test_global_watch_exists(t, z)
    t:plan(6)
    z:delete('/mypath')
    
    local cond = fiber.cond()
    
    local function global_watcher(z_, type, state, path)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.CREATED, 'type is CREATED')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '/mypath', 'path is correct')
        cond:signal()
    end
    
    z:set_watcher(global_watcher)
    
    local _, _, rc = z:exists('/mypath', true)
    t:is(rc, zkconst.api_errors.ZNONODE, 'no node before create')
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not cond:wait(1) then
            t:fail('global watcher timed out')
        else
            t:ok(true, 'global watcher triggered')
        end
    end)
    
    z:create('/mypath')
    
    z:set_watcher(nil)
    z:delete('/mypath')
end


local function test_global_watch_get(t, z)
    t:plan(6)
    
    local cond = fiber.cond()
    
    local function global_watcher(z_, type, state, path)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.CHANGED, 'type is CHANGED')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '/mypath', 'path is correct')
        cond:signal()
    end
    
    z:set_watcher(global_watcher)
    
    z:create('/mypath')
    
    local _, _, rc = z:get('/mypath', true)
    t:is(rc, zkconst.ZOK, 'get ZOK')
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not cond:wait(1) then
            t:fail('global watcher timed out')
        else
            t:ok(true, 'global watcher triggered')
        end
    end)
    
    z:set('/mypath', 'value1')
    
    z:set_watcher(nil)
    z:delete('/mypath')
end


local function test_global_watch_get_children(t, z)
    t:plan(5)
    
    local cond = fiber.cond()
    
    local function global_watcher(z_, type, state, path)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.CHILD, 'type is CHILD')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '/mypath', 'path is correct')
        cond:signal()
    end
    z:set_watcher(global_watcher)
    
    z:create('/mypath')
    local _, rc = z:get_children('/mypath', true)
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not cond:wait(1) then
            t:fail('global watcher timed out')
        else
            t:ok(true, 'global watcher triggered')
        end
    end)
    
    z:create('/mypath/n1')
    
    z:set_watcher(nil)
    z:delete('/mypath/n1')
    z:delete('/mypath')
end


local function test_global_watch_get_children2(t, z)
    t:plan(5)
    
    local cond = fiber.cond()
    
    local function global_watcher(z_, type, state, path)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.CHILD, 'type is CHILD')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '/mypath', 'path is correct')
        cond:signal()
    end
    z:set_watcher(global_watcher)
    
    z:create('/mypath')
    local _, rc = z:get_children2('/mypath', true)
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not cond:wait(1) then
            t:fail('global watcher timed out')
        else
            t:ok(true, 'global watcher triggered')
        end
    end)
    
    z:create('/mypath/n1')
    
    z:set_watcher(nil)
    z:delete('/mypath/n1')
    z:delete('/mypath')
end


local function test_wexists(t, z)
    t:plan(8)
    
    local gcond = fiber.cond()
    local lcond = fiber.cond()
    local ch_cap = 2
    local ch = fiber.channel(ch_cap)
    
    local function global_watcher(z_, type, state, path)
        t:fail('global watcher should not be called')
        gcond:signal()
    end
    z:set_watcher(global_watcher)
    
    
    local extra_context = {k1 = 'v1'}
    local function local_watcher(z_, type, state, path, context)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.CREATED, 'type is CREATED')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '/mypath', 'path is correct')
        t:ok(rawequal(context, extra_context), 'context is ok')
        lcond:signal()
    end
    
    local _, _, rc = z:wexists('/mypath', local_watcher, extra_context)
    t:is(rc, zkconst.api_errors.ZNONODE, 'no node before create')
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not gcond:wait(0.5) then
            t:ok(true, 'global watcher not triggered')
        end
        ch:put(true)
    end)
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not lcond:wait(1) then
            t:fail('local watcher timed out')
        else
            t:ok(true, 'local watcher triggered')
        end
        ch:put(true)
    end)
    
    z:create('/mypath')
    
    for _=1,ch_cap do
        ch:get()
    end
    
    z:set_watcher(nil)
    z:delete('/mypath')
end


local function test_wexists_sleep(t, z)
    t:plan(8)
    
    local gcond = fiber.cond()
    local lcond = fiber.cond()
    local ch_cap = 2
    local ch = fiber.channel(ch_cap)
    
    local function global_watcher(z_, type, state, path)
        t:fail('global watcher should not be called')
        gcond:signal()
    end
    z:set_watcher(global_watcher)
    
    
    local extra_context = {k1 = 'v1'}
    local function local_watcher(z_, type, state, path, context)
        fiber.sleep(1)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.CREATED, 'type is CREATED')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '/mypath', 'path is correct')
        t:ok(rawequal(context, extra_context), 'context is ok')
        lcond:signal()
    end
    
    local _, _, rc = z:wexists('/mypath', local_watcher, extra_context)
    t:is(rc, zkconst.api_errors.ZNONODE, 'no node before create')
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not gcond:wait(0.5) then
            t:ok(true, 'global watcher not triggered')
        end
        ch:put(true)
    end)
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not lcond:wait(1.5) then
            t:fail('local watcher timed out')
        else
            t:ok(true, 'local watcher triggered')
        end
        ch:put(true)
    end)
    
    z:create('/mypath')
    
    for _=1,ch_cap do
        ch:get()
    end
    
    z:set_watcher(nil)
    z:delete('/mypath')
end


local function test_wget(t, z)
    t:plan(8)
    
    local gcond = fiber.cond()
    local lcond = fiber.cond()
    local ch_cap = 2
    local ch = fiber.channel(ch_cap)
    
    local function global_watcher(z_, type, state, path)
        t:fail('global watcher should not be called')
        gcond:signal()
    end
    z:set_watcher(global_watcher)
    
    
    local extra_context = {k1 = 'v1'}
    local function local_watcher(z_, type, state, path, context)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.CHANGED, 'type is CHANGED')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '/mypath', 'path is correct')
        t:ok(rawequal(context, extra_context), 'context is ok')
        lcond:signal()
    end
    
    z:create('/mypath')
    local _, _, rc = z:wget('/mypath', local_watcher, extra_context)
    t:is(rc, zkconst.ZOK, 'path exists')
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not gcond:wait(0.5) then
            t:ok(true, 'global watcher not triggered')
        end
        ch:put(true)
    end)
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not lcond:wait(1) then
            t:fail('local watcher timed out')
        else
            t:ok(true, 'local watcher triggered')
        end
        ch:put(true)
    end)
    
    z:set('/mypath', 'value2')
    
    for _=1,ch_cap do
        ch:get()
    end
    
    z:set_watcher(nil)
    z:delete('/mypath')
end


local function test_wget_children(t, z)
    t:plan(9)
    
    local gcond = fiber.cond()
    local lcond = fiber.cond()
    local ch_cap = 2
    local ch = fiber.channel(ch_cap)
    
    local function global_watcher(z_, type, state, path)
        t:fail('global watcher should not be called')
        gcond:signal()
    end
    z:set_watcher(global_watcher)
    
    
    local extra_context = {k1 = 'v1'}
    local function local_watcher(z_, type, state, path, context)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.CHILD, 'type is CHILD')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '/mypath', 'path is correct')
        t:ok(rawequal(context, extra_context), 'context is ok')
        lcond:signal()
    end
    
    z:create('/mypath')
    local children, rc = z:wget_children('/mypath', local_watcher, extra_context)
    t:is_deeply(children, {}, 'no dhildren yet')
    t:is(rc, zkconst.ZOK, 'path exists')
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not gcond:wait(0.5) then
            t:ok(true, 'global watcher not triggered')
        end
        ch:put(true)
    end)
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not lcond:wait(1) then
            t:fail('local watcher timed out')
        else
            t:ok(true, 'local watcher triggered')
        end
        ch:put(true)
    end)
    
    z:create('/mypath/n1')
    
    for _=1,ch_cap do
        ch:get()
    end
    
    z:set_watcher(nil)
    z:delete('/mypath/n1')
    z:delete('/mypath')
end


local function test_wget_children2(t, z)
    t:plan(9)
    
    local gcond = fiber.cond()
    local lcond = fiber.cond()
    local ch_cap = 2
    local ch = fiber.channel(ch_cap)
    
    local function global_watcher(z_, type, state, path)
        t:fail('global watcher should not be called')
        gcond:signal()
    end
    z:set_watcher(global_watcher)
    
    
    local extra_context = {k1 = 'v1'}
    local function local_watcher(z_, type, state, path, context)
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.CHILD, 'type is CHILD')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '/mypath', 'path is correct')
        t:ok(rawequal(context, extra_context), 'context is ok')
        lcond:signal()
    end
    
    z:create('/mypath')
    local children, _, rc = z:wget_children2('/mypath', local_watcher, extra_context)
    t:is_deeply(children, {}, 'no dhildren yet')
    t:is(rc, zkconst.ZOK, 'path exists')
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not gcond:wait(0.5) then
            t:ok(true, 'global watcher not triggered')
        end
        ch:put(true)
    end)
    
    fiber.create(function()
        -- detach to wait before signal() happens
        if not lcond:wait(1) then
            t:fail('local watcher timed out')
        else
            t:ok(true, 'local watcher triggered')
        end
        ch:put(true)
    end)
    
    z:create('/mypath/n1')
    
    for _=1,ch_cap do
        ch:get()
    end
    
    z:set_watcher(nil)
    z:delete('/mypath/n1')
    z:delete('/mypath')
end


local function main()
    local hosts = get_hosts()
    local z = zookeeper.init(hosts)
    z:start()
    z:wait_connected(10)
    
    tap.test('test_global_watch_connect', test_global_watch_connect)
    tap.test('test_global_watch_connect_watcher_sleep', test_global_watch_connect_watcher_sleep)
    tap.test('test_global_watch_exists', test_global_watch_exists, z)
    tap.test('test_global_watch_get', test_global_watch_get, z)
    tap.test('test_global_watch_get_children', test_global_watch_get_children, z)
    tap.test('test_global_watch_get_children2', test_global_watch_get_children2, z)
    tap.test('test_wexists', test_wexists, z)
    tap.test('test_wexists_sleep', test_wexists_sleep, z)
    tap.test('test_wget', test_wget, z)
    tap.test('test_wget_children', test_wget_children, z)
    tap.test('test_wget_children2', test_wget_children2, z)

    z:close()
end

main()
