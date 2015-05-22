local socketUtil = require "levent.socket_util"
local c          = require "levent.http.c"
local response   = require "levent.http.response"
local util       = require "levent.http.util"

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
        self.parser = c.new(c.HTTP_RESPONSE)
    end

    local msg, left, raw = util.read_message(self.conn, self.parser, self.cached)
    if not msg then
        self.conn:close()
        return nil, left
    end
    msg.raw_response = raw

    self.cached = left
    return response.new(msg)
end

function client:close()
    if self.conn then
        self.conn:close()
        self.conn = nil
    end
end

return client
