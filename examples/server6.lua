local socket = require "levent.socket.c"
local sock, err = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
assert(sock, errmsg)
print("bind:", sock:bind("", 8858))
print("listen:", sock:listen())

while true do
    csock, err = sock:accept()
    if not csock then
        print("accept failed:", err)
        break
    end
    print("new conn from:", csock:getpeername())
    while true do
        msg, err = csock:recv(100)
        --print("to recv:", csock:getsockopt(socket.SOL_SOCKET, socket.SO_NREAD))
        if not msg or #msg == 0 then
            print("recv failed:", csock:getpeername(), err)
            break
        end
        print("recv:", #msg, "bytes")
        nsend, err = csock:send("you said:" .. msg)
        if not nsend then
            print("send failed:", nsend, err)
            break
        end
        print("send:", nsend, "bytes")
    end
end

