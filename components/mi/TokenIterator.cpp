#include "TokenIterator.h"
#include <cstring>

TokenIterator::TokenIterator(const char* data, size_t length, const char sep)
  : data(new char[length+1]),
    current(this->data),
    length(length),
    i(0)
{
  strncpy(this->data, data, length);
  this->data[length] = 0;

  for (size_t ix = 0; ix < length; ix++) {
    if (this->data[ix] == sep) {
      this->data[ix] = 0;
    }
  }
}

const char* TokenIterator::nextToken() {
  if (i >= length) {
    return nullptr;
  }

  char* token = current;
  char* nextToken = current;

  for (; i < length && *nextToken != 0; i++, nextToken++);

  if (i == length) {
    nextToken = nullptr;
  } else {
    i = (nextToken - data);

    if (i < length) {
      nextToken++;
    } else {
      nextToken = nullptr;
    }
  }

  current = nextToken;

  return token;
}

void TokenIterator::reset() {
  current = data;
  i = 0;
}

bool TokenIterator::hasNext() {
  return i < length;
}

TokenIterator::~TokenIterator() {
  delete[] data;
}
