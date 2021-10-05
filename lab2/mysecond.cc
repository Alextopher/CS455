/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h" 
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0


using namespace ns3; //name space declaration

NS_LOG_COMPONENT_DEFINE ("SecondScriptExample"); //logging component that allows you to enable and disable console message logging by reference to the name

int main (int argc, char *argv[])//This is just the declaration of the main function of your program (script)
{
  bool verbose = true; verbose //flag to determine whether or not the UdpEchoClientApplication and UdpEchoServerApplication logging components are enabled
  uint32_t nCsma = 3; // number of ncsma nodes to create

  CommandLine cmd (__FILE__); //declare the command line for printing purposes
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);//prints information to cmd line
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);//prints information to cmd line

  cmd.Parse (argc,argv); //get the value of argc, argv from the command line

  if (verbose)// if verbose is set to true
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO); //These two lines of code enable debug logging at the INFO level for echo clients and servers/
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO); // This will result in the application printing out messages as packets are sent and received during the simulation.
    }

  nCsma = nCsma == 0 ? 1 : nCsma;// if ncsma = 0 set it to 1

  NodeContainer p2pNodes; //declare a container for ns-3 node objects
  p2pNodes.Create (2); // create the ns-3 Node objects that will represent the computers in the simulation.

  NodeContainer csmaNodes; //declare a container for ns-3 node objects
  csmaNodes.Add (p2pNodes.Get (1)); //add node one from p2p nodes to csmaNodes
  csmaNodes.Create (nCsma); // create number of nodes based on value of ncsma

  PointToPointHelper pointToPoint; //instantiates a PointToPointHelper object on the stack
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); //tells PointToPointHelper object to use the value “5Mbps” (five megabits per second) as the “DataRate” when it creates a PointToPointNetDevice object.
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms")); //tells the PointToPointHelper to use the value “2ms” (two milliseconds) as the value of the propagation delay of every point to point channel it subsequently creates.

  NetDeviceContainer p2pDevices; //a list of all of the NetDevice objects that are created by pointtopointhelpers
  p2pDevices = pointToPoint.Install (p2pNodes); //a NetDeviceContainer is created. For each node in the NodeContainer p2pNodes

  CsmaHelper csma; //instantiates a csmahelper object on the stack
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps")); //tells csmahelper object to use the value “100Mbps” (100 megabits per second) as the “DataRate” when it creates a csmaNetDevice object.
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560))); //tells the csmahelper to use the value “6560NS” (6560 nanoseconds) as the value of the propagation delay of every point to point channel it subsequently creates.

  NetDeviceContainer csmaDevices; //a list of all of the NetDevice objects that are created by csmahelpers
  csmaDevices = csma.Install (csmaNodes); //a NetDeviceContainer is created. For each node in the NodeContainer csmanodes

  InternetStackHelper stack; //a topology helper that is to internet stacks what the PointToPointHelper is to point-to-point net devices. 
  stack.Install (p2pNodes.Get (0)); //the Install method takes p2pnodes as a parameter. When it is executed, it will install an Internet Stack (TCP, UDP, IP, etc.) on each of the nodes in the node container.
  stack.Install (csmaNodes); //the Install method takes csmaNodes as a parameter. When it is executed, it will install an Internet Stack (TCP, UDP, IP, etc.) on each of the nodes in the node container.

  Ipv4AddressHelper address; //declare an address helper object and tell it that it should begin allocating IP addresses/ a list of Ipv4Interface objects
  address.SetBase ("10.1.1.0", "255.255.255.0"); //begin allocating IP addresses from the network 10.1.1.0 using the mask 255.255.255.0 to define the allocatable bits. 
                                                 //By default the addresses allocated will start at one and increase monotonically, so the first address allocated from this base will be 10.1.1.1, followed by 10.1.1.2, etc.
  Ipv4InterfaceContainer p2pInterfaces; //declare a container to hold the actual address assignments for p2pinterfaces
  p2pInterfaces = address.Assign (p2pDevices); //make the association between an IP address and a device (p2pdevices) using an Ipv4Interface object, 

  address.SetBase ("10.1.2.0", "255.255.255.0"); //begin allocating IP addresses from the network 10.1.2.0 using the mask 255.255.255.0 to define the allocatable bits. 
  Ipv4InterfaceContainer csmaInterfaces; //declare a container to hold the actual address assignments for p2pinterfaces
  csmaInterfaces = address.Assign (csmaDevices); //make the association between an IP address and a device (csmadevices) using an Ipv4Interface object,

  UdpEchoServerHelper echoServer (9); // declares the UdpEchoServerHelper

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma)); //causes the underlying echo server application to be instantiated and attached to a node /
                                                                                // install a UdpEchoServerApplication on the node found at index number ncsma of the NodeContainer  csmanodes we used to manage our csma nodes. Install will return a container that holds pointers to all of the applications
  //Applications require a time to “start” generating traffic and may take an optional time to “stop”. We provide both.
  serverApps.Start (Seconds (1.0)); //will cause the echo server application to Start (enable itself) at one second into the simulation
  serverApps.Stop (Seconds (10.0)); //Stop (disable itself) at ten seconds into the simulation.

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9); //the underlying UdpEchoClientApplication is managed by UdpEchoClientHelper.  We pass parameters to set the “RemoteAddress” and “RemotePort” Attributes in accordance with our convention to make required Attributes parameters in the helper constructors.
                                                                         //set the remote address of the client to be the IP address assigned to the node on which the server resides. We also tell it to arrange to send packets to port nine.
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1)); //tells the client the maximum number of packets we allow it to send during the simulation.
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0))); // tells the client how long to wait between packets
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024)); //tells the client how large its packet payloads should be. With this particular combination of Attributes, we are telling the client to send one 1024-byte packet.

  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (2.0));  //start the client one second after the server is enabled (at two seconds into the simulation)
  clientApps.Stop (Seconds (10.0)); //stop the client ten seconds after the server is enabled 

  Ipv4GlobalRoutingHelper::PopulateRoutingTables (); //Forces all nodes to do routing table updates

  pointToPoint.EnablePcapAll ("second"); //Tells ns-3 to capture the packets being sent in the simulation and store them to a file
  csma.EnablePcap ("second", csmaDevices.Get (1), true); //destroy all of the objects that were created.

  Simulator::Run ();  //run the simulation.
  Simulator::Destroy (); //destroy all of the objects that were created.
  return 0;
}
