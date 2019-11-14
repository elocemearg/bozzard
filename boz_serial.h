#ifndef _BOZ_SERIAL_H
#define _BOZ_SERIAL_H

void boz_serial_service_send(void);
int boz_serial_enqueue_data_out(char *buf, int length);
void boz_serial_init(void);

#endif
