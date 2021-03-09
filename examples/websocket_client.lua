local levent = require "levent.levent"
local websocket = require "levent.http.websocket"

local function main()
    local cli = websocket.client("ws://echo.websocket.org")
    local text = "Hello World!!!"
    function cli:on_message(opcode, message)
        assert(opcode == 0x1, opcode)
        assert(message == text, message)
        print(message)
        cli:close()
    end
    cli:text(text)
end

levent.start(main)
