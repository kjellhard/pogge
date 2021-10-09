#include "ns3/animation-interface.h"
#include "ns3/flow-monitor-helper.h"

#include <iostream>
#include <fstream>

#include "network.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ScratchSimulator");



class LinearCongruentialGenerator {

public:
  const uint32_t m;
  const uint32_t a;
  const uint32_t c;
  uint32_t prev;

  LinearCongruentialGenerator(uint32_t m, uint32_t a, uint32_t c, uint32_t seed = 0)
  : m(m), a(a), c(c), prev(seed) {}

  uint32_t gen(){
    prev = (a*prev + c) % m;
    return prev;
  }
};

double exp_dist_from_uniform(double lambda, double u)
{
  double ret = -log(u) / lambda;
  return ret;
}

void runPRNGtest() 
{
  const uint32_t m = 100;
  const uint32_t a = 21;
  const uint32_t c = 1;
  const uint32_t seed = 1;
  LinearCongruentialGenerator lcg(m, a, c, seed);

  Ptr<ExponentialRandomVariable> erv = CreateObject<ExponentialRandomVariable>();

  std::ofstream lcg_file;
  lcg_file.open ("lcg_exp_output.txt", std::fstream::in | std::fstream::out | std::fstream::trunc);

  std::ofstream erv_file;
  erv_file.open("erv_output.txt", std::fstream::in | std::fstream::out | std::fstream::trunc);

  const double lambda = 50;

  for(int i = 0; i < 1000; i++){
    //double rn;
    // rn = static_cast<double>(lcg.gen()) / static_cast<double>(m);
    //rn = urv->GetValue(0.0, 1.0);
    //std::cout << rn << "\n";

    double lcg_rn = 1-(static_cast<double>(lcg.gen()) / static_cast<double>(m));
    lcg_file << exp_dist_from_uniform(lambda, lcg_rn) << "\n";
    //lcg_file << lcg_rn << "\n";
    erv_file << erv->GetValue(1.0/lambda, 1.0) << "\n";
  }

  lcg_file.close();
  erv_file.close();
}


static void received_msg (Ptr<Socket> socket1, Ptr<Socket> socket2, Ptr<const Packet> p, const Address &srcAddress , const Address &dstAddress)
{
	//std::cout << "::::: A packet received at the Server! Time:   " << Simulator::Now ().GetSeconds () << std::endl;
	
	Ptr<UniformRandomVariable> rand=CreateObject<UniformRandomVariable>();
	
  p->GetUid();

	if(rand->GetValue(0.0,1.0)<=0.7)
  {
    //std::cout << "::::: Transmitting from GW to Controller   "  << std::endl;
		socket2->SendTo(Create<Packet>(p->GetSize ()),0,srcAddress);
	}
	else
  {
		//std::cout << "::::: Transmitting from Server to Router   "  << std::endl;
		socket1->Send (Create<Packet>(p->GetSize ()));
	}
}

int totalPackets = 0;
int smallPackets = 0;
int addedBytes = 0;

static void GenerateTraffic (Ptr<Socket> socket, Ptr<ExponentialRandomVariable> randomSize,	Ptr<ExponentialRandomVariable> randomTime)
{
	uint32_t pktSize = randomSize->GetInteger (); //Get random value for packet size
	// std::cout << "::::: A packet is generate at Node "<< socket->GetNode ()->GetId () << " with size " << pktSize << " bytes ! Time:   " << Simulator::Now ().GetSeconds () << std::endl;
	
	// We make sure that the message is at least 12 bytes. The minimum length of the UDP header. We would get error otherwise.
	if(pktSize<12){
    smallPackets++;

    addedBytes += (12-pktSize);

		pktSize=12;
	}
  
  //std::cout << "Packet size: " << pktSize << " created packet size: " << Create<Packet>(pktSize)->GetSize() << "\n";

	socket->Send (Create<Packet> (pktSize));

  totalPackets++;

	Time pktInterval = Seconds(randomTime->GetValue ()); //Get random value for next packet generation time 
	Simulator::Schedule (pktInterval, &GenerateTraffic, socket, randomSize, randomTime); //Schedule next packet generation
}

void SchedulePackets(Ptr<Socket> source, double meanTime, double meanSize, double startDelay) {
  
  Ptr<ExponentialRandomVariable> randomTime = CreateObject<ExponentialRandomVariable> ();
  randomTime->SetAttribute ("Mean", DoubleValue (meanTime));
   
   Ptr<ExponentialRandomVariable> randomSize = CreateObject<ExponentialRandomVariable> ();
   randomSize->SetAttribute ("Mean", DoubleValue (meanSize));

   Simulator::ScheduleWithContext(source->GetNode()->GetId(), Seconds(startDelay), &GenerateTraffic, source, randomSize, randomTime);
}

int main (int argc, char *argv[])
{
  //runPRNGtest();

  //RngSeedManager::SetSeed(12);

  const uint32_t nodeCount = 9;

  const double simTime = 1.0; //Seconds
  const double startDelay = 1.0; //Seconds

  Network network(nodeCount, simTime, startDelay);

  network.addP2PLink("5Mbps", 0, 4);
  network.addP2PLink("5Mbps", 1, 5);
  network.addP2PLink("5Mbps", 2, 5);
  network.addP2PLink("5Mbps", 3, 6);
  network.addP2PLink("5Mbps", 4, 6);
  network.addP2PLink("8Mbps", 5, 6);
  network.addP2PLink("10Mbps", 6, 7);
  network.addP2PLink("8Mbps", 6, 8);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  Ptr<Socket> sourceA = network.createConnection(0, 13);
  Ptr<Socket> sourceB = network.createConnection(1, 13);
  Ptr<Socket> sourceC = network.createConnection(2, 13);
  Ptr<Socket> sourceD = network.createConnection(3, 13);

  Ptr<Socket> routerSource = network.createConnection(7, 15);
  Ptr<Socket> clientsSource = network.createConnection(7);

  network.addUdpServer(0);
  network.addUdpServer(1);
  network.addUdpServer(2);
  network.addUdpServer(3);
  network.addUdpServer(8);


  Ptr<UdpServer> server = network.addUdpServer(7);

  server->TraceConnectWithoutContext("RxWithAddresses", MakeBoundCallback (&received_msg, routerSource, clientsSource));


  AnimationInterface anim("network.xml");
  anim.SetConstantPosition(network.nodeContainer.Get(0), -80.0, -20.0);
  anim.SetConstantPosition(network.nodeContainer.Get(1), -80.0, 20.0);
  anim.SetConstantPosition(network.nodeContainer.Get(2), -40.0, 60.0);
  anim.SetConstantPosition(network.nodeContainer.Get(3), 0.0, 40.0);
  anim.SetConstantPosition(network.nodeContainer.Get(4), -40.0, -20.0);
  anim.SetConstantPosition(network.nodeContainer.Get(5), -40.0, 20.0);
  anim.SetConstantPosition(network.nodeContainer.Get(6), 0.0, 0.0);
  anim.SetConstantPosition(network.nodeContainer.Get(7), 0.0, -20.0);
  anim.SetConstantPosition(network.nodeContainer.Get(8), 50.0, 0.0);

  const int meanPacketSize = 100;

  SchedulePackets(sourceA, 0.002, meanPacketSize, startDelay);
  SchedulePackets(sourceB, 0.002, meanPacketSize, startDelay);
  SchedulePackets(sourceC, 0.0005, meanPacketSize, startDelay);
  SchedulePackets(sourceD, 0.001, meanPacketSize, startDelay);

  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowmonHelper;
  flowMonitor = flowmonHelper.InstallAll();

  std::cout<<"Network "<<network.interfaces.GetN()<<'\n';
  
  Simulator::Stop(Seconds (startDelay+simTime));

  Simulator::Run();

  Simulator::Destroy();
  flowMonitor->SerializeToXmlFile ("network.flowmon.xml", false, false);

  std::cout << "Total packets: " << totalPackets << "\n";
  std::cout << "Packets / s: " << totalPackets / simTime << "\n";
  std::cout << "Number of packets under 12 byte: " << smallPackets << ", extra added bytes: " << addedBytes << " bytes \n";

  //NS_LOG_INFO ("Create Applications.");

  // Create a UdpServer application on node Server (S).
  /*uint16_t port_number = 9;  
  ApplicationContainer server_apps;
  UdpServerHelper serverS (port_number);
  server_apps.Add(serverS.Install(network.nodeContainer.Get (7)));
  Ptr<UdpServer> S1 = serverS.GetServer();*/

  //
  // Create a UdpServer application on node A,B,C,D to receive the reply from the server.
  //
  // UdpServerHelper server (port_number);
  // server_apps.Add(server.Install(network.nodeContainer.Get (0)));
  // server_apps.Add(server.Install(network.nodeContainer.Get (1)));
  // server_apps.Add(server.Install(network.nodeContainer.Get (2)));
  // server_apps.Add(server.Install(network.nodeContainer.Get (3)));


  // TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  //  //Transmission Server (S)-> Router (R)
  // Ptr<Socket> sourceRouter = Socket::CreateSocket (network.nodeContainer.Get(7), tid);
  // InetSocketAddress remoteRouter = InetSocketAddress (network.interfaces.GetAddress (15), network.port_number);
  // sourceRouter->Connect (remoteRouter);

  // //Transmission Server (S) -> Client (A or B)
  // Ptr<Socket> sourceClients = Socket::CreateSocket (network.nodeContainer.Get(7), tid);


  // Ptr<Socket> sourceA = Socket::CreateSocket (network.nodeContainer.Get(0), tid);
  // InetSocketAddress remoteA = InetSocketAddress (network.interfaces.GetAddress(11), network.port_number);
  // sourceA->Connect (remoteA);

  // Ptr<Socket> sourceB = Socket::CreateSocket (network.nodeContainer.Get(1), tid);
  // InetSocketAddress remoteB = InetSocketAddress (network.interfaces.GetAddress(11), network.port_number);
  // sourceB->Connect (remoteB);

  // Ptr<Socket> sourceC = Socket::CreateSocket (network.nodeContainer.Get(2), tid);
  // InetSocketAddress remoteC = InetSocketAddress (network.interfaces.GetAddress(11), network.port_number);
  // sourceC->Connect (remoteC);

  // Ptr<Socket> sourceD = Socket::CreateSocket (network.nodeContainer.Get(3), tid);
  // InetSocketAddress remoteD = InetSocketAddress (network.interfaces.GetAddress(11), network.port_number);
  // sourceD->Connect (remoteD);
}
