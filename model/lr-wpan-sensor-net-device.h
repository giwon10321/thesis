#ifndef LR_WPAN_SENSOR_NET_DEVICE_H
#define LR_WPAN_SENSOR_NET_DEVICE_H

#include "lr-wpan-net-device.h"

namespace ns3 {

class LrWpanSensorNetDevice : public LrWpanNetDevice
{
public:
	static TypeId GetTypeId (void);

	LrWpanSensorNetDevice (void);
	virtual ~LrWpanSensorNetDevice (void);

	void SetResidualEnergy (double energy);
	double GetResidualEnergy (void) const;

private:
	void SendRfe (void);

	double m_residualEnergy;
	double m_threshold_voltage;
};

}//namespace ns3
#endif
