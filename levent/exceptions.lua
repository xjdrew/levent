local class = require "levent.class"

local BaseException = class("BaseException")
function BaseException:__tostring()
    return "BaseException"
end

local all = {}
all.BaseException = BaseException
function all.is_exception(exception)
    return class.isinstance(exception, BaseException)
end
return all

