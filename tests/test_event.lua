local ev = require "event.c"
print(ev.version())
local loop = ev.default_loop()
print(loop)
print(loop:now())
local io = ev.new_io()
print(io)
local timer = ev.new_timer()
print(timer)
