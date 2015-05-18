local client  = require "levent.http.client"
local server  = require "levent.http.server"
local request = require "levent.http.request"
local config  = require "levent.http.config"

local http_methods = config.HTTP_METHODS

local function send_request(request)
    local cli = client.new(request.host, request.port)
    local ok, err = cli:send(request)
    if not ok then
        cli:close()
        return nil, err
    end
    local response, err = cli:get_response()
    if err then
        cli:close()
        return nil, err
    end
    cli:close()
    return response
end

local http = {}
function http.get(url, headers)
    local request, err = request.new(http_methods.GET, url, nil, headers)
    if not request then
        return nil, err
    end
    return send_request(request)
end

-- body could be nil, string, or k-v table
function http.post(url, body, headers)
    local request, err = request.new(http_methods.POST, url, body, headers)
    if not request then
        return nil, err
    end
    return send_request(request)
end

function http.client(ip, port)
    return client.new(ip, port)
end

function http.server(ip, port)
    if not ip then
        ip = "0.0.0.0"
    end
    return server.new(ip, port)
end

http.request = request.new
return http

