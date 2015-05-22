local socket = require "levent.socket"
local dns    = require "levent.dns"

local util = {}
function util.create_connection(host, port, timeout)
    local ret, err = dns.resolve(host)
    if not ret then
        return nil, err
    end

    local ip = ret[1]
    local sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if timeout then
        sock:set_timeout(timeout)
    end
    local ok, err = sock:connect(ip, port)
    if not ok then
        sock:close()
        return nil, err
    end
    return sock
end

function util.listen(ip, port)
    local sock, err= socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if not sock then
        return nil, err
    end

    local ok, err = sock:bind(ip, port)
    if not ok then
        return nil, err
    end

    local ok, err = sock:listen()
    if not ok then
        return nil, err
    end
    return sock
end

return util

