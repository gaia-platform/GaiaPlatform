#include <sqlite3.h>

#include <cstdio>

#include <chrono>
#include <sstream>
const constexpr uint32_t c_insert_buffer_stmts = 1000000;
const constexpr uint32_t c_num_vertexes = 10000000;

using namespace std::chrono;

void generate_data(sqlite3* db)
{
    std::stringstream insert_ss;
    insert_ss << "BEGIN TRANSACTION;";

    for (int i = 0; i < c_num_vertexes; i++)
    {
        insert_ss << "INSERT INTO Vertices(id, type) VALUES (" << i << ", " << (i % 5) << ");" << std::endl;

        if (i > 1)
        {
            insert_ss << "INSERT INTO Edges(id, src_id, dest_id) VALUES (" << i << ", " << (i - 1) << ", " << i << ");" << std::endl;
        }

        if (i % c_insert_buffer_stmts == 0 || i == c_num_vertexes - 1)
        {
            fprintf(stdout, "Inserting data: %i...\n", i);

            insert_ss << "COMMIT;";
            char* err_msg = nullptr;
            int rc = sqlite3_exec(db, insert_ss.str().c_str(), nullptr, nullptr, &err_msg);
            insert_ss = {};
            insert_ss << "BEGIN TRANSACTION;";

            if (rc != SQLITE_OK)
            {

                fprintf(stderr, "SQL error: %s\n", err_msg);

                sqlite3_free(err_msg);
                sqlite3_close(db);

                exit(1);
            }
        }
    }
}

void assert_result(sqlite3* db, int rc)
{
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }
}

sqlite3_stmt* query(sqlite3* db, const char* query)
{

    sqlite3_stmt* res;
    int rc = sqlite3_prepare_v2(db, query, -1, &res, nullptr);

    assert_result(db, rc);

    return res;
}

void exec(sqlite3* db, const char* query)
{
    char* err_msg = nullptr;

    int rc = sqlite3_exec(db, query, nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    }
}

// void consume_data(sqlite3* db, sqlite3_stmt* stmt, bool )
//{
//
// }

void query_data(sqlite3* db)
{
    steady_clock::time_point begin = std::chrono::steady_clock::now();
    sqlite3_stmt* stmt = query(db, "SELECT id FROM Vertices WHERE type = ?");
    sqlite3_bind_int(stmt, 1, 1);

    int i = 0;

    while (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_column_int64(stmt, 0);
        i++;
    }
    steady_clock::time_point end = std::chrono::steady_clock::now();

    auto elapsed = std::chrono::duration_cast<milliseconds>(end - begin).count();

    printf("Num of vertexes of type 1 %d in %ldms\n", i, elapsed);
    sqlite3_finalize(stmt);
}

int main()
{

    sqlite3* db;

    int rc = sqlite3_open("test.db", &db);

    assert_result(db, rc);

    printf("Autocommit: %d\n", sqlite3_get_autocommit(db));

    sqlite3_stmt* version_stmt = query(db, "SELECT SQLITE_VERSION()");

    sqlite3_step(version_stmt);
    printf("%s\n", sqlite3_column_text(version_stmt, 0));
    sqlite3_finalize(version_stmt);

    exec(db, "CREATE TABLE IF NOT EXISTS Graph(uuid TEXT DEFAULT \"aaaaaaaa-0000-0000-0000-000000000000\");\n"
             "CREATE TABLE IF NOT EXISTS Vertices(id INTEGER PRIMARY KEY, type INTEGER, pose BLOB, deprecation_count INTEGER, reference INTEGER, session INTEGER, data BLOB, pose_x REAL, pose_y REAL, stamp_sec INTEGER, stamp_nsec INTEGER, level INTEGER, compression TEXT, source TEXT, confidence REAL);\n"
             "CREATE TABLE IF NOT EXISTS Edges(id INTEGER PRIMARY KEY, dest_id INTEGER, src_id INTEGER, gen_id TEXT, pose BLOB, confidence REAL, robust INTEGER);\n"
             "CREATE TABLE IF NOT EXISTS PointEdges(id INTEGER PRIMARY KEY, dest_id INTEGER, dest_pt_id INTEGER, src_id INTEGER, confidence REAL, robust INTEGER);\n"
             "PRAGMA user_version = 4;");

    sqlite3_stmt* count_stmt = query(db, "SELECT COUNT(*) FROM Vertices");

    sqlite3_step(count_stmt);
    int count = sqlite3_column_int(count_stmt, 0);
    sqlite3_finalize(count_stmt);

    if (count == 0)
    {
        printf("No vertexes, generating data...\n");
        generate_data(db);
    }
    else
    {
        printf("Num vertexes: %d\n", count);
    }

    query_data(db);

    sqlite3_close(db);

    return 0;
}
