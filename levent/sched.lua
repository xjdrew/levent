local M = {}

local tinsert = table.insert
local tremove = table.remove
local tunpack = table.unpack

local cocreate = coroutine.create
local coresume = coroutine.resume
local coyield  = coroutine.yield
local corunning = coroutine.running

local main_co = corunning()
local sched_co

local ready_queue = {}

local function _sched()
	while true do
		local co = tremove(ready_queue,  1)
		if not co then
			break
		end
		if co == main_co then
			coyield()
		end
		coresume(co)
	end
end

function _set_ready(co)
	ready_queue[#ready_queue + 1] = co
end

function M.spawn(func, ...)
	local args = {...}
	local co = cocreate(function ()
		func(tunpack(args))
	end)
	_set_ready(co)
end

function M.yield()
	local co = corunning()
	_set_ready(co)
	if co == main_co then
		coresume(sched_co)
	else
		coyield()
	end
end

function M.wait()
	coresume(sched_co)
end

sched_co = cocreate(_sched)
return M

