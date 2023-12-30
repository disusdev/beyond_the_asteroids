#include "cstr.h"

//#include <core/asserts.h>

u64
cstr_length(const char* str)
{
  char* p = (char*)str;
  while (*p != '\0') p++;
  return (p - str);
}

f64
cstr_2flt(char* str, i32* out_len, char end)
{
  f64 val = 0;
  f64 dot_scale = 1.0;
  f64 scale = 1.0;
  f64 neg = 1.0;
  f64 mult = 1.0;
  (*out_len) = 0;
  
  if (*str == '-')
  {
    str++;
    neg = -1.0;
    (*out_len)++;
  }
  
  while (*str != end)
  {
    if (*str == '.')
    {
      dot_scale = 0.1;
    }
    else if (*str == 'e')
    {
      str += 2;
      (*out_len) += 2;
      i32 len = 0;
      i32 i = cstr_2int(str, &len);
      while(i--)
      {
        mult *= 0.1;
      }
      str += len;
      (*out_len) += len;
      break;
    }
    else
    {
      scale = scale * dot_scale;
      val = val * 10.0 * dot_scale + (*str - '0') * scale;
    }
    (*out_len)++;
    str++;
  }
  
  val *= mult;
  val *= neg;
  
  return val;
}

i64
cstr_2int(char* str, i32* out_len)
{
  i64 val = 0;
  i64 neg = 1;
  (*out_len) = 0;
  
  if (*str == '-')
  {
    str++;
    neg = -1;
    (*out_len)++;
  }
  
  while (true)
  {
    if (*str == '\r')
    {
      (*out_len)++;
      str++; continue;
    }
    
    if (*str < 48 || *str > 57)
    {
      break;
    }
    
    val = val * 10 + (*str - '0');
    str++;
    (*out_len)++;
  }
  
  if (val * neg < 0)
  {
    // LOG_ERROR("str: $s, val = $i, neg = $i", str-(*out_len), val, neg);
    // ASSERT(false);
  }
  
  return val * neg;
}
