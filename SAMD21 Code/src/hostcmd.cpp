#include "main.h"
#include "hostcmd.h"

void hostcmd_send(uint8_t cmd, uint8_t *data, uint8_t len) {
  digitalWrite(A0, HIGH);
  uint32_t checksum = 0x00;
  for(int i=0; i<len; i++) checksum += data[i];
  ext.write(header);
  ext.write(cmd);
  ext.write(len);
  ext.write(data, len);
  ext.write(~(uint8_t)(checksum & 0x000000FF)); // negated LSB of summed data
  ext.write(stop);
  digitalWrite(A0, LOW);
}


void hostcmd_send_alive(void) {
  hostcmd_send(NOTIF_ALIVE, NULL, 0);
}

void hostcmd_send_classifier_match(classifier_match_t match) {
  hostcmd_send(NOTIF_CLASSIFIER_MATCH, (uint8_t *)&match, sizeof(match));
}

void hostcmd_send_classifier_error(uint8_t error) {
  hostcmd_send(NOTIF_CLASSIFIER_ERROR, &error, sizeof(error));
}


void hostcmd_send_fault(uint8_t fault) {
  hostcmd_send(NOTIF_FAULT, &fault, sizeof(fault));
}