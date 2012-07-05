//; extern void _topro_cam_init(struct usb_device *udev);

extern unsigned char topro_read_reg(struct usb_device *udev, unsigned char index);
extern int topro_write_reg(struct usb_device *udev, unsigned char index, unsigned char data);
extern int topro_write_i2c(struct usb_device *udev, unsigned char dev, unsigned char reg, unsigned char data_h, unsigned char data_l);
extern int topro_prog_regs(struct usb_device *udev, struct topro_sregs *ptpara);

extern int topro_setting_iso(struct usb_device *udev, unsigned char enset);

extern int topro_bulk_out(struct usb_device *udev, unsigned char bulkctrl, unsigned char *pbulkdata, int bulklen);


extern int topro_prog_gamma(struct pwc_device *udev, unsigned char bulkctrl); //950903
extern int topro_prog_gain(struct usb_device *udev, unsigned char *pgain);

extern void topro_set_hue_saturation(struct usb_device *udev, long hue, long saturation);
extern void topro_autowhitebalance(struct usb_device *udev);
extern void topro_autoexposure(struct pwc_device *pdev);		//950903
extern void topro_SetAutoQuality(struct usb_device *udev);
#ifdef CUSTOM_QTABLE
//extern void UpdateQTable(struct usb_device *udev, UCHAR value);
#endif
extern unsigned long SwDetectMotion(struct pwc_device *pdev);
extern void topro_setiris(struct usb_device *udev, int Exposure);
extern void topro_setexposure(struct usb_device *udev, int Exposure,struct pwc_device *pdev);
extern void topro_set_parameter(struct usb_device *udev);

