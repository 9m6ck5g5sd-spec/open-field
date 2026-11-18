/* The base's layout for stock OP-1 Field OS 1.7.0+0 (Build J, since J4): the first 64 KiB of the module
 * reservation [0x8d2e36cc, 0x8d4e36cc). The base is the part of our code the installed image carries: the load
 * point's stub, the pack loader and the code that applies a pack's table of stock changes. A pack's code is loaded
 * right after it, at 0x8d2f36cc (linker/op1f-1.7.0-pack.ld). */
OUTPUT_ARCH(bfin)
ENTRY(op1f_boot_stub)
MEMORY { MODULE (rwx) : ORIGIN = 0x8d2e36cc, LENGTH = 0x10000 }
SECTIONS {
  __module_reserved_start = ORIGIN(MODULE);
  __module_reserved_end = ORIGIN(MODULE) + LENGTH(MODULE);
  .text : { *(.text .text.*) } > MODULE
  .rodata : ALIGN(4) { *(.rodata .rodata.* .data.rel.ro .data.rel.ro.*) } > MODULE
  .data : ALIGN(4) { *(.data .data.* .sdata .sdata.*) } > MODULE
  __module_initialized_end = .;
  .bss (NOLOAD) : ALIGN(4) {
    __module_bss_start = .;
    *(.bss .bss.* .sbss .sbss.* COMMON)
    __module_bss_end = .;
  } > MODULE
  __module_end = .;
  .init_array : { *(.init_array .init_array.* .ctors .ctors.*) }
  .fini_array : { *(.fini_array .fini_array.* .dtors .dtors.*) }
  /DISCARD/ : { *(.comment .note* .eh_frame .eh_frame_hdr .gcc_except_table) }
  ASSERT(SIZEOF(.init_array) == 0, "Static constructors are not run by the stock loader")
  ASSERT(SIZEOF(.fini_array) == 0, "Static destructors are not supported")
  ASSERT(__module_end <= __module_reserved_end, "The base exceeds its 64 KiB")
}
