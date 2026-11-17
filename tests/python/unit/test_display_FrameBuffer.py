import threading

import pytest

from speculos.mcu.display import FrameBuffer


@pytest.mark.parametrize("model", ["stax", "flex", "apex_p"])
def test_public_screenshot_follows_refreshes_on_touchscreen_models(model):
    fb = FrameBuffer(model)
    first = fb.get_public_screenshot()

    fb.screenshot_pixels[(0, 0)] = 0xFF0000
    if fb.get_public_screenshot() != first:
        raise AssertionError("The public screenshot changed before a refresh")

    fb.update_public_screenshot()
    if fb.get_public_screenshot() == first:
        raise AssertionError("The public screenshot did not change after a refresh")


@pytest.mark.parametrize("model", ["nanox", "nanosp"])
def test_public_screenshot_is_live_on_nano(model):
    fb = FrameBuffer(model)
    first = fb.get_public_screenshot()

    fb.screenshot_pixels[(0, 0)] = 0xFF0000
    if fb.get_public_screenshot() == first:
        raise AssertionError("The public screenshot did not follow the screen content")


def test_public_screenshot_keeps_the_latest_frame_under_concurrent_reads():
    fb = FrameBuffer("flex")
    first_encoding_started = threading.Event()
    release_first_encoding = threading.Event()
    frames = iter([b"old", b"new"])

    def encode():
        frame = next(frames)
        if frame == b"old":
            first_encoding_started.set()
            release_first_encoding.wait(5)
        return frame

    fb._get_screenshot_iobytes_value = encode

    slow_reader = threading.Thread(target=fb.get_public_screenshot)
    slow_reader.start()
    first_encoding_started.wait(5)
    fb.update_public_screenshot()
    fast_reader = threading.Thread(target=fb.get_public_screenshot)
    fast_reader.start()
    release_first_encoding.set()
    slow_reader.join(5)
    fast_reader.join(5)

    if fb.get_public_screenshot() != b"new":
        raise AssertionError("An older frame overwrote the latest public screenshot")
