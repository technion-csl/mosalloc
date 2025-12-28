#pragma once
#include <stddef.h>

struct mallinfo2 {
  size_t arena, ordblks, smblks, hblks, hblkhd,
         usmblks, fsmblks, uordblks, fordblks, keepcost;
};
