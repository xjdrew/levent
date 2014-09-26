local c = require "levent.http.c"
print("test:", c.version())

local url = "http://xjdrew@example.com:8080/t/134467?uid=1827#reply22"
local t = c.parse_url(url)
assert(t.schema == "http")
assert(t.userinfo == "xjdrew")
assert(t.host == "example.com")
assert(t.port == 8080)
assert(t.path == "/t/134467")
assert(t.query == "uid=1827")
assert(t.fragment == "reply22")

local requests = {
    {
        is_response = true,
        name = "curl get",
        raw = "GET /test HTTP/1.1\r\n"..
            "User-Agent: curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1\r\n"..
            "Host: 0.0.0.0=5000\r\n"..
            "Accept: */*\r\n"..
            "\r\n",
        should_keep_alive = true,
        method = "GET",
        query_string = "",
        fragment="",
        request_path = "/test",
        num_headers = 3,
        headers = {
            ["User-Agent"] = "curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1",
            ["Host"] = "0.0.0.0=5000",
            ["Accept"] = "*/*"
        },
        body = ""
    },

}

local responses = {
    {
        name = "google 301",
        is_response = true,
        raw = "HTTP/1.1 301 Moved Permanently\r\n"..
             "Location: http://www.google.com/\r\n"..
             "Content-Type: text/html; charset=UTF-8\r\n"..
             "Date: Sun, 26 Apr 2009 11:11:49 GMT\r\n"..
             "Expires: Tue, 26 May 2009 11:11:49 GMT\r\n"..
             "Cache-Control: public, max-age=2592000\r\n"..
             "Server: gws\r\n"..
             "Content-Length: 219\r\n"..
             "\r\n"..
             "<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n"..
             "<TITLE>301 Moved</TITLE></HEAD><BODY>\n"..
             "<H1>301 Moved</H1>\n"..
             "The document has moved\n"..
             "<A HREF=\"http://www.google.com/\">here</A>.\r\n"..
             "</BODY></HTML>\r\n",
        should_keep_alive = true,
        status_code = 301,
        num_headers = 7,
        headers = {
            ["Location"] = "http://www.google.com/",
            ["Content-Type"] = "text/html; charset=UTF-8",
            ["Date"] = "Sun, 26 Apr 2009 11:11:49 GMT",
            ["Expires"] = "Tue, 26 May 2009 11:11:49 GMT",
            ["Cache-Control"] = "public, max-age=2592000",
            ["Server"] = "gws",
            ["Content-Length"] = "219"
        },
        body = "<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n"..
             "<TITLE>301 Moved</TITLE></HEAD><BODY>\n"..
             "<H1>301 Moved</H1>\n"..
             "The document has moved\n"..
             "<A HREF=\"http://www.google.com/\">here</A>.\r\n"..
             "</BODY></HTML>\r\n",
    }
}

local function message_eq(msg, ret)
    --assert(msg.method == ret.method, msg.name)
end

local function test_message(msg)
    local parser = c.new()
    local ret = {}
    local traversed, err = parser:execute(msg.raw, nil, ret)
    assert(not err)
    assert(traversed == #msg.raw)
    for k,v in pairs(ret) do
        print("-", k,v)
    end

    if ret.headers then
        print("headers ---------:")
        for k,v in pairs(ret.headers) do
            print("+", k,v)
        end
    end
    message_eq(msg, ret)
end

local function test_messages(raw)
    local parser = c.new()
    local from = 0
    while true do
        local ret = {}
        local traversed, err = parser:execute(raw, from, ret)
        if not err then
            for k,v in pairs(ret) do
                print("-", k,v)
            end

            if ret.headers then
                print("headers ---------:")
                for k,v in pairs(ret.headers) do
                    print("+", k,v)
                end
            end
        end
        from = traversed + from
        if err or from >= #raw then
            break
        end
    end
end

--test_message(requests[1])
--test_message(responses[1])
test_messages(requests[1].raw .. requests[1].raw)

