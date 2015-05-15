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
return util
