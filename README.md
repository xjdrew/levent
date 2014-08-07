levent
======
levent is a lua concurrency networking library inspired by [gevent](http://www.gevent.org/).

Features include:

* Fast event loop based on [libev](http://libev.schmorp.de/).
* Cooperative socket library
* Cooperative dns client by pure lua
* Similar api and components with gevent but with much more simpler implementation and better performance.

levent is licensed under MIT license.


get levent
-----------

Install lua 5.2 or newer.

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

running tests
-------------
```
lua tests/test_socket.lua
lua example/echo.lua
```

to-do list
----------
* Cooperative dns query
* Support more platform include windows, openbsd.

