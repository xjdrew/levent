--[[
-- author: xjdrew
-- date: 2014-08-14
--]]

local levent    = require "levent.levent"
local class     = require "levent.class"

local Signal = class("Signal")

function Signal:_init(signum, handler, ...)
    local hub = levent.get_hub()
    self.watcher = hub.loop:signal(signum)
    self.watcher:start(handler, ...)
end

function Signal:unref()
    self.watcher:unref()
end

function Signal:cancel()
    self.watcher:stop()
end

local signal = {}
signal.signal = Signal.new

-- signum
signal.SIGHUP = 1
signal.SIGINT = 2
signal.SIGPIPE = 9
signal.SIGTERM = 15
return signal

