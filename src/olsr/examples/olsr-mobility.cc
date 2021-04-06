#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/olsr-helper.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Olsr-mobility");


#include <cmath>
#include "ns3/point-to-point-module.h"
#include "ns3/v4ping-helper.h"
#include <sstream>
#include "ns3/ns2-mobility-helper.h"
#include "ns3/netanim-module.h"


class OlsrExample 
{
public:
  OlsrExample ();
  /**
   * \brief Configure script parameters
   * \param argc is the command line argument count
   * \param argv is the command line arguments
   * \return true on successful configuration
  */
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /**
   * Report results
   * \param os the output stream
   */
  void Report (std::ostream & os);

  void closeLogStream ();  //---------
private:

  // parameters
  /// Number of nodes
  uint32_t size;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;

  double start;
  double end;

  
  std::string traceFile;    //---------
  std::string logFile;   //---------
  std::ofstream oss;   //---------

  // network
  /// nodes used in the example
  NodeContainer nodes;
  /// devices used in the example
  NetDeviceContainer devices;
  /// interfaces used in the example
  Ipv4InterfaceContainer interfaces;

private:
  /// Create the nodes
  void CreateNodes ();
  /// Create the devices
  void CreateDevices ();
  /// Create the network
  void InstallInternetStack ();
  /// Create the simulation applications
  void InstallApplications ();
  
};

int main (int argc, char **argv)
{
  OlsrExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");
  
  // Enable logging from the ns2 helper
  LogComponentEnable ("Ns2MobilityHelper",LOG_LEVEL_DEBUG);  //---------

  test.Run ();
  test.Report (std::cout);
  // close log file
  test.closeLogStream ();  //---------
  return 0;
}

//-----------------------------------------------------------------------------
OlsrExample::OlsrExample () :
  size (11),
  totalTime (3600),
  pcap (false),
  printRoutes (false),
  start (25.78),
  end (25.78)
{
}

void OlsrExample::closeLogStream ()  //---------
{
		oss.close ();
}


bool
OlsrExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);

  SeedManager::SetSeed (12345);
  CommandLine cmd;
  
  cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile); //---------
  cmd.AddValue ("start", "Minimum available transmission level (dbm).", start);
  cmd.AddValue ("end", "Maximum available transmission level (dbm).", end);  
  cmd.AddValue ("logFile", "Log file", logFile);  //---------
  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);



  cmd.Parse (argc, argv);
  return true;
}

void
OlsrExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

//  AnimationInterface anim ("DPR_olsr.xml");
//  anim.SetMaxPktsPerTraceFile(9000000);
//  anim.SetMobilityPollInterval (Seconds (1));

  
  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
OlsrExample::Report (std::ostream &)
{ 
}

//---------
static void
CourseChange (std::ostream *os, std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition (); // Get position
  Vector vel = mobility->GetVelocity (); // Get velocity

  // Prints position and velocities
  *os << Simulator::Now () << " POS: x=" << pos.x << ", y=" << pos.y
      << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
      << ", z=" << vel.z << std::endl;
}


void
OlsrExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes\n";
  nodes.Create (size);
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }
	
  // Create Ns2MobilityHelper with the specified trace log file as parameter
  Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);   //---------
  
  oss.open (logFile.c_str ());  //---------
  
// configure movements for each node, while reading trace file
  ns2.Install ();    //---------

  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeBoundCallback (&CourseChange, &oss));
}

void
OlsrExample::CreateDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
 // devices = wifi.Install (wifiPhy, wifiMac, nodes); 
  devices = wifi.InstallCustom (wifiPhy, wifiMac, nodes, start, end); 
  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("olsr"));
    }
}

void
OlsrExample::InstallInternetStack ()
{
  OlsrHelper olsr;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (olsr); // has effect on the next Install ()
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("RDPZ_olsr.routes", std::ios::out);
      for (uint32_t i = 0; i < totalTime/100; i++)
      {
      olsr.PrintRoutingTableAllAt (Seconds (i*100), routingStream);
      }
    }
}

void
OlsrExample::InstallApplications ()
{
  V4PingHelper ping (interfaces.GetAddress (size - 1));
  ping.SetAttribute ("Verbose", BooleanValue (true));


  for(uint32_t i = 0; i < size - 1; i++)
  {
    ApplicationContainer p = ping.Install (nodes.Get (i));
    p.Start (Seconds (0));
    p.Stop (Seconds (totalTime) - Seconds (0.001));
  }
}
