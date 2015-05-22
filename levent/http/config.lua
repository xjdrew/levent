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

return config

