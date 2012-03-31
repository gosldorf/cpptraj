#include "NameType.h"

NameType::NameType() :
  NameSize_(6)
{
  c_array_[0]=' ';
  c_array_[1]=' ';
  c_array_[2]=' ';
  c_array_[3]=' ';
  c_array_[4]=' ';
  c_array_[5]='\0';
}

NameType::NameType(const NameType &rhs) :
 NameSize_(6)
{
  c_array_[0] = rhs.c_array_[0];
  c_array_[1] = rhs.c_array_[1];
  c_array_[2] = rhs.c_array_[2];
  c_array_[3] = rhs.c_array_[3];
  c_array_[4] = rhs.c_array_[4];
  // 5 is always NULL
}

NameType::NameType(const char *rhs) :
  NameSize_(6)
{
  const char *ptr = rhs;
  for (unsigned int j = 0; j < NameSize_-1; j++) {
    c_array_[j] = *ptr;
    if (*ptr=='\0') break;
    ++ptr;
  }
  FormatName(true);
}

NameType::NameType(std::string &str) :
  NameSize_(6)
{
  unsigned int ns1 = NameSize_ - 1;
  unsigned int strend = (unsigned int)str.size();
  if (strend > ns1)
    strend = ns1;
  for (unsigned int j = 0; j < strend; j++) 
    c_array_[j] = str[j];
  c_array_[ns1] = '\0';
  FormatName(true);
}
 

/*NameType::NameType(char *rhs) :
  NameSize_(6)
{
  char *ptr = rhs;
  for (unsigned int j = 0; j < NameSize_-1; j++) {
    c_array_[j] = *ptr;
    if (*ptr=='\0') break;
    ++ptr;
  }
  //FormatName(true);
}*/

NameType &NameType::operator=(const NameType &rhs) {
  if (&rhs==this) return *this;
  c_array_[0] = rhs.c_array_[0];
  c_array_[1] = rhs.c_array_[1];
  c_array_[2] = rhs.c_array_[2];
  c_array_[3] = rhs.c_array_[3];
  c_array_[4] = rhs.c_array_[4];
  // 5 is always NULL
  return *this;
}

void NameType::AssignNoFormat(const char *rhs) {
  const char *ptr = rhs;
  for (unsigned int j = 0; j < NameSize_-1; j++) {
    c_array_[j] = *ptr;
    if (*ptr=='\0') break;
    ++ptr;
  }
  FormatName(false);
}

// NameType::ToBuffer()
/// For interfacing with old C stuff. Only set 1st 4 chars.
void NameType::ToBuffer(char *buffer) {
  buffer[0] = c_array_[0];
  buffer[1] = c_array_[1];
  buffer[2] = c_array_[2];
  buffer[3] = c_array_[3];
  buffer[4] = '\0';
}

bool NameType::Match(NameType &maskName) {

  for (int i = 0; i < 5; i++) {
    if (maskName.c_array_[i] == '*') // Mask wildcard: instant match
      return true;
    else if (maskName.c_array_[i] != '?' && 
             maskName.c_array_[i] != c_array_[i]) // Not mask single wildcard and mismatch
      return false;
  }
  return true;
}

/*
NameType &NameType::operator=(const char *rhs) {
  if (rhs==NULL) {
    c_array_[0] = '\0';
    return *this;
  }
  const char *ptr = rhs;
  for (unsigned int j = 0; j < NameSize_-1; j++) {
    c_array_[j] = (char)*ptr;
    if (*ptr=='\0') break;
    ++ptr;
  }
  FormatName();
  // 5 is always NULL
  return *this;
}
*/

bool NameType::operator==(const NameType &rhs) {
  if (c_array_[0] != rhs.c_array_[0]) return false;
  if (c_array_[1] != rhs.c_array_[1]) return false;
  if (c_array_[2] != rhs.c_array_[2]) return false;
  if (c_array_[3] != rhs.c_array_[3]) return false;
  return true;
}

bool NameType::operator==(const char *rhs) {
  NameType tmp(rhs);
  return (*this == tmp);
}

bool NameType::operator!=(const NameType &rhs) {
  if (c_array_[0] != rhs.c_array_[0]) return true;
  if (c_array_[1] != rhs.c_array_[1]) return true;
  if (c_array_[2] != rhs.c_array_[2]) return true;
  if (c_array_[3] != rhs.c_array_[3]) return true;
  return false;
}

bool NameType::operator!=(const char *rhs) {
  NameType tmp(rhs);
  return (*this != tmp);
}

/*
char *NameType::Assign(char *bufferIn) {
  char *ptr = bufferIn;
  for (unsigned int j = 0; j < NameSize_-1; j++) {
    c_array_[j] = *ptr;
    if (*ptr=='\0') break;
    ++ptr;
  }
  FormatName();
  // 5 is always NULL
  return ptr;
}
*/

const char *NameType::operator*() const {
  return c_array_;
}

// NameType::FormatName()
/** For consistency with Amber names, replace any NULL in the first 4 chars
  * with spaces. Remove any leading whitespace. Change any asterisk (*) to
  * prime ('). In cpptraj asterisks are considered reserved characters for
  * atom masks.
  */
void NameType::FormatName(bool replaceAsterisk) {
  // Ensure at least 4 chars long.
  if (c_array_[0]=='\0') {
    c_array_[0]=' ';
    c_array_[1]=' ';
    c_array_[2]=' ';
    c_array_[3]=' ';
  } else if (c_array_[1]=='\0') {
    c_array_[1]=' ';
    c_array_[2]=' ';
    c_array_[3]=' ';
  } else if (c_array_[2]=='\0') {
    c_array_[2]=' ';
    c_array_[3]=' ';
  } else if (c_array_[3]=='\0') {
    c_array_[3]=' ';
  }
  c_array_[4]='\0';
  // Remove leading whitespace.
  if        (c_array_[0]==' ') { // Some leading whitespace
    if (c_array_[1]!=' ') { // [_XXX]
      c_array_[0]=c_array_[1];
      c_array_[1]=c_array_[2];
      c_array_[2]=c_array_[3];
      c_array_[3]=' ';
    } else if (c_array_[2]!=' ') { // [__XX]
      c_array_[0]=c_array_[2];
      c_array_[1]=c_array_[3];
      c_array_[2]=' ';
      c_array_[3]=' ';
    } else if (c_array_[3]!=' ') { // [___X]
      c_array_[0]=c_array_[3];
      c_array_[1]=' ';
      c_array_[2]=' ';
      c_array_[3]=' ';
    }
  }
  if (!replaceAsterisk) return;
  // Replace asterisks with a single quote
  if (c_array_[0]=='*') c_array_[0]='\'';
  if (c_array_[1]=='*') c_array_[1]='\'';
  if (c_array_[2]=='*') c_array_[2]='\'';
  if (c_array_[3]=='*') c_array_[3]='\'';
}

char NameType::operator[](int idx) {
  if (idx < 0 || idx >= (int)NameSize_) return '\0';
  return c_array_[idx];
}

