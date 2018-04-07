/* $Id: WebAuthenticator.cc,v 1.6 2007/05/30 12:59:29 mheiskan Exp $
 * Authentication Engine for query string
 *
 * Copyright (c) 2005 Finnish Meteorological Institute. All rights reserved.
 * Written by Antti Westerberg <antti.westerberg@fmi.fi>
 *
 * Changelog:
 *
 *    $Log: WebAuthenticator.cc,v $
 *    Revision 1.6  2007/05/30 12:59:29  mheiskan
 *    Koodi oli laitettu namespaceen, vaikka deklaraatio ei ollut namespacessa. Otin siten
 * ylimääräisen namespacen pois.
 *
 *    Revision 1.5  2005/06/30 06:24:57  westerba
 *    Fixed std::string related bugs in MD5 message digest calculation.
 *    (MD5Digest was spitting out MD5 digests calculated from erroneous strings)
 *
 *    Revision 1.4  2005/06/28 10:08:37  westerba
 *    Fixed bugs in canonizeQuery() method and hid the MD5Digest implementation for library headers
 *
 *    Revision 1.3  2005/06/28 09:32:07  westerba
 *    Added CVS changelogs
 *
 */

#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <string>

using namespace std;

//
// NO NOT CHANGE THE SECRET KEY UNLESS GLOBAL WORLD SECURITY IS ENDANGERED!
//
// The same secret key is in use in other places also (including scripts
// written in PHP), so they all have to be changed simultaneously!
#define DEFAULT_SECRET ("~-Is0dro5oth3rm-~")

// Query string has to be in the following syntax:
// <query string>&exp=<unix timestamp of expiration>&auth=<MD5 digest>
//
#define MD5_DIGEST_NAME "&auth="
#define MD5_DIGEST_SIZE (sizeof(MD5_DIGEST_NAME) - 1)

#define EXPIRATION_NAME "&exp="
#define EXPIRATION_SIZE (sizeof(EXPIRATION_NAME) - 1)

#include "WebAuthenticator.h"
#include "md5.h"

// ----------------------------------------------------------------------
/*!
 * \brief Constructors for Authenticator class
 *
 * \param secret Secret key used in MD5 message digest calculation
 */
// ----------------------------------------------------------------------
WebAuthenticator::WebAuthenticator(const std::string& secret) : md5_secret(secret) {}
// ----------------------------------------------------------------------
/*!
 * \brief Constructors for Authenticator class using \a DEFAULT secret key
 *
 */
// ----------------------------------------------------------------------
WebAuthenticator::WebAuthenticator() : md5_secret(DEFAULT_SECRET) {}
// ----------------------------------------------------------------------
/*!
 * \brief Destructor for Authenticator class
 */
// ----------------------------------------------------------------------
WebAuthenticator::~WebAuthenticator() {}
// ----------------------------------------------------------------------
/*!
 * \brief Method for verifying the integrity of supplied query string
 *
 * \param query std::string query string to be validated
 *
 * \return \a true if query string is valid,
 * \return \a false if query string is expired or tampered with
 */
// ----------------------------------------------------------------------
bool WebAuthenticator::isValidQuery(const std::string& query)
{
  std::size_t md5_pos = 0;
  std::size_t exp_pos = 0;

  // Find the start of authentication MD5 digest in query string
  if ((md5_pos = query.rfind(MD5_DIGEST_NAME)) == std::string::npos)
  {
    // No authentication digest found, authentication failed
    return false;
  }

  // Compare the MD5 digests
  if (WebAuthenticator::MD5Digest(md5_secret, query.substr(0, md5_pos)) !=
      query.substr(md5_pos + MD5_DIGEST_SIZE, query.length()))
  {
    // MD5 digests didn't match -- QUERY HAS BEEN TAMPERED WITH
    return false;
  }

  // Find the start of expiration time stamp in query string
  //
  // Note: This is optional parameter, and query string
  //       without expiration time will be accepted if
  //       MD5 digests match.
  if ((exp_pos = query.rfind(EXPIRATION_NAME)) != std::string::npos)
  {
    // Sanity check for MD5 vs. Expiration parameter positions
    if (exp_pos > md5_pos)
    {
      // Error in query string syntax, authentication failed
      return false;
    }

    // Get expiration time from query string (KLUDGE!)
    int expiration_time = atoi(
        query.substr(exp_pos + EXPIRATION_SIZE, (md5_pos - exp_pos) - EXPIRATION_SIZE).c_str());

    // Compare expiration time to current unix timestamp
    if (expiration_time < time(NULL))
    {
      // This authentication ticket is expired
      return false;
    }
  }

  // All integrity tests passed, authentication accepted
  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Method for obtaining plain query string (w/o authentication info)
 *
 * \param query std::string query string to be normalized
 *
 * \return query string without
 */
// ----------------------------------------------------------------------
const std::string WebAuthenticator::canonizeQuery(const std::string& query)
{
  std::size_t start_pos = 0;
  std::size_t exp_pos = 0;

  // Find the start of authentication MD5 digest in query string
  if ((start_pos = query.rfind(MD5_DIGEST_NAME)) == std::string::npos)
  {
    // Query string doesn't contain authentication information,
    // just return back the supplied query string.
    return query;
  }

  // Find the start of expiration time stamp in query string
  //
  // Note: This is an optional parameter, so we are not sure
  //       whether it will be found on query string.
  if ((exp_pos = query.rfind(EXPIRATION_NAME)) != std::string::npos)
  {
    // Query string also *DOES CONTAIN* expiration time, so we'll
    // cut the string from the start of the expiration time.
    start_pos = exp_pos;
  }

  // Return a substring of the query string till the start of authentication info
  return query.substr(0, start_pos);
}
