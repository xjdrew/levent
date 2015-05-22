local c      = require "levent.http.c"
local config = require "levent.http.config"
local util   = require "levent.http.util"

local methods = config.HTTP_METHODS
local encode_query_string = util.encode_query_string

local request = {}
request.__index = request
function request.new(method, url, data, headers)
    local parsed = c.parse_url(url)
    if not parsed then
        return nil, "invalid url:"..url
    end

    if not parsed.host then
        return nil, "no host given"
    end

    assert(parsed.schema:upper() == config.HTTP_SCHEMA, "only support http protocal")

    local obj = {}
    obj.host = parsed.host
    obj.port = parsed.port or config.HTTP_PORT
    obj.method = method
    obj.path = parsed.request_path
    obj.query_string = parsed.query_string
    obj.data = data
    obj.headers = {
        ["Host"] = host,
        ["User-Agent"] = config.HTTP_AGENT,
        ["Accept"] = "*/*",
    }

    local req = setmetatable(obj, request)
    req:add_headers(headers)
    return req
end

function request:set_method(method)
    assert(methods[method], method)
    self.method = method
end

function request:get_method(method)
    if self.method then
        return self.method
    end

    if self.data then
        return methods.POST
    end

    return methods.GET
end

-- request path
function request:set_path(path)
    self.path = path
end

function request:set_query_string(query_string)
    self.query_string = query_string
end

-- request data if post, type(data) == "string"
function request:set_data(data)
    if not data then return end
    assert(type(data) == "string", type(data))
    self.data = data
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

    local payload
    if type(self.data) == "table" then
        payload = encode_query_string(self.data)
    else
        payload = self.data
    end

    local len = payload and #payload or 0
    local ct = "Content-Length"
    if len > 0 and not self.headers[ct] then
        self.headers[ct] = len
    end

    local t = {}
    t[1] = string.format("%s %s %s/%s", method, path, config.HTTP_SCHEMA, config.HTTP_VERSION)
    for k,v in pairs(self.headers) do
        t[#t + 1] = string.format("%s: %s", k, v)
    end

    if len > 0 then
        t[#t + 1] = "\r\n" .. payload
    else
        t[#t + 1] = "\r\n"
    end
    return table.concat(t, "\r\n")
end

return request

