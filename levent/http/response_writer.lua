local config = require "levent.http.config"
local util = require "levent.http.util"

local encode_query_string = util.encode_query_string

local response_writer = {}
response_writer.__index = response_writer

local http_status_msg = {
    [100] = "Continue",
    [101] = "Switching Protocols",
    [200] = "OK",
    [201] = "Created",
    [202] = "Accepted",
    [203] = "Non-Authoritative Information",
    [204] = "No Content",
    [205] = "Reset Content",
    [206] = "Partial Content",
    [300] = "Multiple Choices",
    [301] = "Moved Permanently",
    [302] = "Found",
    [303] = "See Other",
    [304] = "Not Modified",
    [305] = "Use Proxy",
    [307] = "Temporary Redirect",
    [400] = "Bad Request",
    [401] = "Unauthorized",
    [402] = "Payment Required",
    [403] = "Forbidden",
    [404] = "Not Found",
    [405] = "Method Not Allowed",
    [406] = "Not Acceptable",
    [407] = "Proxy Authentication Required",
    [408] = "Request Time-out",
    [409] = "Conflict",
    [410] = "Gone",
    [411] = "Length Required",
    [412] = "Precondition Failed",
    [413] = "Request Entity Too Large",
    [414] = "Request-URI Too Large",
    [415] = "Unsupported Media Type",
    [416] = "Requested range not satisfiable",
    [417] = "Expectation Failed",
    [500] = "Internal Server Error",
    [501] = "Not Implemented",
    [502] = "Bad Gateway",
    [503] = "Service Unavailable",
    [504] = "Gateway Time-out",
    [505] = "HTTP Version not supported",
}

function response_writer.new()
    return setmetatable({headers = {}}, response_writer)
end

function response_writer:set_header(header, value)
    local field = util.canonical_header_key(header)
    self.headers[field] = value
end

function response_writer:set_headers(headers)
    if not headers then return end
    for k,v in pairs(headers) do
        local field = util.canonical_header_key(k)
        self.headers[field] = v
    end
end

function response_writer:set_code(code)
    assert(http_status_msg[code], code)
    self.code = code
end

function response_writer:set_data(data)
    self.data = data
end

function response_writer:pack()
    local code = self.code or 200
    local t = {}
    t[1] = string.format("%s/%s %d %s", config.HTTP_SCHEMA, config.HTTP_VERSION, code, http_status_msg[code])

    self.headers["Date"] = os.date("!%a, %d %b %Y %H:%M:%S GMT") -- rfc1123-date
    local cl = "Content-Type"
    if not self.headers[cl] then
        self.headers[cl] = "text/plain"
    end

    local payload
    if type(self.data) == "table" then
        payload = encode_query_string(self.data)
    else
        payload = self.data
    end
    local len = payload and #payload or 0

    local ct = "Content-Length"
    if not self.headers[ct] then
        self.headers[ct] = len
    end

    for k,v in pairs(self.headers) do
        t[#t+1] = string.format("%s: %s", k, tostring(v))
    end

    if len > 0 then
        t[#t + 1] = "\r\n" .. payload
    else
        t[#t + 1] = "\r\n"
    end
    return table.concat(t, "\r\n")
end

return response_writer

