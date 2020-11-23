#include <random>
#include <string>
#include <iostream>
#include "gaia/rules/rules.hpp"
#include "gaia_cameraDemo.h"

#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include <X11/Xlib.h>

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
    {
        s += chrs[dis(gen)%(sizeof(chrs) - 1)];
    }

    return s;
}

int main( int argc, const char** argv )
{
    XInitThreads();
    gaia::db::begin_session();
    gaia::rules::initialize_rules_engine();

    cv::VideoCapture capture;
    cv::Mat frame, image;
    if (!capture.open(0))
    {
        std::cout << "Capture from camera #  0 didn't work" << endl;
        return 1;
    }
    auto tmp = std::string("/tmp/");
    if( capture.isOpened() )
    {
        std::cout << "Video capturing has been started ..." << endl;

        for(;;)
        {
            capture >> frame;
            if( frame.empty() )
                break;
    
            std::string file_path = tmp + random_string(10) + ".tiff";

            imwrite(file_path, frame);
	    
            gaia::db::begin_transaction();
            gaia::cameraDemo::camera_image_writer image = gaia::cameraDemo::camera_image_writer();;
            image.file_name = file_path.c_str();
            image.insert_row();
            gaia::db::commit_transaction();

            char c = (char)cv::waitKey(100);
            if( c == 27 || c == 'q' || c == 'Q' )
                break;
        }
    }
    else
    {
        std::cerr << "capture is not opened" << endl;
    }
    gaia::db::end_session();
    return 0;
}
