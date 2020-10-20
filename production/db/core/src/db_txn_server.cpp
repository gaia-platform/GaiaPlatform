
#include "db_server.hpp"
#include "db_txn.hpp"

gaia::db::gaia_txn_id_t gaia::db::get_txn_id()
{
    return server::s_txn_id;
}

bool gaia::db::is_transaction_active()
{
    return true;
}
