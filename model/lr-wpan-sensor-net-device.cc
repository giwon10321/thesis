#include "lr-wpan-sensor-net-device.h"

#include <ns3/simulator.h>
#include <ns3/log.h>
#include <ns3/uinteger.h>
#include <ns3/node.h>

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("LrWpanSensorNetDevice");

NS_OBJECT_ENSURE_REGISTERED (LrWpanSensorNetDevice);
/*
	capacity : 150mAh
	V : 2.0~3.0
	C : 540
*/
TypeId
LrWpanSensorNetDevice::GetTypeId(void)
{
	static TypeId tid = TypeId ("ns3::LrWpanSensorNetDevice")
		.SetParent<LrWpanNetDevice> ()
		.SetGroupName ("LrWpan")
		.AddConstructor<LrWpanSensorNetDevice> ()
	;

	return tid;
}

LrWpanSensorNetDevice::LrWpanSensorNetDevice (void)
	: LrWpanNetDevice ()
{
	NS_LOG_FUNCTION (this);
	m_updateInterval = Seconds (1.0);
	m_minThresholdVoltage = 2.3;
	m_minVoltage = 2.0;
	m_maxVoltage = 3.0;

	GetMac ()->m_minThresholdVoltage = 2.3;
	GetMac ()->m_minVoltage = 1.8;
	GetMac ()->m_maxVoltage = 3.0;
	GetMac ()->m_maxThresholdVoltage = 0;
 	GetMac ()->m_currentVoltage = 2.9;
	GetMac ()->SetDeviceType (MAC_FOR_SENSOR);
}

LrWpanSensorNetDevice::~LrWpanSensorNetDevice (void)
{
	NS_LOG_FUNCTION (this);
}

void
LrWpanSensorNetDevice::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	m_energyUpdateEvent.Cancel ();
	LrWpanNetDevice::DoDispose ();
}

void
LrWpanSensorNetDevice::DoInitialize (void)
{
	NS_LOG_FUNCTION (this);
	UpdatePower ();
	LrWpanNetDevice::DoInitialize ();
}


void
LrWpanSensorNetDevice::SetResidualEnergy (double energy)
{
	NS_LOG_FUNCTION (this);
	m_residualEnergy = energy;
}


double
LrWpanSensorNetDevice::GetResidualEnergy (void) const
{
	NS_LOG_FUNCTION (this);
	return m_residualEnergy;
}

void
LrWpanSensorNetDevice::SendRfe (void)
{
	GetMac()->SendRfeForEnergy ();
}

void
LrWpanSensorNetDevice::UpdatePower(void)
{
 	NS_LOG_DEBUG ("Updating harvesting power.");

	if (Simulator::IsFinished ())
	{
	  NS_LOG_DEBUG ("Simulation Finished.");
	  return;
	}
	
	m_energyUpdateEvent.Cancel ();
	m_energyUpdateEvent = Simulator::Schedule (m_updateInterval, &LrWpanSensorNetDevice::UpdatePower, this);
}

}//namespace ns3
