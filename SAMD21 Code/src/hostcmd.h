#ifndef HOSTCMD_H
#define HOSTCMD_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

static constexpr uint8_t header = 0xAC;
static constexpr uint8_t stop = 0xFA;

typedef enum __attribute__((packed, aligned(1)))
{
    NOTIF_CLASSIFIER_MATCH,
    NOTIF_CLASSIFIER_ERROR,
    NOTIF_FAULT,
    NOTIF_ALIVE
} notif_type_t;




void hostcmd_send_alive(void);
void hostcmd_send_classifier_match(classifier_match_t match);
void hostcmd_send_classifier_error(uint8_t error);
void hostcmd_send_fault(uint8_t fault);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // HOSTCMD_H