/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/uinteger.h"
#include "ns3/names.h"

#include "tracker-helper.h"

namespace ns3 {

TrackerHelper::TrackerHelper ()
{
	m_factory.SetTypeId (Tracker::GetTypeId ());
	SetAttribute ("Port", UintegerValue (9));
	SetAttribute ("RemoteAddress", AddressValue (Ipv4Address ("10.255.255.255")));
	SetAttribute ("RemotePort", UintegerValue (9));
}
/*
TrackerHelper::TrackerHelper(uint16_t port)
{
	m_factory.SetTypeId (Tracker::GetTypeId ());
	SetAttribute ("Port", UintegerValue (port));
}
TrackerHelper::TrackerHelper(Address address, uint16_t remotePort)
{
	m_factory.SetTypeId (Tracker::GetTypeId ());
	SetAttribute ("RemoteAddress", AddressValue (address));
	SetAttribute ("RemotePort", UintegerValue (remotePort));
}
TrackerHelper::TrackerHelper(Ipv4Address address, uint16_t remotePort)
{
	m_factory.SetTypeId (Tracker::GetTypeId ());
	SetAttribute ("RemoteAddress", AddressValue (address));
	SetAttribute ("RemotePort", UintegerValue (remotePort));
}
TrackerHelper::TrackerHelper(Ipv6Address address, uint16_t remotePort)
{
	m_factory.SetTypeId (Tracker::GetTypeId ());
	SetAttribute ("RemoteAddress", AddressValue (address));
	SetAttribute ("RemotePort", UintegerValue (remotePort));
}
TrackerHelper::TrackerHelper(uint16_t port, Address address, uint16_t remotePort)
{
	m_factory.SetTypeId (Tracker::GetTypeId ());
	SetAttribute ("Port", UintegerValue (port));
	SetAttribute ("RemoteAddress", AddressValue (address));
	SetAttribute ("RemotePort", UintegerValue (remotePort));
}
TrackerHelper::TrackerHelper(uint16_t port, Ipv4Address address, uint16_t remotePort)
{
	m_factory.SetTypeId (Tracker::GetTypeId ());
	SetAttribute ("Port", UintegerValue (port));
	SetAttribute ("RemoteAddress", AddressValue (address));
	SetAttribute ("RemotePort", UintegerValue (remotePort));
}
TrackerHelper::TrackerHelper(uint16_t port, Ipv6Address address, uint16_t remotePort)
{
	m_factory.SetTypeId (Tracker::GetTypeId ());
	SetAttribute ("Port", UintegerValue (port));
	SetAttribute ("RemoteAddress", AddressValue (address));
	SetAttribute ("RemotePort", UintegerValue (remotePort));
}
*/
void
TrackerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
	m_factory.Set (name, value);
}

ApplicationContainer
TrackerHelper::Install (Ptr<Node> node) const
{
	return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TrackerHelper::Install (std::string nodeName) const
{
	Ptr<Node> node = Names::Find<Node> (nodeName);
	return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TrackerHelper::Install (NodeContainer c) const
{
	ApplicationContainer apps;
	for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
	{
		apps.Add (InstallPriv (*i));
	}

	return apps;
}

Ptr<Application>
TrackerHelper::InstallPriv (Ptr<Node> node) const
{
	Ptr<Application> app = m_factory.Create<Tracker> ();
	node->AddApplication (app);

	return app;
}

//-------------------------------------------------------------------------
	/* Fonctions pour le coté client: */
//-------------------------------------------------------------------------

void
TrackerHelper::SetFill (Ptr<Application> app, std::string fill)
{
	app->GetObject<Tracker>()->SetFill (fill);
}
/*
void
TrackerHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
	app->GetObject<Tracker>()->SetFill (fill, dataLength);
}
//*/
/*
void
TrackerHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
	app->GetObject<Tracker>()->SetFill (fill, fillLength, dataLength);
}
//*/
} // namespace ns3

