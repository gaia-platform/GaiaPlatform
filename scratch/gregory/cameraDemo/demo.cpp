#include <random>
#include <string>
#include "event_manager.hpp"
#include "cameraDemo_gaia_generated.h"
extern "C" void initialize_rules();
std::string random_string(std::string::size_type length)
{
   const char chrs[] = "0123456789"
        "abcdefghijklmnopqrstuvwxyz";

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::string::size_type> dis;

    std::string s;

    s.reserve(length);

    while(length--)
        s += chrs[dis(gen)%(sizeof(chrs) - 1)];

    return s;
}

int main( int argc, const char** argv )
{
    gaia_mem_base::init(true);
    gaia::rules::initialize_rules_engine();
gaia::rules::list_subscriptions_t rules;
    //gaia::common::gaia_type_t t = CameraDemo::kCameraImageType;
    //gaia::rules::event_type_t et = gaia::rules::event_type_t::row_insert;
    gaia::rules::list_subscribed_rules(nullptr,nullptr,nullptr,rules);
    for (auto it = rules.begin(); it != rules.end(); ++it)
    {
        cerr << (*it)->ruleset_name << " " << (*it)->rule_name << " " << (int)((*it)->gaia_type) << " " << (int)((*it)->type) << endl;
    }
    cerr << rules.size() << endl;
    cerr << "01" << endl;
    CameraDemo::CameraImage::begin_transaction();
    cerr << "2" << endl;
    CameraDemo::CameraImage image;
    image.set_fileName("sfsdfsdf");
    cerr << "3" << endl;
    image.insert_row();
    cerr << "4" << endl;
    CameraDemo::CameraImage::commit_transaction();
    cerr << "5" << endl;
    return 0;
}
