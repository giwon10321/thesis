#include "lr-wpan-edt-net-device.h"
#include <ns3/log.h>

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("LrWpanEdtNetDevice");

NS_OBJECT_ENSURE_REGISTERED (LrWpanEdtNetDevice);

TypeId
LrWpanEdtNetDevice::GetTypeId(void)
{
	static TypeId tid = TypeId ("ns3::LrWpanEdtNetDevice")
		.SetParent<LrWpanNetDevice> ()
		.SetGroupName ("LrWpan")
		.AddConstructor<LrWpanEdtNetDevice> ()
	;

	return tid;
}

LrWpanEdtNetDevice::LrWpanEdtNetDevice (void)
	: LrWpanNetDevice ()
{
	NS_LOG_FUNCTION (this);
	GetMac ()->SetDeviceType (MAC_FOR_EDT);
}

LrWpanEdtNetDevice::~LrWpanEdtNetDevice (void)
{
	NS_LOG_FUNCTION (this);
}


}//namespace ns3
