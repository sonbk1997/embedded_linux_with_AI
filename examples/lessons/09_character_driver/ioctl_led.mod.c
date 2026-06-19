#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x92997ed8, "_printk" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0xe97c4103, "ioremap" },
	{ 0x395fd571, "misc_register" },
	{ 0x7a1a8e96, "misc_deregister" },
	{ 0xedc03953, "iounmap" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x43b0c9c3, "preempt_schedule" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xda02d67, "jiffies" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0x5f754e5a, "memset" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0x31916f86, "module_layout" },
};

MODULE_INFO(depends, "");

