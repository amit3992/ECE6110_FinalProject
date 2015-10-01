// Amit Kulkarni, Rashmi Mehere, Urvi Sharma, Rasika Subramanian
// akulkarni72
// CAD final project

#include <iostream>
#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/uinteger.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/random-variable-stream.h"
#include "ns3/constant-position-mobility-model.h"

#include "ns3/http-module.h"  

NS_LOG_COMPONENT_DEFINE ("P4");

using namespace ns3;
using namespace std;

int main (int argc, char *argv[])
{

  // Initialize variables

  uint32_t nNodes = 18; // number of nodes
  double start[7]; // Array to store the random start times
  uint32_t browsers = 428; //--> 3000/7 browsers on every node at 100% load. 

  string animFile = "P4.xml" ; // Name of file for animation output
  string linkDelay = "10ms";
  string linkDataRate = "10Mbps";
  string neckDelay = "10ms";
  string neckDataRate = "100Mbps";
  string bottleneckDelay = "10ms";
  string bottleneckDataRate = "100Mbps";
  string queueType = "DropTail";

  //DropTail parameters

  uint32_t segSize = 512; // initial value of segment size
  uint32_t queueSize = 8000; // initial value of queue size
  uint32_t windowSize = 8000; // initial value of window size --> fixed for all experiments

  //Red parameters
  double minTh = 50; // Threshold to trigger probabalistic drops
  double maxTh = 200; // Threshold to trigger forced drops
  uint32_t queueLim = 500; // Number of bytes that can be enqueued
  double load = 0.5; // Vary load according to Jeffay's paper
  double maxP = 1/20;
  double Wq = 1/128;
  
  // Allow users to override the default parameters and set it to new ones from CommandLine.
  CommandLine cmd;
  /*
   * The parameter for the p2p link
   */
  cmd.AddValue ("linkDataRate", "The data rate for the links", linkDataRate);
  cmd.AddValue ("linkDelay", "The delay for the links", linkDelay);
  cmd.AddValue ("bottleneckDataRate", "The data rate for the bottleneck link", bottleneckDataRate);
  cmd.AddValue ("bottleneckDelay", "The delay for the bottleneck link", bottleneckDelay);
  cmd.AddValue ("neckDataRate", "The data rate for the neck link", neckDataRate);
  cmd.AddValue ("neckDelay", "The delay for the neck link", neckDelay);
  cmd.AddValue ("load", "Select load", load);
  cmd.AddValue ("queueType", "Select type of bottleneck queue.", queueType);
  cmd.AddValue ("minTh","Set threshold to trigger probabalistic drops", minTh);
  cmd.AddValue ("maxTh","Set threshold to trigger forced drops", maxTh);
  cmd.AddValue ("Wq", "Weighting factor for average queue length computation", Wq);
  cmd.Parse (argc, argv);

  // Setting TCP characteristics.
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpTahoe"));
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segSize));
  Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue(windowSize));
  Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(false));

  // Setting DropTailQueue characteristics
  Config::SetDefault ("ns3::DropTailQueue::Mode", EnumValue(DropTailQueue::QUEUE_MODE_BYTES));
  Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue(queueSize));

  //Setting RedQueue characteristics
  Config::SetDefault ("ns3::RedQueue::Mode", StringValue ("QUEUE_MODE_BYTES"));
  Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (minTh));
  Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (maxTh));
  Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (queueLim));
  Config::SetDefault ("ns3::RedQueue::LInterm", DoubleValue (maxP)); //--> Inverse of max. probability.

    // Select queue type
  string setQueue;
  if (queueType == "DropTail") {
    setQueue = "ns3::DropTailQueue";
  }
  else if (queueType == "RED") {
    setQueue = "ns3::RedQueue";
  }
  else {
    NS_ABORT_MSG ("Invalid queue type: Use --queueType=RED or --queueType=DropTail");
  }


  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes;
  nodes.Create (nNodes); // Container of all nodes.

  NodeContainer routerNodes;
  routerNodes.Add(nodes.Get(0));  // Container for all the bottleneck nodes.
  routerNodes.Add(nodes.Get(1));
  routerNodes.Add(nodes.Get(2));
  routerNodes.Add(nodes.Get(3));

  NodeContainer neckNodes;
  neckNodes.Add(nodes.Get(1));  // Central link
  neckNodes.Add(nodes.Get(2));

  NodeContainer aNodes;
  aNodes.Add(nodes.Get(0));   // router link A
  aNodes.Add(nodes.Get(1));

  NodeContainer bNodes;
  bNodes.Add(nodes.Get(2));   // router link B
  bNodes.Add(nodes.Get(3));

  // Left links
  NodeContainer r0a0 = NodeContainer(nodes.Get(0), nodes.Get(4));
  NodeContainer r0a1 = NodeContainer(nodes.Get(0), nodes.Get(5));
  NodeContainer r0a2 = NodeContainer(nodes.Get(0), nodes.Get(6));
  NodeContainer r0a3 = NodeContainer(nodes.Get(0), nodes.Get(7));
  NodeContainer r0a4 = NodeContainer(nodes.Get(0), nodes.Get(8));
  NodeContainer r0a5 = NodeContainer(nodes.Get(0), nodes.Get(9));
  NodeContainer r0a6 = NodeContainer(nodes.Get(0), nodes.Get(10));

  // Right links
  NodeContainer r3b0 = NodeContainer(nodes.Get(3), nodes.Get(11));
  NodeContainer r3b1 = NodeContainer(nodes.Get(3), nodes.Get(12));
  NodeContainer r3b2 = NodeContainer(nodes.Get(3), nodes.Get(13));
  NodeContainer r3b3 = NodeContainer(nodes.Get(3), nodes.Get(14));
  NodeContainer r3b4 = NodeContainer(nodes.Get(3), nodes.Get(15));
  NodeContainer r3b5 = NodeContainer(nodes.Get(3), nodes.Get(16));
  NodeContainer r3b6 = NodeContainer(nodes.Get(3), nodes.Get(17));

  NodeContainer leftNodes;    //Container containing all the left leaf nodes
  leftNodes.Add(nodes.Get(4));
  leftNodes.Add(nodes.Get(5));
  leftNodes.Add(nodes.Get(6));
  leftNodes.Add(nodes.Get(7));
  leftNodes.Add(nodes.Get(8));
  leftNodes.Add(nodes.Get(9));
  leftNodes.Add(nodes.Get(10));

  NodeContainer rightNodes;   //Container containing all the right leaf nodes
  rightNodes.Add(nodes.Get(11));
  rightNodes.Add(nodes.Get(12));
  rightNodes.Add(nodes.Get(13));
  rightNodes.Add(nodes.Get(14));
  rightNodes.Add(nodes.Get(15));
  rightNodes.Add(nodes.Get(16));
  rightNodes.Add(nodes.Get(17));

  // This is done to set positions of all the nodes. Refer NetAnim animation for the node postions.
  for(uint32_t i = 0; i < routerNodes.GetN(); ++i) {

  Ptr<Node> node = routerNodes.Get(i);
  Ptr<ConstantPositionMobilityModel> loc = node->GetObject<ConstantPositionMobilityModel>();
  loc = CreateObject<ConstantPositionMobilityModel>();
  node->AggregateObject(loc);
  Vector locVec(i+2, 3, 0);
  loc->SetPosition(locVec);

  }

  // Add left node locations
  for(uint32_t i = 0; i < leftNodes.GetN(); ++i) {

  Ptr<Node> node = leftNodes.Get(i);
  Ptr<ConstantPositionMobilityModel> loc = node->GetObject<ConstantPositionMobilityModel>();
  loc = CreateObject<ConstantPositionMobilityModel>();
  node->AggregateObject(loc);
  Vector locVec(0, 6 - i, 0);
  loc->SetPosition(locVec);

  }

  // Add right node locations
  for(uint32_t i = 0; i < rightNodes.GetN(); ++i) {

  Ptr<Node> node = rightNodes.Get(i);
  Ptr<ConstantPositionMobilityModel> loc = node->GetObject<ConstantPositionMobilityModel>();
  loc = CreateObject<ConstantPositionMobilityModel>();
  node->AggregateObject(loc);
  Vector locVec(7, 6 - i, 0);
  loc->SetPosition(locVec);

}

  NS_LOG_INFO ("Create channels.");
  // We create the channels first without any IP addressing information

  // Link connecting router links not bottleneck link
  PointToPointHelper p2pNeck;
  p2pNeck.SetDeviceAttribute ("DataRate", StringValue (neckDataRate));
  p2pNeck.SetChannelAttribute ("Delay", StringValue (neckDelay));
  NetDeviceContainer aDevices = p2pNeck.Install (aNodes);
  NetDeviceContainer bDevices = p2pNeck.Install (bNodes);

  PointToPointHelper p2pBottleNeck;   // Link connecting bottleneck nodes.
  p2pBottleNeck.SetDeviceAttribute ("DataRate", StringValue (bottleneckDataRate));
  p2pBottleNeck.SetChannelAttribute ("Delay", StringValue (bottleneckDelay));
  p2pBottleNeck.SetQueue(setQueue);
  NetDeviceContainer bottleNeckDevices = p2pBottleNeck.Install (neckNodes);

  PointToPointHelper p2pLink;   // Link connecting edge routers and left/right leaf nodes.
  p2pLink.SetDeviceAttribute ("DataRate", StringValue (linkDataRate));
  p2pLink.SetChannelAttribute ("Delay", StringValue (linkDelay));
  NetDeviceContainer d_r0a0 = p2pLink.Install (r0a0);
  NetDeviceContainer d_r0a1 = p2pLink.Install (r0a1);
  NetDeviceContainer d_r0a2 = p2pLink.Install (r0a2);
  NetDeviceContainer d_r0a3 = p2pLink.Install (r0a3);
  NetDeviceContainer d_r0a4 = p2pLink.Install (r0a4);
  NetDeviceContainer d_r0a5 = p2pLink.Install (r0a5);
  NetDeviceContainer d_r0a6 = p2pLink.Install (r0a6);

  NetDeviceContainer d_r3b0 = p2pLink.Install (r3b0);
  NetDeviceContainer d_r3b1 = p2pLink.Install (r3b1);
  NetDeviceContainer d_r3b2 = p2pLink.Install (r3b2);
  NetDeviceContainer d_r3b3 = p2pLink.Install (r3b3);
  NetDeviceContainer d_r3b4 = p2pLink.Install (r3b4);
  NetDeviceContainer d_r3b5 = p2pLink.Install (r3b5);
  NetDeviceContainer d_r3b6 = p2pLink.Install (r3b6);

  // Create vector for assigning addresses
  NetDeviceContainer devicesArray[] = {aDevices,bDevices,bottleNeckDevices,d_r0a0,d_r0a1,d_r0a2,d_r0a3,d_r0a4,d_r0a5,d_r0a6,d_r3b0,d_r3b1,d_r3b2,d_r3b3,d_r3b4,d_r3b5,d_r3b6};
  std::vector<NetDeviceContainer> devices(devicesArray, devicesArray + sizeof(devicesArray) / sizeof(NetDeviceContainer));
  
  InternetStackHelper internet;
  internet.Install (nodes);   

  NS_LOG_INFO ("Setting the address");
  std::vector<Ipv4InterfaceContainer> interfaces(nNodes-1);
  Ipv4AddressHelper address;

  for(uint32_t i = 0; i < devices.size(); ++i)
{
  std::ostringstream mask;
  mask << "10.1." << i+1 << ".0";
  address.SetBase(mask.str().c_str(), "255.255.255.0");
  interfaces[i] = address.Assign(devices[i]);
}

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

 RngSeedManager::SetSeed(1234); // Assign random seed
 Ptr<UniformRandomVariable> rv = CreateObject<UniformRandomVariable> ();
 rv->SetAttribute ("Stream", IntegerValue (6110));
 rv->SetAttribute ("Min", DoubleValue (0.0));
 rv->SetAttribute ("Max", DoubleValue (0.01));

 for (uint i = 0; i < 7; i++)
 {
    start[i] = rv->GetValue(); // Save the start times for the flows in start array.
 }

  NS_LOG_INFO ("Installing Applications");
  uint16_t port = 9;

 HttpHelper httpHelper[7];
 ApplicationContainer clientApp[500];
 ApplicationContainer serverApp[500];
 uint32_t apps = load * browsers;

 for (uint32_t i = 0; i < 7; ++i )
{
   	for(uint32_t j = 0; j < apps; j++)
	//for(uint32_t j = 0; j < 2; j++)  // --> Limiting number of browsers on each node for debugging
   {
	HttpServerHelper httpServer;
   	httpServer.SetAttribute ("Local", AddressValue (InetSocketAddress (Ipv4Address::GetAny (), port+j)));
   	httpServer.SetAttribute ("HttpController", PointerValue (httpHelper[i].GetController ()));
   	serverApp[j] = httpServer.Install (rightNodes.Get (i));
   	serverApp[j].Start (Seconds (start[i]));
   	serverApp[j].Stop (Seconds (400.0));


   	HttpClientHelper httpClient;
   	httpClient.SetAttribute ("Peer", AddressValue (InetSocketAddress (interfaces[10 + i].GetAddress (1), port+j)));
   	httpClient.SetAttribute ("HttpController", PointerValue (httpHelper[i].GetController ()));
   	clientApp[j] = httpClient.Install (leftNodes.Get (i));
   	clientApp[j].Start (Seconds (start[i]));
   	clientApp[j].Stop (Seconds (400.0));
   }

}


  AnimationInterface animInterface(animFile);

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (400.0));
  Simulator::Run ();
  Simulator::Destroy ();

    cout<<"\nSimulation parameters: "<<endl;
    cout<<"Nodes: "<<nNodes<<endl;
    cout<<"Load: "<<load<<endl;
    cout<<"queueType: "<<queueType<<endl;
    cout<<"Bandwidth of bottle-neck link: "<<bottleneckDataRate<<endl;
    cout<<"Delay of bottle-neck link: "<<bottleneckDelay<<endl;
    
    cout<<" "<<endl;

    if (queueType == "DropTail") 
    {
      cout<<"DropTail Queue Parameters"<<endl;
      cout<<"SegSize: " <<segSize<<endl;
      cout<<"queueSize: "<<queueSize<<endl;
      cout<<"windowSize: "<<windowSize<<endl;

    } 
    
    else if (queueType == "RED")
    {
      cout<<"\nRED Queue Parameters"<<endl;
      cout<<"Threshold to trigger probablistic drops (minTh): " <<minTh<<endl;
      cout<<"Threshold to trigger forced drops (maxTh): "<<maxTh<<endl;
      cout<<"queueLim: "<<queueLim<<endl;
    }

  return 0;
}
