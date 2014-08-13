--[[
-- author: xjdrew
-- date: 2014-08-13
--]]
local hub   = require "levent.hub"
local class = require "levent.class"
local lock  = require "levent.lock"

local function get_fileno(obj)
    local fileno
    if type(obj) == "number" then
        fileno = obj
    else 
        local ok, val = pcall(obj.fileno, obj)
        if ok then
            fileno = val
        end
    end
    if type(fileno) == "number" then
        return true, fileno
    end
    return false, val
end

local SelectResult = class("SelectResult")
function SelectResult:_init()
    self.read = {}
    self.write = {}
    self.event = lock.event()
end

function SelectResult:add_read(socket)
    self.read[#self.read + 1] = socket
    self.event:set()
end

function SelectResult:add_write(socket)
    self.write[#self.write + 1] = socket
    self.event:set()
end

local function list_to_tbl(list, tbl)
    if not list then
        return true
    end

    local ok, fd
    for _, obj in ipairs(list) do
        ok, fd = get_fileno(obj)
        if not ok then
            return false, tostring(obj)
        end
        tbl[fd] = obj
    end
    return true
end

local function select_(rlist, wlist, xlist, sec)
    local watchers = {}
    local loop = hub.loop
    local MAXPRI = loop.EV_MAXPRI
    local result = SelectResult.new()

    local rtbl = {}
    local wtbl = {}
    assert(list_to_tbl(rlist, rtbl))
    assert(list_to_tbl(wlist, wtbl))

    for fd, obj in pairs(rtbl) do
        local watcher = loop:io(fd, loop.EV_READ)
        watcher:set_priority(MAXPRI)
        watcher:start(result.add_read, result, obj)
        watchers[#watchers + 1] = watcher
    end

    for fd, obj in pairs(wtbl) do
        local watcher = loop:io(fd, loop.EV_WRITE)
        watcher:set_priority(MAXPRI)
        watcher:start(result.add_write, result, obj)
        watchers[#watchers + 1] = watcher
    end
    result.event:wait(sec)

    for i=1, #watchers do
        watchers[i]:stop()
    end

    return result.read, result.write
end

local m = {}
m.select_ = select_
return m

