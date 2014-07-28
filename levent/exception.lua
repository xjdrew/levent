local class = require "levent.class"

local BaseException = class("BaseException")
function BaseException:__tostring()
    return "BaseException"
end

local all = {}
all.BaseException = BaseException
return all
