Notice that following steps should be done on all you nodes.
 
1. install Java 6
 
2. install zookeeper
>1. download latest stable zookeeper package from http://zookeeper.apache.org/releases.html#download
 
>2. extract files from package, put them into where you want to install  zookeeper
 
>3. go to ${zookeeper_install_dir}/conf, edit zoo.cfg:
tickTime=2000
dataDir=${zk_data_dir}
clientPort=2181
initLimit=5
syncLimit=2
server.1=${zk_host1}:2888:3888
server.2=${zk_host2}:2888:3888
server.3=${zk_host3}:2888:3888
 
>4. add a text file named as "myid" in ${zk_data_dir}, only have a number to specify the id of current node. For example, for ${zk_host1}, the content will be "1".
 
>5. go to ${zookeeper_install_dir}/bin, start zookeeper:
./zkServer.sh start
 
3. install python 2.6.6
 
4. install storm
>1. download storm package:
wget http://mirror.apache-kr.org/storm/apache-storm-0.9.4/apache-storm-0.9.4.tar.gz
 
>2. extract files from package, put them into where you want to install  storm
 
>3. go to ${storm_install_dir}/conf, edit following properties in storm.yaml
storm.zookeeper.servers: This is a list of the hosts in the Zookeeper cluster for your Storm cluster.
storm.zookeeper.servers:
  - "dev-abd-001.ncl"
  - "dev-abd-002.ncl"
storm.local.dir: The Nimbus and Supervisor daemons require a directory on the local disk to store small amounts of state.
nimbus.host: The worker nodes need to know which machine is the master in order to download topology jars and confs.
supervisor.slots.ports: For each worker machine, you configure how many workers run on that machine with this config. Each worker uses a single port for receiving messages, and this setting defines which ports are open for use.
>4. start supervisor
./storm supervisor &
 
>5. start nimbus on nimbus host
./storm supervisor &
 
>6. start UI on nimbus host
./storm ui &
then you can visit http://${nimbus_host}:8080/index.html to view diagnostics on the cluster and topologies.
 
>7. submit a example toplogies for test:
./storm jar ../examples/storm-starter/storm-starter-topologies-0.9.4.jar storm.starter.WordCountTopology word_count