#ifndef __USB_HOST_APP_H
#define __USB_HOST_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEF_COM_BUF_LEN 1024
#define ENUM_MAX_TRIES 5

void USBH_HostInit(void);
void USBH_App(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_HOST_APP_H */
