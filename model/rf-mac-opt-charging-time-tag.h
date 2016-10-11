#ifndef RF_MAC_OPT_CHARGING_TIME_TAG_H
#define RF_MAC_OPT_CHARGING_TIME_TAG_H

#include <ns3/tag.h>
#include <ns3/simulator.h>

namespace ns3 {

class RfMacOptChargingTimeTag : public Tag
{
public:
  /**
   * Get the type ID.
   *
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;

  RfMacOptChargingTimeTag (void);

  RfMacOptChargingTimeTag (uint8_t lqi);

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  void Set (Time time);

  Time Get (void) const;
private:
  double m_chargingTime;
};


}
#endif /* RF_MAC_OPT_CHARGING_TIME_TAG_H */
