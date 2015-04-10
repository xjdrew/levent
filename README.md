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

Install lua 5.3 or newer(for lua5.2 and below, see branch lua5.2).

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

levent's api is clean, below is a code example for how to run a time limit work

```lua
lua <<SCRIPT
local levent  = require "levent.levent"
local timeout = require "levent.timeout"

function main()
    print("work 1:", timeout.run(0.2, levent.sleep, 0.1))
    print("work 2:", timeout.run(0.1, levent.sleep, 0.2))
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

