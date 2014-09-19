local c = require "levent.http.c"

local url = "http://xjdrew@example.com:8080/t/134467?uid=1827#reply22"
local t = c.parse_url(url)
assert(t.schema == "http")
assert(t.userinfo == "xjdrew")
assert(t.host == "example.com")
assert(t.port == 8080)
assert(t.path == "/t/134467")
assert(t.query == "uid=1827")
assert(t.fragment == "reply22")
