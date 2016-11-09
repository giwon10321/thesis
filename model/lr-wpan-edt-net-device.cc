#include "lr-wpan-edt-net-device.h"
#include "lr-wpan-spectrum-value-helper.h"
#include <ns3/spectrum-value.h>

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

LrWpanEdtNetDevice::LrWpanEdtNetDevice ()
	: LrWpanNetDevice ()
{
	NS_LOG_FUNCTION (this);
	GetMac ()->SetDeviceType (MAC_FOR_EDT);
}
	
LrWpanEdtNetDevice::LrWpanEdtNetDevice (bool isProposed)
	: LrWpanNetDevice ()
{
	NS_LOG_FUNCTION (this);
	GetMac ()->SetDeviceType (MAC_FOR_EDT);
	GetMac ()->isProposed = isProposed;
	GetPhy ()->m_phyPIBAttributes.phyCurrentChannel = 11;
  	GetPhy ()->m_phyPIBAttributes.phyTransmitPower = 30;

  	LrWpanSpectrumValueHelper psdHelper;
  	GetPhy ()->m_txPsd = psdHelper.CreateTxPowerSpectralDensity (GetPhy ()->m_phyPIBAttributes.phyTransmitPower,
                                                    GetPhy ()->m_phyPIBAttributes.phyCurrentChannel);
}

LrWpanEdtNetDevice::~LrWpanEdtNetDevice (void)
{
	NS_LOG_FUNCTION (this);
}


}//namespace ns3
