--[[
-- author: xjdrew
-- date: 2014-07-17
--]]
local c = require "levent.c"

local class = {}
setmetatable(class, class)

function class.issubclass(cls1, cls2)
    local tmp = cls1
    while tmp do
        if tmp == cls2 then
            return true
        end
        tmp = cls1.__base
    end
    return false
end

function class.isinstance(obj, cls)
    if type(obj) ~= "table" then
        return false
    end

    local tmp = getmetatable(obj)
    while tmp do
        if tmp == cls then
            return true
        end
        tmp = getmetatable(tmp)
    end
    return false
end

local function _get_init(cls)
    local _init
    while cls do
        _init = rawget(cls, "_init")
        if _init then
            break
        end
        cls = rawget(cls, "__base")
    end
    return _init
end

local function _class(name, base)
    local cls = {}
    cls.__index = cls
    cls.__name = name

    if type(base) == "table" then
        setmetatable(cls, base)
        cls.__base = base
    end

    function cls.new(...)
        local obj = {}
        setmetatable(obj, cls)
        local _init = _get_init(cls)
        if _init then
            _init(obj, ...)
        end
        return obj
    end

    function cls:__tostring()
        return string.format("%s: 0x%x", (cls.__name or "object"), c.topointer(self))
    end

    function cls:super()
        return cls.__base
    end
    return cls
end

class.__call = function(fun, name, base) 
    return _class(name, base)
end

return class

