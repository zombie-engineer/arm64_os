#!/usr/bin/python2.7

import sys

def usb_rq_get_type(rq):
    return rq & 0xff

def usb_rq_type_get_recipient(rq_type):
    return rq_type & 0xf

def usb_rq_type_get_type(rq_type):
    return ['DEVICE', 'INTERFACE', 'ENDPT', 'OTHER'][(rq_type >> 5) & 3]

def usb_rq_type_get_dir(rq_type):
    return ['HOST>DEV', 'DEV>HOST'][(rq_type >> 7) & 1]

def usb_rq_request_to_str(usb_rq_req):
    return {
        0: "USB_RQ_GET_STATUS",
        1: "USB_RQ_CLEAR_FEATURE",
        3: "USB_RQ_SET_FEATURE",
        5: "USB_RQ_SET_ADDRESS",
        6: "USB_RQ_GET_DESCRIPTOR",
        7: "USB_RQ_SET_DESCRIPTOR",
        8: "USB_RQ_GET_CONFIGURATION",
        9: "USB_RQ_SET_CONFIGURATION",
        10: "USB_RQ_GET_INTERFACE",
        11: "USB_RQ_SET_INTERFACE",
        12: "USB_RQ_SYNCH_FRAME"
    }[usb_rq_req]

def usb_rq_get_request(rq):
    return (rq >> 8) & 0xff

def usb_rq_get_value(rq):
    return (rq >> 16) & 0xffff

def usb_rq_get_index(rq):
    return (rq >> 32) & 0xffff

def usb_rq_get_length(rq):
    return (rq >> 32) & 0xffff

def main(rq):
    s = '{:016x}'.format(rq)

    rq_type = usb_rq_get_type(rq)
    s += ', type:{:02x}({},{},{})'.format(rq_type,
        usb_rq_type_get_recipient(rq_type),
        usb_rq_type_get_type(rq_type),
        usb_rq_type_get_dir(rq_type))

    rq_req = usb_rq_get_request(rq)
    s += ', req:{:02x}({})'.format(rq_req, usb_rq_request_to_str(rq_req))

    rq_value = usb_rq_get_value(rq)
    s += ', value:{:04x}'.format(rq_value)

    rq_index = usb_rq_get_index(rq)
    s += ', index:{:04x}'.format(rq_index)

    rq_length = usb_rq_get_length(rq)
    s += ', length:{:04x}'.format(rq_length)

    print(s)

if __name__ == '__main__':
    rq = int(sys.argv[1], 16)
    main(rq)
