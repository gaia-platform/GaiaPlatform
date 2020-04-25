#include <random>
#include <string>
#include "rules.hpp"
#include "cameraDemo_gaia_generated.h"

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"


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
    gaia_mem_base::init(true);
    gaia::rules::initialize_rules_engine();

    cv::VideoCapture capture;
    cv::Mat frame, image;
    if (!capture.open(0))
    {
        cout << "Capture from camera #  0 didn't work" << endl;
        return 1;
    }
    if( capture.isOpened() )
    {
        cout << "Video capturing has been started ..." << endl;

        for(;;)
        {
            capture >> frame;
            if( frame.empty() )
                break;
    
            std::string file_name = random_string(10) + ".tiff";

	        imwrite(file_name.c_str(), frame);

            begin_transaction();
            CameraDemo::Camera_image image;
            image.set_file_name(file_name.c_str());
            image.insert_row();
            commit_transaction();

            char c = (char)cv::waitKey(100);
            if( c == 27 || c == 'q' || c == 'Q' )
                break;
        }
    }
    else
    {
        cerr << "capture is not opened" << endl;
    }

    return 0;
}
