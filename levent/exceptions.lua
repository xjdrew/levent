local class = require "levent.class"

local BaseException = class("BaseException")
function BaseException:__tostring()
    return "BaseException"
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
all.DnsError = DnsError
function all.is_exception(exception)
    return class.isinstance(exception, BaseException)
end
return all

