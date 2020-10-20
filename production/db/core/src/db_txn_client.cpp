#include "db_client.hpp"
#include "db_txn.hpp"

gaia::db::gaia_txn_id_t gaia::db::get_txn_id()
{
    return client::s_txn_id;
}

gaia::db::gaia_txn_id_t gaia::db::get_begin_ts()
{
    return client::s_log->begin_ts;
}
