local ev = require "event.c"
print(ev.version())
local loop = ev.default_loop()
print(loop:now())
