/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/dvhop-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include <iostream>
#include <cmath>
#include <random>

using namespace ns3;


struct point 
{
    double x,y;
};

float norm (point p) // get the norm of a vector
{
    return pow(pow(p.x,2)+pow(p.y,2),.5);
}

point trilateration(point point1, point point2, point point3, double r1, double r2, double r3) {
    point resultPose;
    //unit vector in a direction from point1 to point 2
    double p2p1Distance = pow(pow(point2.x-point1.x,2) + pow(point2.y - point1.y,2),0.5);
    point ex = {(point2.x-point1.x)/p2p1Distance, (point2.y-point1.y)/p2p1Distance};
    point aux = {point3.x-point1.x,point3.y-point1.y};
    //signed magnitude of the x component
    double i = ex.x * aux.x + ex.y * aux.y;
    //the unit vector in the y direction. 
    point aux2 = { point3.x-point1.x-i*ex.x, point3.y-point1.y-i*ex.y};
    point ey = { aux2.x / norm (aux2), aux2.y / norm (aux2) };
    //the signed magnitude of the y component
    double j = ey.x * aux.x + ey.y * aux.y;
    //coordinates
    double x = (pow(r1,2) - pow(r2,2) + pow(p2p1Distance,2))/ (2 * p2p1Distance);
    double y = (pow(r1,2) - pow(r3,2) + pow(i,2) + pow(j,2))/(2*j) - i*x/j;
    //result coordinates
    double finalX = point1.x+ x*ex.x + y*ey.x;
    double finalY = point1.y+ x*ex.y + y*ey.y;
    resultPose.x = finalX;
    resultPose.y = finalY;
    return resultPose;
}

/**
 * \brief Test script.
 *
 * This script creates a random 2-dimensional grid topology
 *
 */
class DVHopExample
{
public:
  DVHopExample ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /// Report results
  void Report (std::ostream & os);

private:
  ///\name parameters
  //\{
  /// Number of nodes
  uint32_t size;
  /// How many nodes are beacons (must be less than size)
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;
  //\}

  ///\name network
  //\{
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
  //\}

private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();
  void CreateBeacons();
  void Kill();
  void DV();
};

const uint32_t beacons = 10;

int main (int argc, char **argv)
{
  DVHopExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();
  test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
DVHopExample::DVHopExample () :
  size (20),
  totalTime (10),
  pcap (true),
  printRoutes (false)
{
}

bool
DVHopExample::Configure (int argc, char **argv)
{
  // Enable DVHop logs by default. Comment this if too noisy
  LogComponentEnable("DVHopRoutingProtocol", LOG_LEVEL_ERROR);

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);

  cmd.Parse (argc, argv);
  return true;
}

void
DVHopExample::Run ()
{
  //Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  CreateBeacons();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  Simulator::Stop (Seconds (totalTime));

  AnimationInterface anim("animation.xml");

  Simulator::Run ();
  DV();
  Simulator::Destroy ();
}

void
DVHopExample::Kill() {
  // pick 2 random nodes and kill them

  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<int> uni(beacons, size); // guaranteed unbiased

  auto r1 = uni(rng);
  auto r2 = uni(rng);

  Ptr<Ipv4RoutingProtocol> proto = nodes.Get(r1) -> GetObject<Ipv4>() -> GetRoutingProtocol ();
  Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
  dvhop -> Kill();

  proto = nodes.Get(r2) -> GetObject<Ipv4>() -> GetRoutingProtocol ();
  dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
  dvhop -> Kill();
}


void
DVHopExample::Report (std::ostream &)
{
}

void
DVHopExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes\n";
  nodes.Create (size);
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      std::cout << "Creating node: "<< os.str ()<< std::endl ;
      Names::Add (os.str (), nodes.Get (i));
    }
  
  // x,y values
  Ptr<UniformRandomVariable> xs = CreateObject<UniformRandomVariable> ();
  xs->SetAttribute ("Max", DoubleValue (100));
  Ptr<UniformRandomVariable> ys = CreateObject<UniformRandomVariable> ();
  ys->SetAttribute ("Max", DoubleValue (100));
  
  Ptr<ns3::RandomRectanglePositionAllocator> allocator = CreateObject<ns3::RandomRectanglePositionAllocator> ();
  allocator -> SetX(xs);
  allocator -> SetY(ys);

  // Setup in a random grid
  MobilityHelper mobility;
  mobility.SetPositionAllocator(allocator);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel", "");
  mobility.Install (nodes);
}

void
DVHopExample::CreateBeacons ()
{
  // Promote the first 4 nodes to beacons
  for (uint32_t i = 0; i < beacons; i++) {
    ns3::Ptr<ns3::Node> node = nodes.Get(i);
    Vector position = node -> GetObject<MobilityModel> () -> GetPosition();

    Ptr<Ipv4RoutingProtocol> proto = node -> GetObject<Ipv4>() -> GetRoutingProtocol ();
    Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
    dvhop->SetIsBeacon (true);
    dvhop->SetPosition (position.x, position.y);
  }

  // // Increase the position of all nodes 10x
  //   for (int i = 0; i < 4; i++) {
  //   ns3::Ptr<ns3::Node> node = nodes.Get(i);
  //   Vector position = node -> GetObject<MobilityModel> () -> GetPosition();
  //   Vector newposition = Vector(position.x * 20, position.y * 20, position.z);
  //   node -> GetObject<MobilityModel> () -> SetPosition(newposition);

  //   Ptr<Ipv4RoutingProtocol> proto = node -> GetObject<Ipv4>() -> GetRoutingProtocol ();
  //   Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
  //   dvhop->SetPosition (newposition.x, newposition.y);
  // }
}


void
DVHopExample::CreateDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes);

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
}

void
DVHopExample::InstallInternetStack ()
{
  DVHopHelper dvhop;
  // you can configure DVhop attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (dvhop); // has effect on the next Install ()
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);

  Ptr<OutputStreamWrapper> distStream = Create<OutputStreamWrapper>("dvhop.distances", std::ios::out);
  dvhop.PrintDistanceTableAllAt(Seconds(9), distStream);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("dvhop.routes", std::ios::out);
      dvhop.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}

void DVHopExample::DV () {
  // 10.0.0.1 -> 10.0.0.beacons are beacons
  std::map<Ipv4Address, float> hopsize;
  uint32_t i = 0;

  // Calculate expected size per hop
  for (i = 0; i < beacons; i++) {
    Ptr<Ipv4RoutingProtocol> proto = nodes.Get(i) -> GetObject<Ipv4>() -> GetRoutingProtocol ();
    Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
    ns3::dvhop::DistanceTable table = dvhop -> GetDistanceTable();
    std::map<Ipv4Address, ns3::dvhop::BeaconInfo> inner = table.Inner();

    double x1 = dvhop -> GetXPosition();
    double y1 = dvhop -> GetYPosition();

    int hops = 0;
    double sum = 0;

    for (const auto& kv : inner) {
      ns3::dvhop::BeaconInfo info = kv.second;

      double x2 = info.GetPosition().first;
      double y2 = info.GetPosition().second;

      sum += sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
      hops += info.GetHops();
    }

    Ipv4Address ipv4 = (dvhop -> GetIpv4() -> GetAddress(1, 0)).GetAddress();

    if (hops == 0) {
      hopsize[ipv4] = 0;
    } else {
      hopsize[ipv4] = sum / hops;
    }
  }

  // Each node now tries to trilateral
  double error = 0;
  int count = 0;

  for (i = beacons; i < size; i++) {
    auto node = nodes.Get(i);
    Ptr<Ipv4RoutingProtocol> proto = node -> GetObject<Ipv4>() -> GetRoutingProtocol ();
    Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
    ns3::dvhop::DistanceTable table = dvhop -> GetDistanceTable();
    std::map<Ipv4Address, ns3::dvhop::BeaconInfo> inner = table.Inner();

    // We can't trilaterate with less than 3 nodes
    if (inner.size() >= 3) {
      auto itr = inner.begin();

      double xa = itr->second.GetPosition().first;
      double ya = itr->second.GetPosition().second;

      // hops size * hops to node
      double ra = hopsize[itr -> first] * itr->second.GetHops();

      itr++;

      double xb = itr->second.GetPosition().first;
      double yb = itr->second.GetPosition().second;

      // hops size * hops to node
      double rb = hopsize[itr -> first] * itr->second.GetHops();

      itr++;
      
      double xc = itr->second.GetPosition().first;
      double yc = itr->second.GetPosition().second;

      // hops size * hops to node
      double rc = hopsize[itr -> first] * itr->second.GetHops();

      point pa = {xa, ya};
      point pb = {xb, yb};
      point pc = {xc, yc};
      point final = trilateration(pa, pb, pc, ra, rb, rc);

      Vector position = node -> GetObject<MobilityModel> () -> GetPosition();

      // Add distance to the error
      error += sqrt((final.x - position.x) * (final.x - position.x) + (final.y - position.y) * (final.y - position.y));
      count++;
  
      std::cout << final.x << "," << final.y << " | " << position.x << "," << position.y << std::endl;       
    }
  }

  std::cout << error << " | " << count << std::endl;
}
