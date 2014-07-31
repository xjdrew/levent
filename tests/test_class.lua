local Class = require "levent.class"

local times = 0
local A = Class("A")
local B = Class("B")
local C = Class("C", A)

function A:_init(age)
    self.age = age
    times = times + 1
end

function A:name()
    return "A"
end

function A:__tostring()
    return "A:"..self.age
end

function B:name()
    return "B"
end

function B:__tostring()
    return "B"
end

function C:name()
    return "C"
end

function C:__tostring()
    return "C"
end


local a = A.new(8)
local b = B.new()
local c = C.new(10)

assert(times == 2, times)

assert(Class.issubclass(A, A))
assert(Class.isinstance(a, A))

assert("A" == a:name())
assert("A:8" == tostring(a))
assert(Class.isinstance(b, B))
assert("B" == B:name())
assert(Class.issubclass(C, A))
assert("C" == c:name())
assert(Class.isinstance(c, C))
assert(Class.isinstance(c, A))
assert(not Class.isinstance(c, B))

