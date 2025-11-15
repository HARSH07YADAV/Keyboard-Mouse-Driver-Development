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
	{ 0x81a1a811, "_raw_spin_unlock_irqrestore" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0x9c0551c6, "__tasklet_schedule" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0xa6c58518, "sysfs_remove_group" },
	{ 0x9c0551c6, "tasklet_kill" },
	{ 0x6c8f2acc, "input_unregister_device" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0xc2fefbb5, "kmalloc_caches" },
	{ 0x38395bf3, "__kmalloc_cache_noprof" },
	{ 0xcdec1689, "tasklet_init" },
	{ 0x83124e50, "input_allocate_device" },
	{ 0xb535e67f, "input_register_device" },
	{ 0xed75f9ac, "sysfs_create_group" },
	{ 0x6c8f2acc, "input_free_device" },
	{ 0x23f25c0a, "__dynamic_pr_debug" },
	{ 0x2b250410, "input_event" },
	{ 0xd272d446, "__fentry__" },
	{ 0xe8213e80, "_printk" },
	{ 0x0a589842, "simple_strtoul" },
	{ 0xe1e1f979, "_raw_spin_lock_irqsave" },
	{ 0x70eca2ca, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x81a1a811,
	0xd272d446,
	0x9c0551c6,
	0x90a48d82,
	0xd272d446,
	0xa6c58518,
	0x9c0551c6,
	0x6c8f2acc,
	0xcb8b6ec6,
	0xbd03ed67,
	0xc2fefbb5,
	0x38395bf3,
	0xcdec1689,
	0x83124e50,
	0xb535e67f,
	0xed75f9ac,
	0x6c8f2acc,
	0x23f25c0a,
	0x2b250410,
	0xd272d446,
	0xe8213e80,
	0x0a589842,
	0xe1e1f979,
	0x70eca2ca,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"_raw_spin_unlock_irqrestore\0"
	"__x86_return_thunk\0"
	"__tasklet_schedule\0"
	"__ubsan_handle_out_of_bounds\0"
	"__stack_chk_fail\0"
	"sysfs_remove_group\0"
	"tasklet_kill\0"
	"input_unregister_device\0"
	"kfree\0"
	"random_kmalloc_seed\0"
	"kmalloc_caches\0"
	"__kmalloc_cache_noprof\0"
	"tasklet_init\0"
	"input_allocate_device\0"
	"input_register_device\0"
	"sysfs_create_group\0"
	"input_free_device\0"
	"__dynamic_pr_debug\0"
	"input_event\0"
	"__fentry__\0"
	"_printk\0"
	"simple_strtoul\0"
	"_raw_spin_lock_irqsave\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "38F65B9BBD3078EF3528807");
