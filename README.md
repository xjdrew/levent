levent
======
levent is a lua concurrency networking library inspired by [gevent](http://www.gevent.org/).

Features include:

* Fast event loop based on [libev](http://libev.schmorp.de/).
* Simple and clean socket library
* Cooperative dns client by pure lua
* Similar api and components with gevent but with much more simpler implementation and better performance.

levent is licensed under MIT license.


get levent
-----------

Install lua 5.3 or newer(for lua5.2 and older, see branch lua5.2).

Clone [the repository](https://github.com/xjdrew/levent).

Read [test case](https://github.com/xjdrew/levent/tree/master/tests) and [examples](https://github.com/xjdrew/levent/tree/master/examples).

Post feedback and issues on the [bug tracker](https://github.com/xjdrew/levent/issues),


build
------
need cmake 2.8 or newer.

```
cmake .
make
```

ways to build on windows, ref to [blog](http://xjdrew.github.io/blog/2014/08/28/compile-levent/)

code example
------------

levent's api is clean, to request http concurrently is as simple as below:

```lua
lua <<SCRIPT
local levent = require "levent.levent"
local http   = require "levent.http"

local urls = {
    "http://www.google.com",
    "http://yahoo.com",
    "http://example.com",
    "http://qq.com",
}

function request(url)
    local response, err = http.get(url)
    if not response then
        print(url, "error:", err)
    else
        print(url, response:get_code())
    end
end

function main()
    for _, url in ipairs(urls) do
        levent.spawn(request, url)
    end
end

levent.start(main)
SCRIPT
```

running tests
-------------
there are some tests and examples under ```tests``` and ```examples``` to illustrate how to use levent, you can run tests and examples from root folder of levent as below.

```
lua tests/test_socket.lua
lua examples/dns_mass_resolve.lua
```

documents
---------
coming soon~~

licence
-------
The MIT License (MIT)
Copyright (c) 2014 xjdrew

