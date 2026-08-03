#include "../helper/UuidGen.h"
#include <boost/uuid.hpp>

class Event
{
protected:
    boost::uuids::uuid m_id;

public:
    Event() : m_id(uuidGen()) {}
    boost::uuids::uuid getUuid() const { return m_id; }
};