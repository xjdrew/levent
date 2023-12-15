local crypto    = require "crypto"
local levent    = require "levent.levent"
local util      = require "levent.socket_util"
local client    = require "levent.http.client"
local config    = require "levent.http.config"
local request   = require "levent.http.request"
local server    = require "levent.http.server"

local assert        = assert
local pcall         = pcall
local rawset        = rawset
local select        = select
local setmetatable  = setmetatable
local tonumber      = tonumber
local mrandom       = math.random
local schar         = string.char
local sformat       = string.format
local sgsub         = string.gsub
local spack         = string.pack
local ssub          = string.sub
local sunpack       = string.unpack
local tconcat       = table.concat

local http_methods = config.HTTP_METHODS
local codes = config.WebSocket.CODES

local CONTINUATION  = 0x0 
local TEXT          = 0x1
local BINARY        = 0x2
local CLOSE         = 0x8
local PING          = 0x9
local PONG          = 0xA

local MAX_FRAME_BINLEN = 16
local MAX_FRAME_SIZE = 1 << MAX_FRAME_BINLEN
local MAX_BUFFER_LENGTH = 1 << 8

local registrable = {
    on_message  = true,
    on_ping     = true,
    on_pong     = true,
    on_close    = true,
    on_error    = true,
}

local sha1 = crypto.digest.new("sha1")
local function get_sha1(...)
    assert(..., "Nil input")
    sha1:reset()
    for i = 1, select("#", ...) do
        sha1:update(select(i, ...))
    end
    return sgsub(sha1:final(), "%x%x", function(hex)
        return schar(tonumber(hex, 0x10))
    end)
end

local function push_string(t, s, n)
    if n < MAX_BUFFER_LENGTH then
        n = n + 1
    else
        t[1] = tconcat(t, "", 1, n)
        n = 2
    end
    t[n] = s
    return n
end

local function apply_mask(str, mask)
    return sgsub(str, "..?.?.?", function(part)
        local n = #part
        local m = mask >> ((4 - n) << 3)
        local fmt = sformat(">I%d", n)
        return spack(fmt, sunpack(fmt, part) ~ m)
    end)
end

local function get_payload(conn, length)
    local payload_buffer = {}
    local stage = length >> MAX_FRAME_BINLEN
    if stage > 0 then
        local n = 0
        for _ = 1, stage do
            n = push_string(payload_buffer, util.read_full(conn, MAX_FRAME_SIZE), n)
        end
        length = length & (MAX_FRAME_SIZE - 1)
        if length > 0 then
            n = push_string(payload_buffer, util.read_full(conn, length), n)
        end
        return tconcat(payload_buffer, "", 1, n)
    end
    return util.read_full(conn, length)
end

local function read(conn)
    local fin_op, mask_len = sunpack(">I1I1", util.read_full(conn, 2))
    local fin       = fin_op & 0x80 ~= 0
    local opcode    = fin_op & 0x0F
    local mask      = mask_len & 0x80 ~= 0
    local length    = mask_len & 0x7F
    if length == 126 then
        length = sunpack(">I2", util.read_full(conn, 2))
    elseif length == 127 then
        length = sunpack(">I8", util.read_full(conn, 8))
    end
    local payload
    if mask then
        mask = sunpack(">I4", util.read_full(conn, 4))
        payload = apply_mask(get_payload(conn, length), mask)
    else
        payload = get_payload(conn, length)
    end

    return fin, opcode, payload
end

local function write(conn, opcode, payload, mask, not_fin)
    local length = #payload
    if length > MAX_BUFFER_LENGTH and opcode ~= CLOSE and opcode ~= PING and opcode ~= PONG then
        for l = 1, length, MAX_BUFFER_LENGTH do
            local r = l + MAX_BUFFER_LENGTH - 1
            write(conn, l == 1 and opcode or 0x0, ssub(payload, l, r), mask, r < length)
        end
        return
    end
    local prefix = 
        (not_fin and 0x0000 or 0x8000) |    -- fin
        (opcode << 8) |                     -- opcode
        (mask and 0x0080 or 0x0000)         -- mask
    if length < 126 then
        prefix = spack(">I2", prefix | length)
    elseif length < (1 << 16) then
        prefix = spack(">I2I2", prefix | 126, length)
    else
        prefix = spack(">I2I8", prefix | 127, length)
    end
    if mask then
        mask = mrandom(0, 0xFFFFFFFF)
        payload = apply_mask(payload, mask)
        mask = spack(">I4", mask)
    else
        mask = ""
    end
    conn:sendall(sformat("%s%s%s", prefix, mask, payload))
end

local function encode_close_msg(code, reason)
    local fmt = reason and sformat(">I2c%d", #reason) or ">I2"
    return spack(fmt, code, reason)
end

local function decode_close_msg(payload)
    local length = #payload
    if length == 0 then
        return
    end
    local fmt = length > 2 and sformat(">I2c%d", length - 2) or sformat(">I%d", length)
    return sunpack(fmt, payload)
end

local ws = {}
ws.__index = ws

local function try_handle(self, method, ...)
    return self[method] and self[method](self, ...)
end

local function run(self)
    local conn = self.conn
    local t, n, first_opcode = {}, 0
    while true do
        if self.conn == nil then
            return
        end
        local fin, opcode, payload = read(conn)
        if opcode == CLOSE then
            try_handle(self, "on_close", decode_close_msg(payload))
            self:close(codes.CLOSE_GOING_AWAY)
        elseif opcode == PING then
            try_handle(self, "on_ping", payload)
            self:pong(payload)
        elseif opcode == PONG then
            try_handle(self, "on_pong", payload)
        else
            if opcode == CONTINUATION then
                if not first_opcode then
                    return self:error(codes.CLOSE_PROTOCOL_ERROR)
                end
            else
                if first_opcode then
                    return self:error(codes.CLOSE_PROTOCOL_ERROR)
                end
                first_opcode = opcode
                n = 0
            end
            n = push_string(t, payload, n)
            if fin then
                local message = tconcat(t, "", 1, n)
                try_handle(self, "on_message", first_opcode, message)
                first_opcode = nil
            end
        end
    end
end

function ws:__newindex(k, v)
    assert(registrable[k], k)
    rawset(self, k, v)
end

function ws:send(opcode, payload)
    write(self.conn, opcode, payload, self.mask)
end

function ws:text(message)
    self:send(TEXT, message)
end

function ws:binary(binstr)
    self:send(BINARY, binstr)
end

function ws:close(code, reason)
    code = code or codes.CLOSE_NORMAL
    local close_msg = encode_close_msg(code, reason)
    self:send(CLOSE, close_msg)
    self.conn:close()
    self.conn = nil
end

function ws:ping(payload)
    self:send(PING, payload)
end

function ws:pong(payload)
    self:send(PONG, payload)
end

function ws:error(...)
    try_handle(self, "on_error", ...)
    self:close(...)
end

function ws.client(url)
    local key = crypto.base64(crypto.rand.bytes(16))
    local request_headers = {
        Connection = "upgrade",
        Upgrade = "websocket",
        ["Sec-WebSocket-Key"] = key,
        ["Sec-WebSocket-Version"] = config.WebSocket.VERSION,
    }
    local request, err = request.new(http_methods.GET, url, nil, request_headers)
    assert(request, err)
    local cli = client.new(request.host, request.port)
    local ok, err = cli:send(request)
    assert(ok, err)
    local response, err = cli:get_response()
    assert(response, err)
    local response_code = response:get_code()
    assert(response_code == 101, response_code)
    local response_headers = response:get_headers()
    assert(response_headers.Connection and response_headers.Connection:lower() == "upgrade", response_headers.Connection)
    assert(response_headers.Upgrade and response_headers.Upgrade:lower() == "websocket", response_headers.Upgrade)
    local accept = response_headers["Sec-Websocket-Accept"]
    assert(accept and accept == crypto.base64(get_sha1(key, config.WebSocket.GUID)))
    local obj = setmetatable({
        conn = cli.conn,
        mask = true,
    }, ws)
    levent.spawn(run, obj)
    return obj
end

return ws
