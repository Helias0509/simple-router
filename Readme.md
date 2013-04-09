INTRODUCTION
-------------

This is an implematation of a router with the help of Mininet. The router will receive raw Ethernet frames. It will process the packets just like a real router, then forward them to the correct outgoing interface using a routing table.

The router will route real packets from an emulated host (client) to two emulated application servers (http server 1/2) sitting behind the router. The application servers are each running an HTTP server. We are able to access these servers using regular client software. In addition, we can ping and traceroute to and through a functioning Internet router. A sample routing topology is shown below:

![Topology](http://irl.cs.ucla.edu/~yingdi/cs118/topo-sample.png)
