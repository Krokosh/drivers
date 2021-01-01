#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x7d8d5184, "module_layout" },
	{ 0xcbb9a452, "usb_deregister" },
	{ 0x7a087b1c, "usb_register_driver" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0x38284ef7, "usb_interrupt_msg" },
	{ 0x84b2ec28, "usb_alloc_urb" },
	{ 0xc7d3513d, "usb_set_configuration" },
	{ 0x47773797, "usb_set_interface" },
	{ 0xe60a7e80, "devm_kmalloc" },
	{ 0xa28921a, "usb_control_msg" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "usbcore");

MODULE_ALIAS("usb:v03EEpFF01d*dc*dsc*dp*ic*isc*ip*in*");
