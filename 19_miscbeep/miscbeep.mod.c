#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xfa985410, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x51204841, __VMLINUX_SYMBOL_STR(platform_driver_unregister) },
	{ 0xda113ac9, __VMLINUX_SYMBOL_STR(__platform_driver_register) },
	{ 0x4ee6e196, __VMLINUX_SYMBOL_STR(misc_register) },
	{ 0xc61ff1f2, __VMLINUX_SYMBOL_STR(gpiod_direction_output_raw) },
	{ 0xde6fb46c, __VMLINUX_SYMBOL_STR(of_get_named_gpio_flags) },
	{ 0xfbed6f8c, __VMLINUX_SYMBOL_STR(of_find_node_opts_by_path) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xfbc74f64, __VMLINUX_SYMBOL_STR(__copy_from_user) },
	{ 0xd9285795, __VMLINUX_SYMBOL_STR(misc_deregister) },
	{ 0x389329b3, __VMLINUX_SYMBOL_STR(gpiod_set_raw_value) },
	{ 0x3089e8ae, __VMLINUX_SYMBOL_STR(gpio_to_desc) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "EB2559389B43BC189CACDCF");
