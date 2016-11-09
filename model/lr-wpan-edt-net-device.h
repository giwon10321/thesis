#ifndef LR_WPAN_EDT_NET_DEVICE_H
#define LR_WPAN_EDT_NET_DEVICE_H

#include "lr-wpan-net-device.h"

namespace ns3 {

class LrWpanEdtNetDevice : public LrWpanNetDevice
{
public:
	static TypeId GetTypeId (void);

	LrWpanEdtNetDevice (void);
	LrWpanEdtNetDevice (bool isProposed);
	virtual ~LrWpanEdtNetDevice (void);

private:

};

}//namespace ns3
#endif
