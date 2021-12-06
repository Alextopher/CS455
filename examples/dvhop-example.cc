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

const uint32_t beacons = 20;

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
  size (50),
  totalTime (50),
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

  Simulator::Schedule (Seconds(5), &DVHopExample::Kill);
  Simulator::Schedule (Seconds(10), &DVHopExample::Kill);
  Simulator::Schedule (Seconds(15), &DVHopExample::Kill);
  Simulator::Schedule (Seconds(20), &DVHopExample::Kill);
  Simulator::Schedule (Seconds(25), &DVHopExample::Kill);
  Simulator::Schedule (Seconds(30), &DVHopExample::Kill);
  Simulator::Schedule (Seconds(35), &DVHopExample::Kill);
  Simulator::Schedule (Seconds(40), &DVHopExample::Kill);
  Simulator::Schedule (Seconds(45), &DVHopExample::Kill);
  Simulator::Schedule (Seconds(50), &DVHopExample::Kill);

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

  Ptr<Ipv4RoutingProtocol> proto = nodes.Get(r2) -> GetObject<Ipv4>() -> GetRoutingProtocol ();
  Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
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
  xs->SetAttribute ("Max", DoubleValue (400));
  Ptr<UniformRandomVariable> ys = CreateObject<UniformRandomVariable> ();
  ys->SetAttribute ("Max", DoubleValue (400));
  
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

    Ipv4Address ipv4 = (dvhop -> GetIpv4() -> GetAddress(0, 0)).GetAddress();

    if (hops == 0) {
      hopsize[ipv4] = 0;
    } else {
      hopsize[ipv4] = sum / hops;
    }
  }

  // Each node now tries to trilateral
  for (i = beacons; i < size; i++) {
    Ptr<Ipv4RoutingProtocol> proto = nodes.Get(i) -> GetObject<Ipv4>() -> GetRoutingProtocol ();
    Ptr<dvhop::RoutingProtocol> dvhop = DynamicCast<dvhop::RoutingProtocol> (proto);
    ns3::dvhop::DistanceTable table = dvhop -> GetDistanceTable();
    std::map<Ipv4Address, ns3::dvhop::BeaconInfo> inner = table.Inner();

    // We can't trilaterate with less than 3 nodes
    if (inner.size() >= 3) {
      auto a = inner.begin();
      double xa = a->second.GetPosition().first;
      double ya = a->second.GetPosition().second;

      // hops size * hops to node
      double ra = hopsize[a -> first] * a->second.GetHops();

      auto b = inner.begin();
      double xb = b->second.GetPosition().first;
      double yb = b->second.GetPosition().second;

      // hops size * hops to node
      double rb = hopsize[b -> first] * b->second.GetHops();

      auto c = inner.begin();
      double xc = b->second.GetPosition().first;
      double yc = b->second.GetPosition().second;

      // hops size * hops to node
      double rc = hopsize[a -> first] * a->second.GetHops();

      double S = (xc * xc - xb * xb + yc * yc - yb * yb + rb * rb + rc * rc) / 2.0;
      double T = (xa * xa - xb * xb + ya * ya - yb * yb + rb * rb - ra * ra) / 2.0;

      double y = ((T * (xb - xc)) - (S * (xb - xa))) / (((ya - yb) * (xb - xc)) - ((yc - yb) * (xb - xa)));
      double x = ((y * (ya - yb)) - T) / (xb - xa);

      std::cout << x << "," << y << "|" << dvhop -> GetXPosition() << "," << dvhop -> GetYPosition();      
    }
  }
}
