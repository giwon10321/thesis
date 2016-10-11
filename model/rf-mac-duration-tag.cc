#include "rf-mac-duration-tag.h"
#include <ns3/integer.h>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RfMacDurationTag);

RfMacDurationTag::RfMacDurationTag ()
{
}

void
RfMacDurationTag::Set (Time time)
{
  m_duration = time.ToDouble (Time::US); 
}

Time
RfMacDurationTag::Get (void) const
{
  return MicroSeconds (m_duration);
}

TypeId
RfMacDurationTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RfMacDurationTag")
    .SetParent<Tag> ()
    .AddConstructor<RfMacDurationTag> ()
    .SetGroupName("LrWpan")
  ;
  return tid;
}

  // inherited function, no need to doc.
TypeId
RfMacDurationTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
  
  // inherited function, no need to doc.
uint32_t
RfMacDurationTag::GetSerializedSize (void) const
{
  return 8;
}  
  // inherited function, no need to doc.
void
RfMacDurationTag::Serialize (TagBuffer i) const
{
  i.WriteDouble (m_duration);
}
  
  // inherited function, no need to doc.
void
RfMacDurationTag::Deserialize (TagBuffer i)
{
  m_duration = i.ReadDouble ();
}

  // inherited function, no need to doc.
void
RfMacDurationTag::Print (std::ostream &os) const
{}

}
