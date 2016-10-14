#ifndef RF_MAC_GROUP_TAG_H
#define RF_MAC_GROUP_TAG_H

#include <ns3/tag.h>

namespace ns3 {

class RfMacGroupTag : public Tag
{
public:
  /**
   * Get the type ID.
   *
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * Create a RfMacGroupTag with the default LQI 0.
   */
  RfMacGroupTag (void);

  /**
   * Create a RfMacGroupTag with the given LQI value.
   */

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  /**
   * Set the LQI to the given value.
   *
   * \param lqi the value of the LQI to set
   */
  void Set (uint8_t group);

  /**
   * Get the LQI value.
   *
   * \return the LQI value
   */
  uint8_t Get (void) const;
private:
  /**
   * The current LQI value of the tag.
   */
  uint8_t m_group;
};


}
#endif /* RF_MAC_GROUP_TAG_H */
