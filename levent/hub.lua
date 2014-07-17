local Class = require "levent.class"

local Hub = Class()
function Hub:_init()
end

local Waiter = Class()
function Waiter:_init()
    self.co = nil
    self.value = nil
end

local M = {}
M.Hub = Hub
M.Waiter = Waiter
return M

