#include "rf-mac-type-tag.h"
#include <ns3/integer.h>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RfMacTypeTag);

TypeId
RfMacTypeTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RfMacTypeTag")
    .SetParent<Tag> ()
    .SetGroupName ("LrWpan")
    .AddConstructor<RfMacTypeTag> ()
  ;
  return tid;
}

TypeId
RfMacTypeTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

RfMacTypeTag::RfMacTypeTag (void)
  : m_type (RF_MAC_RESERVED)
{
}

uint32_t
RfMacTypeTag::GetSerializedSize (void) const
{
  return sizeof (uint8_t);
}

void
RfMacTypeTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_type);
}

void
RfMacTypeTag::Deserialize (TagBuffer i)
{
  m_type = i.ReadU8 ();
}

void
RfMacTypeTag::Print (std::ostream &os) const
{
  // os << "Lqi = " << m_type;
}

void
RfMacTypeTag::Set (enum RfMacTypeTag::RfMacType type)
{
  m_type = type;
}

enum RfMacTypeTag::RfMacType
RfMacTypeTag::Get (void) const
{
  switch (m_type)
    {
    case 0:
      return RF_MAC_RFE;
      break;
    case 1:
      return RF_MAC_CFE;
      break;
    case 2:
      return RF_MAC_CFE_ACK;
      break;
    case 3:
      return RF_MAC_ENERGY;
      break;
    case 4:
      return RF_MAC_DATA;
      break;
    default:
      return RF_MAC_RESERVED;
    }
}

bool
RfMacTypeTag::IsRfe (void) const
{
  return (m_type == RF_MAC_RFE);
}

bool
RfMacTypeTag::IsCfe (void) const
{
  return (m_type == RF_MAC_CFE);
}

bool
RfMacTypeTag::IsCfeAck (void) const
{
  return (m_type == RF_MAC_CFE_ACK);
}

bool
RfMacTypeTag::IsEnergy (void) const
{
  return (m_type == RF_MAC_ENERGY);
}

bool
RfMacTypeTag::IsData (void) const
{
  return (m_type == RF_MAC_DATA);
}
}
