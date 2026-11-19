from speculos.main import get_flash_load_size

TEXT = {"p_type": "PT_LOAD", "p_offset": 0x10000, "p_vaddr": 0xC0DE0000, "p_filesz": 0x42000}
NVM_DATA = {"p_type": "PT_LOAD", "p_offset": 0x52000, "p_vaddr": 0xC0E22000, "p_filesz": 0x12A4}
BSS = {"p_type": "PT_LOAD", "p_offset": 0x60000, "p_vaddr": 0xDA7A0000, "p_filesz": 0}
PHDR = {"p_type": "PT_PHDR", "p_offset": 0x34, "p_vaddr": 0xC0DD0034, "p_filesz": 0xA0}


def test_single_text_segment():
    size = get_flash_load_size([PHDR, TEXT, BSS], TEXT)
    if size != 0x42000:
        raise AssertionError(f"Unexpected size {size:#x}")


def test_nvm_data_segment_following_text_is_mapped():
    size = get_flash_load_size([TEXT, NVM_DATA, BSS, PHDR], TEXT)
    if size != 0x432A4:
        raise AssertionError(f"Unexpected size {size:#x}")


def test_segment_not_laid_out_as_in_memory_is_ignored():
    data_in_ram = {"p_type": "PT_LOAD", "p_offset": 0x52000, "p_vaddr": 0xDA7A0000, "p_filesz": 0x100}
    size = get_flash_load_size([TEXT, data_in_ram], TEXT)
    if size != 0x42000:
        raise AssertionError(f"Unexpected size {size:#x}")
