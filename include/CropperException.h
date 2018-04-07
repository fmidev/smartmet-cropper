// ======================================================================
/*!
 * \brief Interface of class CropperException
 */
// ======================================================================

#ifndef CROPPEREXCEPTION_H
#define CROPPEREXCEPTION_H

#include <string>

class CropperException
{
 public:
  CropperException(int theStatus, const std::string& theMessage)
      : itsStatus(theStatus), itsMessage(theMessage)
  {
  }

  int status() const { return itsStatus; }
  const std::string& what() const { return itsMessage; }

 private:
  CropperException();
  int itsStatus;
  std::string itsMessage;
};

#endif  // CROPPEREXCEPTION_H

// ======================================================================
