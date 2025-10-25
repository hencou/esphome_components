#include <stddef.h>

#ifndef _TOKEN_ITERATOR_H
#define _TOKEN_ITERATOR_H

class TokenIterator {
public:
  TokenIterator(const char* data, size_t length, char sep = '/');
  ~TokenIterator();

  bool hasNext();
  const char* nextToken();
  void reset();

private:
  char* data;
  char* current;
  size_t length;
  size_t i;
};

#endif
