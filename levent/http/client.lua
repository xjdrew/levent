local socketUtil = require "levent.socket_util"
local c          = require "levent.http.c"
local response   = require "levent.http.response"

local client = {}
client.__index = client

function client.new(host, port)
    local obj = {
        host = host,
        port = port,
    }
    return setmetatable(obj, client)
end

function client:send(request)
    if not self.conn then
        local conn, err = socketUtil.create_connection(self.host, self.port)
        if not conn then
            return false, err
        end
        self.conn = conn
    end

    local chunk = request:pack()
    local _, err =  self.conn:sendall(chunk)
    if err then
        self:close()
        return false, err
    end
    return true
end

function client:get_response()
    assert(self.conn, "not connected")
    if not self.parser then
        self.parser = c.new()
    end

    local ret = {}
    local raw = {}
    while not ret.complete do
        local parsed, s, err
        if self.cached then
            s = self.cached
        else
            s , err = self.conn:recv(4096)
            if not s then
                self:close()
                return nil, err
            end
        end

        parsed, err = self.parser:execute(s, nil, ret)
        if err then
            self:close()
            return nil, c.http_errno_name(err)
        end

        -- socket closed by peer
        if #s == 0 then
            break
        end

        if parsed < #s then
            table.insert(raw, s:sub(1, parsed))
            self.cached = s:sub(parsed+1)
        else
            table.insert(raw, s)
        end
    end

    ret.raw_response = table.concat(raw)
    return response.new(ret)
end

function client:close()
    if self.conn then
        self.conn:close()
        self.conn = nil
    end
end

return client
