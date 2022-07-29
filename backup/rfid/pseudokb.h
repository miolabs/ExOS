#ifndef RFID_PSEUDOKB_H
#define RFID_PSEUDOKB_H

void pseudokb_service(const char *device);
void pseudokb_handler(int unit, char *text, int length);

#endif // RFID_PSEUDOKB_H

