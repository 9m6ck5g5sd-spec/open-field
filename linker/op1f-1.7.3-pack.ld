/* A pack's layout for stock OP-1 Field OS 1.7.3+0: the room for our code after the loader's 64 KiB,
 * [0x8d2f4234, 0x8d4e4234). The loader (linker/op1f-1.7.3-base.ld) reads the pack file at start-up, puts its bytes
 * here, clears the zeroed data after them and applies the pack's table of changes. tools/pack_build.py takes this
 * file for a profile whose module_base is 0x8d2e4234 and replaces the line MEMORY's ORIGIN and LENGTH for each place
 * it links a pack for. */
OUTPUT_ARCH(bfin)
ENTRY(field_module_entry)
MEMORY { MODULE (rwx) : ORIGIN = 0x8d2f4234, LENGTH = 0x1f0000 }
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
  ASSERT(__module_end <= __module_reserved_end, "The pack exceeds the reservation")
}
