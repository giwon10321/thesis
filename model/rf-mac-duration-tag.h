#ifndef RF_MAC_DURATION_TAG_H
#define RF_MAC_DURATION_TAG_H

#include <ns3/tag.h>
#include <ns3/simulator.h>

namespace ns3 {

class RfMacDurationTag : public Tag
{
public:
  /**
   * Get the type ID.
   *
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;

  RfMacDurationTag (void);

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  void Set (Time time);

  Time Get (void) const;
private:
  double m_duration;
};


}
#endif /* RF_MAC_DURATION_TAG_H */
