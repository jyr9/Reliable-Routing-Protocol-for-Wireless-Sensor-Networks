// 文件路径: ns-3.40/scratch/thesis-sim.cc
// 更新测

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/energy-module.h" // 必须包含能量模块
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThesisSimulation");

int main (int argc, char *argv[])
{
  // 1. 仿真参数定义 (对应表 4-2)
  uint32_t nNodes = 50;           // 节点数量
  double totalTime = 100.0;       // 仿真时间 (秒)
  std::string phyMode ("DsssRate11Mbps");
  double initialEnergy = 100.0;   // 初始能量 (焦耳)

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nNodes", "Number of nodes", nNodes);
  cmd.AddValue ("totalTime", "Simulation time", totalTime);
  cmd.Parse (argc, argv);

  // 2. 创建节点
  NodeContainer nodes;
  nodes.Create (nNodes);

  // 3. 设置 WiFi 物理层与 MAC 层
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac"); // 设置为 Ad-hoc 模式

  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

  // 4. 设置移动模型 (Random Waypoint)
  MobilityHelper mobility;
  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
  
  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  
  mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                             "Speed", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=20.0]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"),
                             "PositionAllocator", PointerValue (taPositionAlloc));
  mobility.SetPositionAllocator (taPositionAlloc);
  
  mobility.Install (nodes);

  // 5. 安装 AODV 路由协议
  AodvHelper aodv; 
  // 在之后的小节里修改 src/aodv 的代码后，这里调用的就是修改后的协议
  
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv);
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // 6. 安装能量模型 (为模糊逻辑提供数据)
  BasicEnergySourceHelper basicSourceHelper;
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (initialEnergy));
  
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // 设置 WiFi 耗能参数 (Tx, Rx 等)
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.0197));

  EnergySourceContainer sources = basicSourceHelper.Install (nodes);
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (devices, sources);

  // 7. 设置业务流 (UDP Ping)
  // 让节点 0 发送数据给 节点 n-1
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (nNodes - 1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (totalTime));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (nNodes - 1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.25))); // 1秒4包
  echoClient.SetAttribute ("PacketSize", UintegerValue (512));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (totalTime));

  // 8. 启动仿真
  NS_LOG_INFO ("Starting Simulation...");
  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
