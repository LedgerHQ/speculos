#pragma once

// the number of parameters of a syscall is stored in the syscall id
#define SYSCALL_NUMBER_OF_PARAMETERS(id) (((id) >> 24) & 0xf)

// clang-format off
#define SYSCALL_get_api_level_ID_IN                                   0x00000001
#define SYSCALL_halt_ID_IN                                            0x00000002
#define SYSCALL_nvm_write_ID_IN                                       0x03000003
#define SYSCALL_nvm_erase_ID_IN                                       0x02000121
#define SYSCALL_cx_aes_set_key_hw_ID_IN                               0x020000b2
#define SYSCALL_cx_aes_reset_hw_ID_IN                                 0x000000b3
#define SYSCALL_cx_aes_block_hw_ID_IN                                 0x020000b4
#define SYSCALL_cx_des_set_key_hw_ID_IN                               0x020000af
#define SYSCALL_cx_des_reset_hw_ID_IN                                 0x000000b0
#define SYSCALL_cx_des_block_hw_ID_IN                                 0x020000b1
#define SYSCALL_cx_bn_lock_ID_IN                                      0x02000112
#define SYSCALL_cx_bn_unlock_ID_IN                                    0x000000b6
#define SYSCALL_cx_bn_is_locked_ID_IN                                 0x000000b7
#define SYSCALL_cx_bn_alloc_ID_IN                                     0x02000113
#define SYSCALL_cx_bn_alloc_init_ID_IN                                0x04000114
#define SYSCALL_cx_bn_destroy_ID_IN                                   0x010000bc
#define SYSCALL_cx_bn_nbytes_ID_IN                                    0x0200010d
#define SYSCALL_cx_bn_init_ID_IN                                      0x03000115
#define SYSCALL_cx_bn_rand_ID_IN                                      0x010000ea
#define SYSCALL_cx_bn_copy_ID_IN                                      0x020000c0
#define SYSCALL_cx_bn_set_u32_ID_IN                                   0x020000c1
#define SYSCALL_cx_bn_get_u32_ID_IN                                   0x020000eb
#define SYSCALL_cx_bn_export_ID_IN                                    0x030000c3
#define SYSCALL_cx_bn_cmp_ID_IN                                       0x030000c4
#define SYSCALL_cx_bn_cmp_u32_ID_IN                                   0x030000c5
#define SYSCALL_cx_bn_is_odd_ID_IN                                    0x02000118
#define SYSCALL_cx_bn_xor_ID_IN                                       0x030000c8
#define SYSCALL_cx_bn_or_ID_IN                                        0x030000c9
#define SYSCALL_cx_bn_and_ID_IN                                       0x030000ca
#define SYSCALL_cx_bn_tst_bit_ID_IN                                   0x030000cb
#define SYSCALL_cx_bn_set_bit_ID_IN                                   0x020000cc
#define SYSCALL_cx_bn_clr_bit_ID_IN                                   0x020000cd
#define SYSCALL_cx_bn_shr_ID_IN                                       0x020000ce
#define SYSCALL_cx_bn_shl_ID_IN                                       0x0200011c
#define SYSCALL_cx_bn_cnt_bits_ID_IN                                  0x020000ec
#define SYSCALL_cx_bn_add_ID_IN                                       0x03000119
#define SYSCALL_cx_bn_sub_ID_IN                                       0x0300011a
#define SYSCALL_cx_bn_mul_ID_IN                                       0x030000d2
#define SYSCALL_cx_bn_mod_add_ID_IN                                   0x040000d3
#define SYSCALL_cx_bn_mod_sub_ID_IN                                   0x040000d4
#define SYSCALL_cx_bn_mod_mul_ID_IN                                   0x040000d5
#define SYSCALL_cx_bn_reduce_ID_IN                                    0x030000d6
#define SYSCALL_cx_bn_mod_sqrt_ID_IN                                  0x0400011d
#define SYSCALL_cx_bn_mod_pow_bn_ID_IN                                0x040000d7
#define SYSCALL_cx_bn_mod_pow_ID_IN                                   0x050000ed
#define SYSCALL_cx_bn_mod_pow2_ID_IN                                  0x050000ee
#define SYSCALL_cx_bn_mod_invert_nprime_ID_IN                         0x030000da
#define SYSCALL_cx_bn_mod_u32_invert_ID_IN                            0x03000116
#define SYSCALL_cx_bn_is_prime_ID_IN                                  0x020000ef
#define SYSCALL_cx_bn_next_prime_ID_IN                                0x010000f0
#define SYSCALL_cx_bn_rng_ID_IN                                       0x020001dd
#define SYSCALL_cx_bn_gf2_n_mul_ID_IN                                 0x05000046
#define SYSCALL_cx_mont_alloc_ID_IN                                   0x020000dc
#define SYSCALL_cx_mont_init_ID_IN                                    0x020000dd
#define SYSCALL_cx_mont_init2_ID_IN                                   0x030000de
#define SYSCALL_cx_mont_to_montgomery_ID_IN                           0x030000df
#define SYSCALL_cx_mont_from_montgomery_ID_IN                         0x030000e0
#define SYSCALL_cx_mont_mul_ID_IN                                     0x040000e1
#define SYSCALL_cx_mont_pow_ID_IN                                     0x050000e2
#define SYSCALL_cx_mont_pow_bn_ID_IN                                  0x040000e3
#define SYSCALL_cx_mont_invert_nprime_ID_IN                           0x030000e4
#define SYSCALL_cx_ecdomain_size_ID_IN                                0x0200012e
#define SYSCALL_cx_ecdomain_parameters_length_ID_IN                   0x0200012f
#define SYSCALL_cx_ecdomain_parameter_ID_IN                           0x04000130
#define SYSCALL_cx_ecdomain_parameter_bn_ID_IN                        0x03000131
#define SYSCALL_cx_ecdomain_generator_ID_IN                           0x04000132
#define SYSCALL_cx_ecdomain_generator_bn_ID_IN                        0x02000133
#define SYSCALL_cx_ecpoint_alloc_ID_IN                                0x020000f1
#define SYSCALL_cx_ecpoint_destroy_ID_IN                              0x010000f2
#define SYSCALL_cx_ecpoint_init_ID_IN                                 0x050000f3
#define SYSCALL_cx_ecpoint_init_bn_ID_IN                              0x030000f4
#define SYSCALL_cx_ecpoint_export_ID_IN                               0x050000f5
#define SYSCALL_cx_ecpoint_export_bn_ID_IN                            0x030000f6
#define SYSCALL_cx_ecpoint_compress_ID_IN                             0x0400012c
#define SYSCALL_cx_ecpoint_decompress_ID_IN                           0x0400012d
#define SYSCALL_cx_ecpoint_add_ID_IN                                  0x0300010e
#define SYSCALL_cx_ecpoint_neg_ID_IN                                  0x0100010f
#define SYSCALL_cx_ecpoint_scalarmul_ID_IN                            0x03000110
#define SYSCALL_cx_ecpoint_scalarmul_bn_ID_IN                         0x02000111
#define SYSCALL_cx_ecpoint_rnd_scalarmul_ID_IN                        0x03000127
#define SYSCALL_cx_ecpoint_rnd_scalarmul_bn_ID_IN                     0x02000128
#define SYSCALL_cx_ecpoint_rnd_fixed_scalarmul_ID_IN                  0x03000129
#define SYSCALL_cx_ecpoint_double_scalarmul_ID_IN                     0x07000148
#define SYSCALL_cx_ecpoint_double_scalarmul_bn_ID_IN                  0x0500014a
#define SYSCALL_cx_ecpoint_cmp_ID_IN                                  0x030000fb
#define SYSCALL_cx_ecpoint_is_on_curve_ID_IN                          0x020000fc
#define SYSCALL_cx_ecpoint_is_at_infinity_ID_IN                       0x0200014b
#define SYSCALL_cx_ecpoint_x25519_ID_IN                               0x0300001b
#define SYSCALL_cx_ecpoint_x448_ID_IN                                 0x03000060
#define SYSCALL_cx_crc32_hw_ID_IN                                     0x02000102 // API levels < 18
#define SYSCALL_cx_crc_hw_ID_IN                                       0x04000102 // API levels >= 18
#define SYSCALL_cx_get_random_bytes_ID_IN                             0x02000107
#define SYSCALL_cx_trng_get_random_data_ID_IN                         0x02000106
#define SYSCALL_os_perso_erase_all_ID_IN                              0x0000004b
#define SYSCALL_os_perso_set_seed_ID_IN                               0x0400004e
#define SYSCALL_os_perso_derive_and_set_seed_ID_IN                    0x0700004f
#define SYSCALL_os_perso_set_words_ID_IN                              0x02000050
#define SYSCALL_os_perso_finalize_ID_IN                               0x00000051
#define SYSCALL_os_perso_isonboarded_ID_IN                            0x0000009f
#define SYSCALL_os_perso_setonboardingstatus_ID_IN                    0x04000094
#define SYSCALL_os_perso_derive_node_bip32_ID_IN                      0x05000053
#define SYSCALL_os_perso_derive_node_with_seed_key_ID_IN              0x080000a6
#define SYSCALL_os_perso_derive_eip2333_ID_IN                         0x040000a7

// Endorsement syscalls
// -- Pre API_LEVEL_23
#define SYSCALL_os_endorsement_get_code_hash_ID_IN                    0x01000055
#define SYSCALL_os_endorsement_get_public_key_ID_IN                   0x03000056
#define SYSCALL_os_endorsement_get_public_key_certificate_ID_IN       0x03000057
#define SYSCALL_os_endorsement_key1_get_app_secret_ID_IN              0x01000058
#define SYSCALL_os_endorsement_key1_sign_data_ID_IN                   0x03000059
#define SYSCALL_os_endorsement_key2_derive_sign_data_ID_IN            0x0300005a
#define SYSCALL_os_endorsement_key1_sign_without_code_hash_ID_IN      0x0300005b
// -- API_LEVEL_23 and above
#define SYSCALL_ENDORSEMENT_get_code_hash_ID_IN                       0x01000055
#define SYSCALL_ENDORSEMENT_get_public_key_ID_IN                      0x03000056
#define SYSCALL_ENDORSEMENT_get_public_key_certificate_ID_IN          0x03000057
#define SYSCALL_ENDORSEMENT_key1_get_app_secret_ID_IN                 0x01000058
#define SYSCALL_ENDORSEMENT_key1_sign_data_ID_IN                      0x04000059
#define SYSCALL_ENDORSEMENT_key2_derive_and_sign_data_ID_IN           0x0400005a
#define SYSCALL_ENDORSEMENT_key1_sign_without_code_hash_ID_IN         0x0400005b

#define SYSCALL_os_perso_set_pin_ID_IN                                0x0300004c
#define SYSCALL_os_perso_set_current_identity_pin_ID_IN               0x0200004d
#define SYSCALL_os_global_pin_is_validated_ID_IN                      0x000000a0
#define SYSCALL_os_global_pin_check_ID_IN                             0x020000a1
#define SYSCALL_os_global_pin_invalidate_ID_IN                        0x0000005d
#define SYSCALL_os_global_pin_retries_ID_IN                           0x0000005e
#define SYSCALL_os_registry_count_ID_IN                               0x0000005f
#define SYSCALL_os_registry_get_ID_IN                                 0x02000122
#define SYSCALL_os_ux_ID_IN                                           0x01000064
#define SYSCALL_os_ux_result_ID_IN                                    0x01000065
#define SYSCALL_os_lib_call_ID_IN                                     0x01000067
#define SYSCALL_os_lib_end_ID_IN                                      0x00000068
#define SYSCALL_os_flags_ID_IN                                        0x0000006a
#define SYSCALL_os_version_ID_IN                                      0x0200006b
#define SYSCALL_os_serial_ID_IN                                       0x0200006c
#define SYSCALL_os_seph_features_ID_IN                                0x0000006e
#define SYSCALL_os_seph_version_ID_IN                                 0x0200006f
#define SYSCALL_os_bootloader_version_ID_IN                           0x02000073
#define SYSCALL_os_setting_get_ID_IN                                  0x03000070
#define SYSCALL_os_setting_set_ID_IN                                  0x03000071
#define SYSCALL_os_get_memory_info_ID_IN                              0x01000072
#define SYSCALL_os_registry_get_tag_ID_IN                             0x06000123
#define SYSCALL_os_registry_get_current_app_tag_ID_IN                 0x03000074
#define SYSCALL_os_registry_delete_app_and_dependees_ID_IN            0x01000124
#define SYSCALL_os_registry_delete_all_apps_ID_IN                     0x00000125
#define SYSCALL_os_sched_exec_ID_IN                                   0x01000126
#define SYSCALL_os_sched_exit_ID_IN                                   0x0100009a
#define SYSCALL_os_sched_is_running_ID_IN                             0x0100009b
#define SYSCALL_os_sched_create_ID_IN                                 0x0700011b
#define SYSCALL_os_sched_kill_ID_IN                                   0x01000078
#define SYSCALL_io_seph_send_ID_IN                                    0x02000083
#define SYSCALL_io_seph_is_status_sent_ID_IN                          0x00000084
#define SYSCALL_io_seph_recv_ID_IN                                    0x03000085
#define SYSCALL_nvm_write_page_ID_IN                                  0x0100010a
#define SYSCALL_nvm_erase_page_ID_IN                                  0x01000136
#define SYSCALL_try_context_get_ID_IN                                 0x00000087
#define SYSCALL_try_context_set_ID_IN                                 0x0100010b
#define SYSCALL_os_sched_last_status_ID_IN                            0x0100009c
#define SYSCALL_os_sched_yield_ID_IN                                  0x0100009d
#define SYSCALL_os_sched_switch_ID_IN                                 0x0200009e
#define SYSCALL_os_sched_current_task_ID_IN                           0x0000008b
#define SYSCALL_os_allow_protected_flash_ID_IN                        0x0000008e
#define SYSCALL_os_deny_protected_flash_ID_IN                         0x00000091
#define SYSCALL_os_allow_protected_ram_ID_IN                          0x00000092
#define SYSCALL_os_deny_protected_ram_ID_IN                           0x00000093
#define SYSCALL_os_customca_verify_ID_IN                              0x03000090
#define SYSCALL_screen_clear_ID_IN                                    0x00000079
#define SYSCALL_screen_update_ID_IN                                   0x0000007a
#define SYSCALL_screen_set_keepout_ID_IN                              0x0400007b
#define SYSCALL_screen_set_brightness_ID_IN                           0x0100008c
#define SYSCALL_bagl_hal_draw_bitmap_within_rect_ID_IN                0x0900007c
#define SYSCALL_bagl_hal_draw_rect_ID_IN                              0x0500007d
#define SYSCALL_io_button_read_ID_IN                                  0x0000008f
#define SYSCALL_os_seph_serial_ID_IN                                  0x0200006d

#define SYSCALL_nbgl_front_draw_rect_ID_IN                            0x01fa0000
#define SYSCALL_nbgl_front_draw_horizontal_line_ID_IN                 0x03fa0001
#define SYSCALL_nbgl_front_draw_img_ID_IN                             0x04fa0002
#define SYSCALL_nbgl_front_refresh_area_legacy_ID_IN     	          0x02fa0003 // API levels 6-7-8-9
#define SYSCALL_nbgl_front_refresh_area_ID_IN                         0x03fa0003
#define SYSCALL_nbgl_front_draw_img_file_ID_IN                        0x05fa0004
#define SYSCALL_touch_get_last_info_ID_IN                             0x01fa000b
#define SYSCALL_nbgl_get_font_ID_IN                                   0x01fa000c
#define SYSCALL_nbgl_screen_reinit_ID_IN                              0x00fa000d
#define SYSCALL_nbgl_front_draw_img_rle_legacy_ID_IN                  0x00fa000e // API levels 7-8-9
#define SYSCALL_nbgl_front_draw_img_rle_10_ID_IN                      0x04fa0010 // API level 10
#define SYSCALL_nbgl_front_draw_img_rle_ID_IN                         0x05fa0010 // API level_12

#define SYSCALL_ox_bls12381_sign_ID_IN                                0x05000103
#define SYSCALL_cx_hash_to_field_ID_IN                                0x06000104
#define SYSCALL_cx_bls12381_aggregate_ID_IN                           0x05000105
#define SYSCALL_cx_bls12381_key_gen_ID_IN                             0x03000108

#define SYSCALL_os_pki_load_certificate_ID_IN                         0x060000aa
#define SYSCALL_os_pki_verify_ID_IN                                   0x040000ab
#define SYSCALL_os_pki_get_info_ID_IN                                 0x040000ac

// API level 24 io-revamp
#define SYSCALL_os_io_init_ID_IN                                      0x01000084
#define SYSCALL_os_io_start_ID_IN                                     0x01000085
#define SYSCALL_os_io_stop_ID_IN                                      0x01000086
#define SYSCALL_os_io_tx_cmd_ID_IN                                    0x04000088
#define SYSCALL_os_io_rx_evt_ID_IN                                    0x03000089
#define SYSCALL_os_io_seph_tx_ID_IN                                   0x03000082
#define SYSCALL_os_io_seph_se_rx_event_ID_IN                          0x05000083

// clang-format on
