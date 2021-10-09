#include "ns3/core-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/animation-interface.h"
#include "ns3/mobility-helper.h"

#include "ns3/traffic-control-helper.h"
#include "ns3/net-device-container.h"

#include <string>

using namespace ns3;


/*
    A: 0
    B: 1
    C: 2
    D: 3
    E: 4
    F: 5
    G: 6
    S: 7
*/

class Network {
public:
  NodeContainer nodeContainer;
  InternetStackHelper stack;
  Ipv4AddressHelper adress;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
  TrafficControlHelper tch;
  QueueDiscContainer qdiscs;

  uint32_t i = 0;
  
  uint16_t port_number = 9;  
  ApplicationContainer server_apps;

private:
  uint32_t ip_it = 1;

  std::vector<Ptr<Socket>> sources;
  std::vector<InetSocketAddress> remotes;

public:
  Network(uint32_t nodes, double simTime, double appStartDelay);

  void addP2PLink(const std::string& dataRate, uint32_t nodeIdA, uint32_t nodeIdB);

  Ptr<UdpServer> addUdpServer(uint32_t nodeId);

  Ptr<Socket> createConnection(uint32_t nodeId);

  Ptr<Socket> createConnection(uint32_t nodeId, uint32_t destinationAdressId);

  void addQueues();
};
