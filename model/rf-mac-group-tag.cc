#include "rf-mac-group-tag.h"
#include <ns3/integer.h>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RfMacGroupTag);

TypeId
RfMacGroupTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RfMacGroupTag")
    .SetParent<Tag> ()
    .SetGroupName ("LrWpan")
    .AddConstructor<RfMacGroupTag> ()
  ;
  return tid;
}

TypeId
RfMacGroupTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

RfMacGroupTag::RfMacGroupTag (void)
  : m_group (0)
{
}

uint32_t
RfMacGroupTag::GetSerializedSize (void) const
{
  return sizeof (uint8_t);
}

void
RfMacGroupTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_group);
}

void
RfMacGroupTag::Deserialize (TagBuffer i)
{
  m_group = i.ReadU8 ();
}

void
RfMacGroupTag::Print (std::ostream &os) const
{
  os << "Lqi = " << m_group;
}

void
RfMacGroupTag::Set (uint8_t group)
{
  m_group = group;
}

uint8_t
RfMacGroupTag::Get (void) const
{
  return m_group;
}

}
