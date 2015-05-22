local config = require "levent.http.config"
local util   = require "levent.http.util"

local methods  = config.HTTP_METHODS
local parse_query_string = util.parse_query_string

local request_reader = {}
request_reader.__index = request_reader

-- msg {
--  method string # GET, POST, PUT, HEAD ...
--  request_url string # /index?param=value
--  request_path string # /index
--  query_string string # param=value
--  headers table # all headers
--  body string # post data
--  remote_host string
--  remote_port int
-- }
function request_reader.new(msg)
    return setmetatable(msg, request_reader)
end

-- query form
function request_reader:get_form()
    if self.forms then
        return self.forms
    end

    self.forms = {}
    if self.method == methods.POST or self.method == methods.PUT then
        parse_query_string(self.body, self.forms)
    end
    return self.forms
end

-- all
function request_reader:get_args()
    if self.args then
        return self.args
    end

    self.args = {}
    if self.query_string then
        parse_query_string(self.query_string, self.args)
    end

    local forms = self:get_form()
    for k, v in pairs(forms) do
        self.args[k] = v
    end
    return self.args
end

return request_reader

