#ifndef APP_H
#define APP_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif


extern Uart ext;

typedef enum {
  GLASS_BREAK = 1,
  Z_OPENSET,
} classifier_match_t;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif