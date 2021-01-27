#!/usr/bin/env python3
# coding: utf8

"""
Add the custom exception ReadError.
"""


class ReadError(Exception):
    """
    Raised when can_read method in SeProxyHal class have no more data available.
    """
    pass
