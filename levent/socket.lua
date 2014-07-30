local class = require "levent.class"
local c     = require "socket.c"
local errno = require "errno.c"
local hub   = require "levent.hub"

local function _need_block(err)
    if err == errno.EWOULDBLOCK or err == errno.EAGAIN then
        return true
    end
    return false
end

local function _wait(watcher, timeout)
    local timer
    if timeout and timeout > 0 then
        timer = 
    end
end

local Socket = class("Socket")

function Socket:_init(family, type, protocol, cobj)
    if sock then
        assert(type(cobj.fileno) == "function", cobj)
        self.cobj = cobj
    else
        self.cobj = c.socket(family, type, protocol)
    end

    self.cobj:setblocking(0)
    self._read_event = loop:io(self.cobj:fileno(), loop.EV_READ)
    self._write_event = loop:io(self.cobj:fileno(), loop.EV_WRITE)
    -- timeout
    self.timeout = nil
end

function Socket:setblocking(flag)
    if flag then
        self.timeout = nil
    else
        self.timeout = 0.0
    end
end

function Socket:set_timeout(sec)
    self.timeout = sec
end

function Socket:get_timeout()
    return self.timeout
end

function Socket:bind(ip, port)
    return self.cobj:bind(ip, port)
end

function Socket:listen(backlog)
    return self.cobj:listen(backlog)
end

function Socket:accept() 
    local cobj = self.cobj
    local csock, err
    while true do
        csock, err = cobj:accept()
        if csock then
            break
        end
        if self.timeout == 0.0 or not _need_block(err) then
            break
        end
        _wait(self._read_event, self.timeout)
    end
    if csock then
        return Socket:new(nil, nil, nil, csock)
    else
        return nil, err
    end
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

