#define CONFIG_PGTABLE_LEVELS 3

#define SECTION_SHIFT		PMD_SHIFT
#define SWAPPER_BLOCK_SHIFT	SECTION_SHIFT

// Physical address space size
#define CONFIG_ARM64_PA_BITS 48

/* PAGE_SHIFT - number of bits, that describe address inside a page
 * If page size is 4096, then maximum address within a page is 0xfff
 * which is 12 bits
 */
#define PAGE_SHIFT 12

#define PTRS_PER_PTE		(1 << (PAGE_SHIFT - 3))

/*
 * PMD_SHIFT determines the size a level 2 page table entry can map.
 */
#if CONFIG_PGTABLE_LEVELS > 2
#define PMD_SHIFT		PGTABLE_LEVEL_SHIFT(2)
#define PMD_SIZE		(_AC(1, UL) << PMD_SHIFT)
#define PMD_MASK		(~(PMD_SIZE-1))
#define PTRS_PER_PMD		PTRS_PER_PTE
#endif

#define PGDIR_SHIFT		PGTABLE_LEVEL_SHIFT(4 - CONFIG_PGTABLE_LEVELS)
// which is 30

#define PTRS_PER_PGD		(1 << (VA_BITS - PGDIR_SHIFT)) 
// which is  1 << (48 - 30) = 1 << 18


/*
 * Hardware page table definitions.
 *
 * Level 1 descriptor (PUD).
 */
#define PUD_TYPE_TABLE  (_AT(pudval_t, 3) << 0)
#define PUD_TABLE_BIT   (_AT(pudval_t, 1) << 1)
#define PUD_TYPE_MASK   (_AT(pudval_t, 3) << 0)
#define PUD_TYPE_SECT   (_AT(pudval_t, 1) << 0)

/*
 * Level 2 descriptor (PMD).
 */
#define PMD_TYPE_MASK   (_AT(pmdval_t, 3) << 0)
#define PMD_TYPE_FAULT  (_AT(pmdval_t, 0) << 0)
#define PMD_TYPE_TABLE  (_AT(pmdval_t, 3) << 0)
#define PMD_TYPE_SECT   (_AT(pmdval_t, 1) << 0)
#define PMD_TABLE_BIT   (_AT(pmdval_t, 1) << 1)

/*
 * Section
 */
#define PMD_SECT_VALID  (_AT(pmdval_t, 1) << 0)
#define PMD_SECT_USER   (_AT(pmdval_t, 1) << 6)   /* AP[1] */
#define PMD_SECT_RDONLY (_AT(pmdval_t, 1) << 7)   /* AP[2] */
#define PMD_SECT_S      (_AT(pmdval_t, 3) << 8)
#define PMD_SECT_AF     (_AT(pmdval_t, 1) << 10)
#define PMD_SECT_NG     (_AT(pmdval_t, 1) << 11)
#define PMD_SECT_CONT   (_AT(pmdval_t, 1) << 52)
#define PMD_SECT_PXN    (_AT(pmdval_t, 1) << 53)
#define PMD_SECT_UXN    (_AT(pmdval_t, 1) << 54)

/*
 * Level 3 descriptor (PTE).
 */
#define PTE_TYPE_MASK   (_AT(pteval_t, 3) << 0)
#define PTE_TYPE_FAULT  (_AT(pteval_t, 0) << 0)
#define PTE_TYPE_PAGE   (_AT(pteval_t, 3) << 0)
#define PTE_TABLE_BIT   (_AT(pteval_t, 1) << 1)
#define PTE_USER        (_AT(pteval_t, 1) << 6)   /* AP[1] */
#define PTE_RDONLY      (_AT(pteval_t, 1) << 7)   /* AP[2] */
#define PTE_SHARED      (_AT(pteval_t, 3) << 8)   /* SH[1:0], inner shareable */
#define PTE_AF          (_AT(pteval_t, 1) << 10)  /* Access Flag */
#define PTE_NG          (_AT(pteval_t, 1) << 11)  /* nG */
#define PTE_DBM         (_AT(pteval_t, 1) << 51)  /* Dirty Bit Management */
#define PTE_CONT        (_AT(pteval_t, 1) << 52)  /* Contiguous range */
#define PTE_PXN         (_AT(pteval_t, 1) << 53)  /* Privileged XN */
#define PTE_UXN         (_AT(pteval_t, 1) << 54)  /* User XN */
#define PTE_HYP_XN      (_AT(pteval_t, 1) << 54)  /* HYP XN */

#define PTE_ADDR_LOW    (((_AT(pteval_t, 1) << (48 - PAGE_SHIFT)) - 1) << PAGE_SHIFT)
#ifdef CONFIG_ARM64_PA_BITS_52
#define PTE_ADDR_HIGH   (_AT(pteval_t, 0xf) << 12)
#define PTE_ADDR_MASK   (PTE_ADDR_LOW | PTE_ADDR_HIGH)
#else
#define PTE_ADDR_MASK   PTE_ADDR_LOW
#endif

/*
 * AttrIndx[2:0] encoding (mapping attributes defined in the MAIR* registers).
 */
#define PTE_ATTRINDX(t)   (_AT(pteval_t, (t)) << 2)
#define PTE_ATTRINDX_MASK (_AT(pteval_t, 7) << 2)



/*
 * Highest possible physical address supported. 
 */

/*
 * PHYS_MASK_SHIFT is 48
 */
#define PHYS_MASK_SHIFT (CONFIG_ARM64_PA_BITS) 
/*
 * PHYS_MASK is 0xffff ffff ffff
 */
#define PHYS_MASK       ((1UL) << PHYS_MASK_SHIFT) - 1)

/* PGTABLE_LEVEL_SHIFT(n) - number of bits, that describe virtual addresses
 * at level n. If maximum number of bits for a virtual address is 48 and
 * bits[11:0]  - bits of page  (PAGE_SHIFT)
 * bits[20:12] - bits of level 3 translation
 * bits[29:21] - bits of level 2 translation
 * bits[38:30] - bits of level 1 translation
 * bits[47:39] - bits of level 0 translation
 */
#define PGTABLE_LEVEL_SHIFT(n) ((PAGE_SHIFT - 3) * (4 - (n)) + 3)

/* PGDIR_SHIFT - PGTABLE_LEVEL_SHIFT(4 - 3) = 9 * (4 - 1) + 3 = 30
 * 
 */
#define PGDIR_SHIFT PGTABLE_LEVEL_SHIFT(4 - CONFIG_PGTABLE_LEVELS)

/*
 * Setup the initial page tables. We only setup the barest amount which is
 * required to get the kernel running. The following sections are required:
 *   - identity mapping to enable the MMU (low address, TTBR0)
 *   - first few MB of the kernel linear mapping to jump to once the MMU has
 *     been enabled
 */

void create_page_tables(char *idmap_pg_dir, char *swapper_page_end, char *idmap_text_start, char *idmap_text_end)
{
  /*
   * Create the identity mapping.
   */
  __inval_dcache_area(idmap_pg_dir, swapper_page_end - idmap_pg_dir);
  memset(idmap_pg_dir, 0, swapper_page_end - idmap_pg_dir);
  swapper_mmu_flags = SWAPPER_MM_MMUFLAGS;
  map_memory(idmap_pg_dir, swapper_end - identity, idmap_text_start, idmap_text_end, SWAPPER_MM_MMUFLAGS, idmap_text_end, 1 << (PHYS_MASK_SHIFT - PGDIR_SHIFT));
   
  // adrp  x0, idmap_pg_dir          // x0 - offset to start of identity map
  // adrp  x3, __idmap_text_start    // x3 - virtual address

  adrp  x5, __idmap_text_end
  clz x5, x5
  
  adr_l x6, idmap_t0sz
  str x5, [x6]
  dmb sy
  dc  ivac, x6    // Invalidate potentially stale cache line

  /*
   * If VA_BITS == 48, we don't have to configure an additional
   * translation level, but the top-level table has more entries.
   */
  // #1 << (48 - 30) = 1 << 18
  mov x4, #1 << (PHYS_MASK_SHIFT - PGDIR_SHIFT)
  str_l x4, idmap_ptrs_per_pgd, x5
1:
  ldr_l x4, idmap_ptrs_per_pgd
  mov x5, x3                 // __pa(__idmap_text_start)
  adr_l x6, __idmap_text_end // __pa(__idmap_text_end)

  // map_memory, tbl, rtbl, vstart, vend, flags, phys, pgds, istart, iend, tmp, count, sv
  map_memory      x0,   x1,     x3,   x6,    x7,   x3,   x4,    x10,  x11, x12,   x13, x14
  
}

__create_page_tables:
  mov x28, lr

  /*
   * Invalidate the idmap and swapper page tables to avoid potential
   * dirty cache lines being evicted.
   */
  adrp  x0, idmap_pg_dir     // page address for identity map page directory
  adrp  x1, swapper_pg_end   // page address for sqwapper page end
  sub x1, x1, x0             // x1 = swapper_end - identity
  bl  __inval_dcache_area    // 

  /*
   * Clear the idmap and swapper page tables.
   */
  adrp  x0, idmap_pg_dir
  adrp  x1, swapper_pg_end
  sub x1, x1, x0
1:  stp xzr, xzr, [x0], #16
  stp xzr, xzr, [x0], #16
  stp xzr, xzr, [x0], #16
  stp xzr, xzr, [x0], #16
  subs  x1, x1, #64
  b.ne  1b

  mov x7, SWAPPER_MM_MMUFLAGS

  /*
   * Create the identity mapping.
   */
  adrp  x0, idmap_pg_dir          // x0 - offset to start of identity map
  adrp  x3, __idmap_text_start    // x3 - virtual address

  /*
   * VA_BITS may be too small to allow for an ID mapping to be created
   * that covers system RAM if that is located sufficiently high in the
   * physical address space. So for the ID map, use an extended virtual
   * range in that case, and configure an additional translation level
   * if needed.
   *
   * Calculate the maximum allowed value for TCR_EL1.T0SZ so that the
   * entire ID map region can be mapped. As T0SZ == (64 - #bits used),
   * this number conveniently equals the number of leading zeroes in
   * the physical address of __idmap_text_end.
   */
  adrp  x5, __idmap_text_end
  clz x5, x5
  cmp x5, TCR_T0SZ(VA_BITS) // default T0SZ small enough?
  b.ge  1f      // .. then skip VA range extension

  adr_l x6, idmap_t0sz
  str x5, [x6]
  dmb sy
  dc  ivac, x6    // Invalidate potentially stale cache line

  /*
   * If VA_BITS == 48, we don't have to configure an additional
   * translation level, but the top-level table has more entries.
   */
  // #1 << (48 - 30) = 1 << 18
  mov x4, #1 << (PHYS_MASK_SHIFT - PGDIR_SHIFT)
  str_l x4, idmap_ptrs_per_pgd, x5
1:
  ldr_l x4, idmap_ptrs_per_pgd
  mov x5, x3        // __pa(__idmap_text_start)
  adr_l x6, __idmap_text_end    // __pa(__idmap_text_end)

  // map_memory, tbl, rtbl, vstart, vend, flags, phys, pgds, istart, iend, tmp, count, sv
  map_memory x0, x1, x3, x6, x7, x3, x4, x10, x11, x12, x13, x14

  /*
   * Map the kernel image (starting with PHYS_OFFSET).
   */
  adrp  x0, swapper_pg_dir
  mov_q x5, KIMAGE_VADDR + TEXT_OFFSET  // compile time __va(_text)
  add x5, x5, x23     // add KASLR displacement
  mov x4, PTRS_PER_PGD
  adrp  x6, _end      // runtime __pa(_end)
  adrp  x3, _text     // runtime __pa(_text)
  sub x6, x6, x3      // _end - _text
  add x6, x6, x5      // runtime __va(_end)

  map_memory x0, x1, x5, x6, x7, x3, x4, x10, x11, x12, x13, x14

  /*
   * Since the page tables have been populated with non-cacheable
   * accesses (MMU disabled), invalidate the idmap and swapper page
   * tables again to remove any speculatively loaded cache lines.
   */
  adrp  x0, idmap_pg_dir
  adrp  x1, swapper_pg_end
  sub x1, x1, x0
  dmb sy
  bl  __inval_dcache_area

  ret x28
ENDPROC(__create_page_tables)

/*
 *  __inval_dcache_area(kaddr, size)
 *
 *  Ensure that any D-cache lines for the interval [kaddr, kaddr+size)
 *  are invalidated. Any partial lines at the ends of the interval are
 *  also cleaned to PoC to prevent data loss.
 *
 *  - kaddr   - kernel address
 *  - size    - size in question
 */
ENTRY(__inval_dcache_area)
  /* FALLTHROUGH */

/*
 *  __dma_inv_area(start, size)
 *  - start   - virtual start address of region
 *  - size    - size in question
 */
__dma_inv_area:
  add x1, x1, x0
  dcache_line_size x2, x3
  sub x3, x2, #1
  tst x1, x3        // end cache line aligned?
  bic x1, x1, x3
  b.eq  1f
  dc  civac, x1     // clean & invalidate D / U line
1:  tst x0, x3        // start cache line aligned?
  bic x0, x0, x3
  b.eq  2f
  dc  civac, x0     // clean & invalidate D / U line
  b 3f
2:  dc  ivac, x0      // invalidate D / U line
3:  add x0, x0, x2
  cmp x0, x1
  b.lo  2b
  dsb sy
  ret
ENDPIPROC(__inval_dcache_area)
ENDPROC(__dma_inv_area)

/*
 * Macro to create a table entry to the next page.
 *
 *  tbl:  page table address
 *  virt: virtual address
 *  shift:  #imm page table shift
 *  ptrs: #imm pointers per table page
 *
 * Preserves: virt
 * Corrupts:  ptrs, tmp1, tmp2
 * Returns: tbl -> next level table page address
 */
  .macro  create_table_entry, tbl, virt, shift, ptrs, tmp1, tmp2
  add \tmp1, \tbl, #PAGE_SIZE
  mov \tmp2, \tmp1
  orr \tmp2, \tmp2, #PMD_TYPE_TABLE // address of next table and entry type
  lsr \tmp1, \virt, #\shift
  sub \ptrs, \ptrs, #1
  and \tmp1, \tmp1, \ptrs   // table index
  str \tmp2, [\tbl, \tmp1, lsl #3]
  add \tbl, \tbl, #PAGE_SIZE    // next level table page
  .endm

/*
 * Macro to populate page table entries, these entries can be pointers to the next level
 * or last level entries pointing to physical memory.
 *
 *  tbl:  page table address
 *  rtbl: pointer to page table or physical memory
 *  index:  start index to write
 *  eindex: end index to write - [index, eindex] written to
 *  flags:  flags for pagetable entry to or in
 *  inc:  increment to rtbl between each entry
 *  tmp1: temporary variable
 *
 * Preserves: tbl, eindex, flags, inc
 * Corrupts:  index, tmp1
 * Returns: rtbl
 */
  .macro populate_entries, tbl, rtbl, index, eindex, flags, inc, tmp1
.Lpe\@: phys_to_pte \tmp1, \rtbl
  orr \tmp1, \tmp1, \flags  // tmp1 = table entry
  str \tmp1, [\tbl, \index, lsl #3]
  add \rtbl, \rtbl, \inc  // rtbl = pa next level
  add \index, \index, #1
  cmp \index, \eindex
  b.ls  .Lpe\@
  .endm

/*
 * Compute indices of table entries from virtual address range. If multiple entries
 * were needed in the previous page table level then the next page table level is assumed
 * to be composed of multiple pages. (This effectively scales the end index).
 *
 *  vstart: virtual address of start of range
 *  vend: virtual address of end of range
 *  shift:  shift used to transform virtual address into index
 *  ptrs: number of entries in page table
 *  istart: index in table corresponding to vstart
 *  iend: index in table corresponding to vend
 *  count:  On entry: how many extra entries were required in previous level, scales
 *        our end index.
 *    On exit: returns how many extra entries required for next page table level
 *
 * Preserves: vstart, vend, shift, ptrs
 * Returns: istart, iend, count
 */
  .macro compute_indices, vstart, vend, shift, ptrs, istart, iend, count
  lsr \iend, \vend, \shift
  mov \istart, \ptrs
  sub \istart, \istart, #1
  and \iend, \iend, \istart // iend = (vend >> shift) & (ptrs - 1)
  mov \istart, \ptrs
  mul \istart, \istart, \count
  add \iend, \iend, \istart // iend += (count - 1) * ptrs
          // our entries span multiple tables

  lsr \istart, \vstart, \shift
  mov \count, \ptrs
  sub \count, \count, #1
  and \istart, \istart, \count

  sub \count, \iend, \istart
  .endm

/*
 * Map memory for specified virtual address range. Each level of page table needed supports
 * multiple entries. If a level requires n entries the next page table level is assumed to be
 * formed from n pages.
 *
 *  tbl:  location of page table
 *  rtbl: address to be used for first level page table entry (typically tbl + PAGE_SIZE)
 *  vstart: start address to map
 *  vend: end address to map - we map [vstart, vend]
 *  flags:  flags to use to map last level entries
 *  phys: physical address corresponding to vstart - physical memory is contiguous
 *  pgds: the number of pgd entries
 *
 * Temporaries: istart, iend, tmp, count, sv - these need to be different registers
 * Preserves: vstart, vend, flags
 * Corrupts:  tbl, rtbl, istart, iend, tmp, count, sv
 */

void map_memory(char *tbl, char *rtbl, char *vstart, char *vend, uint64_t flags)
{
  int i;
  char *var0;
  char *var1;

  var0 = tbl + PAGE_SIZE;
  var1 = rtbl;
  count = compute_indices(vstart, vend, flags, PGDIR_SHIFT, pgds, istart, iend, 0);
  rtbl  = populate_entries(tbl, rtbl, istart, iend, PMD_TYPE_TABLE, PAGE_SIZE);
  tbl   = var1;
  sv    = rtbl;
  count = compute_indices(vstart, vend, PGTABLE_LEVEL_SHIFT(2), PTRS_PER_PTE, istart, iend, count);
  count = phys & ~(SWAPPER_BLOCK_SIZE - 1)
  rtbl  = populate_entries(tbl, rtbl, istart, iend, flags, SWAPPER_BLOCK_SIZE);
}
  .macro map_memory, tbl, rtbl, vstart, vend, flags, phys, pgds, istart, iend, tmp, count, sv
  add \rtbl, \tbl, #PAGE_SIZE
  mov \sv, \rtbl
  mov \count, #0
  compute_indices \vstart, \vend, #PGDIR_SHIFT, \pgds, \istart, \iend, \count
  populate_entries \tbl, \rtbl, \istart, \iend, #PMD_TYPE_TABLE, #PAGE_SIZE, \tmp
  mov \tbl, \sv
  mov \sv, \rtbl

  compute_indices \vstart, \vend, #SWAPPER_BLOCK_SHIFT, #PTRS_PER_PTE, \istart, \iend, \count
  bic \count, \phys, #SWAPPER_BLOCK_SIZE - 1
  populate_entries \tbl, \count, \istart, \iend, \flags, #SWAPPER_BLOCK_SIZE, \tmp
  .endm

	/*
	 * @dst: destination register (32 or 64 bit wide)
	 * @sym: name of the symbol
	 * @tmp: optional 64-bit scratch register to be used if <dst> is a
	 *       32-bit wide register, in which case it cannot be used to hold
	 *       the address
	 */
	.macro	ldr_l, dst, sym, tmp=
	.ifb	\tmp
	adrp	\dst, \sym
	ldr	\dst, [\dst, :lo12:\sym]
	.else
	adrp	\tmp, \sym
	ldr	\dst, [\tmp, :lo12:\sym]
	.endif
	.endm

	/*
	 * @src: source register (32 or 64 bit wide)
	 * @sym: name of the symbol
	 * @tmp: mandatory 64-bit scratch register to calculate the address
	 *       while <src> needs to be preserved.
	 */
	.macro	str_l, src, sym, tmp
	adrp	\tmp, \sym
	str	\src, [\tmp, :lo12:\sym]
	.endm
