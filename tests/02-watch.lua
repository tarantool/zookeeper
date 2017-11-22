#!/usr/bin/env tarantool

package.path = "../?/init.lua;./?/init.lua"
package.cpath = "../?.so;../?.dylib;./?.so;./?.dylib"

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
    
    local ret = cond:wait(1)
    if ret == false then
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
        -- fiber.create(function()
            -- fiber.yield()
            t:ok(rawequal(z_, z), 'z instance is the same')
            t:is(type, zkconst.watch_types.CREATED, 'type is CREATED')
            t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
            t:is(path, '/mypath', 'path is correct')
            cond:signal()
        -- end)
    end
    
    z:set_watcher(global_watcher)
    
    
    local _, _, rc = z:exists('/mypath', true)
    t:is(rc, zkconst.api_errors.ZNONODE, 'no node before create')
    
    print(z:create('/mypath'))
    
    local ret = cond:wait(5)
    if ret == false then
        t:fail('global watcher timed out')
    else
        t:ok(true, 'global watcher triggered')
    end
    
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
    
    z:set('/mypath', 'value1')
    
    local ret = cond:wait(1)
    if ret == false then
        t:fail('global watcher timed out')
    else
        t:ok(true, 'global watcher triggered')
    end
    
    z:set_watcher(nil)
    z:delete('/mypath')
end


local function test_global_watch_get_children(t, z)
    t:plan(7)
    
    local cond = fiber.cond()
    
    local function global_watcher(z_, type, state, path)
        if type == zkconst.watch_types.SESSION then
            return
        end
        
        t:ok(rawequal(z_, z), 'z instance is the same')
        t:is(type, zkconst.watch_types.CREATED, 'type is CREATED')
        t:is(state, zkconst.states.CONNECTED, 'state is CONNECTED')
        t:is(path, '/mypath', 'path is correct')
        cond:signal()
    end
    z:set_watcher(global_watcher)
    
    local _, rc = z:get_children('/mypath', true)
    t:is(rc, zkconst.api_errors.ZNONODE, 'no node before create')
    
    z:create('/mypath')
    
    local ret = cond:wait(1)
    if ret == false then
        t:fail('global watcher timed out')
    else
        t:ok(true, 'global watcher triggered')
    end
    
    z:set_watcher(nil)
    z:delete('/mypath')
end


local function main()
    local hosts = get_hosts()
    local z = zookeeper.init(hosts)
    z:start()
    z:wait_connected()
    
    -- tap.test('test_global_watch_connect', test_global_watch_connect)
    tap.test('test_global_watch_exists', test_global_watch_exists, z)
    -- tap.test('test_global_watch_get', test_global_watch_get, z)
    -- tap.test('test_global_watch_get_children', test_global_watch_get_children, z)

    z:close()
end

main()
