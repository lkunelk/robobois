#include <stdio.h>
#include <iostream>
#include <descriptor.h>

#include "opencv2/core.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/imgproc.hpp"
using namespace cv;
using namespace cv::xfeatures2d;


// Callable SURF descriptor

/** @function main */
int Descriptor::compareImages( cv::Mat img_object, cv::Mat img_scene, double ratio)
{

//   Mat img_object = imread( argv[1], IMREAD_GRAYSCALE );
//   Mat img_scene = imread( argv[2], IMREAD_GRAYSCALE );

  if( !img_object.data || !img_scene.data )
  { std::cout<< " --(!) Error reading images " << std::endl; return -1; }

  // -- Step 1 & 2: Detect the keypoints and calculate descriptors using SURF Detector
  int minHessian = 400;
  Ptr<SURF> detector = SURF::create(minHessian);

  std::vector<KeyPoint> keypoints_object, keypoints_scene;
  Mat descriptors_object, descriptors_scene;
  detector->detectAndCompute(img_object, Mat(), keypoints_object, descriptors_object);
  detector->detectAndCompute(img_scene, Mat(), keypoints_scene, descriptors_scene);

  //-- Step 3: Matching descriptor vectors using FLANN matcher
  // FlannBasedMatcher matcher;
  // std::vector<std::vector<cv::DMatch>> matches;
  // matcher.match( descriptors_object, descriptors_scene, matches );

  std::vector<std::vector<cv::DMatch>> matches;
  cv::BFMatcher matcher;
  matcher.knnMatch(descriptors_object, descriptors_scene, matches, 2);  // Find two nearest matches
  std::vector< DMatch > good_matches;
  for (int i = 0; i < matches.size(); ++i)
  {
      if (matches[i][0].distance < ratio * matches[i][1].distance)
      {
          good_matches.push_back(matches[i][0]);
      }
  }


  double max_dist = 0; double min_dist = 100;

  //-- Quick calculation of max and min distances between keypoints
  // for( int i = 0; i < descriptors_object.rows; i++ )
  // { double dist = matches[i].distance;
  //   if( dist < min_dist ) min_dist = dist;
  //   if( dist > max_dist ) max_dist = dist;
  // }

  // printf("-- Max dist : %f \n", max_dist );
  // printf("-- Min dist : %f \n", min_dist );

  //-- Draw only "good" matches (i.e. whose distance is less than 3*min_dist )


  // VISUALIZING THE MATCHES
  Mat img_matches;
  drawMatches( img_object, keypoints_object, img_scene, keypoints_scene,
               good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
               std::vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

  //-- Localize the object
  std::vector<Point2f> obj;
  std::vector<Point2f> scene;

  for( int i = 0; i < good_matches.size(); i++ )
  {
    //-- Get the keypoints from the good matches
    obj.push_back( keypoints_object[ good_matches[i].queryIdx ].pt );
    scene.push_back( keypoints_scene[ good_matches[i].trainIdx ].pt );
  }

  Mat H = findHomography( obj, scene, RANSAC );

  //-- Get the corners from the image_1 ( the object to be "detected" )
  std::vector<Point2f> obj_corners(4);
  obj_corners[0] = cvPoint(0,0); obj_corners[1] = cvPoint( img_object.cols, 0 );
  obj_corners[2] = cvPoint( img_object.cols, img_object.rows ); obj_corners[3] = cvPoint( 0, img_object.rows );
  std::vector<Point2f> scene_corners(4);
  try
  {
    /* code */
      perspectiveTransform( obj_corners, scene_corners, H);
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    return -1;
  }
  

  //-- Draw lines between the corners (the mapped object in the scene - image_2 )
  line( img_matches, scene_corners[0] + Point2f( img_object.cols, 0), scene_corners[1] + Point2f( img_object.cols, 0), Scalar(0, 255, 0), 4 );
  line( img_matches, scene_corners[1] + Point2f( img_object.cols, 0), scene_corners[2] + Point2f( img_object.cols, 0), Scalar( 0, 255, 0), 4 );
  line( img_matches, scene_corners[2] + Point2f( img_object.cols, 0), scene_corners[3] + Point2f( img_object.cols, 0), Scalar( 0, 255, 0), 4 );
  line( img_matches, scene_corners[3] + Point2f( img_object.cols, 0), scene_corners[0] + Point2f( img_object.cols, 0), Scalar( 0, 255, 0), 4 );

  rectangle(img_matches, scene_corners[0] + Point2f( img_object.cols, 0),  scene_corners[2] + Point2f( img_object.cols, 0), Scalar(255, 255, 0), 1);

  cv::Point2f pt1 = scene_corners[0] + Point2f( img_object.cols, 0);
  cv::Point2f pt2 = scene_corners[1] + Point2f( img_object.cols, 0);
  cv::Point2f pt3 = scene_corners[2] + Point2f( img_object.cols, 0);
  cv::Point2f pt4 = scene_corners[3] + Point2f( img_object.cols, 0);

  float area = pt1.x * pt2.y - pt1.y * pt2.x + pt2.x * pt3.y - pt2.y * pt3.x + pt3.x * pt4.y - pt3.y * pt4.x + pt4.x * pt1.y - pt4.y * pt1.x;
  area = area / 2;
  std::cout << "Area of Bounding Box: " << area << std::endl;
  //-- Show detected matches
  // imshow( "Good Matches & Object detection", img_matches );
  std::cout << "Number of Good matches: " << good_matches.size() << std::endl;


  // while(cv::waitKey(1) != 27) {
  // }
  return area;
  }