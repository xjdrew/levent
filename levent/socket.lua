--[[
-- author: xjdrew
-- date: 2014-07-17
--]]

local c       = require "levent.socket.c"
local errno   = require "levent.errno.c"

local levent  = require "levent.levent"
local class   = require "levent.class"
local timeout = require "levent.timeout"

local closed_socket = setmetatable({}, {__index = function(t, key)
    if key == "send" or key == "recv" or key == "recvex" or key=="sendto" or key == "recvfrom" or key == "accept" then
        return function(...)
            return nil, errno.EBADF
        end
    end
end})

local function _wait(watcher, sec)
    local t
    if sec and sec > 0 then
        t = timeout.start_new(sec)
    end

    local hub = levent.get_hub()
    local ok, excepiton = xpcall(hub.wait, debug.traceback, hub, watcher)
    if not ok then
        print(ok, excepiton)
    end
    if t then
        t:cancel()
    end
    return ok, excepiton
end

local Socket = class("Socket")

function Socket:_init(cobj)
    assert(type(cobj.fileno) == "function", cobj)
    self.cobj = cobj
    self.cobj:setblocking(false)
    local hub = levent.get_hub()
    local loop = hub.loop
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
    self.cobj:setsockopt(c.SOL_SOCKET, c.SO_REUSEADDR, 1)
    local ok, code = self.cobj:bind(ip, port)
    if not ok then
        return false, errno.strerror(code)
    end
    return true
end

function Socket:listen(backlog)
    local ok, code = self.cobj:listen(backlog)
    if not ok then
        return false, errno.strerror(code)
    end
    return true
end

function Socket:_need_block(err) 
    if self.timeout == 0 then
        return false
    end

    if err ~= errno.EWOULDBLOCK and err ~= errno.EAGAIN then
        return false
    end

    return true
end

function Socket:accept() 
    local csock, err
    while true do
        csock, err = self.cobj:accept()
        if csock then
            break
        end

        if not self:_need_block(err) then
            return nil, errno.strerror(err)
        end

        local ok, exception = _wait(self._read_event, self.timeout)
        if not ok then
            return nil, exception
        end
    end
    return Socket.new(csock)
end

function Socket:_recv(func, ...)
    local cobj = self.cobj
    while true do
        local data, err = func(cobj, ...)
        if data then
            return data
        end

        if not self:_need_block(err) then
            return nil, err
        end

        local ok, exception = _wait(self._read_event, self.timeout)
        if not ok then
            return nil, exception
        end
    end
end

-- args:len
function Socket:recvfrom(len)
    return self:_recv(self.cobj.recvfrom, len)
end

-- args: len
function Socket:recv(len)
    local msg
    local cobj = self.cobj
    while true do
        msg = cobj:recv_pop(len)
        if msg then
            return msg
        end
        local total, err = cobj:recv_push()
        if total == 0 then
            return ""
        end
        if total and total >= len then
            return cobj:recv_pop(len)
        end

        if not total and not self:_need_block(err) then
            return nil, err
        end

        local ok, exception = _wait(self._read_event, self.timeout)
        if not ok then
            return nil, exception
        end
    end
end

function Socket:_send(func, ...)
    local cobj = self.cobj
    while true do
        local nwrite, err = func(cobj, ...)
        if nwrite then
            return nwrite
        end

        if not self:_need_block(err) then
            return nil, err
        end
        local ok, exception = _wait(self._write_event, self.timeout)
        if not ok then
            return nil, exception
        end
    end
end

-- args: data, from
-- from: count from 0
function Socket:send(data, from)
    return self:_send(self.cobj.send, data, from)
end

-- args: 
function Socket:sendto(ip, port, data, from)
    return self:_send(self.cobj.sendto, ip, port, data, from)
end

-- from: count from 0
function Socket:sendall(data, from)
    local from = from or 0
    local total = #data - from
    local sent = 0
    while sent < total do
        local nwrite, err = self:send(data, from + sent)
        if not nwrite then
            return sent, err
        end
        sent = sent + nwrite
    end
    return sent
end

function Socket:connect(ip, port)
    while true do
        local ok, err = self.cobj:connect(ip, port)
        if ok then
            return ok
        end
        if err == errno.EISCONN then
            return true
        end
        if self.timeout == 0.0 or (err ~= errno.EINPROGRESS and err ~= errno.EWOULDBLOCK and err ~= errno.EALREADY) then
            return ok, err
        end
        local ok, exception = _wait(self._write_event, self.timeout)
        if not ok then
            return nil, exception
        end
    end
end

function Socket:fileno()
    return self.cobj:fileno()
end

function Socket:getpeername()
    return self.cobj:getpeername()
end

function Socket:getsockname()
    return self.cobj:getsockname()
end

function Socket:getsockopt(level, optname, buflen)
    return self.cobj:getsockopt(level, optname, buflen)
end

function Socket:setsockopt(level, optname, value)
    return self.cobj:setsockopt(level, optname, value)
end

function Socket:close()
    local hub = levent.get_hub()
    if self.cobj ~= closed_socket then
        hub:cancel_wait(self._read_event)
        hub:cancel_wait(self._write_event)
        self.cobj:close()
        self.cobj = closed_socket
    end
end

local socket = {}
function socket.socket(family, _type, protocol)
    local cobj, err = c.socket(family, _type, protocol)
    if not cobj then
        return nil, errno.strerror(err)
    end
    return Socket.new(cobj)
end

return setmetatable(socket, {__index = c} )

