local c = require "levent.http.c"
print("test:", c.version())

local url = "http://xjdrew@example.com:8080/t/134467?uid=1827#reply22"
local t = c.parse_url(url)
assert(t.schema == "http")
assert(t.userinfo == "xjdrew")
assert(t.host == "example.com")
assert(t.port == 8080)
assert(t.request_path == "/t/134467")
assert(t.query_string == "uid=1827")
assert(t.fragment == "reply22")

local requests = {
    {
        name = "curl get",
        is_request = true,
        raw = "GET /test HTTP/1.1\r\n"..
            "User-Agent: curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1\r\n"..
            "Host: 0.0.0.0=5000\r\n"..
            "Accept: */*\r\n"..
            "\r\n",
        keepalive = true,
        http_major = 1,
        http_minor = 1,
        method = "GET",
        request_url= "/test",
        request_path = "/test",
        num_headers = 3,
        headers = {
            ["User-Agent"] = "curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1",
            ["Host"] = "0.0.0.0=5000",
            ["Accept"] = "*/*"
        },
    },
    {
        name = "firefox get",
        is_request = true,
        raw= "GET /favicon.ico HTTP/1.1\r\n" ..
             "Host: 0.0.0.0=5000\r\n" ..
             "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n" ..
             "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n" ..
             "Accept-Language: en-us,en;q=0.5\r\n" ..
             "Accept-Encoding: gzip,deflate\r\n" ..
             "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n" ..
             "Keep-Alive: 300\r\n" ..
             "Connection: keep-alive\r\n" ..
             "\r\n",
        keepalive= true,
        http_major= 1,
        http_minor= 1,
        method= "GET",
        request_url= "/favicon.ico",
        request_path= "/favicon.ico",
        num_headers= 8,
        headers=
            { ["Host"]= "0.0.0.0=5000",
              ["User-Agent"] = "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0",
              ["Accept"]= "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
              ["Accept-Language"]= "en-us,en;q=0.5",
              ["Accept-Encoding"]= "gzip,deflate",
              ["Accept-Charset"]= "ISO-8859-1,utf-8;q=0.7,*;q=0.7",
              ["Keep-Alive"]= "300",
              ["Connection"]= "keep-alive",
            }
    },
    {
        name= "dumbfuck",
        is_request=true,
        raw= "GET /dumbfuck HTTP/1.1\r\n" ..
             "aaaaaaaaaaaaa:++++++++++\r\n" ..
             "\r\n",
        keepalive= true,
        http_major= 1,
        http_minor= 1,
        method= "GET",
        request_path= "/dumbfuck",
        request_url= "/dumbfuck",
        num_headers= 1,
        headers=
          { ["aaaaaaaaaaaaa"] =  "++++++++++"
          }
    },
    {
        is_request = "true",
        name= "fragment in uri",
        raw= "GET /forums/1/topics/2375?page=1#posts-17408 HTTP/1.1\r\n"..
                 "\r\n",
        keepalive = true,
        http_major = 1,
        http_minor = 1,
        method = "GET",
        request_url = "/forums/1/topics/2375?page=1#posts-17408",
        query_string = "page=1",
        fragment = "posts-17408",
        request_path = "/forums/1/topics/2375",
        num_headers = 0,
    },
    {
        name= "get no headers no body",
        is_request = "true",
        raw= "GET /get_no_headers_no_body/world HTTP/1.1\r\n" ..
             "\r\n",
        keepalive= true,
        http_major= 1,
        http_minor= 1,
        method= "GET",
        request_path= "/get_no_headers_no_body/world",
        request_url= "/get_no_headers_no_body/world",
        num_headers= 0,
    },
    {
        name = "get one header no body",
        is_request = true,
        raw= "GET /get_one_header_no_body HTTP/1.1\r\n" ..
             "Accept: */*\r\n" ..
             "\r\n",
        keepalive= true,
        http_major= 1,
        http_minor= 1,
        method= "GET",
        request_path= "/get_one_header_no_body",
        request_url= "/get_one_header_no_body",
        num_headers= 1,
        headers= { ["Accept"] =  "*/*" },
    },
    {
        name= "get funky content length body hello",
        is_request=true,
        raw= "GET /get_funky_content_length_body_hello HTTP/1.0\r\n"..
             "conTENT-Length: 5\r\n"..
             "\r\n" ..
             "HELLO",
        keepalive= false,
        http_major= 1,
        http_minor= 0,
        method= "GET",
        request_path= "/get_funky_content_length_body_hello",
        request_url= "/get_funky_content_length_body_hello",
        num_headers= 1,
        headers= {["conTENT-Length"]="5"} ,
        body= "HELLO",
    },
    {
        name= "post identity body world",
        is_request = true,
        raw= "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"..
             "Accept: */*\r\n" ..
             "Transfer-Encoding: identity\r\n"..
             "Content-Length: 5\r\n"..
             "\r\n"..
             "World",
        keepalive= true,
        http_major= 1,
        http_minor= 1,
        method= "POST",
        query_string= "q=search",
        fragment= "hey",
        request_path= "/post_identity_body_world",
        request_url= "/post_identity_body_world?q=search#hey",
        num_headers= 3,
        headers= { ["Accept"] = "*/*",
            ["Transfer-Encoding"] = "identity",
            ["Content-Length"] = "5"
        },
        body= "World"
    },
    {
        name= "post - chunked body: all your base are belong to us",
        is_request = true,
        raw= "POST /post_chunked_all_your_base HTTP/1.1\r\n" ..
             "Transfer-Encoding: chunked\r\n" ..
             "\r\n" ..
             "1e\r\nall your base are belong to us\r\n" ..
             "0\r\n" ..
             "\r\n",
        keepalive= true,
        http_major= 1,
        http_minor= 1,
        method= "POST",
        request_path= "/post_chunked_all_your_base",
        request_url= "/post_chunked_all_your_base",
        num_headers= 1,
        headers= { ["Transfer-Encoding"] =  "chunked"},
        body= "all your base are belong to us"
    },
    {
        name= "two chunks ; triple zero ending",
        is_request = true,
        raw= "POST /two_chunks_mult_zero_end HTTP/1.1\r\n" ..
             "Transfer-Encoding: chunked\r\n" ..
             "\r\n" ..
             "5\r\nhello\r\n"..
             "6\r\n world\r\n"..
             "000\r\n"..
             "\r\n",
        keepalive= true,
        http_major= 1,
        http_minor= 1,
        method= "POST",
        request_path= "/two_chunks_mult_zero_end",
        request_url= "/two_chunks_mult_zero_end",
        num_headers= 1,
        headers= { ["Transfer-Encoding"] = "chunked"},
        body= "hello world",
    },
    {
        name= "chunked with trailing headers. blech.",
        is_request=true,
        raw= "POST /chunked_w_trailing_headers HTTP/1.1\r\n" ..
             "Transfer-Encoding: chunked\r\n" ..
             "\r\n" ..
             "5\r\nhello\r\n" ..
             "6\r\n world\r\n" ..
             "0\r\n" ..
             "Vary: *\r\n" ..
             "Content-Type: text/plain\r\n" ..
             "\r\n",
        keepalive=true,
        http_major= 1,
        http_minor= 1,
        method= "POST",
        request_path= "/chunked_w_trailing_headers",
        request_url= "/chunked_w_trailing_headers",
        num_headers= 3,
        headers= { ["Transfer-Encoding"] = "chunked",
            ["Vary"] = "*",
            ["Content-Type"] = "text/plain",
        },
        body= "hello world"
    }
}

local responses = {
    {
        name = "google 301",
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
        keepalive = true,
        http_major = 1,
        http_minor = 1,
        status_code = 301,
        status = "Moved Permanently",
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
    assert(ret.complete, msg.name)
    assert(msg.http_major == ret.http_major, msg.name)
    assert(msg.http_minor == ret.http_minor, msg.name)

    if msg.is_request then
        assert(msg.method == ret.method, msg.name)
    else
        assert(msg.status_code == ret.status_code, msg.name)
        assert(msg.status == ret.status, msg.name)
    end

    assert(msg.keepalive == ret.keepalive, msg.name)
    assert(msg.request_url == ret.request_url, msg.name)
    if msg.request_url and msg.method ~= "CONNECT" then
        local url = c.parse_url(msg.request_url)
        assert(msg.host == url.host, msg.name)
        assert(msg.userinfo == url.userinfo, msg.name)
        assert(msg.request_path == url.request_path, msg.name)
        assert(msg.query_string == url.query_string, msg.name)
        assert(msg.fragment == url.fragment, msg.name)
        assert(msg.port == url.port, msg.name)
    end

    assert(msg.body == ret.body, msg.name)
    local num_headers = 0
    for header, value in pairs(ret.headers) do
        assert(msg.headers[header] == value, msg.name .. ":" .. header)
        num_headers = num_headers + 1
    end
    assert(msg.num_headers == num_headers, msg.name)
    assert(msg.upgrade == ret.upgrade, msg.name)
end

local function test_message(msg)
    local parser = c.new()
    local ret = {}
    local _, err = parser:execute(msg.raw, nil, ret)
    assert(not err, msg.name)
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

for i, request in ipairs(requests) do
    test_message(request)
    print("request:", i, "pass")
end
for i, response in ipairs(responses) do
    test_message(response)
    print("response:", i, "pass")
end
--test_messages(requests[1].raw .. requests[1].raw)

print("test pass")
