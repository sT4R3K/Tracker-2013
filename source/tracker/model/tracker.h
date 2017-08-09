/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TRACKER_H__
#define __TRACKER_H__

#include <iostream>

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"

#include "ns3/mobility-module.h"

#include "ns3/traced-callback.h"

namespace ns3 {

struct Meet
{
	int id; // Vehicle id.
	Time t; // Meet time. 
	Vector p;
};

struct Condidat
{
	bool positive;
	Ipv4Address ip;
	double dist;
};

struct Parameter
{
	//std::string typedt;
	int maxdst;
	double maxdur;
	//int maxstb;
};

class Socket;
class Packet;

class Tracker : public Application
{
public:
	static TypeId GetTypeId (void);
	Tracker();
	virtual ~Tracker();
	void Track ();

	/* Server functions */

	/* Client functions */
	void SetRemote (Address ip, uint16_t port);	
	void SetRemote (Ipv4Address ip, uint16_t port);
	void SetRemote (Ipv6Address ip, uint16_t port);
	void ChangeRemote (Address ip, uint16_t port);
	void ChangeRemote (Ipv4Address ip, uint16_t port);
	void ChangeRemote (Ipv6Address ip, uint16_t port);

	void SetDataSize (uint32_t dataSize); //To delete
	uint32_t GetDataSize (void) const;

	void SetFill (std::string fill);
	//void SetFill (uint8_t fill, uint32_t dataSize);
	//void SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize);

	void PrintData (Ptr<Packet> packet);
	std::string GetData (Ptr<Packet> packet);

	void ScheduleTransmit(ns3::Time);

protected:
	virtual void DoDispose (void);

private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);

	/* Server functions */
	void StartServer (void);
	void StopServer (void);
	void HandleRead (Ptr<Socket> socket);

	/* Client functions */
	void StartClient (void);
	void StopClient (void);
	void ClientHandleRead (Ptr<Socket> socket);

	//void ScheduleTransmit (Time dt);
	void Send (void);
	//void SendTo ();

	/* Meets management */

	void AddMeetEntry (int id, int x, int y);
	Meet* GetMeetEntry (int id);
	void PrintMeets ();
	int FindDash (std::string str);

	Vector GetPosition ();

	void ScheduleHello (Time dt);
	void SayHello ();

	/* Data collect management */

	void StartCollect ();
	Ipv4Address GetNextInitiator ();
	void ScheduleNextInitiatorElection ();
	void NextInitiatorElection ();

	void AddCondidatEntry (bool p, Ipv4Address ip, int x, int y);
	void PrintCandidats ();
	double CalculDistance (int x1, int y1, int x2, int y2);

	//-------------------
	// Data:
	//-------------------

	/* Server data */
	uint16_t ms_port; // ms: member server.
	Ptr<Socket> ms_socket;
	Ptr<Socket> ms_socket6;
	Address ms_local;

	/* Client data */
	uint32_t mc_count; // mc: member client.
	uint32_t mc_size;

	uint32_t mc_dataSize;
	uint8_t *mc_data;

	uint32_t mc_sent;
	Ptr<Socket> mc_socket;
	Address mc_peerAddress;
	uint16_t mc_peerPort;
	EventId mc_sendEvent;

	TracedCallback<Ptr<const Packet> > mc_txTrace;

	/* Meets management data */

	std::vector<Meet> m_meets;
	Time m_interval;

	/* Data collect data */

	bool m_col_active;
	int m_col_id;
	uint16_t m_col_initiator;
	Ipv4Address m_ip_initiator;
	uint16_t m_col_target;
	Time m_col_startTime;
	Parameter m_col_param;
	int m_col_count_dur;
	//int m_col_count_stb;
	//tab_views;
	bool m_election_active;

	std::vector<Condidat> m_condidats;
};

} // namespace ns3

#endif /* __TRACKER_H__ */

