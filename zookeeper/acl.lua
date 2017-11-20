local msgpack = require 'msgpack'

local driver = require 'zookeeper.driver'
local const = require 'zookeeper.const'
local NULL = msgpack.NULL

local permissions = const.permissions
local ACLList = setmetatable({}, {
    __call = function(_, acls)
        if type(acls) ~= 'table' then
            error('acls must be a table')
        end
        
        for _, acl in ipairs(acls) do
            acl.perms = tonumber(acl.perms)
            if type(acl.perms) ~= 'number' or acl.perms < 1 or acl.perms > 31 then
                error('perms must be a valid integer')
            end
            
            if acl.scheme == nil or type(acl.scheme) ~= 'string' then
                error('scheme must be a string')
            end
            
            if acl.id == nil or type(acl.id) ~= 'string' then
                error('id must be a string')
            end
        end
        
        return driver.build_acl_list(acls)
    end,
})

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

ACLList.check_acl = function(obj)
    return getmetatable(obj) == getmetatable(ACLS.OPEN_ACL_UNSAFE)
end

return {
    ACLList = ACLList,
    ACLS = ACLS,
    permissions = permissions,
}
