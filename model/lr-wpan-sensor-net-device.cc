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
		.AddAttribute ("ResidualEnergy",
									"",
									DoubleValue(3.0),
									MakeDoubleAccessor (&LrWpanSensorNetDevice::SetResidualEnergy,
																			&LrWpanSensorNetDevice::GetResidualEnergy),
									MakeDoubleChecker<double>())
	;

	return tid;
}

LrWpanSensorNetDevice::LrWpanSensorNetDevice (void)
	: LrWpanNetDevice ()
{
	NS_LOG_FUNCTION (this);
	m_threshold_voltage = 4.0;
}

LrWpanSensorNetDevice::~LrWpanSensorNetDevice (void)
{
	NS_LOG_FUNCTION (this);
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


}//namespace ns3
