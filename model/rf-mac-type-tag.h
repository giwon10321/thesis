#ifndef RF_MAC_TYPE_TAG_H
#define RF_MAC_TYPE_TAG_H

#include <ns3/tag.h>
#include <ns3/simulator.h>

namespace ns3 {

class RfMacTypeTag : public Tag
{
public:
  enum RfMacType
  {
    RF_MAC_RFE = 0,
    RF_MAC_CFE = 1,
    RF_MAC_CFE_ACK = 2,
    RF_MAC_ENERGY = 3,
    RF_MAC_DATA = 4,
    RF_MAC_RESERVED
  };
  /**
   * Get the type ID.
   *
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;

  RfMacTypeTag (void);

  RfMacTypeTag (RfMacType);

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  void Set (RfMacType type);

  enum RfMacType Get (void) const;

  bool IsRfe (void) const;
  bool IsCfe (void) const;
  bool IsCfeAck (void) const;
  bool IsEnergy (void) const;
  
private:
  uint8_t m_type;
};


}
#endif /* RF_MAC_TYPE_TAG_H */
