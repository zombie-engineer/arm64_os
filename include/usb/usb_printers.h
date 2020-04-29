#pragma once
#include <usb/usb.h>
#include <common.h>

static inline void print_usb_hub_descriptor(struct usb_hub_descriptor *h)
{
  printf("hub_desc:%p:type:%d,len:%d,numports:%d,attr:%02x-%02x,pwrdelay:%d,maxpow:%d,rem:%x,msk:%02x\r\n",
    h,
    h->header.descriptor_type,
    h->header.length,
    h->port_count,
    h->attributes.hi, h->attributes.lo,
    h->power_good_delay,
    h->maximum_hub_power,
    h->device_removable,
    h->port_power_ctrl_mask);
}

static inline void print_usb_device_descriptor(struct usb_device_descriptor *d)
{
  printf("dev_desc:%p:type:%d,len:%d,usb:%d,clss:%d,sclss:%d,mps:%d\r\n",
    d,
    d->descriptor_type,
    d->length,
    d->bcd_usb,
    d->device_class,
    d->device_subclass,
    d->max_packet_size_0);

  printf("-------:ven:%04x,prod:%04x,dev:%x,manu:%d,produ:%d,seri:%d,num_conf:%d\r\n",
    d->id_vendor,
    d->id_product,
    d->bcd_device,
    d->i_manufacturer,
    d->i_product,
    d->i_serial_number,
    d->num_configurations);
}

static inline void print_usb_configuration_desc(const struct usb_configuration_descriptor *c)
{
  printf("cfg_desc:%p:type:%d,len:%d,tlen:%d,inum:%d,val:%d,str:%d,at:%x,pwr:%d\r\n",
    c,
    c->header.descriptor_type,
    c->header.length,
    c->total_length,
    c->num_interfaces,
    c->configuration_value,
    c->iconfiguration,
    c->attributes,
    c->max_power);
}


static inline void print_usb_endpoint_desc(struct usb_endpoint_descriptor *e)
{
  printf("ep_desc:%p:type:%d,len:%d,ep_addr:%02x,attr:%02x,mpsz:%d,intv:%d\r\n",
    e, 
    e->header.descriptor_type,
    e->header.length,
    e->endpoint_address,
    e->attributes, 
    e->max_packet_size,
    e->interval
    );
}

static inline void print_usb_interface_desc(struct usb_interface_descriptor *i)
{
  printf("iface_desc:%p:type:%d,len:%d,num:%d,alt:%d,ep_cnt:%d\r\n",
    i,
    i->header.descriptor_type,
    i->header.length,
    i->number,
    i->alt_setting,
    i->endpoint_count);
  printf("iface_desc:--:class:%d,subclass:%d,proto:%d,string:%d\r\n",
    i->class,
    i->subclass,
    i->protocol,
    i->string_index);
}

