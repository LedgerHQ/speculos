#!/usr/bin/env python3
# coding: utf8


class ReadError(Exception):
    """
    Raised when can_read method in SeProxyHal class have no more data available.
    """
    pass


class WriteError(Exception):
    """
    Raised when can_read method in SeProxyHal class have no more data available.
    """
    pass
