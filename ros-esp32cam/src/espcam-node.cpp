#include "curl/curl.h" // has to go before opencv headers
#include <stdio.h>
#include <stdlib.h>
#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <opencv2/highgui/highgui.hpp>
#include <cv_bridge/cv_bridge.h>
#include <iostream>
#include <vector>
using namespace std;

#include <opencv2/opencv.hpp>
using namespace cv;

//curl writefunction to be passed as a parameter
// we can't ever expect to get the whole image in one piece,
// every router / hub is entitled to fragment it into parts
// (like 1-8k at a time),
// so insert the part at the end of our stream.
size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    vector<uchar> *stream = (vector<uchar>*)userdata;
    size_t count = size * nmemb;
    stream->insert(stream->end(), ptr, ptr + count);
    return count;
}


int main(int argc , char** argv)
{
    vector<uchar> stream;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.2.165/cam-hi.jpg"); //the img url
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // pass the writefunction
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream); // pass the stream ptr to the writefunction
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10); // timeout if curl_easy hangs, 

    ros::init(argc, argv , "esp32cam");    
    ros::NodeHandle nh;   
    image_transport::ImageTransport it(nh);
    image_transport::Publisher pub = it.advertise("esp32cam/image", 1);
    sensor_msgs::ImagePtr msg;
    ros::Rate loop_rate(5);
    while (nh.ok()) {
       vector<uchar> stream;
       CURL *curl = curl_easy_init();
       curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.2.165/cam-hi.jpg"); //the img url
       curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // pass the writefunction
       curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream); // pass the stream ptr to the writefunction
       curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10); // timeout if curl_easy hangs, 

       CURLcode res = curl_easy_perform(curl); // start curl

       if (res != CURLE_OK) {
          fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
          } 

       else {
          Mat image = imdecode(stream, -1); 
          if (!image.empty()){
              namedWindow( "Image output", CV_WINDOW_AUTOSIZE );
              imshow("Image output",image);
              msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", image).toImageMsg();
              pub.publish(msg);
              cv::waitKey(1);
              }
       }
       curl_easy_cleanup(curl); // cleanup
       ros::spinOnce();
       loop_rate.sleep();
    }	
}

