local c = require "levent.http.c"

local HTTP_PORT = 80
local HTTPS_PORT = 443

local function request(cmd, url, body, headers)
    local parsed = c.parse_url(url)
    if not parsed then
        return nil, "invalid url"
    end

    if not parsed.host then
        return nil, "no host given"
    end
end

local http = {}
function http.get(url, headers)
end

function http.post(url, body, headers)
end
return http

