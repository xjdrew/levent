local config   = require "levent.http.config"
local httpUtil = require "levent.http.util"

local http_methods = config.HTTP_METHODS

local request = {}
request.__index = request
function request.new(host, port)
    local obj = {}
    obj.host = host
    obj.port = port or config.HTTP_PORT
    obj.headers = {
        ["HOST"] = host,
        ["USER-AGENT"] = config.HTTP_AGENT,
        ["ACCEPT"] = "*/*",
    }
    return setmetatable(obj, request)
end

function request:set_method(method)
    assert(http_methods[method], method)
    self.method = method
end

function request:get_method(method)
    if self.method then
        return self.method
    end

    if self.data then
        return http_methods.POST
    end

    return http_methods.GET
end

-- request path
function request:set_path(path)
    self.path = path
end

function request:set_query_string(query_string)
    self.query_string = query_string
end

-- request payload if post, type(payload) == "string"
function request:set_payload(payload)
    if not payload then return end
    assert(type(payload) == "string", type(payload))
    self.payload = payload
end

function request:add_headers(headers)
    if not headers then return end
    for k,v in pairs(headers) do
        local field = util.canonical_header_key(k)
        self.headers[field] = v
    end
end

-- gen request data
function request:pack()
    local method = self:get_method()
    local path = self.path or config.HTTP_ABS_PATH

    if self.query_string then
        path = string.format("%s?%s", path, self.query_string)
    end

    local len = self.payload and #self.payload or 0
    local ct = "Content-Length"
    if len > 0 and not self.headers[ct] then
        self.headers[ct] = len
    end

    local t = {}
    t[1] = string.format("%s %s %s/%s", method, path, config.HTTP_SCHEMA, config.HTTP_VERSION)
    for k,v in pairs(self.headers) do
        t[#t + 1] = string.format("%s: %s", k, v)
    end

    if self.payload then
        t[#t + 1] = self.payload
    end
    t[#t + 1] = "\r\n"
    return table.concat(t, "\r\n")
end

return request

