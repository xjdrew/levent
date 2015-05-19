local c              = require "levent.http.c"
local levent         = require "levent.levent"
local httpUtil       = require "levent.http.util"
local socketUtil     = require "levent.socket_util"
local responseWriter = require "levent.http.response_writer"
local requestReader  = require "levent.http.request_reader"

local server = {}
server.__index = server

function server.new(ip, port)
    local obj = {
        ip = ip,
        port = port,
        handlers = {}
    }
    return setmetatable(obj, server)
end

function server:register(handlers)
    for k, v in pairs(handlers) do
        assert(not self.handlers[k], "repeated register:" .. k)
        self.handlers[k] = v
    end
end

function server:handle_one_request(msg)
    local path = msg.request_path
    local rsp = responseWriter.new()
    local handler = self.handlers[path]
    if not handler then
        rsp:set_code(404)
    else
        req = requestReader.new(msg)
        local ok, err = xpcall(handler, debug.traceback, rsp, req)
        if not ok then
            print(string.format("handle request[%s] failed: %s", path, err))
            rsp:set_code(502)
            rsp:set_data(err)
        end
    end

    return rsp
end

function server:handle_conn(conn)
    local host, port = conn:getpeername()
    print("new connection:", host, port)
    local parser = c.new(c.HTTP_REQUEST)
    local cached
    while true do
        local msg, left = httpUtil.read_message(conn, parser, cached)
        if not msg then
            print("read request failed:", left)
            break
        end

        -- connection close
        if not msg.complete then
            break
        end
        local url = c.parse_url(msg.request_url)
        if not url then
            break
        end

        -- url info
        msg.host = url.host
        msg.port = url.port
        msg.request_path = url.request_path
        msg.query_string = url.query_string
        msg.userinfo = url.userinfo

        -- remote info
        msg.remote_host = host
        msg.remote_port = port

        local rsp = self:handle_one_request(msg)
        if not rsp then
            break
        end

        conn:sendall(rsp:pack())

        if not msg.keepalive then
            break
        end

        cached = left
    end
    conn:close()
    print("connection close:", host, port)
end

function server:serve()
    local ln, err = socketUtil.listen(self.ip, self.port)
    if not ln then
        return false, err
    end

    self.ln = ln
    while self.ln do
        local conn, err = ln:accept()
        if conn then
            levent.spawn(self.handle_conn, self, conn)
        end

        if err then
            print("accept error:", err)
        end
    end
    return true
end

function server:close()
    if self.ln then
        local ln = self.ln
        self.ln = nil
        ln:close()
    end
end

return server

