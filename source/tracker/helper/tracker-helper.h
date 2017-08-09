/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TRACKER_HELPER_H__
#define __TRACKER_HELPER_H__

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"

#include "ns3/tracker.h"

namespace ns3 {

class TrackerHelper
{
public:
	TrackerHelper ();
/*
	TrackerHelper(uint16_t port);
	TrackerHelper(Address ip, uint16_t remotePort);
	TrackerHelper(Ipv4Address ip, uint16_t remotePort);
	TrackerHelper(Ipv6Address ip, uint16_t remotePort);
	TrackerHelper(uint16_t port, Address ip, uint16_t remotePort);
	TrackerHelper(uint16_t port, Ipv4Address ip, uint16_t remotePort);
	TrackerHelper(uint16_t port, Ipv6Address ip, uint16_t remotePort);
//*/
	void SetAttribute (std::string name, const AttributeValue &value);
	ApplicationContainer Install (Ptr<Node> node) const;
	ApplicationContainer Install (std::string nodeName) const;
	ApplicationContainer Install (NodeContainer c) const;

	/* Server functions */

	/* Client functions */
	void SetFill (Ptr<Application> app, std::string fill);
	//void SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength);
	//void SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength);

private:
	Ptr<Application> InstallPriv (Ptr<Node> node) const;
	ObjectFactory m_factory;

};

} // namespace ns3

#endif /* __TRACKER_HELPER_H__ */

