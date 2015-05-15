local config = require "levent.http.config"

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

function response_writer:add_header(header, value)
    self.headers[header:upper()] = value
end

function response_writer:add_headers(headers)
    if not headers then return end
    for k,v in pairs(headers) do
        self.headers[k:upper()] = v
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
    for k,v in pairs(self.headers) do
        table.insert(t, string.format("%s: %s", k, v))
    end

    table.insert(t, string.format("Date: %s", os.date("!%a, %d %b %Y %H:%M:%S GMT"))) -- rfc1123-date
    local cl = "CONTENT-TYPE"
    if not self.headers[cl] then
        table.insert(t, string.format("%s: %s", cl, "text/plain"))
    end

    local len = self.data and #self.data or 0
    local ct = "CONTENT-LENGTH"
    if len > 0 and not self.headers[ct] then
        table.insert(t, string.format("%s: %d", ct, len))
    end
    if len > 0 then
        table.insert(t, "\r\n" .. self.data)
    else
        table.insert(t, "\r\n")
    end
    return table.concat(t, "\r\n")
end

return response_writer

