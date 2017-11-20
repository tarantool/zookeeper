local driver = require 'zookeeper.driver'

local fields = {
    'watch_types',
    'states',
    'log_level',
    'errors',
    'api_errors',
    'create_flags',
    'permissions'
}

local M = {
    ZOK = driver.errors.ZOK
}

local function make_rev_map(m)
    local rev_m = {}
    for k, v in pairs(m) do
        rev_m[v] = k
    end
    return rev_m
end

for _, f in ipairs(fields) do
    assert(driver[f] ~= nil,
           string.format("Missing '%s' const field from driver", f))
    
    M[f] = driver[f]
    M[f..'_rev'] = make_rev_map(M[f])
end

return M
