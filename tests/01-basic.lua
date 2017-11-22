#!/usr/bin/env tarantool

package.path = "../?/init.lua;./?/init.lua"
package.cpath = "../?.so;../?.dylib;./?.so;./?.dylib"

local tap = require 'tap'
local zookeeper = require 'zookeeper'
local zkacl = require 'zookeeper.acl'
local zkconst = require 'zookeeper.const'

local function test_connection(t, z)
    t:plan(5)
    
    t:isnt(z, nil, 'z is valid')
    t:is(z:state(), zkconst.states.NOTCONNECTED, 'z is in NOTCONNECTED state')
    t:is(z:is_connected(), false, 'z:is_connected == false')
    
    z:start()
    z:wait_connected()
    
    t:is(z:state(), zkconst.states.CONNECTED, 'z moved to CONNECTED state')
    t:ok(z:is_connected(), 'z:is_connected == true')
end

local function test_op_on_not_connected(t, z)
    local operations = {
        {'create', '/path'},
        {'get', '/path'},
        {'exists', '/path'},
        {'delete', '/path'},
        {'set', '/path', 'value'},
        {'get_children', '/path'},
        {'get_children2', '/path'},
        {'sync', '/path'},
        {'wexists', '/path'},
        {'wget', '/path'},
        {'wget_children', '/path'},
        {'wget_children2', '/path'},
        {'get_acl', '/path'},
        {'set_acl', '/path', nil, zkacl.ACLS.OPEN_ACL_UNSAFE}
    }
    
    t:plan(#operations * 2)
    
    local function check_op(name, ...)
        local ok, err = pcall(z[name], z, ...)
        t:is(ok, false, name .. ' failed on not connected zookeeper')
        t:is(err, 'zookeeper not connected', 'correct fail message on not connected')
    end
    
    for _, op in ipairs(operations) do
        check_op(unpack(op))
    end
end


local function test_create(t, z)
    t:plan(11)
    
    local path, rc = z:create('/newpath')
    t:is(path, '/newpath', 'create /newpath successful')
    t:is(rc, zkconst.ZOK, 'create rc is ZOK')
    
    local path, rc = z:create('/newpath')
    t:is(path, nil, 'create /newpath again returned nil')
    t:is(rc, zkconst.api_errors.ZNODEEXISTS, 'create again rc is ZNODEEXISTS')
    
    z:delete('/newpath')
    
    local path, rc = z:create('/new/path')
    t:is(path, nil, 'create /new/path when no /new returned nil')
    t:is(rc, zkconst.api_errors.ZNONODE, 'create /new/path rc is ZNONODE')
    
    local _, rc = z:create('/newpath', 'my value')
    t:is(rc, zkconst.ZOK, 'create with value is ZOK')
    
    local value = z:get('/newpath')
    t:is(value, 'my value', 'get after create returns value')
    
    z:delete('/newpath')
    
    local acl2_ref = zkacl.ACLList({ {
        perms=bit.bor(zkconst.permissions.READ, zkconst.permissions.WRITE),
        scheme='world',
        id='anyone'
    } })
    local path, rc = z:create('/newpath', nil, acl2_ref)
    t:is(path, '/newpath', 'path acl is ok')
    t:is(rc, zkconst.ZOK, 'ZOK')
    
    local acl2 = z:get_acl('/newpath')
    t:is(acl2, acl2_ref, 'ACL changed')
    
    z:delete('/newpath')
end


local function test_ensure_path(t, z)
    t:plan(1)
    
    local rc = z:ensure_path('/p1/p2/p3/p4')
    t:is(rc, zkconst.ZOK, 'ZOK')
    
    rc = z:delete('/p1/p2/p3/p4')
    t:is(rc, zkconst.ZOK, 'ZOK on delete p4')
    
    rc = z:delete('/p1/p2/p3')
    t:is(rc, zkconst.ZOK, 'ZOK on delete p3')
    
    rc = z:delete('/p1/p2')
    t:is(rc, zkconst.ZOK, 'ZOK on delete p2')
    
    rc = z:delete('/p1')
    t:is(rc, zkconst.ZOK, 'ZOK on delete p1')
end


local function test_exists(t, z)
    t:plan(6)
    
    local exists, stat, rc = z:exists('/newpath')
    t:is(exists, false, 'node not exists before create')
    t:is(stat.mtime, 0, 'stat fields are 0')
    t:is(rc, zkconst.api_errors.ZNONODE, 'ZNONODE error')
    
    z:create('/newpath')
    
    local exists, stat, rc = z:exists('/newpath')
    t:ok(exists, 'node exists after create')
    t:isnt(stat.mtime, 0, 'stat fields are not 0')
    t:is(rc, zkconst.ZOK, 'ZOK')
    
    z:delete('/newpath')
end


local function test_get(t, z)
    t:plan(6)
    
    local value, stat, rc = z:get('/newpath')
    t:is(value, nil, 'node not exists before create')
    t:is(stat.mtime, 0, 'stat fields are 0')
    t:is(rc, zkconst.api_errors.ZNONODE, 'ZNONODE error')
    
    z:create('/newpath', 'value1')
    
    local value, stat, rc = z:get('/newpath')
    t:is(value, 'value1', 'node exists after create')
    t:isnt(stat.mtime, 0, 'stat fields are not 0')
    t:is(rc, zkconst.ZOK, 'ZOK')
    
    z:delete('/newpath')
end


local function test_delete(t, z)
    t:plan(6)
    
    local rc = z:delete('/newpath')
    t:is(rc, zkconst.api_errors.ZNONODE, 'ZNONODE error')
    
    z:create('/newpath', 'value1')
    
    local rc = z:delete('/newpath')
    t:is(rc, zkconst.ZOK, 'ZOK')
end


local function test_set(t, z)
    t:plan(8)
    
    local exists, stat, rc = z:set('/newpath', 'value2')
    t:is(exists, false, 'node not exists before create')
    t:is(stat.mtime, 0, 'stat fields are 0')
    t:is(rc, zkconst.api_errors.ZNONODE, 'ZNONODE error')
    
    z:create('/newpath', 'value1')
    local value1, stat1 = z:get('/newpath')
    
    local exists, stat2, rc = z:set('/newpath', 'value2')
    t:ok(exists, 'node exists after create')
    t:ok(stat2.mtime > stat1.mtime, 'stat.mtime increased')
    t:ok(stat2.version > stat1.version, 'stat.version increased')
    t:is(rc, zkconst.ZOK, 'ZOK')
    
    local value2, stat2 = z:get('/newpath')
    t:is(value2, 'value2', 'value changed')
    
    z:delete('/newpath')
end


local function test_get_children(t, z)
    t:plan(4)
    
    local children, rc = z:get_children('/newpath')
    t:is(children, nil, 'get_children errored')
    t:is(rc, zkconst.api_errors.ZNONODE, 'ZNONODE error')
    
    z:create('/newpath')
    z:create('/newpath/n1')
    z:create('/newpath/n2')
    z:create('/newpath/n3')
    
    local children, rc = z:get_children('/newpath')
    t:is(rc, zkconst.ZOK, 'ZOK')
    t:is_deeply(children, {'n1', 'n2', 'n3'}, 'children returned')
    
    z:delete('/newpath/n1')
    z:delete('/newpath/n2')
    z:delete('/newpath/n3')
    z:delete('/newpath')
end


local function test_get_children2(t, z)
    t:plan(5)
    
    local children, stat, rc = z:get_children2('/newpath')
    t:is(children, nil, 'get_children errored')
    t:is(rc, zkconst.api_errors.ZNONODE, 'ZNONODE error')
    
    z:create('/newpath')
    z:create('/newpath/n1')
    z:create('/newpath/n2')
    z:create('/newpath/n3')
    
    local _, stat1 = z:get('/newpath')
    
    local children, stat2, rc = z:get_children2('/newpath')
    t:is(rc, zkconst.ZOK, 'ZOK')
    t:is_deeply(children, {'n1', 'n2', 'n3'}, 'children returned')
    t:is_deeply(stat2, stat1, 'stat is correct')
    
    z:delete('/newpath/n1')
    z:delete('/newpath/n2')
    z:delete('/newpath/n3')
    z:delete('/newpath')
end


local function test_get_acl(t, z)
    t:plan(3)
    
    z:create('/newpath')
    
    local acl, _, rc = z:get_acl('/newpath')
    t:is(rc, zkconst.ZOK, 'ZOK')
    t:isnt(acl, nil, 'acl is not nil')
    t:is_deeply(acl, zkacl.ACLS.OPEN_ACL_UNSAFE, 'ACL is OPEN_ACL_UNSAFE')
    
    z:delete('/newpath')
end


local function test_set_acl(t, z)
    t:plan(2)
    
    z:create('/newpath')
    
    local acl1 = z:get_acl('/newpath')
    
    local acl2_ref = zkacl.ACLList({ {
        perms=bit.bor(zkconst.permissions.READ, zkconst.permissions.WRITE),
        scheme='world',
        id='anyone'
    } })
    local rc = z:set_acl('/newpath', acl2_ref)
    t:is(rc, zkconst.ZOK, 'ZOK')
    
    local acl2 = z:get_acl('/newpath')
    t:is(acl2, acl2_ref, 'ACL changed')
    
    z:delete('/newpath')
end


local function main()
    local hosts = os.getenv('ZOOKEEPER') or '127.0.0.1:2181'
    local z = zookeeper.init(hosts)
    
    tap.test('test_op_on_not_connected', test_op_on_not_connected, z)
    tap.test('test_connection', test_connection, z)
    tap.test('test_create', test_create, z)
    tap.test('test_ensure_path', test_ensure_path, z)
    tap.test('test_exists', test_exists, z)
    tap.test('test_get', test_get, z)
    tap.test('test_delete', test_delete, z)
    tap.test('test_set', test_set, z)
    tap.test('test_get_children', test_get_children, z)
    tap.test('test_get_children2', test_get_children2, z)
    tap.test('test_get_acl', test_get_acl, z)
    tap.test('test_set_acl', test_set_acl, z)

    z:close()
end

main()
