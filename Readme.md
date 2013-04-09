INTRODUCTION
-------------

This is an implematation of a router with the help of the Mininet, an instant virtual network. The router will receive raw Ethernet frames. It will process the packets just like a real router, then forward them to the correct outgoing interface using a routing table.

The router will route real packets from an emulated host (client) to two emulated application servers (http server 1/2) sitting behind the router. The application servers are each running an HTTP server. We are able to access these servers using regular client software. In addition, we can ping and traceroute to and through a functioning Internet router. Depends on how you configure the network, you may have different number of hosts and clients. In this project, I have set up a simple toplogy which you can see in the topology.text.

MININET
--------
This project runs on top of Mininet which was built at Stanford. Mininet allows you to emulate a topology on a single machine. It provides the needed isolation between the emulated nodes so that your router node can process and forward real Ethernet frames between the hosts like a real router. More information about the Mininet can be found here: http://yuba.stanford.edu/foswiki/bin/view/OpenFlow/Mininet

SET UP
--------
Install needed tools

    $ sudo apt-get update
    $ sudo apt-get install -y git vim-nox python-setuptools flex bison traceroute

Install Mininet

    $ git clone git://github.com/mininet/mininet
    $ cd mininet
    $ ./util/install.sh -fnv

Install POX

    $ git clone http://github.com/noxrepo/pox

Install ltprotocol

    $ git clone git://github.com/dound/ltprotocol.git
    $ cd ltprotocol 
    $ sudo python setup.py install
    
TEST THE ROUTER
----------------
Run Mininet 

    $ ./run_mininet.sh

Run POX Controller

    $ ./run_pox.sh
    
Run the router

    $ ./sr
    
Test the router using the Mininet
    
    mininet> client ping -c 3 192.168.2.2
    mininet> client traceroute -n 192.168.2.2
    mininet> client wget http://192.168.2.2
