local config = {}

config.HTTP_SCHEMA  = "HTTP"
config.HTTP_VERSION = "1.1"
config.HTTP_PORT = 80
-- config.HTTPS_PORT = 443

config.HTTP_AGENT = "levent/1.0"
config.HTTP_ABS_PATH = "/"

-- supported methods
-- http methods is case-sensitive
config.HTTP_METHODS = {
    OPTIONS = "OPTIONS",
    GET     = "GET",
    HEAD    = "HEAD",
    POST    = "POST",
    PUT     = "PUT",
    DELETE  = "DELETE",
    -- TRACE   = "TRACE",
    -- CONNECT = "CONNECT",
}

config.VALID_SCHEMA = {
    http    = true,
    ws      = true,
}

config.WebSocket = {
    VERSION = 13,
    GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
    CODES = {
        [1000] = "normal closure",
        NORMAL_CLOSURE = 1000,

        [1001] = "going away",
        GOING_AWAY = 1001,

        [1002] = "protocol error",
        PROTOCOL_ERROR = 1002,
    }
}

return config
