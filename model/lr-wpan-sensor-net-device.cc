#include "lr-wpan-sensor-net-device.h"
#include <ns3/log.h>

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("LrWpanSensorNetDevice");

NS_OBJECT_ENSURE_REGISTERED (LrWpanSensorNetDevice);

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
	m_thresholdVoltage = 2.3;
	m_minVoltage = 1.8;
	m_maxVoltage = 3.0;
	GetMac ()->SetDeviceType (MAC_FOR_SENSOR);
}

LrWpanSensorNetDevice::~LrWpanSensorNetDevice (void)
{
	NS_LOG_FUNCTION (this);
}

void
LrWpanSensorNetDevice::DoInitilize (void)
{
	NS_LOG_FUNCTION (this);
	UpdatePower ();
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
	NS_LOG_FUNCTION (this);
	 NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                << "s BasicEnergyHarvester(" << GetNode ()->GetId () << "): Updating harvesting power.");
	m_energyUpdateEvent = Simulator::Schedule (m_updateInterval, &LrWpanSensorNetDevice::UpdatePower, this);
}

}//namespace ns3
