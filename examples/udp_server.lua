local socket = require "levent.socket.c"
local sock, err = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
assert(sock, errmsg)
print("bind:", sock:bind("0.0.0.0", 8859))

while true do
    local msg, peer_addr, peer_port = sock:recvfrom(100)
    if not msg then
        print("recvfrom failed:", peer_addr)
        break
    end
    print("recv msg from:", string.format("%s:%d",peer_addr, peer_port), "len:", #msg)
    local response = string.rep(msg, 1000)
    local nsend, err = sock:sendto(peer_addr, peer_port, response)
    if not nsend then
        print("send failed:", nsend, err)
        break
    end
    print("send:", nsend, "bytes")
end

