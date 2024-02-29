/* $Id: WebAuthenticator.h,v 1.5 2007/05/30 13:00:08 mheiskan Exp $
 * Copyright (c) 2005 Finnish Meteorological Institute. All rights reserved.
 * Written by Antti Westerberg <antti.westerberg@fmi.fi>
 *
 * Changelog:
 *
 *   $Log: WebAuthenticator.h,v $
 *   Revision 1.5  2007/05/30 13:00:08  mheiskan
 *   Muutin MD5Digest metodin julkiseksi.
 *
 *   Revision 1.4  2005/06/30 06:24:57  westerba
 *   Fixed std::string related bugs in MD5 message digest calculation.
 *   (MD5Digest was spitting out MD5 digests calculated from erroneous strings)
 *
 *   Revision 1.3  2005/06/28 10:10:41  westerba
 *   Hid MD5 implementation from library headers  (MD5Digest method is now in md5.cc)
 *
 *   Revision 1.2  2005/06/28 09:32:07  westerba
 *   Added CVS changelogs
 *
 *
 */

#ifndef _HAVE_WEBAUTHENTICATOR_H
#define _HAVE_WEBAUTHENTICATOR_H

#include <iostream>
#include <string>

class WebAuthenticator
{
 private:
  // Authentication parameters
  std::string md5_secret;

 public:
  // ----------------------------------------------------------------------
  /*!
   * \brief Constructor for WebAuthenticator class
   *
   * \param secret Secret key used in MD5 message digest calculation
   */
  // ----------------------------------------------------------------------
  WebAuthenticator();
  WebAuthenticator(const std::string& secret);

  // ----------------------------------------------------------------------
  /*!
   * \brief Destructor for WebAuthenticator class
   */
  // ----------------------------------------------------------------------
  ~WebAuthenticator();

  // ----------------------------------------------------------------------
  /*!
   * \brief Method for verifying the integrity of query string
   *
   * \param query std::string query string to be validated
   *
   * \return \a true if query string is valid,
   * \return \a false if query string is expired or tampered with
   */
  // ----------------------------------------------------------------------
  bool isValidQuery(const std::string& query);

  // ----------------------------------------------------------------------
  /*!
   * \brief Method for obtaining plain query string (w/o authentication info)
   *
   * \param query std::string query string to be normalized
   *
   * \return query string without
   */
  // ----------------------------------------------------------------------
  const std::string canonizeQuery(const std::string& query);

  // MD5 message digest calculation method (in md5.cc)
  const std::string MD5Digest(const std::string& secretStr, const std::string& myStr);
};

#endif  // _HAVE_WEBAUTHENTICATOR_H
