#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

MODULE_INFO(intree, "Y");

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

MODULE_INFO(staging, "Y");

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf006ae3f, "module_layout" },
	{ 0xf36861cd, "vb2_ioctl_reqbufs" },
	{ 0x1b5d080d, "vchiq_mmal_port_set_format" },
	{ 0x8bd12a6c, "kmalloc_caches" },
	{ 0x921e87f2, "v4l2_event_unsubscribe" },
	{ 0xa8937392, "vchiq_mmal_port_disable" },
	{ 0xf9a482f9, "msleep" },
	{ 0x1fdc7df2, "_mcount" },
	{ 0xcc574346, "video_device_release_empty" },
	{ 0x42c2dd7a, "param_ops_int" },
	{ 0x3c99d531, "v4l2_ctrl_log_status" },
	{ 0xce312042, "vchiq_mmal_port_parameter_set" },
	{ 0xe84efe5c, "v4l2_device_unregister" },
	{ 0xc40c9cea, "v4l2_ctrl_handler_free" },
	{ 0x6b25ae0e, "v4l2_ctrl_new_std" },
	{ 0x16d512c8, "vb2_fop_poll" },
	{ 0xd927575a, "vb2_ioctl_streamon" },
	{ 0xb43f9365, "ktime_get" },
	{ 0x7046090a, "vb2_ops_wait_prepare" },
	{ 0x96ad7d57, "__video_register_device" },
	{ 0x91715312, "sprintf" },
	{ 0x9674bd90, "__platform_driver_register" },
	{ 0xab6acd5a, "v4l2_device_register" },
	{ 0x9a091582, "vb2_fop_read" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x456e53b3, "v4l2_ctrl_new_std_menu" },
	{ 0xe6ac336c, "vchiq_mmal_component_finalise" },
	{ 0xdcb764ad, "memset" },
	{ 0x22f10f28, "vb2_vmalloc_memops" },
	{ 0xc9320983, "vb2_fop_mmap" },
	{ 0x6e32e60f, "vb2_ioctl_qbuf" },
	{ 0x9a76f11f, "__mutex_init" },
	{ 0x7c32d0f0, "printk" },
	{ 0x37a45bf1, "vchiq_mmal_port_connect_tunnel" },
	{ 0x38401988, "video_unregister_device" },
	{ 0x399d9c18, "v4l2_ctrl_subscribe_event" },
	{ 0xc2d60088, "vb2_plane_vaddr" },
	{ 0xe0a2c5a8, "vb2_buffer_done" },
	{ 0x5792f848, "strlcpy" },
	{ 0x2ce3246c, "vchiq_mmal_component_enable" },
	{ 0x41cc682c, "vb2_ioctl_prepare_buf" },
	{ 0x3a24bb74, "vb2_ioctl_create_bufs" },
	{ 0xc1c12de6, "_dev_err" },
	{ 0x59c28e82, "vb2_ioctl_dqbuf" },
	{ 0x73577d20, "vchiq_mmal_finalise" },
	{ 0x9956a2c8, "v4l2_ctrl_new_int_menu" },
	{ 0xe1380126, "vchiq_mmal_component_init" },
	{ 0xf5ef842e, "v4l_bound_align_image" },
	{ 0x30b4d907, "vb2_fop_release" },
	{ 0x77c18fa0, "video_devdata" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0xcf8e06f9, "vchiq_mmal_component_disable" },
	{ 0x37b90fbc, "v4l2_ctrl_auto_cluster" },
	{ 0xd0be7911, "mmal_vchi_buffer_init" },
	{ 0x18575f49, "kmem_cache_alloc_trace" },
	{ 0xf8a13bcd, "v4l2_fh_open" },
	{ 0x4bd01acd, "vchiq_mmal_port_enable" },
	{ 0xacc92eaa, "vb2_ioctl_querybuf" },
	{ 0x37a0cba, "kfree" },
	{ 0x6192e1a2, "vchiq_mmal_version" },
	{ 0x23edcef2, "param_array_ops" },
	{ 0xff5785b, "vchiq_mmal_submit_buffer" },
	{ 0xadc17cd5, "v4l2_ctrl_handler_init_class" },
	{ 0xbc5ab8dd, "vb2_ops_wait_finish" },
	{ 0xaca4dd80, "vchiq_mmal_init" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x29361773, "complete" },
	{ 0x28318305, "snprintf" },
	{ 0x46745e83, "mmal_vchi_buffer_cleanup" },
	{ 0x5058e2be, "platform_driver_unregister" },
	{ 0xa0e3cbe7, "vb2_ioctl_streamoff" },
	{ 0x4d1ff60a, "wait_for_completion_timeout" },
	{ 0x1395990d, "video_ioctl2" },
	{ 0xac9a3ad0, "vchiq_mmal_port_parameter_get" },
	{ 0xf65663e7, "vb2_queue_init" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=videobuf2-v4l2,bcm2835-mmal-vchiq,videodev,videobuf2-vmalloc,videobuf2-common,v4l2-common";


MODULE_INFO(srcversion, "0F765D307016ED9D9D341E3");
