cndns: ChinaDNS
---

cndns 是一个简单的 ChinaDNS 实现。

### 用法

```
./cndns <bind_addr> <bind_port> <remote_dns> <max_time>
```

`bind_addr` = 监听地址；`bind_port` = 监听端口；`remote_dns` = 远端 DNS；`max_time` = 等待时间。例如：

```
./cndns 0.0.0.0 53 1.0.0.1 50
```

会把 DNS 流量转发至 `1.0.0.1`，监听来自 `1.0.0.1` 的应答，直到超过 50 毫秒没有回应。

### 原理

国内的 DNS 投毒是这样工作的：

```
----> DNS Query
<==== Respond
<=-=- Fake Respond             ______________             _________________
                _____         |              |           |                 |
               |     |------->| Other ISP... |--- ... -->| Real DNS Server |
-------------->|     |<=======|______________|<== ... ===|_________________|
       <=======| ISP |      ______________
               |     |---->|              |
<-=-=-=-=-=-=-=|_____|<=-=-| DNS Poisoner |
   \                       |______________|
    \
     +-- arrived earlier then the real respond
```

国内的 ISP 引出旁路接入 GFW，GFW 监听到 DNS 查询后立即用目标 DNS 服务器 IP 作为源 IP 返回假结果。但因为投毒在旁路，真正的 DNS 请求有被正常送达目标 DNS，目标 DNS 也有正常应答。

所以，若想要获得正确的结果，不能在获得一个应答后马上相信这是正确的结果，而应该等待一段时间。通常等待直到比目标 DNS 的 RTT 稍长。

比如，

```
PING 1.0.0.1 (1.0.0.1) 56(84) bytes of data.
64 bytes from 1.0.0.1: icmp_seq=1 ttl=53 time=14.8 ms
64 bytes from 1.0.0.1: icmp_seq=2 ttl=52 time=18.6 ms
64 bytes from 1.0.0.1: icmp_seq=3 ttl=53 time=18.3 ms
64 bytes from 1.0.0.1: icmp_seq=4 ttl=53 time=18.3 ms
64 bytes from 1.0.0.1: icmp_seq=5 ttl=53 time=15.1 ms
64 bytes from 1.0.0.1: icmp_seq=6 ttl=54 time=14.8 ms
64 bytes from 1.0.0.1: icmp_seq=7 ttl=53 time=14.5 ms
64 bytes from 1.0.0.1: icmp_seq=8 ttl=53 time=14.7 ms
64 bytes from 1.0.0.1: icmp_seq=9 ttl=52 time=18.7 ms
^C
--- 1.0.0.1 ping statistics ---
9 packets transmitted, 9 received, 0% packet loss, time 8014ms
rtt min/avg/max/mdev = 14.591/16.471/18.766/1.846 ms
```

那么取 50ms 就是个合理的值。投毒的假结果往往 10 毫秒内就抵达了客户端，低于到目标 DNS 的 RTT。
