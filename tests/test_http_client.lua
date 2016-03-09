local levent = require "levent.levent"
local http   = require "levent.http"

local urls = {
    "http://example.com",
    "http://qq.com",
}

function request(url)
    print("++++++++++++ request:", url)
    local response, err = http.get(url)
    if not response then
        print(url, "error:", err)
    else
        print("-------------------", url, "-------------------")
        print("code:", response:get_code())
        local headers = response:get_headers()
        if headers then
            for k,v in pairs(headers) do
                print(k, "--->", v)
            end
        end
        print("------------ body -----------------------------")
        print(response:get_data())
        print("------------------- end -------------------")
    end
end

function main()
    for _, url in ipairs(urls) do
        levent.spawn(request, url)
    end
end

levent.start(main)

