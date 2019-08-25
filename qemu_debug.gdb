set pagination off
# b smmuv3_translate
# b smmuv3_notify_iova
# b smmu_init_irq
# b get_pte
# b smmuv3_register_types
# b tg2granule
# b pa_range
# b smmuv3_record_event
# b iova_level_offset
# b smmu_base_realize
# b smmu_iommu_mr
# b smmu_find_add_as
# b smmu_find_smmu_pcibus
# b smmu_ptw
# b smmu_ptw_64
# b select_tt
# b aarch64_a53_initfn
# b tb_htable_lookup
#b vmsa_tcr_el1_write
#b vmsa_ttbr_write
#b tlb_hit
#b tlb_entry
# commands
#   silent
#   printf "tlb_entry 0x%x\n", addr
#   if addr < 0x81354
#     c
#   end
#   r
# end
# b cpu-exec.c:732
# # b cpu_exec 
# commands
#   if tb->pc != 0x88110
#     c
#   end
# end
# 
# b arm_cpu_do_interrupt_aarch64

b get_phys_addr_lpae
r
