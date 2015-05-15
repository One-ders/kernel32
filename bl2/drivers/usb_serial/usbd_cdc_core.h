#define USB_CDC_CONFIG_DESC_SIZ                (67)
#define USB_CDC_DESC_SIZ                       (67-9)

#define CDC_DESCRIPTOR_TYPE                     0x21

#define DEVICE_CLASS_CDC                        0x02
#define DEVICE_SUBCLASS_CDC                     0x00


#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define STANDARD_ENDPOINT_DESC_SIZE             0x09

#define CDC_DATA_IN_PACKET_SIZE(a)                 *((unsigned short int *)(a->dev.config_descriptor + 57))

#define CDC_DATA_OUT_PACKET_SIZE(a)                *((unsigned short int *)(a->dev.config_descriptor + 64))

#define SEND_ENCAPSULATED_COMMAND               0x00
#define GET_ENCAPSULATED_RESPONSE               0x01
#define SET_COMM_FEATURE                        0x02
#define GET_COMM_FEATURE                        0x03
#define CLEAR_COMM_FEATURE                      0x04
#define SET_LINE_CODING                         0x20
#define GET_LINE_CODING                         0x21
#define SET_CONTROL_LINE_STATE                  0x22
#define SEND_BREAK                              0x23
#define NO_CMD                                  0xFF
