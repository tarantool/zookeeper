local fiber = require 'fiber'
local fio = require 'fio'
local msgpack = require 'msgpack'

local driver = require 'zookeeper.driver'
local zookeeper_acl = require 'zookeeper.acl'
local const = require 'zookeeper.const'
local NULL = msgpack.NULL


local zookeeper_methods

local function zookeeper_new(handle, hosts, timeout, default_acl)
    if default_acl == nil then
        default_acl = zookeeper_acl.ACLS.OPEN_ACL_UNSAFE
    end
    
    return setmetatable({
        hosts = hosts,
        timeout = timeout,
        default_acl = default_acl,
        
        _handle = handle,
        _f = NULL
    }, {
        __index = zookeeper_methods,
        __gc = function(self)
            self:close()
        end,
    })
end


local function _split_parent_path(path)
    local last_char = string.sub(path, #path, #path)
    if last_char == '/' then
        path = string.sub(path, 0, #path - 1)
    end
    local parent, rest = fio.dirname(path), fio.basename(path)
    if parent == '/' then
        return nil, path
    end
    return parent, rest
end


zookeeper_methods = {
    start = function(self)
        if self._f ~= nil and self._f:status() ~= 'dead' then
            error('zookeeper is already started')
        end
        
        self._f = fiber.create(function()
            fiber.self():name('zookeeper_process')
            driver.process(self._handle)
        end)
    end,
    
    close = function(self)
        if self._f ~= nil and self._f:status() ~= 'dead' then
            self._f:cancel()
        end
        self._f = NULL
        driver.close(self._handle)
    end,
    
    set_watcher = function(self, watcher_func, context)
        driver.set_watcher(self._handle, watcher_func, self, context)
    end,
    
    client_id = function(self)
        return driver.client_id(self._handle)
    end,
    
    add_auth = function(self, scheme, cert)
        return driver.add_auth(self._handle, scheme, cert)
    end,
    
    state = function(self)
        return driver.state(self._handle)
    end,
    
    is_connected = function(self)
        local ok, s = pcall(self.state, self)
        if ok then
            return s == const.states.CONNECTED
        end
        
        return false
    end,
    
    wait_connected = function(self, timeout)
        return driver.wait_connected(self._handle, timeout)
    end,
    
    create = function(self, path, value, acl, flags)
        if acl == nil then
            acl = self.default_acl
        else
            if not zookeeper_acl.ACLList.check_acl(acl) then
                error("acl must be a zookeeper.acl.ACLList instance")
            end
        end
        
        return driver.create(self._handle, path, value, acl, flags)
    end,
    
    ensure_path = function(self, path)
        local exists, _, rc = self:exists(path)
        if exists then
            return const.ZOK
        elseif rc ~= const.ZOK and rc ~= const.api_errors.ZNONODE then
            return rc
        end
        
        local parent, last_node = _split_parent_path(path)
        if parent ~= nil then
            local rc = self:ensure_path(parent)
            if rc ~= const.ZOK then
                return rc
            end
        end
        local _, rc = self:create(path)
        return rc
    end,
    
    exists = function(self, path, watch)
        return driver.exists(self._handle, path, watch)
    end,
    
    delete = function(self, path, version)
        return driver.delete(self._handle, path, version)
    end,
    
    get = function(self, path, watch)
        return driver.get(self._handle, path, watch)
    end,
    
    set = function(self, path, value, version)
        return driver.set(self._handle, path, value, version)
    end,
    
    get_children = function(self, path, watch)
        return driver.get_children(self._handle, path, watch)
    end,
    
    get_children2 = function(self, path, watch)
        return driver.get_children2(self._handle, path, watch)
    end,
    
    sync = function(self, path)
        return driver.sync(self._handle, path)
    end,
    
    wexists = function(self, path, watcher_func, context)
        return driver.wexists(self._handle, path, watcher_func, self, context)
    end,
    
    wget = function(self, path, watcher_func, context)
        return driver.wget(self._handle, path, watcher_func, self, context)
    end,
    
    wget_children = function(self, path, watcher_func, context)
        return driver.wget_children(self._handle,
            path, watcher_func, self, context)
    end,
    
    wget_children2 = function(self, path, watcher_func, context)
        return driver.wget_children2(self._handle,
            path, watcher_func, self, context)
    end,
    
    get_acl = function(self, path)
        return driver.get_acl(self._handle, path)
    end,
    
    set_acl = function(self, path, acl, version)
        return driver.set_acl(self._handle, path, version, acl)
    end,
}


return {
    init = function(hosts, timeout, opts)
        if hosts == nil then
            -- default host:port
            hosts = '127.0.0.1:2181'
        end
        
        timeout = tonumber(timeout)
        if timeout == nil then
            timeout = 1 * 24 * 60 * 60 * 1000 -- 30000
        end
        
        if opts == nil then
            opts = {}
        end
        
        local handle = driver.init(hosts, timeout,
                                   opts.clientid,
                                   opts.flags,
                                   opts.reconnect_timeout)
        return zookeeper_new(handle, hosts, timeout, opts.default_acl)
    end,
    zerror = driver.zerror,
    deterministic_conn_order = driver.deterministic_conn_order,
    set_log_level = driver.set_log_level,
    const = const,
}
