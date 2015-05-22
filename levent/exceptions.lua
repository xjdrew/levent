local class = require "levent.class"

local BaseException = class("BaseException")
function BaseException:__tostring()
    return "BaseException"
end

local CancelWaitError = class("CancelWaitError", BaseException)
function CancelWaitError:_init(msg)
    self.msg = msg
end

function CancelWaitError:__tostring()
    if self.msg then
        return "CancelWaitError:" .. self.msg
    end
    return "CancelWaitError"
end

local DnsError = class("DnsError", BaseException)
function DnsError:_init(info)
    self.info = info
end

function DnsError:__tostring()
    if self.info then
        return "DnsError:" .. self.info
    end
    return "DnsError"
end

local all = {}
all.BaseException = BaseException
all.CancelWaitError = CancelWaitError 
all.DnsError = DnsError
function all.is_exception(exception)
    return class.isinstance(exception, BaseException)
end
return all

