# thrift-bench
Benchmark for fbthrift, wangle, asyncio, apache thrift

Our initial experiments with fbthrift gave very high latency (900 micro sec) for a simple roundtrip between two server. Thats when we decided figure out what was going wrong and why we were getting such high latencies. 

We did 3 additional experiments to measure latency (single threaded, 4k) to cross check against hardware and other frameworks.

- folly async sockets without any framework. If we get good numbers from this we know that the hardware and the OS stack is not the problem.
- wangle, without the thrift
- thrift:golang (Not committed in this repository)
- apache thrift

All the experiments were carried on the same machine.

The latencies for 4k single threaded request/response for these were

| Benchmark	| Latency in usec |
| --------- | --------------- |
| folly async sockets |	50 |
| thrift+golang | 101 |
| wangle |	138 |
| fbthrift |	900 |

We did similar experiment with both client and servers running as separate process on the same machine.

| Benchmark	| Latency in usec |
| --------- | --------------- |
| folly async sockets |	21 |
| wangle |	100 |
| fbthrift |	368 |
| apache thrift | 31 |

We suspect something is wrong in the thrift part of fbthrift, the wangle part seems to be fine. Our suspicion is that it is either a compilation or tuneable which is set incorrectly.

## Running AsyncIO Benchmark
cd into the directory
```
$ cd AsyncIO/
```
run make
```
$ make
```
start server
```
$ ./net-server
```
run client
```
$ ./net-client
```

**Follow similar steps to run other benchmarks**
