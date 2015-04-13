local response = {}
response.__index = response

function response.new(parsed)
    return setmetatable(parsed, response)
end

function response:get_status()
    return self.status
end

function response:get_code()
    return self.status_code
end

function response:get_headers()
    return self.headers
end

function response:get_payload()
    return self.body
end

function response:get_version()
    return self.http_major, self.http_minor
end

function response:get_raw()
    return self.raw_response
end

return response
