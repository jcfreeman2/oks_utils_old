#ifndef __DATA_EDITOR_EXCEPTION_H
#define __DATA_EDITOR_EXCEPTION_H

#include <ers/ers.h>

ERS_DECLARE_ISSUE(
  OksDataEditor,
  InternalProblem,
  "internal problem: " << problem,
  ((const char *)problem)
)

ERS_DECLARE_ISSUE(
  OksDataEditor,
  Problem,
  "there is a problem: " << problem,
  ((const char *)problem)
)

#endif
