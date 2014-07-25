local class = require "levent.class"
local loop  = require "levent.loop"
local c     = require "socket.c"

local Socket = class("Socket")
function Socket:_init(family, type, protocol)
    self.cobj = c.socket(family, type, protocol)
    self.cobj:setblocking(0)
    self._read_event = loop:io(self.cobj:fileno(), loop.EV_READ)
    self._read_event = loop:io(self.cobj:fileno(), loop.EV_WRITE)

    -- never timeout
    self.timeout = nil
end

function Socket:connect(ip, port)
end

local socket = {}
for k,v in pairs(c) do
    if type(v) ~= "function" then
        socket[k] = v
    end
end
function socket.socket(family, type, protocol)
    return Socket:new(family, type, protocol)
end

return socket

