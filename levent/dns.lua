--[[
-- author: xjdrew
-- date: 2014-08-06
-- lua dns resolver library
-- conform to rfc1035 (http://tools.ietf.org/html/rfc1035), rfc2782 (https://tools.ietf.org/html/rfc2782)
-- support ipv6, conform to rfc1886(http://tools.ietf.org/html/rfc1886), rfc2874(http://tools.ietf.org/html/rfc2874)
--]]

-- [[
-- resource record type:
-- TYPE            value and meaning
-- A               1 a host address
-- NS              2 an authoritative name server
-- MD              3 a mail destination (Obsolete - use MX)
-- MF              4 a mail forwarder (Obsolete - use MX)
-- CNAME           5 the canonical name for an alias
-- SOA             6 marks the start of a zone of authority
-- MB              7 a mailbox domain name (EXPERIMENTAL)
-- MG              8 a mail group member (EXPERIMENTAL)
-- MR              9 a mail rename domain name (EXPERIMENTAL)
-- NULL            10 a null RR (EXPERIMENTAL)
-- WKS             11 a well known service description
-- PTR             12 a domain name pointer
-- HINFO           13 host information
-- MINFO           14 mailbox or mail list information
-- MX              15 mail exchange
-- TXT             16 text strings
-- AAAA            28 a ipv6 host address
-- SRV             33 a DNS RR for specifying the location of services
-- only appear in the question section:
-- AXFR            252 A request for a transfer of an entire zone
-- MAILB           253 A request for mailbox-related records (MB, MG or MR)
-- MAILA           254 A request for mail agent RRs (Obsolete - see MX)
-- *               255 A request for all records
-- 
-- resource recode class:
-- IN              1 the Internet
-- CS              2 the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
-- CH              3 the CHAOS class
-- HS              4 Hesiod [Dyer 87]
-- only appear in the question section:
-- *               255 any class
-- ]]

--[[
-- struct header {
--  uint16_t tid     # identifier assigned by the program that generates any kind of query.
--  uint16_t flags   # flags
--  uint16_t qdcount # the number of entries in the question section.
--  uint16_t ancount # the number of resource records in the answer section.
--  uint16_t nscount # the number of name server resource records in the authority records section.
--  uint16_t arcount # the number of resource records in the additional records section.
-- }
--
-- request body:
-- struct request {
--  string name
--  uint16_t atype
--  uint16_t class
-- }
--
-- response body:
-- struct response {
--  string name
--  uint16_t atype
--  uint16_t class
--  uint16_t ttl
--  uint16_t rdlength
--  string rdata
-- }
--]]
local levent     = require "levent.levent"
local socket     = require "levent.socket"
local class      = require "levent.class"
local ltimeout    = require "levent.timeout"
local exceptions = require "levent.exceptions"

local pack = string.pack
local unpack = string.unpack

local dns = {}
dns.DEFAULT_HOSTS       = "/etc/hosts"
dns.DEFAULT_RESOLV_CONF = "/etc/resolv.conf"
dns.DEFAULT_PORT        = 53

local MAX_DOMAIN_LEN = 1024
local MAX_LABEL_LEN = 63
local MAX_PACKET_LEN = 2048
local DNS_HEADER_LEN = 12

-- local hosts
local local_hosts

-- dns server address
local dns_addrs

--------------------------------------------------------------------------------------
-- util function

-- return name type: 'ipv4', 'ipv6', or 'hostname'
local function guess_name_type(name)
    if name:match("^[%d%.]+$") then
        return "ipv4"
    end

    if name:find(":") then
        return "ipv6"
    end

    return "hostname"
end

local function is_valid_hostname(name)
    if #name > MAX_DOMAIN_LEN then
        return false
    end

    if not name:match("^[%l%d-%.]+$") then
        return false
    end

    for w in name:gmatch("([%w-]+)%.?") do
        if #w > MAX_LABEL_LEN then
            return false
        end
    end

    return true
end

-- http://man7.org/linux/man-pages/man5/hosts.5.html
local function parse_hosts()
    if not dns.DEFAULT_HOSTS then
        return
    end

    local f = io.open(dns.DEFAULT_HOSTS)
    if not f then
        return
    end

    local rts = {}
    for line in f:lines() do
        local ip, hosts = string.match(line, "^%s*([%[%]%x%.%:]+)%s+([^#;]+)")
        if not ip or not hosts then
            goto continue
        end

        local family = guess_name_type(ip)
        if family == "hostname" then
            goto continue
        end

        for host in hosts:gmatch("%S+") do
            host = host:lower()
            local rt = rts[host]
            if not rt then
                rt = {}
                rts[host] = rt
            end

            if not rt[family] then
                rt[family] = {}
            end
            table.insert(rt[family], ip)
        end

        ::continue::
    end
    return rts
end

-- http://man7.org/linux/man-pages/man5/resolv.conf.5.html
local function parse_resolv_conf()
    if not dns.DEFAULT_RESOLV_CONF then
        return
    end

    local f = io.open(dns.DEFAULT_RESOLV_CONF)
    if not f then
        return
    end

    local servers = {}
    for line in f:lines() do
        local server = line:match("^%s*nameserver%s+([^#;%s]+)")
        if server then
            table.insert(servers, {host = server, port = dns.DEFAULT_PORT})
        end
    end
    f:close()
    return servers
end

local function get_nameservers()
    if not dns_addrs then
        dns_addrs = assert(parse_resolv_conf(), "parse resolve conf failed")
    end
    return dns_addrs
end

local function set_nameservers(addrs)
    dns_addrs = addrs
end

local function exception(info)
    return exceptions.DnsError.new(info)
end

--------------------------------------------------------------------------------------
-- dns protocol

local QTYPE = {
    A       = 1,
    TXT     = 16,
    AAAA    = 28,
    SRV     = 33,
}

local QCLASS = {
    IN = 1,
}

local SECTION = {
    AN = 1,
    NS = 2,
    AR = 3,
}

local next_tid = 1
local function gen_tid()
    local tid = next_tid
    next_tid = next_tid + 1
    return tid
end

local function pack_header(t)
    return pack(">HHHHHH", t.tid, t.flags, t.qdcount, t.ancount or 0, t.nscount or 0, t.arcount or 0)
end

local function pack_question(name, qtype, qclass)
    local labels = {}
    for w in name:gmatch("([_%w%-]+)%.?") do
        table.insert(labels, string.pack("s1",w))
    end
    table.insert(labels, '\0')
    table.insert(labels, string.pack(">HH", qtype, qclass))
    return table.concat(labels)
end

-- unpack a resource name
local function unpack_name(chunk, left)
    local t = {}
    local jump_pointer
    local tag, offset, label
    while true do
        tag, left = unpack("B", chunk, left)
        if tag & 0xc0 == 0xc0 then
            -- pointer
            offset,left = unpack(">H", chunk, left - 1)
            offset = offset & 0x3fff
            if not jump_pointer then
                jump_pointer = left
            end
            -- offset is base 0, need to plus 1
            left = offset + 1
        elseif tag == 0 then
            break
        else
            label, left = unpack("s1", chunk, left - 1)
            t[#t+1] = label
        end
    end
    return table.concat(t, "."), jump_pointer or left
end

local unpack_type = {
    [QTYPE.A] = function(ans, chunk)
        if #chunk ~= 4 then
            return exception("bad A record value length: " .. #chunk)
        end
        local a, b, c, d = unpack("BBBB", chunk)
        ans.address = string.format("%d.%d.%d.%d", a, b, c, d)
    end,
    [QTYPE.AAAA] = function(ans, chunk)
        if #chunk ~= 16 then
            return exception("bad AAAA record value length: " .. #chunk)
        end
        local a,b,c,d,e,f,g,h = unpack(">HHHHHHHH", chunk)
        ans.address = string.format("%x:%x:%x:%x:%x:%x:%x:%x", a, b, c, d, e, f, g, h)
    end,
    [QTYPE.SRV] = function(ans, chunk)
        if #chunk < 7 then
            return exception("bad SRV record value length: " .. #chunk)
        end

        local left
        ans.priority, ans.weight, ans.port, left = unpack(">HHH", chunk)
        ans.target, left = unpack_name(chunk, left)
    end,
    [QTYPE.TXT] = function(ans, chunk)
        local left = 1
        ans.txt = {}
        while left < #chunk do
            local txt
            txt, left = unpack("s1", chunk, left)
            table.insert(ans.txt, txt)
        end
    end,
}

local function unpack_section(answers, section, chunk, left, count)
    for _ = 1, count do
        local ans = {}
        ans.section = section
        ans.name, left = unpack_name(chunk, left)

        local rdata
        ans.qtype, ans.class, ans.ttl, rdata, left = unpack(">HHI4s2", chunk, left)

        local unpack_rdata = unpack_type[ans.qtype]
        if unpack_rdata then
            local err = unpack_rdata(ans, rdata)
            if err then
                return nil, err
            end

            table.insert(answers, ans)
        end
    end

    return left
end

local function unpack_response(chunk)
    if #chunk < DNS_HEADER_LEN then
        return nil, exception("truncated")
    end

    -- unpack header
    local tid, flags, qdcount, ancount, nscount, arcount, left = unpack(">HHHHHH", chunk)

    if flags & 0x8000 == 0 then
        return nil, exception("bad QR flag in the DNS response")
    end

    if flags & 0x200 ~= 0 then
        return nil, exception("truncated")
    end

    local code = flags & 0xf
    if code ~= 0 then
        return nil, exception("code:" .. code)
    end

    if qdcount ~= 1 then
        return nil, exception("qdcount error " .. qdcount)
    end

    local qname
    qname, left = unpack_name(chunk, left)

    local qtype, class
    qtype, class, left = unpack(">HH", chunk, left)

    local answers = {
        tid = tid,
        code = code,
        qname = qname,
        qtype = qtype,
    }

    local err
    local sections = {
        {section = SECTION.AN, count = ancount},
        {section = SECTION.NS, count = nscount},
        {section = SECTION.AR, count = arcount},
    }
    for _, section in ipairs(sections) do
        left, err = unpack_section(answers, section.section, chunk, left, section.count)
        if not left then
            return nil, err
        end
    end

    return answers
end

local function request(server, chunk, timeout, type)
    local sock, err = socket.socket(socket.AF_INET, type == "tcp" and socket.SOCK_STREAM or socket.SOCK_DGRAM)
    if not sock then
        return nil, err
    end

    if timeout then
        sock:set_timeout(timeout)
    end

    local ok, err = sock:connect(server.host, server.port)
    if not ok then
        return nil, err
    end

    if type == "tcp" then
        chunk = pack(">H", #chunk) .. chunk
    end

    local sent, err = sock:send(chunk)
    if err then
        return nil, err
    end

    if sent ~= #chunk then
        return nil, exception("send failed")
    end

    local resp, err
    if type == "tcp" then
        local header
        header, err = sock:recv(2)
        if err then
            return nil, err
        end
        local len = unpack(">H", header)
        resp, err = sock:recv(len)
    else
        -- only accept first packet, drop others
        resp, err = sock:recv(MAX_PACKET_LEN)
    end
    sock:close()
    if err then
        return nil, err
    end

    return unpack_response(resp)
end

--------------------------------------------------------------------------------------
-- cached

local weak = {__mode = "kv"}
local cached = {}
local function query_cache(qtype, name)
    local qcache = cached[qtype]
    if not qcache then
        return
    end

    local t = qcache[name]
    if t then
        if t.expired < levent.now() then
            qcache[name] = nil
            return
        end
        return t.data
    end
end

local function set_cache(qtype, name, ttl, data)
    if ttl and ttl > 0 and data then
        local qcache = cached[qtype]
        if not qcache then
            qcache = setmetatable({}, weak)
            cached[qtype] = qcache
        end
        qcache[name] = {
            expired = levent.now() + ttl,
            data = data
        }
    end
end

--------------------------------------------------------------------------------------

local resolve_type = {
    [QTYPE.A] = {
        family = "ipv4",
        normalize = function(answers)
            local ret = {}
            local ttl
            for _, ans in ipairs(answers) do
                if ans.qtype == QTYPE.A then
                    table.insert(ret, ans.address)
                    ttl = ans.ttl
                end
            end
            return ret, ttl
        end,
    },
    [QTYPE.AAAA] = {
        family = "ipv6",
        normalize = function(answers)
            local ret = {}
            local ttl
            for _, ans in ipairs(answers) do
                if ans.qtype == QTYPE.AAAA then
                    table.insert(ret, ans.address)
                    ttl = ans.ttl
                end
            end
            return ret, ttl
        end,
    },
    [QTYPE.SRV] = {
        normalize = function(answers)
            local servers = {}
            for _, ans in ipairs(answers) do
                if ans.qtype == QTYPE.SRV then
                    servers[ans.target] = {
                        host = ans.target,
                        port = ans.port,
                        weight = ans.weight,
                        priority = ans.priority,
                        ttl = ans.ttl,
                    }
                elseif ans.qtype == QTYPE.A or ans.qtype == QTYPE.AAAA then
                    assert(servers[ans.name])
                    servers[ans.name].host = ans.address
                end
            end

            local ret = {}
            local ttl
            for _, server in pairs(servers) do
                table.insert(ret, server)
                ttl = server.ttl
            end
            return ret, ttl
        end,
    },
    [QTYPE.TXT] = {
        normalize = function(answers)
            local ret = {}
            local ttl
            for _, ans in ipairs(answers) do
                if ans.qtype == QTYPE.TXT then
                    for _, txt in ipairs(ans.txt) do
                        table.insert(ret, txt)
                    end
                    ttl = ans.ttl
                end
            end
            return ret, ttl
        end,
    },
}
-- parse local hosts
local function local_resolve(name, qtype)
    local hosts = local_hosts
    if not hosts then
        local_hosts = parse_hosts()
        hosts = local_hosts
    end

    local family = resolve_type[qtype].family
    local t = hosts[name]
    if t then
        return t[family]
    end
    return nil
end

local function remote_resolve(name, qtype, timeout)
    local ret = query_cache(qtype, name)
    if ret then
        return ret
    end

    local question_header = {
        tid = gen_tid(),
        flags = 0x100, -- flags: 00000001 00000000, set RD
        qdcount = 1,
    }
    local req = pack_header(question_header) .. pack_question(name, qtype, QCLASS.IN)

    local nameservers = get_nameservers()
    local valid_nameserver = {}
    local valid_amount = 1
    local result, err

    for _, server in ipairs(nameservers) do
        if not result then
            result, err = request(server, req, timeout)
            if class.isinstance(err, exceptions.DnsError) and err.info == "truncated" then
                result, err = request(server, req, timeout, "tcp")
            end
        end

        if ltimeout.is_timeout(err) then
            table.insert(valid_nameserver, #valid_nameserver+1, server)
        else
            table.insert(valid_nameserver, valid_amount, server)
            valid_amount = valid_amount + 1
        end
    end
    set_nameservers(valid_nameserver)

    if not result then
        return nil, err
    elseif #result == 0 then
        return nil, exception("no answers")
    end

    local ttl
    local normalize = resolve_type[qtype].normalize
    result, ttl = normalize(result)

    set_cache(qtype, name, ttl, result)
    return result
end

local function dns_resolve(name, qtype, timeout)
    assert(resolve_type[qtype], "unknown resolve qtype " .. qtype)
    local answers = local_resolve(name, qtype)
    if answers then
        return answers
    end

    return remote_resolve(name, qtype, timeout)
end

-- set your preferred dns server or use default
function dns.set_servers(servers)
    dns_addrs = {}
    for _, server in ipairs(servers) do
        if type(server) == "table" then
            assert(server.host)
            table.insert(dns_addrs, {host = server.host, port = server.port or dns.DEFAULT_PORT})
        else
            table.insert(dns_addrs, {host = server, port = dns.DEFAULT_PORT})
        end
    end
end

function dns.resolve(name, qtype, timeout)
    name = name:lower()
    timeout = timeout or 1
    qtype = qtype or QTYPE.A
    local ntype = guess_name_type(name)
    if ntype ~= "hostname" then
        return {name}
    end

    if not is_valid_hostname(name) then
        return nil, exception("illegal name")
    end
    return dns_resolve(name, qtype, timeout)
end

dns.QTYPE = QTYPE

return dns
