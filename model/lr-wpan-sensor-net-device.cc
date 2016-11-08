#include "lr-wpan-sensor-net-device.h"

#include <ns3/simulator.h>
#include <ns3/log.h>
#include <ns3/uinteger.h>
#include <ns3/node.h>

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("LrWpanSensorNetDevice");

NS_OBJECT_ENSURE_REGISTERED (LrWpanSensorNetDevice);
/*
	capacity : 30mAh
	V : 2.0~3.0
	Q : 108
	C : 108/3 = 36

	T = 132.3 * 20/E max rx

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
	m_updateInterval = Seconds (0.1);
	m_minThresholdVoltage = 2.7;
	m_minVoltage = 2.0;
	m_maxVoltage = 3.0;

	m_currentVoltage = 2.9;

	m_amh = 0.03;
	m_q = m_amh * 3600;
	m_c = m_q / m_maxVoltage;

	GetMac ()->m_minThresholdVoltage = m_minThresholdVoltage;
	GetMac ()->m_maxThresholdVoltage = m_maxVoltage;
	GetMac ()->m_minVoltage = m_minVoltage;
	GetMac ()->m_maxVoltage = m_maxVoltage;
 	GetMac ()->m_currentVoltage = m_maxVoltage;
	GetMac ()->SetDeviceType (MAC_FOR_SENSOR);

	GetMac ()->SetRfMacEnergyIndicationCallback (MakeCallback(&LrWpanSensorNetDevice::RfMacEnergyIndication, this));
	GetPhy ()->SetRfMacEnergyConsumtionCallback (MakeCallback(&LrWpanSensorNetDevice::RfMacEnergyConsumtion, this));
}

LrWpanSensorNetDevice::~LrWpanSensorNetDevice (void)
{
	NS_LOG_FUNCTION (this);
}

void
LrWpanSensorNetDevice::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	m_updateEvent.Cancel ();
	LrWpanNetDevice::DoDispose ();
}

void
LrWpanSensorNetDevice::DoInitialize (void)
{
	NS_LOG_FUNCTION (this);
	Simulator::ScheduleNow (&LrWpanSensorNetDevice::Update, this);
	LrWpanNetDevice::DoInitialize ();
}

void
LrWpanSensorNetDevice::SendRfe (void)
{
	GetMac()->SendRfeForEnergy ();
}

void
LrWpanSensorNetDevice::RfMacEnergyIndication (double energy)
{
	NS_LOG_DEBUG ("Received energy: "<<energy);
	double volt = energy / m_q;
	NS_LOG_DEBUG ("Voltage: "<<volt);
	m_currentVoltage += volt;
	if(m_currentVoltage > m_maxVoltage)
		{
			m_currentVoltage = m_maxVoltage;	
		}
	GetMac ()->m_currentVoltage = m_currentVoltage;
	// m_updateEvent = Simulator::Schedule (m_updateInterval, &LrWpanSensorNetDevice::Update, this);
}

void
LrWpanSensorNetDevice::RfMacEnergyConsumtion (double energy)
{
	NS_LOG_DEBUG ("Consumed energy: "<<energy);
	//V = E / Q
	double volt = energy / m_q;
	NS_LOG_DEBUG ("Voltage: "<<volt);
	m_currentVoltage -= volt;
	GetMac ()->m_currentVoltage = m_currentVoltage;
	// this->CheckVoltage ();
}

void
LrWpanSensorNetDevice::Update (void)
{
	// NS_LOG_FUNCTION (this);
	m_updateEvent.Cancel ();
	if (Simulator::IsFinished ())
		{
			NS_LOG_DEBUG ("Simulation Finished");
			return ;
		}
	// m_currentVoltage -= 0.1;
	// this->CheckVoltage ();
	// m_updateEvent = Simulator::Schedule (m_updateInterval, &LrWpanSensorNetDevice::Update, this);
}

void
LrWpanSensorNetDevice::CheckVoltage (void)
{
	if(m_currentVoltage <= m_minThresholdVoltage)
		{
			m_updateEvent.Cancel ();
			this->SendRfe ();
			return ;
		}
}

}//namespace ns3
