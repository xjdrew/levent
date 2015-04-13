local c       = require "levent.http.c"
local client  = require "levent.http.client"
local request = require "levent.http.request"
local config  = require "levent.http.config"

local http_methods = config.HTTP_METHODS

local function create_request(method, url, data, headers)
    local parsed = c.parse_url(url)
    if not parsed then
        return nil, "invalid url"
    end

    assert(parsed.schema:upper() == config.HTTP_SCHEMA, "only support http protocal")
    if not parsed.host then
        return nil, "no host given"
    end

    local req = request.new(parsed.host, parsed.port)
    req:set_method(method)
    req:set_path(parsed.request_path)
    req:set_query_string(parsed.query_string)
    req:set_payload(data)
    req:add_headers(headers)
    return req
end

local function send_request(request)
    local cli = client.new(request.host, request.port)
    local ok, err = cli:send(request)
    if not ok then
        return nil, err
    end
    local response, err = cli:get_response()
    if err then
        return nil, err
    end
    cli:close()
    return response
end

local http = {}
function http.get(url, headers)
    local request, err = create_request(http_methods.GET, url, nil, headers)
    if not request then
        return nil, err
    end
    return send_request(request)
end

function http.post(url, body, headers)
    local request, err = create_request(http_methods.POST, url, body, headers)
    if not request then
        return nil, err
    end
    return send_request(request)
end
return http

