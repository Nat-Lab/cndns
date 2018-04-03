cndns: ChinaDNS
---

cndns 是一个简单的 ChinaDNS 实现。

### 用法

```
usage: ./cndns
   -s <remote_dns>     remote DNS
   -m <min_time>       minimum respond time (ms)
   -M [max_time]       lookup timeout (ms) (default: min_time + 1000)
   -l [bind_addr]      local address (default: 0.0.0.0)
   -p [bind_port]      local port (default: 53)
   -S                  strict mode (do nothing when timed out)
   -v                  debug output
```

`-s remote_dns`: 远端 DNS，即想要使用的 DNS 服务器。

`-m min_time`: 最短响应时间，若有短于此时间的 DNS 应答，会被暂时忽略。若有超过此时间的 DNS 应答，会立即返回结果。

`-M max_time`: 最长等待时间，若超过此时间没有新的 DNS 应答，则返回最后接到的应答（默认：min\_time + 1000）。

`-l bind_addr`: 本地 DNS 监听地址（默认：0.0.0.0）。

`-p bind_port`: 本地 DNS 监听端口（默认：53）。

`-S`：严格模式（超时后不要返回最后接到的应答）。

`-v`：输出每个应答抵达的时间。

例如：

```
./cndns -s1.0.0.1 -m15
```

会把 DNS 流量转发至 `1.0.0.1`，监听来自 `1.0.0.1` 的应答，并按照下面的行为处理 DNS 应答：

条件|行为
---|---
响应时间 < 15ms|暂时忽略
15ms < 响应时间 < 1015ms|立刻返回
15ms ~ 1015ms 区间没有应答|返回最后接到的应答

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

那么，15ms 内的应答都不可能是来自真正的目标 DNS 服务器，使用 `-m15` 参数忽略它们。
