local levent = require "levent.levent"
local http   = require "levent.http"

local urls = {
    "http://www.google.com",
    "http://yahoo.com",
}

function request(url)
    local response, err = http.get(url)
    if not response then
        print(url, "error:", err)
    else
        print("-------------------", url, "-------------------")
        print("code:", response:get_code())
        --print(response:get_raw())
        local headers = response:get_headers()
        for k,v in pairs(headers) do
            print(k, "--->", v)
        end
        print("------------------- end -------------------")
    end
end

function main()
    for _, url in ipairs(urls) do
        levent.spawn(request, url)
    end
end

levent.start(main)

