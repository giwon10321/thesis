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

	double m_residualEnergy;
	double m_minThresholdVoltage;

	double m_minVoltage;
	double m_maxVoltage;

protected:
	virtual void DoInitialize (void);
	virtual void DoDispose (void);
private:

	void SendRfe (void);
	void UpdatePower (void);

	TracedValue<double> m_harvestedPower;
	TracedValue<double> m_totalEnergyHarvestedJ;

	Time m_updateInterval;
	EventId m_energyUpdateEvent;
};

}//namespace ns3
#endif
