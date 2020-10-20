#include "db_client.hpp"
#include "txn_metadata.hpp"

namespace gaia
{
namespace db
{
gaia_txn_id_t txn_metadata_t::get_current_txn_id()
{
    return client_t::get_txn_id();
}

} // namespace db
} // namespace gaia
