/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <cmath>

#include "ns3/network-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/ipv4.h"

#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/address-utils.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/mobility-module.h"
#include "ns3/mobility-model.h"

#include "tracker.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Tracker");
NS_OBJECT_ENSURE_REGISTERED (Tracker);

TypeId
Tracker::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::Tracker")
		.SetParent<Application> ()
		.AddConstructor<Tracker> ()
		.AddAttribute ("Port", "Port on which we listen for incoming packets.",
						UintegerValue (9),
						MakeUintegerAccessor (&Tracker::ms_port),
						MakeUintegerChecker<uint16_t> ())
		.AddAttribute ("MaxPackets",
						"The maximum number of packets the application will send",
						UintegerValue (100),
						MakeUintegerAccessor (&Tracker::mc_count),
						MakeUintegerChecker<uint32_t> ())
		.AddAttribute ( "Interval",
						"The time to wait between hello packets",
						TimeValue (Seconds (3.0)),
						MakeTimeAccessor (&Tracker::m_interval),
						MakeTimeChecker ())
		.AddAttribute ( "RemoteAddress",
						"The destination Address of the outbound packets",
						AddressValue(),
						MakeAddressAccessor (&Tracker::mc_peerAddress),
						MakeAddressChecker ())
		.AddAttribute ( "RemotePort",
						"The destination port of the outbound packets",
						UintegerValue (0),
						MakeUintegerAccessor (&Tracker::mc_peerPort),
						MakeUintegerChecker<uint16_t> ())
		.AddAttribute ("PacketSize", "Size of echo data in outbound packets",
						UintegerValue (100),
						MakeUintegerAccessor(&Tracker::SetDataSize,
											&Tracker::GetDataSize),
						MakeUintegerChecker<uint32_t> ())
		.AddAttribute ( "Initiator",
						"The collect initiator",
						UintegerValue(0),
						MakeUintegerAccessor (&Tracker::m_col_initiator),
						MakeUintegerChecker<uint16_t> ())
		.AddAttribute ( "Target",
						"Target Vehicle",
						UintegerValue(9),
						MakeUintegerAccessor (&Tracker::m_col_target),
						MakeUintegerChecker<uint16_t> ())
		.AddAttribute ( "startTime",
						"The collect start time",
						TimeValue (Seconds (10.0)),
						MakeTimeAccessor (&Tracker::m_col_startTime),
						MakeTimeChecker ())
		.AddTraceSource ("Tx", "A new packet is craeted and is sent",
						MakeTraceSourceAccessor (&Tracker::mc_txTrace))
		/* Add attributes */
		;
	return tid;
}

Tracker::Tracker ()
{
	NS_LOG_FUNCTION (this);

	/* Server data */

	/* Client data */
	mc_sent = 0;
	mc_socket = 0;
	mc_sendEvent = EventId ();
	mc_data = 0;
	mc_dataSize = 0;

	/* Data collect data */
	m_col_active = false;
	m_col_id = 0;
	m_col_count_dur = 0;
	//m_col_count_stb = 0;
	m_col_param.maxdst = 20;
	m_col_param.maxdur = 40.0;
	m_election_active = false;
}

Tracker::~Tracker ()
{
	NS_LOG_FUNCTION (this);
	
	/* Server data */
	ms_socket = 0;
	ms_socket6 = 0;

	/* Client data */
	mc_socket = 0;

	delete [] mc_data;
	mc_data = 0;
	mc_dataSize = 0;
}

void
Tracker::Track ()
{
	StartCollect ();
}

void
Tracker::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}

void
Tracker::StartApplication (void)
{
	StartServer ();
	StartClient ();
//--------------------------------------------
	unsigned int x = GetNode ()->GetId ();
	if (x == m_col_initiator)
	{
		Simulator::Schedule (m_col_startTime, &Tracker::Track, this);

		Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4>();
		m_ip_initiator = ipv4->GetAddress (1, 0).GetLocal ();
	}
}

void
Tracker::StopApplication (void)
{
	StopClient ();
	StopServer ();
}

void
Tracker::PrintData (Ptr<Packet> packet)
{
	uint8_t *buffer = new uint8_t [packet->GetSize ()]; 
	packet->CopyData (buffer, packet->GetSize ()); 
	for (uint i = 0; i < packet->GetSize () - 1; i++)
	{
		std::cout << buffer[i];
	}
	std::cout << std::endl;
}

std::string
Tracker::GetData (Ptr<Packet> packet)
{
	std::ostringstream oss;
	uint8_t *buffer = new uint8_t [packet->GetSize ()]; 
	packet->CopyData (buffer, packet->GetSize ());
	for (uint i = 0; i < packet->GetSize () - 1; i++)
	{
		oss << buffer[i];
	}

	std::string data;
	data = oss.str ();
	return data;
}


//-------------------------------------------------------------------------
	/* Fontions pour le coté serveur: */
//-------------------------------------------------------------------------

void
Tracker::StartServer (void)
{
	NS_LOG_FUNCTION (this);

	if (ms_socket == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		ms_socket = Socket::CreateSocket (GetNode (), tid);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), ms_port);
		ms_socket->Bind (local);

		ms_socket->SetAllowBroadcast (true);

		if (addressUtils::IsMulticast (ms_local))
		{
			Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (ms_socket);
			if (udpSocket)
			{
				udpSocket->MulticastJoinGroup (0, ms_local);
			}
			else
			{
				NS_FATAL_ERROR ("Error: Failed to join multicast group");
			}
		}
	}
	if (ms_socket6 == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		ms_socket6 = Socket::CreateSocket (GetNode(), tid);
		Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::GetAny (), ms_port);
		ms_socket6->Bind (local6);
		if (addressUtils::IsMulticast (local6))
		{
			Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (ms_socket6);
			if (udpSocket)
			{
				udpSocket->MulticastJoinGroup (0, local6);
			}
			else
			{
				NS_FATAL_ERROR ("Error: Failed to join multicast group");
			}
		}
	}

	ms_socket->SetRecvCallback (MakeCallback (&Tracker::HandleRead, this));
	ms_socket6->SetRecvCallback (MakeCallback (&Tracker::HandleRead, this));
}

void
Tracker::StopServer ()
{
	NS_LOG_FUNCTION (this);

	if (ms_socket != 0)
	{
		ms_socket->Close ();
		ms_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}
	if (ms_socket6 != 0)
	{
		ms_socket6->Close ();
		ms_socket6->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}
}

void
Tracker::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
		
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom (from)))
	{
		std::string data = GetData (packet).c_str ();
		int tPacket = std::atoi (data.c_str ());
		data = data.substr (FindDash (data));

		int temp = std::atoi (data.c_str ());
			
//*
		if (tPacket != 0 && !(tPacket == 2 && temp == 2))
		{
			Ptr<Ipv4> ipv4 = GetNode ()->GetObject<Ipv4>();
			if (InetSocketAddress::IsMatchingType (from))
			{
				std::cout << "--------------------------------------------------------------------------------------------------" << std::endl;
				NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s node " << GetNode ()->GetId () << " with address " << ipv4->GetAddress (1, 0).GetLocal () << " received " <<packet->GetSize () << " bytes from " <<
							InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
							InetSocketAddress::ConvertFrom (from).GetPort ());
			}
			else if (Inet6SocketAddress::IsMatchingType (from))
			{
				NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " <<packet->GetSize () << " bytes from " <<
							Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
							InetSocketAddress::ConvertFrom (from).GetPort ());
			}
		}	
//*/

		if (tPacket == 0) //Hello packet
		{
			int id = std::atoi(data.c_str ());
			data = data.substr (FindDash (data));
			int x = std::atoi(data.c_str ());
			data = data.substr (FindDash (data));
			int y = std::atoi(data.c_str ());
			AddMeetEntry (id, x, y);
			//PrintMeets ();
		}
		else if (tPacket == 1) //Collect request packet
		{
			std::cout << "---> REQ (request)." << std::endl;
			//m_col_initiator = std::atoi (data.c_str ());
			//data = data.substr (FindDash (data));
			//take the ip
			//data = data.substr (FindDash (data));
			//take the col id
			if (m_col_id == std::atoi (data.c_str ()))
			{
				SetFill ("2-2");
			}
			else
			{ 
				//data = data.substr (FindDash (data));
				//take the maxdst
				//data = data.substr (FindDash (data));
				//take the maxdur
				data = data.substr (FindDash (data));
				Meet *meet = 0;
				meet = GetMeetEntry (std::atoi(data.c_str ()));
				if (meet)
				{
					std::ostringstream oss;
					oss << "2-1-";
					oss << GetPosition ().x;
					oss << "-";
					oss << GetPosition ().y;
					oss << "-";
					oss << meet->t.GetSeconds ();
					oss << "-";
					oss << meet->p.x;
					oss << "-";
					oss << meet->p.y;
					SetFill(oss.str ());
				}
				else
				{
					std::ostringstream oss;
					oss << "2-0-";
					oss << GetPosition ().x;
					oss << "-";
					oss << GetPosition ().y;
					SetFill(oss.str ());
				}		
			}
			//std::cout << "type 1 " << data << ", " << GetNode ()->GetId () << std::endl;
			ChangeRemote (InetSocketAddress::ConvertFrom (from).GetIpv4 (), 9);
			Send ();
			ChangeRemote (Ipv4Address ("10.255.255.255"), 9);
		}
		else if (tPacket == 2) //Collect request reply packet
		{
			tPacket = std::atoi(data.c_str ());
			data = data.substr (FindDash (data));
			
			if (tPacket == 0)
			{
				std::cout << "---> RN (negative reply)" << std::endl;
				int x = std::atoi(data.c_str ());
				data = data.substr (FindDash (data));
				int y = std::atoi(data.c_str ());
				AddCondidatEntry (0, InetSocketAddress::ConvertFrom (from).GetIpv4 (), x, y);
				std::cout << "Actual candidats: " << std::endl;
				PrintCandidats ();
				if (!m_election_active)
					ScheduleNextInitiatorElection ();
			}
			else if (tPacket == 1)
			{
				std::cout << "---> RP (positive reply)." << std::endl;
				int x = std::atoi(data.c_str ());
				data = data.substr (FindDash (data));
				int y = std::atoi(data.c_str ());
				AddCondidatEntry (1, InetSocketAddress::ConvertFrom (from).GetIpv4 (), x, y);
				std::cout << "Actual candidats: " << std::endl;
				PrintCandidats ();
				
				// Do something with collected data
				data = data.substr (FindDash (data));
				double meet_t = std::atof(data.c_str ());
				data = data.substr (FindDash (data));
				int meet_x = std::atoi(data.c_str ());
				data = data.substr (FindDash (data));
				int meet_y = std::atoi(data.c_str ());

				std::ostringstream oss;
				oss << "4-";
				oss << meet_t;
				oss << "-";
				oss << meet_x;
				oss << "-";
				oss << meet_y;
				SetFill (oss.str ());

				ChangeRemote (m_ip_initiator, 9);
				Send ();
				ChangeRemote (Ipv4Address ("10.255.255.255"), 9);
				
				if (!m_election_active)
					ScheduleNextInitiatorElection ();
			}
			else
			{
				/* Ignore */
			}
		}
		else if (tPacket == 3)
		{
			std::cout << "---> UNI (you are next initiator)." << std::endl;

			m_ip_initiator = Ipv4Address ((data.substr (0, FindDash (data) - 1)).c_str ()); //take the ip of principal initiator
			data = data.substr (FindDash (data));
			m_col_id = std::atoi(data.c_str ()); //take the collect id
			data = data.substr (FindDash (data));
			m_col_param.maxdst = std::atoi(data.c_str ()); //take the maxdst
			data = data.substr (FindDash (data));
			m_col_param.maxdur = std::atof(data.c_str ()); //take the maxdur
			data = data.substr (FindDash (data));
			m_col_target = std::atoi(data.c_str ()); // take the target id
			data = data.substr (FindDash (data));

			std::cout << "\tCurrent parameters: " << std::endl;
			std::cout << "\t - IPr: " << m_ip_initiator << std::endl;
			std::cout << "\t - col id: " << m_col_id << std::endl;
			std::cout << "\t - maxdur: " << m_col_param.maxdur << std::endl;
			std::cout << "\t - maxdst: " << m_col_param.maxdst << std::endl;
			std::cout << "\t - target id: " << m_col_target << std::endl;

			if (m_col_param.maxdur <= 0)
			{
				std::cout << "maxdur reached... stopping collect..." << std::endl;
			}
			else if (m_col_param.maxdst <= 0)
			{
				std::cout << "maxdst reached... stopping collect..." << std::endl;
			}
			else
			{
				StartCollect (); //continue collect
			}
		}
		else if (tPacket == 4)
		{
			std::cout << "---> Obj ////////// \\\\\\\\\\\\\\\\\\\\" << std::endl;

			int meet_t = std::atof(data.c_str ());
			data = data.substr (FindDash (data));
			int meet_x = std::atoi(data.c_str ());
			data = data.substr (FindDash (data));
			int meet_y = std::atoi(data.c_str ());

			std::cout << "\tThe target vehicle met at:" << std::endl;
			std::cout << "\t - Time: " << meet_t << std::endl;
			std::cout << "\t - Position:" << std::endl;
			std::cout << "\t   - X = " << meet_x << std::endl;
			std::cout << "\t   - Y = " << meet_y << std::endl;	
			std::cout << "\t \\\\\\\\\\\\\\\\\\\\ //////////" << std::endl;
		}
/*
		std::ostream *ost = 0;
		Ptr< OutputStreamWrapper > stream = Create < OutputStreamWrapper > (ost);
		//GetNode ()->GetObject<Ipv4RoutingProtocol>()-> PrintRoutingTable (stream);
		//std::cout << os << std::endl;
//*/			
		//PrintData (packet);
/*
		packet->RemoveAllPacketTags ();

		NS_LOG_LOGIC ("Echoing packet");
		socket->SendTo (packet, 0, from);
		socket->SendTo (packet, 0, from);

		if(InetSocketAddress::IsMatchingType (from))
		{
			NS_LOG_INFO ("At time" << Simulator::Now ().GetSeconds () << "s server sent " << packet->GetSize () << " bytes to " <<
						InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
						InetSocketAddress::ConvertFrom (from).GetPort ());
		}
		else if (Inet6SocketAddress::IsMatchingType (from))
		{
			NS_LOG_INFO ("At time" << Simulator::Now ().GetSeconds () << "s server sent " << packet->GetSize () << " bytes to " << 
						Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
						InetSocketAddress::ConvertFrom (from).GetPort ());
		}
//*/
	}
}

//-------------------------------------------------------------------------
	/* Fonctions pour le coté client: */
//-------------------------------------------------------------------------

void
Tracker::StartClient (void)
{
	NS_LOG_FUNCTION (this);

	if (mc_socket == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		mc_socket = Socket::CreateSocket (GetNode (), tid);

		mc_socket->SetAllowBroadcast (true);

		if (Ipv4Address::IsMatchingType (mc_peerAddress) == true)
		{
			mc_socket->Bind ();
			mc_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(mc_peerAddress), mc_peerPort));
		}
		else if (Ipv6Address::IsMatchingType (mc_peerAddress) == true)
		{
			mc_socket->Bind6 ();
			mc_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(mc_peerAddress), mc_peerPort));
		}
	}

	mc_socket->SetRecvCallback (MakeCallback (&Tracker::ClientHandleRead, this));

	ScheduleHello (Seconds (0.)); 
}

void
Tracker::StopClient ()
{
	NS_LOG_FUNCTION (this);

	if (mc_socket != 0)
	{
		mc_socket->Close ();
		mc_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
		mc_socket = 0;
	}

	Simulator::Cancel (mc_sendEvent);
}

void
Tracker::ClientHandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom (from)))
	{
		if (InetSocketAddress::IsMatchingType (from))
		{
			NS_LOG_INFO ("At time" << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
						InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
						Inet6SocketAddress::ConvertFrom (from).GetPort ());
		}
		else if (Inet6SocketAddress::IsMatchingType (from))
		{
			NS_LOG_INFO ("At time" << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from "<<
						Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
						Inet6SocketAddress::ConvertFrom (from).GetPort ());
		}
	}
}

void
Tracker::SetRemote (Address ip, uint16_t port)
{
	NS_LOG_FUNCTION (this << ip << port);
	mc_peerAddress = ip;
	mc_peerPort = port;
}

void
Tracker::SetRemote (Ipv4Address ip, uint16_t port)
{
	NS_LOG_FUNCTION (this << ip << port);
	mc_peerAddress = Address (ip);
	mc_peerPort = port;
}

void
Tracker::SetRemote (Ipv6Address ip, uint16_t port)
{
	NS_LOG_FUNCTION (this << ip << port);
	mc_peerAddress = Address (ip);
	mc_peerPort = port;
}

void
Tracker::ChangeRemote (Address ip, uint16_t port)
{
	StopClient ();
	SetRemote (ip, port);
	StartClient ();
}

void
Tracker::ChangeRemote (Ipv4Address ip, uint16_t port)
{
	StopClient ();
	SetRemote (ip, port);
	StartClient ();
}

void
Tracker::ChangeRemote (Ipv6Address ip, uint16_t port)
{
	StopClient ();
	SetRemote (ip, port);
	StartClient ();
}
//*
void
Tracker::SetDataSize (uint32_t dataSize)
{
	NS_LOG_FUNCTION (this << dataSize);
	delete [] mc_data;
	mc_data = 0;
	mc_dataSize = 0;
	mc_size = dataSize;
}
//*/
uint32_t
Tracker::GetDataSize (void) const
{
 	NS_LOG_FUNCTION (this);
 	return mc_size;
}

void
Tracker::SetFill (std::string fill)
{
	NS_LOG_FUNCTION (this << fill);

	uint32_t dataSize = fill.size () + 1;

	if (dataSize != mc_dataSize)
	{
		delete [] mc_data;
		mc_data = new uint8_t [dataSize];
		mc_dataSize = dataSize;
	}

	memcpy (mc_data, fill.c_str (), dataSize);

	mc_size = dataSize;
}
/*
void
Tracker::SetFill (uint8_t fill, uint32_t dataSize)
{
	NS_LOG_FUNCTION (this << fill << dataSize);
	if (dataSize != mc_dataSize)
	{
		delete [] mc_data;
		mc_data = new uint8_t [dataSize];
		mc_dataSize = dataSize;
	}

	memset (mc_data, fill, dataSize);

	mc_size = dataSize;
}
//*/
/*
void
Tracker::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
	NS_LOG_FUNCTION (this << fill << fillSize << dataSize);
	if (dataSize != mc_dataSize)
	{
		delete [] mc_data;
		mc_data = new uint8_t [dataSize];
		mc_dataSize = dataSize;
	}

	if (fillSize >= dataSize)
	{
		memcpy (mc_data, fill, dataSize);
		mc_size = dataSize;
		return;
	}

	uint32_t filled = 0;
	while (filled + fillSize < dataSize)
	{
		memcpy (&mc_data[filled], fill, fillSize);
		filled += fillSize;
	}

	memcpy (&mc_data[filled], fill, dataSize - filled);

	mc_size = dataSize;
}
//*/
void
Tracker::ScheduleTransmit (Time dt)
{
	NS_LOG_FUNCTION (this << dt);
	mc_sendEvent = Simulator::Schedule (dt, &Tracker::Send, this);
}

void
Tracker::Send (void)
{
	NS_LOG_FUNCTION (this);

	NS_ASSERT (mc_sendEvent.IsExpired ());

	Ptr<Packet> p;
	if (mc_dataSize)
	{
		NS_ASSERT_MSG (mc_dataSize == mc_size, "Tracker::Send(): mc_size and mc_dataSize inconsistent");
		NS_ASSERT_MSG (mc_data, "Tracker::Send(): mc_dataSize but no mc_data");
		p = Create<Packet> (mc_data, mc_dataSize);
	}
	else
	{
		p = Create<Packet> (mc_size);
	}

	mc_txTrace (p);
	mc_socket->Send (p);

	++mc_sent;

	if (Ipv6Address::IsMatchingType (mc_peerAddress)) //Ipv4
	{
		NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << mc_size << " bytes to " <<
					Ipv4Address::ConvertFrom (mc_peerAddress) << " port " << mc_peerPort);
	}
	else if (Ipv6Address::IsMatchingType (mc_peerAddress))
	{
		NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << mc_size << " bytes to " <<
					Ipv6Address::ConvertFrom (mc_peerAddress) << " port " << mc_peerPort);
	}
/*
	if (mc_sent < mc_count)
	{
		//ScheduleTransmit (mc_interval);
	}
//*/
}

//-------------------------------------------------------------------------
	/* Meets management */
//-------------------------------------------------------------------------

void
Tracker::AddMeetEntry (int id, int x, int y)
{
	Meet *meet = GetMeetEntry (id);
	Vector pos;
	pos.x = x;
	pos.y = y;
	
	if (meet == 0)
	{
		Meet n;
		n.id = id;
		n.t = Simulator::Now ();
		n.p = pos;
		m_meets.push_back (n);	
	}
	else
	{
		meet->t = Simulator::Now ();
		meet->p = pos;
	}
}

Meet*
Tracker::GetMeetEntry (int id)
{
	for(unsigned int i = 0; i < m_meets.size (); i++)
	{
		if (m_meets[i].id == id)
			return &(m_meets[i]);
	}

	return 0;
}

void
Tracker::PrintMeets ()
{
	for(unsigned int i = 0; i < m_meets.size (); i++)
		std::cout << "id: " << m_meets[i].id << ", t: " << m_meets[i].t << ", p: x = " << m_meets[i].p.x << " y = " << m_meets[i].p.y << " | ";
	std::cout << std::endl;
}

void
Tracker::SayHello ()
{
	NS_LOG_FUNCTION (this);

	std::ostringstream oss;
	oss << "0-";
	oss << GetNode ()->GetId ();
	oss << "-";
	
	Vector pos = GetPosition ();
	oss << pos.x;
	oss << "-";
	oss << pos.y;
	
	SetFill (oss.str ());

	Send ();

	ScheduleHello (Time(Seconds (3.0))); //When set to m_interval something doesn't work
}

void
Tracker::ScheduleHello (Time dt)
{
	NS_LOG_FUNCTION (this << dt);
	Simulator::Schedule (dt, &Tracker::SayHello, this);	
}

Vector
Tracker::GetPosition ()
{
	Ptr<MobilityModel> mob = GetNode ()->GetObject<MobilityModel>();
	Vector pos = mob->GetPosition ();
	return pos;
}

int
Tracker::FindDash (std::string str)
{
	unsigned int i = 0;
	while (i < str.size ())
	{
		if (str[i] == '-')
			return i+1;
		i++;
	}
	return 0;
}

//-------------------------------------------------------------------------
	/* Data collect management */
//-------------------------------------------------------------------------

void
Tracker::StartCollect ()
{
	if (m_col_active)
	{
		std::cout << "Une collecte est en cours !" << std::endl;
		return;
	}

	//m_col_active = true;
	
	unsigned int x = GetNode ()->GetId ();
	if (x == m_col_initiator)
	{
		m_col_id++;
	}
	
	std::ostringstream oss;
	oss << "1-";
	//oss << GetNode ()-> GetId ();
	//oss << "-";
	//oss << m_ip_initiator;
	//oss << "-";
	oss << m_col_id;
	oss << "-";
	//oss << m_col_param.maxdst;
	//oss << "-";
	//oss << m_col_param.maxdur;
	//oss << "-";
	oss << m_col_target;
	
	SetFill (oss.str ());
	Send ();
}

Ipv4Address
Tracker::GetNextInitiator ()
{
	NS_LOG_FUNCTION (this);
	Condidat nextInitiator;
	nextInitiator = m_condidats[0];

	for(unsigned int i = 0; i < m_condidats.size (); i++)
	{
		if (m_condidats[i].positive < nextInitiator.positive)
		{
		}
		else if (m_condidats[i].positive > nextInitiator.positive)
		{
			nextInitiator = m_condidats[i];
		}
		else if (abs (m_condidats[i].dist - 50 ) < abs (nextInitiator.dist - 50))
		{
			nextInitiator = m_condidats[i];
		}
	}

	return nextInitiator.ip;
}

void
Tracker::AddCondidatEntry (bool p, Ipv4Address ip, int x, int y)
{
	Condidat c;
	c.ip = ip;
	if (p)
		c.positive = true;
	else
		c.positive = false;
	c.dist = CalculDistance (x, y, GetPosition ().x, GetPosition ().y);
	m_condidats.push_back (c);
}

double
Tracker::CalculDistance (int x1, int y1, int x2, int y2)
{
	NS_LOG_FUNCTION (this << x1 << y1 << x2 << y2);
	return sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
}

void
Tracker::PrintCandidats ()
{
	for(unsigned int i = 0; i < m_condidats.size (); i++)
		std::cout << "ip: " << m_condidats[i].ip << ", p: " << m_condidats[i].positive << ", dist: x = " << m_condidats[i].dist << " | ";
	std::cout << std::endl;
}

/* Not used
Condidat*
Tracker::GetCondidatEntry (Ipv4Address ip)
{
	for(unsigned int i = 0; i < m_condidats.size (); i++)
	{
		if (m_condidats[i].ip == ip)
			return &(m_condidats[i]);
	}

	return 0;
}
*/

void
Tracker::ScheduleNextInitiatorElection ()
{
	NS_LOG_FUNCTION (this);
	m_election_active = true;
	Simulator::Schedule (Time (Seconds (1.0)), &Tracker::NextInitiatorElection, this);	
}

void
Tracker::NextInitiatorElection ()
{
	int maxdst = m_col_param.maxdst-1;
	double maxdur = m_col_param.maxdur - (Simulator::Now ().GetSeconds () - m_col_startTime.GetSeconds ());

	ChangeRemote (GetNextInitiator (), 9);
	std::ostringstream oss;
	oss << "3-";
	oss << m_ip_initiator;
	oss << "-";
	oss << m_col_id;
	oss << "-";
	oss << maxdst;
	oss << "-";
	oss << maxdur;
	oss << "-";
	oss << m_col_target;
	SetFill (oss.str ());
	Send ();
	ChangeRemote (Ipv4Address ("10.255.255.255"), 9);
}

} // namespace ns3
