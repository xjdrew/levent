local c = require "levent.http.c"

local util = {}

function util.read_message(conn, parser, left)
    local msg = {}
    local raw = {}
    while not msg.complete do
        local parsed, s, err
        if left then
            s = left
        else
            s, err = conn:recv(4096)
            if not s then
                return nil, err
            end
        end

        parsed, err = parser:execute(s, nil, msg)
        if err then
            return nil, c.http_errno_name(err)
        end

        -- socket closed by peer
        if #s == 0 then
            break
        end

        if parsed < #s then
            table.insert(raw, s:sub(1, parsed))
            left = s:sub(parsed+1)
        else
            table.insert(raw, s)
        end
    end

    -- return raw msg at last
    return msg, left, table.concat(raw)
end

function util.canonical_header_key(key)
    return key:lower():gsub("%f[^\0%-]%l",string.upper)
end

local function escape(s)
    return s:gsub("[^a-zA-Z0-9-_.~]", function(c) 
        return string.format("%%%02x",string.byte(c)) 
    end)
end

local function unescape(s)
    return s:gsub("%%%x%x", function(ss) 
        return string.char(tonumber("0x" .. ss:sub(2))) 
    end)
end

function util.parse_query_string(s, t)
    if not s then
        return
    end

    for k,v in s:gmatch("([^&;]+)=([^&;]+)") do
        if #k > 0 then
            t[unescape(k)] = unescape(v)
        end
    end
end

function util.encode_query_string(t)
    if not t then
        return ""
    end

    local tt = {}
    for k,v in pairs(t) do
        if type(v) ~= "string" then
            v = tostring(v)
        end
        tt[#tt + 1] = string.format("%s=%s",escape(k),escape(v))
    end
    return table.concat(tt, "&")
end

util.escape = escape
util.unescape = unescape
return util

