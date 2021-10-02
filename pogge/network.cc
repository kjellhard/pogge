#include "network.h"

Network::Network(uint32_t nodes, double simTime, double appStartDelay) 
{
    nodeContainer.Create(nodes);
    stack.Install(nodeContainer);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    MobilityHelper help;
    help.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    help.InstallAll();

    server_apps.Start(Seconds (appStartDelay));
    server_apps.Stop(Seconds (simTime+appStartDelay));
}

void Network::addP2PLink(const std::string& dataRate, uint32_t nodeIdA, uint32_t nodeIdB)
{
    PointToPointHelper p2pHelper;
    p2pHelper.SetDeviceAttribute("DataRate", StringValue(dataRate));
    p2pHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1000p"));

    devices.Add(p2pHelper.Install(nodeContainer.Get(nodeIdA), nodeContainer.Get(nodeIdB)));

    std::string ip = "10.1." + std::to_string(ip_it++) + ".0";

    adress.SetBase(ip.c_str(), "255.255.255.0");
    //interfaces = adress.Assign(devices);
    interfaces.Add(adress.Assign(devices));
}

Ptr<UdpServer> Network::addUdpServer(uint32_t nodeId) 
{
    UdpServerHelper server(port_number);
    server_apps.Add(server.Install(nodeContainer.Get(nodeId)));

    return server.GetServer();
}

Ptr<Socket> Network::createConnection(uint32_t nodeId)
{
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

    Ptr<Socket> source = Socket::CreateSocket(nodeContainer.Get(nodeId), tid);

    sources.push_back(source);

    return source;
}

Ptr<Socket> Network::createConnection(uint32_t nodeId, uint32_t destinationAdressId)
{
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

    Ptr<Socket> source = Socket::CreateSocket (nodeContainer.Get(nodeId), tid);
    InetSocketAddress remote = InetSocketAddress (interfaces.GetAddress(destinationAdressId), port_number);
    source->Connect (remote);

    sources.push_back(source);
    remotes.push_back(remote);

    return source;
}
