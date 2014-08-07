local tinsert = table.insert
local tconcat = table.concat
local sformat = string.format

local function toprintable(value)
    local t = type(value)
    if t == 'number' then
        return tostring(value)
    elseif t == 'string' then
        return sformat('%q', value)
    elseif t == 'boolean' then
        return value and 'true' or 'false'
    end
    return
end

local function is_array(t)
    local array_count = #t
    local hash_count  = 0
    for k,v in pairs(t) do
        hash_count = hash_count + 1
    end
    if array_count ~= hash_count then
        return false
    end

    for i=1,array_count do
        if t[i] == nil then
            return false
        end
    end
    return true
end

local ser_array
local ser_kvpairs

local function ser_table(t, index)
    local tmp = {}
    if is_array(t) then
        tinsert(tmp, ser_array(t, index))
    else
        tinsert(tmp, ser_kvpairs(t, index))
    end
    return tconcat(tmp)
end

ser_array = function (t, index)
    local space = string.rep(' ', index)
    local table_space = string.rep(' ', index - 4)

    local tmp = {}
    tinsert(tmp, '{\n')
    tinsert(tmp, space)
    for k,v in ipairs(t) do
        if type(v) == "table" then
            tinsert(tmp, ser_table(v, index + 4))
            tinsert(tmp, ',\n')
            tinsert(tmp, space)
        else
            local value = toprintable(v)
            tinsert(tmp, value)
            tinsert(tmp, ", ")
        end
    end
    
    tinsert(tmp, '\n')
    tinsert(tmp, table_space)
    tinsert(tmp, '}')
    return tconcat(tmp)
end

ser_kvpairs = function (t, index)
        local space = string.rep(' ', index)
        local table_space = string.rep(' ', index - 4)

		local tmp={}
        tinsert(tmp, '{\n')
		for k,v in pairs(t) do
            local key = toprintable(k)
            local value = toprintable(v)

            tinsert(tmp, space)
            tinsert(tmp, '[')
            tinsert(tmp, key)
            tinsert(tmp, '] = ')
            if value then
                tinsert(tmp, value)
            elseif type(v) == 'table' then
                tinsert(tmp, ser_table(v, index + 4))
            else
                error()
            end
            tinsert(tmp, ',\n')
        end
        tinsert(tmp, table_space)
        tinsert(tmp, '}')
		return tconcat(tmp)
end

local function serialize(T)
	return ser_table(T, 4)
end

return serialize
