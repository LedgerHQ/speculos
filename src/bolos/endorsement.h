#pragma once

unsigned long sys_os_endorsement_get_public_key(uint8_t index, uint8_t *buffer);
unsigned long sys_os_endorsement_key1_sign_data(uint8_t *data,
                                                size_t dataLength,
                                                uint8_t *signature);
unsigned int sys_os_endorsement_get_code_hash(uint8_t *buffer);
unsigned int
sys_os_endorsement_get_public_key_certificate(unsigned char index,
                                              unsigned char *buffer);
