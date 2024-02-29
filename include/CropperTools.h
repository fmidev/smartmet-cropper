// ======================================================================
/*!
 * \file
 * \brief Utility subroutines for cropper
 */
// ======================================================================

#ifndef CROPPERTOOLS_H
#define CROPPERTOOLS_H

#include <memory>
#include <string>

class NFmiArea;
class NFmiPoint;

#include <imagine/NFmiColorTools.h>
#include <imagine/NFmiImage.h>
#include <newbase/NFmiAreaFactory.h>

void usage(const std::string& theProgName);
void set_timezone(const std::string& theZone);
const std::string format_time(const ::time_t theTime);
void http_output_image(const std::string& theFile);
const std::string cachename(const std::string& tehQueryString);
bool not_modified(const std::string& theFile);
bool http_output_cache(const char* theQueryString);
NFmiAreaFactory::return_type create_map(const std::string& theMap);
const NFmiPoint find_location(const std::string& theName);
const std::string get_suffix(const std::string& theFilename);
void http_output_image(const Imagine::NFmiImage& theImage,
                       const std::string& theFile,
                       const std::string& theType,
                       bool theCacheFlag);
void parse_geometry(const std::string& theGeometry, int& x1, int& y1, int& width, int& height);
void parse_center_geometry(
    const std::string& theGeometry, int& xc, int& yc, int& width, int& height);

NFmiAreaFactory::return_type parse_latlon_geometry(
    const std::string& theGeometry, int& xc, int& yc, int& width, int& height);
NFmiAreaFactory::return_type parse_named_geometry(
    const std::string& theGeometry, int& xc, int& yc, int& width, int& height);
std::unique_ptr<Imagine::NFmiImage> crop_corner(const Imagine::NFmiImage& theImage,
                                                int theX1,
                                                int theY1,
                                                int theWidth,
                                                int theHeight,
                                                int& theXoff,
                                                int& theYoff);
std::unique_ptr<Imagine::NFmiImage> crop_center(const Imagine::NFmiImage& theImage,
                                                int theXC,
                                                int theYC,
                                                int theWidth,
                                                int theHeight,
                                                int& theXoff,
                                                int& theYoff);

Imagine::NFmiColorTools::Color parse_color(const std::string& theColor);
const std::vector<std::string> extract_timestamps(const std::string& theString);

const ::tm parse_stamp(const std::string& theStamp);
std::string make_timestamp(const std::string& theFilename,
                           const std::string& theType,
                           const std::string& theFormat);
void draw_timestamp(Imagine::NFmiImage& theImage,
                    const std::string& theOptions,
                    const std::string& theFilename);
void draw_labels(Imagine::NFmiImage& theImage,
                 const NFmiArea& theArea,
                 int theXoff,
                 int theYoff,
                 const std::string& theOptions);
void draw_center(Imagine::NFmiImage& theImage, const std::string& theOptions, int theX, int theY);
void draw_image(Imagine::NFmiImage& theImage, const std::string& theOptions);
void reduce_colors(Imagine::NFmiImage& theImage, const std::string& theSpecs);
int domain(int argc, const char* argv[]);

#endif  // CROPPERTOOLS_H

// ======================================================================
