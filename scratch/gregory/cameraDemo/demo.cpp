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
    //gaia::rules::event_manager_t::get();
    initialize_rules();
    gaia_mem_base::init(true);
    CameraDemo::CameraImage::begin_transaction();
    CameraDemo::CameraImage image;
    image.set_fileName("sfsdfsdf");
    image.update_row();
    CameraDemo::CameraImage::commit_transaction();
    return 0;
}
